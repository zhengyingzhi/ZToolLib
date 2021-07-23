#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ztl_atomic.h"
#include "ztl_errors.h"
#include "ztl_evloop_private.h"
#include "ztl_network.h"
#include "ztl_mem.h"
#include "ztl_times.h"


#if defined(_WIN32)
extern struct ztl_event_ops iocpops;
extern struct ztl_event_ops selectops;
#elif defined(__linux__)
extern struct ztl_event_ops epollops;
#else
extern struct ztl_event_ops selectops;
#endif//_WIN32


#define ZTL_DEFAULT_CONNECT_SIZE    16384


static ztl_event_ops_t* _event_ops_provider(ZTL_EV_POLL_METHOD method)
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
    if (method == ZTL_EPM_Epoll || method == ZTL_EPM_Default)
        lpops = &epollops;
    else if(method == ZTL_EPM_Select)
        lpops = &selectops;
    else
        lpops = &epollops;
#else
    lpops = NULL;
#endif

    return lpops;
}

static int _pipe_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, int events)
{
    (void)evloop;
    (void)conn;
    (void)events;
    fprintf(stderr, "_pipe_handler\n");
    return 0;
}

//////////////////////////////////////////////////////////////////////////
int ztl_evloop_create(ztl_evloop_t** pevloop, int size)
{
    ztl_event_ops_t*    evops;
    ztl_evloop_t*       evloop;

    evops = _event_ops_provider(ZTL_EPM_Default);
    if (!evops) {
        return ZTL_ERR_NotImpl;
    }

    evloop = (ztl_evloop_t*)ALLOC(sizeof(ztl_evloop_t));
    if (!evloop) {
        *pevloop = NULL;
        return ZTL_ERR_AllocFailed;
    }

    memset(evloop, 0, sizeof(ztl_evloop_t));
    evloop->running     = 0;
    evloop->looponce    = 0;
    evloop->timer_id    = 1;
    evloop->pipe_fd[0]  = INVALID_SOCKET;
    evloop->pipe_fd[1]  = INVALID_SOCKET;

    evloop->event_size  = size > 0 ? size : ZTL_DEFAULT_EVLOOP_SIZE;
    evloop->fired_events= ALLOC(evloop->event_size * sizeof(ztl_fired_event_t));

    // FIXME: use a hash table for <fd, ztl_connection_t>
    evloop->connections = ALLOC(ZTL_DEFAULT_CONNECT_SIZE * sizeof(ztl_connection_t*));
    memset(evloop->connections, 0, ZTL_DEFAULT_CONNECT_SIZE * sizeof(ztl_connection_t*));

    ztl_thread_mutex_init(&evloop->lock, NULL);
    ztl_evtimer_init(&evloop->timers);
    evloop->timepoint = 0;

    evloop->evops = evops;
    *pevloop = evloop;

    return 0;
}

int ztl_evloop_init(ztl_evloop_t* evloop)
{
    int rv;
    // ztl_evloop_make_pipes(evloop);
    rv = evloop->evops->init(&evloop->evops_ctx);
    if (rv != 0) {
        return rv;
    }

    return rv;
}

int ztl_evloop_start(ztl_evloop_t* evloop)
{
    int rv;
    evloop->running = 1;
    rv = evloop->evops->start(evloop->evops_ctx);
    return rv;
}

int ztl_evloop_stop(ztl_evloop_t* evloop)
{
    int rv;
    evloop->running = 0;
    rv = evloop->evops->stop(evloop->evops_ctx);
    return rv;
}

int ztl_evloop_make_pipes(ztl_evloop_t* evloop)
{
    sockhandle_t pipe_fd0;
    pipe_fd0 = evloop->pipe_fd[0];
    if (!IS_VALID_SOCKET(pipe_fd0))
    {
        sockhandle_t pipefds[2];
        make_sockpair(pipefds, AF_INET);
        set_closeonexec(pipefds[0]);
        set_closeonexec(pipefds[1]);

        evloop->pipe_fd[0] = pipefds[0];
        evloop->pipe_fd[1] = pipefds[1];

        return ztl_evloop_add(evloop, pipefds[1], ZEV_POLLIN, _pipe_handler, NULL);
    }
    return 0;
}

