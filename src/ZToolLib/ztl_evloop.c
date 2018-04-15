#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ztl_evloop_private.h"
#include "ztl_network.h"


ztl_event_ops_t* evops;

#if defined(_WIN32)
extern struct ztl_event_ops iocpops;
extern struct ztl_event_ops selectops;
#elif defined(__linux__)
extern struct ztl_event_ops epollops;
#else
#endif

static ztl_event_ops_t* _ztl_event_ops_provider(ZTL_EV_POLL_METHOD method)
{
    ztl_event_ops_t* lpops = NULL;

#if defined(_WIN32)
    if (method == ZTL_EPM_Select)
        lpops = &selectops;
    else if (method == ZTL_EPM_IOCP)
        lpops = &iocpops;
    else
        lpops = &selectops;
#elif defined(__linux__)
    if (method == ZPM_Epoll)
        lpops = &epollops;
    else
        lpops = &epollops;
#else
    lpops = NULL;
#endif

    return lpops;
}

static int _ztl_make_listen(sockhandle_t* plisten_fd, uint32_t listen_addr, uint16_t listen_port, bool reuse_addr)
{
    sockhandle_t lfd;
    char listen_ip[32] = "";

    lfd = create_socket(SOCK_STREAM);
    set_nonblock(lfd, true);

    inetaddr_to_string(listen_ip, sizeof(listen_ip), listen_addr);

    int rv = tcp_listen(lfd, listen_ip, listen_port, reuse_addr, 64);
    if (rv < 0) {
        rv = get_errno();
        close_socket(lfd);
        return rv;
    }

    *plisten_fd = lfd;

    return rv;
}

static int _ztl_listen_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS events)
{
    ztl_connection_t* newconn;
    while (evloop->running)
    {
        newconn = ztl_do_accept(evloop);
        if (!newconn) {
            break;
        }

        // set other member

        evloop->handler(evloop, newconn, ZEV_NEWCONN);
    }
    return 0;
}

static int _ztl_pipe_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS events)
{
    return 0;
}

int ztl_evloop_create(ztl_evloop_t** pevloop, ZTL_EV_POLL_METHOD method)
{
    ztl_event_ops_t*    evsel;
    ztl_evloop_t*       evloop;

    evsel = _ztl_event_ops_provider(method);
    if (!evsel) {
        return -1;  // enot support
    }

    evloop = (ztl_evloop_t*)malloc(sizeof(ztl_evloop_t));
    if (evloop)
    {
        memset(evloop, 0, sizeof(ztl_evloop_t));
        evloop->thrnum      = 0;
        evloop->running     = 0;
        evloop->timeoutMS   = ZTL_DEF_POLL_TIMEOUT_MS;
        evloop->nexited     = 0;
        evloop->looponce    = 0;

        evloop->listen_conn.sockfd  = INVALID_SOCKET;
        evloop->pipeconn[0].sockfd = INVALID_SOCKET;
        evloop->pipeconn[1].sockfd = INVALID_SOCKET;

        // create connection objects pool
        ztl_event_timer_init(&evloop->timers);
        evloop->conn_mp = ztl_mp_create(sizeof(ztl_connection_t), ZTL_DEF_CONN_INIT_COUNT, 1);
    }

    *pevloop = evloop;

    return 0;
}

