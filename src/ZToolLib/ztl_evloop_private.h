/*
 * Copyright(C) Yingzhi Zheng.
 * Copyright(C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_EVENT_LOOP_PRIVATE_H_
#define _ZTL_EVENT_LOOP_PRIVATE_H_

#include <stdint.h>

#include "ztl_network.h"
#include "ztl_evloop.h"
#include "ztl_mempool.h"
#include "ztl_threads.h"

#define ZTL_DEF_POLL_TIMEOUT_MS     200
#define ZTL_DEF_CONN_INIT_COUNT     256

typedef struct ztl_event_ops        ztl_event_ops_t;
typedef struct ztl_timer_event_st   ztl_timer_event_t;

typedef struct ztl_fired_event_st {
    sockhandle_t    fd;
    int             events;
}ztl_fired_event_t;

struct ztl_timer_event_st {
    ztl_rbtree_node_t           node;
    struct ztl_timer_event_st*  prev;
    struct ztl_timer_event_st*  next;
    ztl_timer_handler_t         handler;
    ztl_timer_finalizer_t       finalizer;
    void*                       udata;
    uint64_t                    timer_id;
    int32_t                     timeout_ms;
};

struct ztl_evloop_st
{
    sockhandle_t        pipe_fd[2];
    int                 timeout_ms;
    int                 running;
    uint32_t            looponce;
    uint64_t            timer_id;
    void*               userdata;
    sockhandle_t        listen_fd;  // will be removed

    ztl_timer_event_t*  idle_timers;
    ztl_timer_event_t*  work_timers;
    ztl_evtimer_t       timers;
    uint64_t            timepoint;
    uint32_t            timer_count;

    int                 event_size;
    ztl_fired_event_t*  fired_events;
#ifdef _MSC_VER
    ztl_connection_t**  connections;  // FIXME: use a hash table?
#else
    ztl_connection_t**  connections;
#endif//_MSC_VER
    ztl_thread_mutex_t  lock;
    int                 io_thread_id;

    void*               evops_ctx;
    ztl_event_ops_t*    evops;
};

struct ztl_event_ops
{
    int(*init)(void** evops_ctx);
    int(*start)(void* evops_ctx);

    int(*add)(void* evops_ctx, sockhandle_t fd, int reqevents, int flags);
    int(*del)(void* evops_ctx, sockhandle_t fd, int delevents, int flags);
    int(*poll)(void* evops_ctx, ztl_fired_event_t* fired_events, int size, int ms);

    int(*stop)(void* evops_ctx);
    int(*destroy)(void* evops_ctx);

    const char* name;
};


/* some common native helpers */
int ztl_do_recv(ztl_connection_t* conn);

int ztl_do_send(ztl_connection_t* conn);

ztl_connection_t* ztl_do_accept(ztl_evloop_t* evloop, sockhandle_t listenfd);

ztl_timer_event_t* ztl_timer_node_new(ztl_evloop_t* evloop);
ztl_timer_event_t* ztl_timer_node_find(ztl_evloop_t* evloop, uint64_t timer_id);
int ztl_timer_node_save(ztl_evloop_t* evloop, ztl_timer_event_t* node);
int ztl_timer_node_remove(ztl_evloop_t* evloop, ztl_timer_event_t* node);
int ztl_timer_node_free_all(ztl_evloop_t* evloop);


#endif//_ZTL_EVENT_LOOP_PRIVATE_H_
