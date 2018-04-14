#include <stdlib.h>
#include <stdio.h>
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

ztl_event_ops_t* event_ops_provider()
{
    ztl_event_ops_t* lpops = NULL;

#if defined(_WIN32)
    //lpops = &iocpops;
    lpops = &selectops;
#elif defined(__linux__)
    lpops = &epollops;
#else
    lpops = NULL;
#endif

    return lpops;
}


ztl_evloop_t* evloop_create()
{
    ztl_evloop_t* pev;
    pev = (ztl_evloop_t*)malloc(sizeof(ztl_evloop_t));
    if (pev != NULL)
    {
        memset(pev, 0, sizeof(ztl_evloop_t));
        pev->thrnum     = 0;
        pev->running    = 0;
        pev->timeoutMS  = ZTL_DEF_POLL_TIMEOUT_MS;
        pev->nexited    = 0;
        pev->looponce_flag = 0;

        pev->pipefds[0] = INVALID_SOCKET;
        pev->pipefds[1] = INVALID_SOCKET;

        // create connection objects pool
        ztl_event_timer_init(&pev->timers);
        pev->conn_mp = ztl_mp_create(sizeof(ztl_connection_t), ZTL_DEF_CONN_INIT_COUNT, 1);

        // os operations
        pev->evsel = event_ops_provider();
        pev->evsel->init(pev);
    }
    return pev;
}


int evloop_start(ztl_evloop_t* ev, event_config_t* config)
{
    int rv;
    sockhandle_t lfd;
    char listen_ip[32] = "";

    if (ev->listenfd <= 0)
    {
        ev->handler = config->handler;

        // create listen fd
        lfd = create_socket(SOCK_STREAM);
        set_nonblock(lfd, true);

        inetaddr_to_string(listen_ip, sizeof(listen_ip), config->listen_addr);

        rv = tcp_listen(lfd, listen_ip, config->listen_port, true, 64);
        if (rv < 0) {
            close_socket(lfd);
            return rv;
        }

        ev->listen_port = config->listen_port;
        ev->listen_addr = config->listen_addr;

        // todo: socket pair also make a ztl_connection_t

        // make a pair sockets, pipe[0] for send, pipe[1] for read
        make_sockpair(ev->pipefds, AF_INET);
        set_closeonexec(ev->pipefds[0]);
        set_closeonexec(ev->pipefds[1]);

        ev->listenfd    = lfd;
        ev->thrnum      = config->thread_num;
        ev->running     = 1;
        ev->timeoutMS   = config->poll_timeout_ms;
        ev->nexited     = 0;
    }

    // start io work thread
    rv = ev->evsel->start(ev);

    return rv;
}

int evloop_stop(ztl_evloop_t* ev)
{
    int rv;
    // stop io thread
    ev->running = 0;
    rv = ev->evsel->stop(ev);
    return rv;
}

int evloop_add(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents)
{
    int rv;
    rv = evloop->evsel->add(evloop, conn, reqevents);
    return rv;
}

int evloop_del(ztl_evloop_t* evloop, ztl_connection_t* conn)
{
    int rv;
    rv = evloop->evsel->del(evloop, conn);
    return rv;
}
//
//uint32_t evloop_addtimer(ztl_evloop_t* ev, uint32_t timeoutMS, pfontimer func, void* arg)
//{
//    return event_timer_add(ev->timers, timeoutMS, arg, func);
//}
//
//int evloop_deltimer(ztl_evloop_t* ev, uint32_t timerid)
//{
//    return event_timer_del(ev->timers, timerid);
//}

int evloop_destroy(ztl_evloop_t* ev)
{
    int rv;

    if (ev->running == 0) {
        evloop_stop(ev);
    }

    rv = ev->evsel->destroy(ev);

    if (ev->listenfd > 0) {
        close_socket(ev->listenfd);
    }

    if (ev->pipefds[0] > 0) {
        close_socket(ev->pipefds[0]);
        close_socket(ev->pipefds[1]);
    }

    ztl_mp_release(ev->conn_mp);

    return rv;
}


void evloop_set_usedata(ztl_evloop_t* evloop, void* userdata)
{
    evloop->userdata = userdata;
}

void* evloop_get_usedata(ztl_evloop_t* evloop)
{
    return evloop->userdata;
}

sockhandle_t evloop_get_listenfd(ztl_evloop_t* evloop)
{
    return evloop->listenfd;
}

