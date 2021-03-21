#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "ztl_evloop_private.h"

#include "ztl_atomic.h"
#include "ztl_errors.h"
#include "ztl_evtimer.h"
#include "ztl_mempool.h"
#include "ztl_network.h"
#include "ztl_utils.h"

#ifdef _WIN32

typedef struct select_ctx_st
{
    sockhandle_t    maxfd;
    fd_set in_read_set;
    fd_set in_write_set;
    fd_set in_except_set;
    fd_set out_read_set;
    fd_set out_write_set;
    fd_set out_except_set;
}select_ctx_t;


static int select_init(void** evops_ctx);
static int select_start(void* evops_ctx);
static int select_add(void* evops_ctx, sockhandle_t fd, int reqevents, int flags);
static int select_del(void* evops_ctx, sockhandle_t fd, int delevents, int flags);
static int select_poll(void* evops_ctx, ztl_fired_event_t* fired_events, int size, int ms);
static int select_stop(void* evops_ctx);
static int select_destroy(void* evops_ctx);


/* the impl interfaces */
struct ztl_event_ops selectops = {
    select_init,
    select_start,
    select_add,
    select_del,
    select_poll,
    select_stop,
    select_destroy,
    "select",
};


static inline void _fd_set_copy(struct fd_set *out, const struct fd_set *in) {
    out->fd_count = in->fd_count;
    memcpy(out->fd_array, in->fd_array, in->fd_count * (sizeof(SOCKET)));
}


static int select_init(void** evops_ctx)
{
    select_ctx_t* lpctx;
    lpctx = (select_ctx_t*)malloc(sizeof(select_ctx_t));
    if (!lpctx) {
        return ZTL_ERR_AllocFailed;
    }

    memset(lpctx, 0, sizeof(select_ctx_t));
    FD_ZERO(&lpctx->in_read_set);
    FD_ZERO(&lpctx->in_write_set);
    FD_ZERO(&lpctx->in_except_set);
    FD_ZERO(&lpctx->out_read_set);
    FD_ZERO(&lpctx->out_write_set);
    FD_ZERO(&lpctx->out_except_set);

    *evops_ctx = lpctx;
    return 0;
}

static int select_start(void* evops_ctx)
{
    return 0;
}

static int select_add(void* evops_ctx, sockhandle_t fd, int reqevents, int flags)
{
    (void)flags;

    select_ctx_t* lpctx;
    lpctx = (select_ctx_t*)evops_ctx;

#ifndef _WIN32
    if (lpctx->maxfd < fd)
        lpctx->maxfd = fd;
#endif

    if (reqevents & ZEV_POLLIN)
        FD_SET(fd, &lpctx->in_read_set);
    if (reqevents & ZEV_POLLOUT)
        FD_SET(fd, &lpctx->in_write_set);
    return 0;
}

static int select_del(void* evops_ctx, sockhandle_t fd, int delevents, int flags)
{
    (void)flags;

    select_ctx_t* lpctx;
    sockhandle_t* lfdarray;
    sockhandle_t  lmaxfd;

    lpctx = (select_ctx_t*)evops_ctx;
    lmaxfd = 0;

    if (delevents & ZEV_POLLIN)
        FD_CLR(fd, &lpctx->in_read_set);
    if (delevents & ZEV_POLLOUT)
        FD_CLR(fd, &lpctx->in_write_set);
    FD_CLR(fd, &lpctx->in_except_set);

    // FIXME: re-find the max fd
    lfdarray = lpctx->in_read_set.fd_array;
    for (uint32_t i = 0; i < lpctx->in_read_set.fd_count; ++i)
    {
        if (lfdarray[i] > lmaxfd)
            lmaxfd = lfdarray[i];
    }

    lfdarray = lpctx->in_write_set.fd_array;
    for (uint32_t i = 0; i < lpctx->in_write_set.fd_count; ++i)
    {
        if (lfdarray[i] > lmaxfd)
            lmaxfd = lfdarray[i];
    }

    lpctx->maxfd = lmaxfd;

    return 0;
}

static int select_poll(void* evops_ctx, ztl_fired_event_t* fired_events, int size, int ms)
{
    uint32_t num, i;
    struct timeval ltv;
    select_ctx_t* lpctx;
    // sockhandle_t* lfdarray;

    lpctx = (select_ctx_t*)evops_ctx;

    _fd_set_copy(&lpctx->out_read_set, &lpctx->in_read_set);
    _fd_set_copy(&lpctx->out_write_set, &lpctx->in_write_set);
    _fd_set_copy(&lpctx->out_except_set, &lpctx->in_except_set);

    ltv.tv_sec = ms / 1000;
    ltv.tv_usec = (ms % 1000) * 1000;
    num = select(0, &lpctx->out_read_set,
                    &lpctx->out_write_set,
                    &lpctx->out_except_set, &ltv);
    if (num <= 0) {
        return num;
    }

    num = ztl_max(lpctx->out_read_set.fd_count, lpctx->out_write_set.fd_count);
    for (i = 0; i < num; ++i)
    {
        fired_events[i].events = 0;
        if (i < lpctx->out_read_set.fd_count)
        {
            fired_events[i].fd = lpctx->out_read_set.fd_array[i];
            fired_events[i].events |= ZEV_POLLIN;
        }
        if (i < lpctx->out_write_set.fd_count)
        {
            fired_events[i].fd = lpctx->out_write_set.fd_array[i];
            fired_events[i].events |= ZEV_POLLOUT;
        }
    }

    return num;
}

static int select_stop(void* evops_ctx)
{
    return 0;
}

static int select_destroy(void* evops_ctx)
{
    select_ctx_t* lpctx;
    lpctx = (select_ctx_t*)evops_ctx;

    if (lpctx) {
        free(lpctx);
    }
    return 0;
}

#endif//_WIN32
