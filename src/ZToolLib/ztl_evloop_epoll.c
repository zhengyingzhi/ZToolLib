#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ztl_evloop_private.h"
#include "ztl_network.h"
#include "ztl_utils.h"
#include "ztl_mempool.h"
#include "ztl_atomic.h"


#if defined( _DEBUG ) && !defined( DEBUG )
#define DEBUG
#endif

#ifdef __linux__

#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

struct epoll_ctx_st
{
    int epfd;
};
typedef struct epoll_ctx_st epoll_ctx_t;

#define ZTL_THE_CTX(ev)     ((epoll_ctx_t*)ev->context)

static int epoll_init(ztl_evloop_t* ev);
static int epoll_destroy(ztl_evloop_t* ev);
static int epoll_start(ztl_evloop_t* ev);
static int epoll_stop(ztl_evloop_t* ev);
static int epoll_add(ztl_evloop_t* ev, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents);
static int epoll_del(ztl_evloop_t* ev, ztl_connection_t* conn);

struct ztl_event_ops epollops = {
    epoll_init,
    epoll_start,
    epoll_add,
    epoll_del,
    epoll_stop,
    epoll_destroy,
    "epoll",
};

/// pre-declare
static int _epoll_do_accept(ztl_evloop_t* ev);
static int _epoll_loop(ztl_evloop_t* ev, int timeoutMS);

static void* epoll_loop_entry(void* arg)
{
    //logdebug("thread [%u] running...\n", get_thread_id());
    ztl_evloop_t* lpev = (ztl_evloop_t*)arg;

    bool looponce_flag = ztl_atomic_add(&lpev->looponce_flag, 0) == 0 ? true : false;

    while (lpev->running)
    {
        _epoll_loop(lpev, lpev->timeoutMS);

        // expire timers...
        //update_time(lpev->timers);
        //event_timer_expire(lpev->timers);

        if (looponce_flag)
            lpev->handler(lpev, NULL, ZEV_LOOPONCE);
    }

    ztl_atomic_add(&lpev->nexited, 1);

    //logdebug("thread [%u] exit.\n", get_thread_id());
    return NULL;
}

static int epoll_init(ztl_evloop_t* ev)
{
    epoll_ctx_t* lpctx;
    lpctx = (epoll_ctx_t*)malloc(sizeof(epoll_ctx_t));
    if (!lpctx)
    {
        return -1;
    }

    memset(lpctx, 0, sizeof(epoll_ctx_t));
    ev->context = lpctx;

    // create epoll fd
    lpctx->epfd = epoll_create(1024);
    if (lpctx->epfd < 0)
    {
        return -1;
    }

    // create connection objects pool
    ev->conn_mp = ztl_mp_create(sizeof(ztl_connection_t), 256);

    // event timer
    //ev->timers = event_timer_create();

    return 0;
}

static int epoll_destroy(ztl_evloop_t* ev)
{
    epoll_ctx_t* lpctx = ZTL_THE_CTX(ev);
    if (lpctx)
        close(lpctx->epfd);

    return 0;
}

static int epoll_start(ztl_evloop_t* ev)
{
    ztl_connection_t* conn;
    conn = ztl_new_connection(ev, ev->listenfd);
    conn->port = ev->listen_port;
    conn->addr = ev->listen_addr;

    evloop_add(ev, conn, ZEV_POLLIN);

    // TODO: where we add pipefd to epoll??
    epoll_ctx_t* lpctx = ZTL_THE_CTX(ev);
    if (ev->pipefds[1] > 0)
    {
        epoll_event reqev;
        reqev.events = EPOLLIN | EPOLLONESHOT;
        reqev.data.ptr = NULL;
        epoll_ctl(lpctx->epfd, EPOLL_CTL_ADD, ev->pipefds[1], &reqev);
    }

    if (ev->thrnum <= 0) {
        ev->thrnum = get_cpu_number();
    }

    // start work threads
    pthread_t thrid;
    int i = 0;
    for (i = 0; i < ev->thrnum; ++i) {
        pthread_create(&thrid, NULL, epoll_loop_entry, ev);
    }

    return 0;
}

