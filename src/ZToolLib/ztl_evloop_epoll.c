#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ztl_evloop_private.h"
#include "ztl_network.h"
#include "ztl_utils.h"
#include "ztl_mempool.h"
#include "ztl_atomic.h"
#include "ztl_threads.h"


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

#define ZTL_THE_CTX(ev)     ((epoll_ctx_t*)ev->ctx)

static int epoll_init(ztl_evloop_t* ev);
static int epoll_destroy(ztl_evloop_t* ev);
static int epoll_start(ztl_evloop_t* ev);
static int epoll_poll(ztl_evloop_t* evloop, int timeoutMS);
static int epoll_stop(ztl_evloop_t* ev);
static int epoll_add(ztl_evloop_t* ev, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents);
static int epoll_del(ztl_evloop_t* ev, ztl_connection_t* conn);

struct ztl_event_ops epollops = {
    epoll_init,
    epoll_start,
    epoll_add,
    epoll_del,
    epoll_poll,
    epoll_stop,
    epoll_destroy,
    "epoll",
};


static void* _epoll_loop_entry(void* arg)
{
    //logdebug("thread [%u] running...\n", get_thread_id());
    ztl_evloop_t* evloop = (ztl_evloop_t*)arg;

    bool looponce = ztl_atomic_add(&evloop->looponce, 0) == 0 ? true : false;

    while (evloop->running)
    {
        ztl_evloop_update_polltime(evloop);
        ztl_evloop_expire(evloop);

        epoll_poll(evloop, evloop->timeoutMS);

        if (looponce)
            evloop->handler(evloop, NULL, ZEV_LOOPONCE);
    }

    ztl_atomic_add(&evloop->nexited, 1);

    //logdebug("thread [%u] exit.\n", get_thread_id());
    return NULL;
}

static int epoll_init(ztl_evloop_t* evloop)
{
    epoll_ctx_t* lpctx;
    lpctx = (epoll_ctx_t*)malloc(sizeof(epoll_ctx_t));
    if (!lpctx)
    {
        return -1;
    }

    memset(lpctx, 0, sizeof(epoll_ctx_t));
    evloop->ctx = lpctx;

    // create epoll fd
    lpctx->epfd = epoll_create(1024);
    if (lpctx->epfd < 0)
    {
        return -1;
    }

    // create connection objects pool
    evloop->conn_mp = ztl_mp_create(sizeof(ztl_connection_t), 256, 1);

    // event timer
    ztl_event_timer_init(&evloop->timers);

    return 0;
}

static int epoll_destroy(ztl_evloop_t* evloop)
{
    epoll_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    if (lpctx) {
        close(lpctx->epfd);
        free(lpctx);
        evloop->ctx = NULL;
    }

    if (evloop->conn_mp) {
        ztl_mp_release(evloop->conn_mp);
        evloop->conn_mp = NULL;
    }

    return 0;
}

static int epoll_start(ztl_evloop_t* evloop)
{
    if (evloop->thrnum <= 0) {
        evloop->thrnum = get_cpu_number();
    }

    // start work threads
    pthread_t thrid;
    int i = 0;
    for (i = 0; i < evloop->thrnum; ++i) {
        pthread_create(&thrid, NULL, _epoll_loop_entry, evloop);
    }

    return 0;
}

static int epoll_stop(ztl_evloop_t* evloop)
{
    ztl_atomic_set(&evloop->running, 0);

    // TODO: use 'post_pipe_job' funcion
    char sbuf[256] = "stop";
    evloop->pipeconn[0].wbuf = sbuf;
    evloop->pipeconn[0].wsize = strlen(sbuf);
    evloop->pipeconn[0].send(&evloop->pipeconn[0]);

    if (INVALID_SOCKET != evloop->listen_conn.sockfd) {
        close_socket(evloop->listen_conn.sockfd);
        evloop->listen_conn.sockfd = INVALID_SOCKET;
    }

    while (ztl_atomic_add(&evloop->nexited, 0) != (uint32_t)evloop->thrnum) {
        sleep_ms(1);
    }

    return 0;
}

static int epoll_add(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents)
{
    struct epoll_event reqev;
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

    epoll_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    if (conn->added == 0) {
        conn->added = 1;
        return epoll_ctl(lpctx->epfd, EPOLL_CTL_ADD, conn->sockfd, &reqev);
    }
    else {
        return epoll_ctl(lpctx->epfd, EPOLL_CTL_MOD, conn->sockfd, &reqev);
    }
}

static int epoll_del(ztl_evloop_t* evloop, ztl_connection_t* conn)
{
    struct epoll_event reqev;
    reqev.events = EPOLLIN | EPOLLOUT;
    reqev.data.u64 = 0; /* avoid valgrind warning */
    reqev.data.ptr = conn;

    epoll_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    conn->added = 0;
    int rv = epoll_ctl(lpctx->epfd, EPOLL_CTL_DEL, conn->sockfd, &reqev);
    return rv;
}

static int epoll_poll(ztl_evloop_t* evloop, int timeoutMS)
{
    int numev;
    const int max_nevent = 64;
    struct epoll_event epevents[max_nevent];
    epoll_ctx_t* lpctx = ZTL_THE_CTX(evloop);

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
        struct epoll_event* lpee = epevents + i;
        ztl_connection_t* conn = (ztl_connection_t*)lpee->data.ptr;

        // process in event
        if (lpee->events & (EPOLLIN | EPOLLERR | EPOLLHUP))
        {
            conn->handler(evloop, conn, ZEV_POLLIN);
        }

        // if the connection is closed...
        if (conn->disconncted)
        {
            //logdebug("[%u] the connection is broken for %d\n", get_thread_id(), conn->sockfd);
            ztl_free_connection(evloop, conn);
            continue;
        }

        // process out event
        if (lpee->events & (EPOLLOUT | EPOLLERR | EPOLLHUP))
        {
            conn->handler(evloop, conn, ZEV_POLLOUT);
        }
    }

    return numev;
}


#endif//__linux__