int ztl_evloop_add(ztl_evloop_t* evloop, sockhandle_t fd, int reqevents,
                   ztl_ev_handler_t handler, void* udata)
{
    int rv;
    ztl_connection_t* conn;

    if ((int)fd > evloop->event_size)
    {
        fprintf(stderr, "evloop_add fd:%d>size:%d\n", (int)fd, evloop->event_size);
        errno = ERANGE;
        return ZTL_ERR_OutOfRange;
    }

    conn = ztl_connection_find(evloop, fd);
    if (!conn)
    {
        conn = ztl_connection_new(evloop, fd, 0, 0);
        ztl_connection_save(evloop, conn);
    }

    conn->userdata1 = udata;
    conn->events |= reqevents;
    if (reqevents & ZEV_POLLIN)
        conn->read_handler = handler;
    if (reqevents & ZEV_POLLOUT)
        conn->write_handler = handler;

    rv = evloop->evops->add(evloop->evops_ctx, fd, reqevents, conn->events);
    if (rv < 0) {
        return rv;
    }

    // update maxfd ?
    return 0;
}

int ztl_evloop_del(ztl_evloop_t* evloop, sockhandle_t fd, int reqevents)
{
    int rv;
    int flags;
    ztl_connection_t* conn;
    conn = ztl_connection_find(evloop, fd);
    flags = conn ? conn->events : ZEV_NONE;

    rv = evloop->evops->del(evloop->evops_ctx, fd, reqevents, flags);
    return rv;
}

int ztl_evloop_looponce(ztl_evloop_t* evloop, int timeout_ms)
{
    int i, nevents;
    int rfired;
    ztl_fired_event_t*  fired_ev;
    ztl_connection_t*   conn;

    nevents = evloop->evops->poll(evloop->evops_ctx, evloop->fired_events, 64, timeout_ms);
    for (i = 0; i < nevents; ++i)
    {
        rfired = 0;
        fired_ev = evloop->fired_events + i;
        conn = ztl_connection_find(evloop, fired_ev->fd);

        if (conn->events & fired_ev->events & ZEV_POLLIN)
        {
            rfired = 1;
            conn->read_handler(evloop, conn, fired_ev->events);
        }

        if (conn->events & fired_ev->events & ZEV_POLLOUT)
        {
            if (!rfired || conn->read_handler != conn->write_handler)
                conn->write_handler(evloop, conn, fired_ev->events);
        }
    }

    return 0;
}

int ztl_evloop_loop(ztl_evloop_t* evloop, int ms)
{
    evloop->io_thread_id = (int)ztl_thread_self();
    int ms_left;

    while (evloop->running)
    {
        evloop->timepoint = get_timestamp();
        ms_left = ztl_evloop_expire(evloop, evloop->timepoint);
        ztl_evloop_looponce(evloop, ms < ms_left ? ms : ms_left);
    }
    return 0;
}

int ztl_evloop_release(ztl_evloop_t* evloop)
{
    int rv, i;
    ztl_connection_t* conn;

    if (!evloop) {
        return ZTL_ERR_NotCreated;
    }

    if (evloop->running == 0) {
        ztl_evloop_stop(evloop);
    }

    rv = evloop->evops->destroy(evloop->evops_ctx);

    // release all the connections
    for (i = 0; i < ZTL_DEFAULT_CONNECT_SIZE; ++i)
    {
        conn = evloop->connections[i];
        if (conn)
        {
            close_socket(conn->fd);
            conn->fd = INVALID_SOCKET;

            conn->free(conn);
            evloop->connections[i] = NULL;
        }
    }

    ztl_thread_mutex_destroy(&evloop->lock);
    ztl_timer_node_free_all(evloop);
    free(evloop);

    return rv;
}

void* ztl_evloop_get_usedata(ztl_evloop_t* evloop)
{
    return evloop->userdata;
}

void ztl_evloop_set_usedata(ztl_evloop_t* evloop, void* userdata)
{
    evloop->userdata = userdata;
}

//////////////////////////////////////////////////////////////////////////
int ztl_connection_free(ztl_connection_t* conn)
{
    uint32_t refcount;
    refcount = ztl_atomic_dec(&conn->refcount, 1);
    if (refcount != 1) {
        return refcount;
    }

    conn->events = ZEV_NONE;
    if (IS_VALID_SOCKET(conn->fd)) {
        ztl_connection_remove(conn->evloop, conn->fd);
    }

    FREE(conn);

    return 0;
}

int ztl_connection_save(ztl_evloop_t* evloop, ztl_connection_t* conn)
{
    ztl_connection_t* old_conn;
    old_conn = evloop->connections[conn->fd];
    if (old_conn) {
        // pass
    }
    evloop->connections[conn->fd] = conn;
    return 0;
}

ztl_connection_t* ztl_connection_remove(ztl_evloop_t* evloop, sockhandle_t fd)
{
    ztl_connection_t* conn;
    conn = evloop->connections[fd];
    evloop->connections[fd] = NULL;
    return conn;
}

