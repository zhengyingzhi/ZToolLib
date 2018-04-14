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

typedef struct ztl_event_timer_s ztl_event_timer_t;

struct ztl_event_timer_s
{
    ztl_rbtree_t        event_timers;
    ztl_rbtree_node_t   event_timer_sentinel;
    uint64_t            last_time;
};

/* init the timer container
 */
void ztl_event_timer_init(ztl_event_timer_t* et);

/* add a timer into the container
 * @timeoutMS is the timedout millisec from now, not the absolute time
 * @timerset the 'timer' whether added already
 */
int ztl_event_timer_add(ztl_event_timer_t* et, ztl_rbtree_node_t* timer, 
    uint32_t timeoutMS, int timerset);

/* delete the timer from container */
int ztl_event_timer_del(ztl_event_timer_t* et, ztl_rbtree_node_t* timer);

/* expire timedout timers */
void ztl_event_timer_expire(ztl_event_timer_t* et, uint64_t currtime, 
    ztl_evt_handler_pt handler, void* ctx);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_EVENT_TIMER_H_