static int epoll_stop(ztl_evloop_t* ev)
{
    ztl_atomic_set(&ev->running, 0);

    // TODO: use 'post_pipe_job' funcion
    char sbuf[256] = "stop";
    int rv = send(ev->pipefds[0], sbuf, strlen(sbuf), 0);

    if (INVALID_SOCKET != ev->listenfd) {
        close_socket(ev->listenfd);
    }

    while (ztl_atomic_add(&ev->nexited, 0) != (uint32_t)ev->thrnum) {
        sleep_ms(1);
    }

    return rv;
}

static int epoll_add(ztl_evloop_t* ev, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents)
{
    epoll_event reqev;
    reqev.events = EPOLLONESHOT | EPOLLET;
    reqev.data.u64 = 0; /* avoid valgrind warning */
    reqev.data.ptr = conn;

    if (reqevents & ZEV_POLLIN) {
        reqev.events |= EPOLLIN;
    }
    if (reqevents & ZEV_POLLOUT) {
        reqev.events |= EPOLLOUT;
    }
    conn->reqevents = reqev.events;

    epoll_ctx_t* lpctx = ZTL_THE_CTX(ev);
    if (conn->added == 0) {
        conn->added = 1;
        return epoll_ctl(lpctx->epfd, EPOLL_CTL_ADD, conn->sockfd, &reqev);
    }
    else {
        return epoll_ctl(lpctx->epfd, EPOLL_CTL_MOD, conn->sockfd, &reqev);
    }
}

static int epoll_del(ztl_evloop_t* ev, ztl_connection_t* conn)
{
    epoll_event reqev;
    reqev.events = EPOLLIN | EPOLLOUT;
    reqev.data.u64 = 0; /* avoid valgrind warning */
    reqev.data.ptr = conn;

    epoll_ctx_t* lpctx = ZTL_THE_CTX(ev);
    conn->added = 0;
    int rv = epoll_ctl(lpctx->epfd, EPOLL_CTL_DEL, conn->sockfd, &reqev);
    return rv;
}

static int _epoll_do_accept(ztl_evloop_t* ev)
{
    ztl_connection_t* newconn;
    while (ev->running)
    {
        newconn = ztl_do_accept(ev);
        if (!newconn) {
            break;
        }

        // set other member

        lpev->handler(lpev, lpNewConn, ZEV_NEWCONN);
    }
    return 0;
}

static int _epoll_loop(ztl_evloop_t* ev, int timeoutMS)
{
    int numev;
    const int max_nevent = 64;
    epoll_event epevents[max_nevent];
    epoll_ctx_t* lpctx = ZTL_THE_CTX(ev);

    numev = epoll_wait(lpctx->epfd, epevents, max_nevent, timeoutMS);

    if (numev < 0)
    {
        if (errno == EINTR) {
            // no error, continue
            numev = 0;
        }
        return numev;
    }
    else if (numev == 0)
    {
        return 0;
    }

#if defined(_DEBUG) || defined(DEBUG)
    //logdebug("*[%u] evloop_loop: numev=%d*\n", get_thread_id(), numev);
#endif

    for (int i = 0; i < numev; ++i)
    {
        epoll_event* lpee = epevents + i;
        ztl_connection_t* conn = (ztl_connection_t*)lpee->data.ptr;

        // if pipe fd
        if (conn == NULL)
        {
            char pipebuf[256] = "";
            recv(ev->pipefds[1], pipebuf, 255, 0);
            if (strcmp(pipebuf, "stop") == 0)
            {
                ev->running = 0;
                fprintf(stderr, "!pipe fd got stop cmd!\n");
                break;
            }
            continue;
        }

        // if listen fd
        if (conn->sockfd == ev->listenfd)
        {
            _epoll_do_accept(ev);

            // register listen fd to epoll
            epoll_add(ev, conn, ZEV_POLLIN);
            continue;
        }

        // process in event
        if (lpee->events & (EPOLLOIN | EPOLLERR | EPOLLHUP))
        {
            lpev->handler(lpev, lpNewConn, ZEV_POLLIN);
        }

        // if the connection is closed...
        if (conn->disconncted)
        {
            //logdebug("[%u] the connection is broken for %d\n", get_thread_id(), conn->sockfd);
            ztl_free_connection(ev, conn);
            continue;
        }

        // process out event
        if (lpee->events & (EPOLLOUT | EPOLLERR | EPOLLHUP))
        {
            lpev->handler(lpev, lpNewConn, ZEV_POLLOUT);
        }
    }//for

    return numev;
}


#endif//__linux__
