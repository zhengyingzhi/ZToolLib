#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "ztl_evloop_private.h"

#include "ztl_mempool.h"
#include "ztl_evtimer.h"
#include "ztl_network.h"
#include "ztl_atomic.h"
#include "ztl_utils.h"

#ifdef _WIN32

#include <process.h>


#define _ZTL_MAX_CONN_OBJ   65536

typedef struct select_ctx_st
{
    int    maxfd;
    fd_set in_read_set;
    fd_set in_write_set;
    fd_set in_except_set;
    fd_set out_read_set;
    fd_set out_write_set;
    fd_set out_except_set;
    ztl_connection_t* conn_map[65536];
}select_ctx_t;

#define ZTL_THE_CTX(evl)      ((select_ctx_t*)evl->ctx)


static int select_init(ztl_evloop_t* evloop);
static int select_start(ztl_evloop_t* evloop);
static int select_add(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents);
static int select_del(ztl_evloop_t* evloop, ztl_connection_t* conn);
static int select_poll(ztl_evloop_t* evloop, int timeoutMS);
static int select_stop(ztl_evloop_t* evloop);
static int select_destroy(ztl_evloop_t* evloop);

static int _select_do_accept(ztl_evloop_t* evloop);
static int _select_process(ztl_evloop_t* evloop, select_ctx_t* pctx);
static int _select_do_poll(ztl_evloop_t* evloop, select_ctx_t* pctx, int timeoutMS);

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

static inline void _set_connection(select_ctx_t* pctx, ztl_connection_t* conn, sockhandle_t sockfd) {
    pctx->conn_map[sockfd] = conn;
}

static inline ztl_connection_t* _get_connection(select_ctx_t* pcts, int sockfd) {
    return pcts->conn_map[sockfd];
}

static inline void _fd_set_copy(struct fd_set *out, const struct fd_set *in) {
    out->fd_count = in->fd_count;
    memcpy(out->fd_array, in->fd_array, in->fd_count * (sizeof(SOCKET)));
}

/// work thread entry
#ifdef _WIN32
static unsigned __stdcall _select_loop_entry(void* arg)
#else
static void* _select_loop_entry(void* arg)
#endif
{
    ztl_evloop_t* evloop = (ztl_evloop_t*)arg;
    select_ctx_t* lpctx = ZTL_THE_CTX(evloop);

    int    num;

    while (evloop->running)
    {
        ztl_evloop_update_polltime(evloop);
        ztl_evloop_expire(evloop);

        num = select_poll(evloop, evloop->timeoutMS);
        if (num < 0) {
            break;
        }

        if (evloop->looponce)
            evloop->handler(evloop, NULL, ZEV_LOOPONCE);
    }
    return 0;
}

static int _select_do_accept(ztl_evloop_t* evloop)
{
    ztl_connection_t* conn;
    while (evloop->running)
    {
        conn = ztl_do_accept(evloop);
        if (!conn) {
            break;
        }

        // set other member
        _set_connection(ZTL_THE_CTX(evloop), conn, conn->sockfd);

        evloop->handler(evloop, conn, ZEV_NEWCONN);
    }
    return 0;
}

static int _select_process(ztl_evloop_t* evloop, select_ctx_t* pctx)
{
    uint32_t i;
    SOCKET* lfdarray;
    ztl_connection_t* lpConn;

    lfdarray = pctx->out_read_set.fd_array;
    for (i = 0; i < pctx->out_read_set.fd_count; ++i)
    {
        // handle read event
        lpConn = _get_connection(pctx, lfdarray[i]);

        // notify client
        lpConn->handler(evloop, lpConn, ZEV_POLLIN);

        // we could release the resource when all event processed
        if (lpConn->disconncted)
        {
            // remove from sets, conn_map
            select_del(evloop, lpConn);
            _set_connection(pctx, NULL, lpConn->sockfd);

            close_socket(lpConn->sockfd);
            lpConn->sockfd = INVALID_SOCKET;

            ztl_mp_free(evloop->conn_mp, lpConn);
        }
    }//for

    lfdarray = pctx->out_write_set.fd_array;
    for (i = 0; i < pctx->out_write_set.fd_count; ++i)
    {
        lpConn = _get_connection(pctx, lfdarray[i]);
        if (!lpConn || lpConn->disconncted == 1) {
            continue;
        }

        lpConn->handler(evloop, lpConn, ZEV_POLLOUT);
    }

    lfdarray = pctx->out_except_set.fd_array;
    for (i = 0; i < pctx->out_except_set.fd_count; ++i)
    {
        // none
    }
    return 0;
}

