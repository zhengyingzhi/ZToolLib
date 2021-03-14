/*
 * Copyright(C) Yingzhi Zheng.
 * Copyright(C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_EVENT_TIMER_H_
#define _ZTL_EVENT_TIMER_H_

#include <stdint.h>
#include <stdbool.h>

#include "ztl_rbtree.h"

#define ZTL_TIMER_INFINITE       (ztl_msec_t) -1
#define ZTL_TIMER_LAZY_DELAY     300

#ifdef __cplusplus
extern "C" {
#endif
    
typedef void(*ztl_evt_handler_pt)(void* ctx, ztl_rbtree_node_t* node);

typedef struct ztl_evtimer_s ztl_evtimer_t;

struct ztl_evtimer_s
{
    ztl_rbtree_t        event_timers;
    ztl_rbtree_node_t   event_timer_sentinel;
    uint64_t            last_time;
};

/* init the timer container
 */
void ztl_evtimer_init(ztl_evtimer_t* et);

void ztl_evtimer_update_time(ztl_evtimer_t* et, uint64_t currtime);

/* add a timer into the container
 * @timeout_ms is the timedout millisec from now, not the absolute time
 * @timerset the 'timer' flag whether added already
 */
int ztl_evtimer_add(ztl_evtimer_t* et, ztl_rbtree_node_t* timer,
    uint32_t timeout_ms, int timerset);

/* delete the timer from container */
int ztl_evtimer_del(ztl_evtimer_t* et, ztl_rbtree_node_t* timer);

/* expire timedout timers,
 * @return the nearest time ms left, -1 is none
 */
int ztl_evtimer_expire(ztl_evtimer_t* et, uint64_t currtime,
    ztl_evt_handler_pt handler, void* ctx);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_EVENT_TIMER_H_