ztl_connection_t* ztl_connection_new(ztl_evloop_t* evloop, sockhandle_t ns,
    uint32_t fd_addr, uint16_t fd_port)
{
    ztl_connection_t* conn;
    conn = (ztl_connection_t*)ALLOC(sizeof(ztl_connection_t));
    memset(conn, 0, sizeof(ztl_connection_t));

    conn->fd        = ns;
    conn->addr      = fd_addr;
    conn->port      = fd_port;
    conn->events    = ZEV_NONE;
    conn->refcount  = 1;
    conn->evloop    = evloop;

    conn->recv = ztl_do_recv;
    conn->send = ztl_do_send;
    conn->free = ztl_connection_free;

    return conn;
}

ztl_connection_t* ztl_connection_find(ztl_evloop_t* evloop, sockhandle_t fd)
{
    ztl_connection_t* conn;
    conn = evloop->connections[fd];
    return conn;
}

void ztl_evloop_lock(ztl_evloop_t* evloop)
{
    ztl_thread_mutex_lock(&evloop->lock);
}

void ztl_evloop_unlock(ztl_evloop_t* evloop)
{
    ztl_thread_mutex_unlock(&evloop->lock);
}

void ztl_evloop_timestamp_update(ztl_evloop_t* evloop)
{
    evloop->timepoint = get_timestamp();
}

uint64_t ztl_evloop_timestamp_get(ztl_evloop_t* evloop)
{
    return evloop->timepoint;
}

//////////////////////////////////////////////////////////////////////////
static void _timer_event_handler(void* ctx, ztl_rbtree_node_t* node)
{
    ztl_evloop_t*       evloop;
    ztl_timer_event_t*  timer;

    evloop = (ztl_evloop_t*)ctx;
    timer = (ztl_timer_event_t*)node;
    if (timer->handler)
        timer->handler(evloop, timer->timer_id, timer->udata);
    if (timer->finalizer)
        timer->finalizer(evloop, timer->timer_id, timer->udata);
    ztl_timer_node_remove(evloop, timer);
}

uint64_t ztl_evloop_addtimer(ztl_evloop_t* evloop, uint32_t timeout_ms,
    ztl_timer_handler_t handler,
    ztl_timer_finalizer_t finalizer, void* udata)
{
    int safe, rv;
    ztl_timer_event_t* timer;

    safe = 1;
    if (evloop->io_thread_id != (int)ztl_thread_self()) {
        safe = 0;
        ztl_thread_mutex_lock(&evloop->lock);
    }

    if (evloop->timepoint == 0)
        evloop->timepoint = get_timestamp();

    timer = ztl_timer_node_new(evloop);
    timer->handler      = handler;
    timer->finalizer    = finalizer;
    timer->udata        = udata;
    timer->timer_id     = ztl_atomic_add64(&evloop->timer_id, 1);
    timer->timeout_ms   = timeout_ms;
    ztl_timer_node_save(evloop, timer);

    rv = ztl_evtimer_add(&evloop->timers, &timer->node, timeout_ms, 0);

    if (!safe)
        ztl_thread_mutex_unlock(&evloop->lock);
    return rv;
}

int ztl_evloop_deltimer(ztl_evloop_t* evloop, uint64_t timer_id)
{
    int safe, rv;
    ztl_timer_event_t* timer;
    timer = ztl_timer_node_find(evloop, timer_id);
    if (!timer) {
        return -1;
    }

    safe = 1;
    if (evloop->io_thread_id != (int)ztl_thread_self()) {
        safe = 0;
        ztl_thread_mutex_lock(&evloop->lock);
    }

    ztl_timer_node_remove(evloop, timer);
    rv = ztl_evtimer_del(&evloop->timers, &timer->node);

    if (timer->finalizer)
        timer->finalizer(evloop, timer_id, timer->udata);

    if (!safe)
        ztl_thread_mutex_unlock(&evloop->lock);
    return rv;
}

int ztl_evloop_expire(ztl_evloop_t* evloop, uint64_t currtime)
{
    int safe, ms_left;
    safe = 1;

#if 0
    if (evloop->io_thread_id != (int)ztl_thread_self()) {
        safe = 0;
        ztl_thread_mutex_lock(&evloop->lock);
    }
#endif//0

    ms_left = ztl_evtimer_expire(&evloop->timers, currtime,
                                 _timer_event_handler, evloop);

    if (!safe)
        ztl_thread_mutex_unlock(&evloop->lock);
    return ms_left;
}

uint32_t ztl_evloop_timer_count(ztl_evloop_t* evloop)
{
    return ztl_atomic_add(&evloop->timers.count, 0);
}