int ztl_evloop_init(ztl_evloop_t* evloop, ztl_evconfig_t* config)
{
    int rv;
    ztl_connection_t* lconn;
    ztl_connection_t *pipeconn0, *pipeconn1;

    // init listen event (listen fd could be from outside)
    lconn = &evloop->listen_conn;
    if (lconn->sockfd == INVALID_SOCKET || lconn->sockfd == 0)
    {
        if (config->listen_fd > 0) {
            lconn->sockfd = config->listen_fd;
        }
        else {
            bool reuse_addr = config->reuse_adddr ? true : false;
            rv = _ztl_make_listen(&lconn->sockfd, config->listen_addr, config->listen_port, reuse_addr);
            if (rv != 0) {
                return rv;
            }
        }
    }

    lconn->addr     = config->listen_addr;
    lconn->port     = config->listen_port;
    lconn->evloop   = evloop;
    lconn->userdata = evloop;
    lconn->handler  = _ztl_listen_handler;       // listen event handler

    // add listen event
    evloop->evsel->add(evloop, lconn, ZEV_POLLIN);

    // init pipe event
    pipeconn0 = &evloop->pipeconn[0];
    pipeconn1 = &evloop->pipeconn[1];
    if (pipeconn0->sockfd == INVALID_SOCKET || pipeconn0->sockfd == 0)
    {
        sockhandle_t pipefds[2];
        make_sockpair(pipefds, AF_INET);
        set_closeonexec(pipefds[0]);
        set_closeonexec(pipefds[1]);

        pipeconn0->sockfd   = pipefds[0];
        pipeconn0->evloop   = evloop;
        pipeconn0->userdata = evloop;
        pipeconn0->handler  = NULL;
        pipeconn1->recv     = NULL;
        pipeconn0->send     = ztl_do_send;

        pipeconn1->sockfd   = pipefds[1];
        pipeconn1->evloop   = evloop;
        pipeconn1->userdata = evloop;
        pipeconn1->handler  = _ztl_pipe_handler;
        pipeconn1->recv     = ztl_do_recv;
        pipeconn0->send     = NULL;

        // add recv pipe event
        evloop->evsel->add(evloop, pipeconn1, ZEV_POLLIN);
    }

    evloop->handler     = config->handler;
    evloop->timer_handler = config->timer_handler;

    evloop->thrnum      = config->thread_num;
    evloop->timeoutMS   = config->poll_timeout_ms;
    evloop->nexited     = 0;

    return rv;
}

int ztl_evloop_start(ztl_evloop_t* evloop)
{
    int rv;

    evloop->running = 1;

    // start io work thread
    rv = evloop->evsel->start(evloop);

    return rv;
}

int ztl_evloop_stop(ztl_evloop_t* ev)
{
    int rv;
    // stop io thread
    ev->running = 0;
    rv = ev->evsel->stop(ev);
    return rv;
}

int ztl_evloop_add(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents)
{
    int rv;
    rv = evloop->evsel->add(evloop, conn, reqevents);
    return rv;
}

int ztl_evloop_del(ztl_evloop_t* evloop, ztl_connection_t* conn)
{
    int rv;
    rv = evloop->evsel->del(evloop, conn);
    return rv;
}


int ztl_evloop_loop(ztl_evloop_t* evloop, int timeoutMS)
{
    int rv;
    rv = evloop->evsel->poll(evloop, timeoutMS);
    return rv;
}


int ztl_evloop_release(ztl_evloop_t* evloop)
{
    int rv;

    if (!evloop) {
        return -1;
    }

    if (evloop->running == 0) {
        ztl_evloop_stop(evloop);
    }

    rv = evloop->evsel->destroy(evloop);

    if (evloop->listen_conn.sockfd > 0) {
        close_socket(evloop->listen_conn.sockfd);
        evloop->listen_conn.sockfd = INVALID_SOCKET;
    }

    ztl_mp_release(evloop->conn_mp);

    free(evloop);

    return rv;
}

void ztl_evloop_set_usedata(ztl_evloop_t* evloop, void* userdata)
{
    evloop->userdata = userdata;
}

void* ztl_evloop_get_usedata(ztl_evloop_t* evloop)
{
    return evloop->userdata;
}

sockhandle_t ztl_evloop_get_listenfd(ztl_evloop_t* evloop)
{
    return evloop->listen_conn.sockfd;
}



//////////////////////////////////////////////////////////////////////////
int ztl_evloop_addtimer(ztl_evloop_t* evloop, ztl_rbtree_node_t* timer, uint32_t timeoutMS)
{
    ztl_event_timer_add(&evloop->timers, timer, timeoutMS, 0);
    return 0;
}

int ztl_evloop_deltimer(ztl_evloop_t* evloop, ztl_rbtree_node_t* timer)
{
    ztl_event_timer_del(&evloop->timers, timer);
    return 0;
}

int ztl_evloop_expire(ztl_evloop_t* evloop)
{
    ztl_event_timer_expire(&evloop->timers, evloop->timepoint, evloop->timer_handler, evloop);
    return 0;
}