static int _select_do_poll(ztl_evloop_t* evloop, select_ctx_t* pctx, int timeoutMS)
{
    int num;
    struct timeval ltv;
    _fd_set_copy(&pctx->out_read_set, &pctx->in_read_set);
    _fd_set_copy(&pctx->out_write_set, &pctx->in_write_set);
    _fd_set_copy(&pctx->out_except_set, &pctx->in_except_set);

    ltv.tv_sec = timeoutMS / 1000;
    ltv.tv_usec = (timeoutMS % 1000) * 1000;
    num = select(0, &pctx->out_read_set, &pctx->out_write_set, &pctx->out_except_set, &ltv);
    return num;
}


static int select_init(ztl_evloop_t* evloop)
{
    select_ctx_t* lpctx;
    lpctx = (select_ctx_t*)malloc(sizeof(select_ctx_t));
    if (!lpctx)
    {
        return -1;
    }

    memset(lpctx, 0, sizeof(select_ctx_t));
    evloop->ctx = lpctx;

    lpctx->maxfd = evloop->listen_conn.sockfd;

    FD_ZERO(&lpctx->in_read_set);
    FD_ZERO(&lpctx->in_write_set);
    FD_ZERO(&lpctx->in_except_set);
    FD_ZERO(&lpctx->out_read_set);
    FD_ZERO(&lpctx->out_write_set);
    FD_ZERO(&lpctx->out_except_set);

    memset(lpctx->conn_map, 0, sizeof(ztl_connection_t*) * _ZTL_MAX_CONN_OBJ);

    return 0;
}

static int select_start(ztl_evloop_t* evloop)
{
    select_ctx_t* lpctx = ZTL_THE_CTX(evloop);

#if 0
    if (evloop->thrnum <= 0) {
        evloop->thrnum = get_cpu_number();
    }
#endif//0
    evloop->thrnum = 1;     // for easy debug

    // start work threads
    for (int i = 0; i < evloop->thrnum; ++i)
    {
#ifdef _WIN32
        _beginthreadex(NULL, 0, _select_loop_entry, evloop, 0, NULL);
#else
        pthread_t ltid;
        pthread_create(&ltid, NULL, _select_loop_entry, evloop);
#endif//_WIN32
    }
    return 0;
}

static int select_add(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents)
{
    select_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    if (conn->added == 0)
    {
        _set_connection(lpctx, conn, conn->sockfd);
        conn->added = 1;
    }

#ifndef _WIN32
    if (lpctx->maxfd < conn->sockfd)
        lpctx->maxfd = conn->sockfd;
#endif

    if (reqevents & ZEV_POLLIN)
    {
        FD_SET(conn->sockfd, &lpctx->in_read_set);
    }

    if (reqevents & ZEV_POLLOUT)
    {
        FD_SET(conn->sockfd, &lpctx->in_write_set);
    }
    return 0;
}

static int select_del(ztl_evloop_t* evloop, ztl_connection_t* conn)
{
    select_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    conn->added = 0;

    FD_CLR(conn->sockfd, &lpctx->in_read_set);
    FD_CLR(conn->sockfd, &lpctx->in_write_set);
    FD_CLR(conn->sockfd, &lpctx->in_except_set);

    uint32_t lmaxfd = 0;

    SOCKET* lfdarray;

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

static int select_poll(ztl_evloop_t* evloop, int timeoutMS)
{
    select_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    int num = _select_do_poll(evloop, lpctx, timeoutMS);
    if (num > 0) {
        _select_process(evloop, lpctx);
    }

    return num;
}

static int select_stop(ztl_evloop_t* evloop)
{
    if (NULL == evloop) {
        return -1;
    }

    select_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    ztl_atomic_set(&evloop->running, 0);

    if (INVALID_SOCKET != evloop->listen_conn.sockfd) {
        close_socket(evloop->listen_conn.sockfd);
        evloop->listen_conn.sockfd = INVALID_SOCKET;
    }

    while ((uint32_t)ztl_atomic_add(&evloop->nexited, 0) < (uint32_t)evloop->thrnum) {
        Sleep(1);
    }
    return 0;
}

static int select_destroy(ztl_evloop_t* evloop)
{
    if (evloop == NULL) {
        return -1;
    }

#if 0
    select_ctx_t* lpctx = ZTL_THE_CTX(evloop);

EV_DESTROY_FINISH:
    ztl_mp_destroy(lpctx->iodata_mp);
#endif

    return 0;
}


#endif//_WIN32

