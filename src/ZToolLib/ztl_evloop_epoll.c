#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ztl_evloop_private.h"
#include "ztl_evtimer.h"
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


static int epoll_init(void** evops_ctx);
static int epoll_destroy(void* evops_ctx);
static int epoll_start(void* evops_ctx);
static int epoll_add(void* evops_ctx, sockhandle_t fd, int reqevents, int flags);
static int epoll_del(void* evops_ctx, sockhandle_t fd, int delevents, int flags);
static int epoll_poll(void* evops_ctx, ztl_fired_event_t* fired_events, int ms);
static int epoll_stop(void* evops_ctx);

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


static int epoll_init(void** evops_ctx)
{
    epoll_ctx_t* lpctx;
    lpctx = (epoll_ctx_t*)malloc(sizeof(epoll_ctx_t));
    if (!lpctx)
    {
        return -1;
    }

    memset(lpctx, 0, sizeof(epoll_ctx_t));
    lpctx->epfd = epoll_create(1024);
    if (lpctx->epfd < 0)
    {
        return -1;
    }

    *evops_ctx = lpctx;
    return 0;
}

static int epoll_destroy(void* evops_ctx)
{
    epoll_ctx_t* lpctx;
    lpctx = (epoll_ctx_t*)evops_ctx;
    if (lpctx)
    {
        close(lpctx->epfd);
        free(lpctx);
    }

    return 0;
}

static int epoll_start(void* evops_ctx)
{
    return 0;
}

static int epoll_stop(void* evops_ctx)
{
    return 0;
}

static int epoll_add(void* evops_ctx, sockhandle_t fd, int reqevents, int flags)
{
    epoll_ctx_t* lpctx;
    struct epoll_event ee;
    int op;

    lpctx = (epoll_ctx_t*)evops_ctx;
    op = flags == ZEV_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;

    // merged old events
    reqevents |= flags;
    if (reqevents & ZEV_POLLIN)
        ee.events |= EPOLLIN;
    if (reqevents & ZEV_POLLOUT)
        ee.events |= EPOLLOUT;

    // ee.events = EPOLLONESHOT | EPOLLET;
    ee.data.u64 = 0; /* avoid valgrind warning */
    // ee.data.ptr = conn;
    ee.fd = fd;

    return epoll_ctl(lpctx->epfd, op, fd, &ee);
}

static int epoll_del(void* evops_ctx, sockhandle_t fd, int delevents, int flags)
{
    epoll_ctx_t* lpctx;
    struct epoll_event ee;
    int op;

    lpctx = (epoll_ctx_t*)evops_ctx;
    flags = flags & ~delevents;

    ee.events = 0;
    if (flags & ZEV_POLLIN)
        ee.events |= EPOLLIN;
    if (flags & ZEV_POLLOUT)
        ee.events |= EPOLLOUT;

    // ee.events = EPOLLIN | EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    // ee.data.ptr = conn;
    ee.data.fd = fd;

    op = flags == ZEV_NONE ? EPOLL_CTL_DEL else EPOLL_CTL_MOD;
    return epoll_ctl(lpctx->epfd, op, fd, &ee);
}

static int epoll_poll(void* evops_ctx, ztl_fired_event_t* fired_events, int ms)
{
    int numev, events;
    const int max_nevent = 64;
    struct epoll_event epevents[max_nevent];
    struct epoll_event* lpee;
    epoll_ctx_t* lpctx;
    lpctx = (epoll_ctx_t*)evops_ctx;

    numev = epoll_wait(lpctx->epfd, epevents, max_nevent, ms);
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
        events = 0;
        lpee = epevents + i;
        if (lpee->events & (EPOLLIN | EPOLLERR))
            events |= ZEV_POLLIN;
        if (lpee->events & (EPOLLOUT | EPOLLERR | EPOLLHUP))
            events |= ZEV_POLLOUT;

        fired_events[i].fd = lpee->data.fd;
        fired_events[i].events = lpee->data.events;
    }

    return numev;
}

#endif//__linux__
