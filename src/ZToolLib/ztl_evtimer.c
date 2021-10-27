#include <stdlib.h>

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "ztl_atomic.h"
#include "ztl_errors.h"
#include "ztl_evtimer.h"


void ztl_evtimer_init(ztl_evtimer_t* et)
{
    et->last_time = 0;
    et->count = 0;
    rbtree_init(&et->event_timers, &et->event_timer_sentinel, 
                rbtree_insert_timer_value);
}

void ztl_evtimer_update_time(ztl_evtimer_t* et, uint64_t currtime)
{
    et->last_time = currtime;
}

int ztl_evtimer_add(ztl_evtimer_t* et, rbtree_node_t* timer, 
    uint32_t timeout_ms, int timerset)
{
    msec_t      key;
    msec_int_t  diff;

    if (timeout_ms == 0) {
        return ZTL_ERR_InvalParam;
    }

    /* Currently no lock, since event timer is only working at IO thread */

    key = et->last_time + timeout_ms;

    if (timerset) {

        /*
        * Use a previous timer value if difference between it and a new
        * value is less than ZTL_TIMER_LAZY_DELAY milliseconds: this allows
        * to minimize the rbtree operations for fast connections.
        */

        diff = (msec_int_t)(key - timer->key);
        if ((msec_t)abs((int)diff) < ZTL_TIMER_LAZY_DELAY) {
            return 1;
        }

        ztl_evtimer_del(et, timer);
    }

    timer->key = key;
    rbtree_insert(&et->event_timers, timer);
    atomic_add(&et->count, 1);
    return 0;
}

int ztl_evtimer_del(ztl_evtimer_t* et, rbtree_node_t* timer)
{
    rbtree_delete(&et->event_timers, timer);
    atomic_dec(&et->count, 1);

#if defined(ZTL_DEBUG)
    timer->left = 0;
    timer->right = 0;
    timer->parent = 0;
#endif

    return 0;
}

rbtree_node_t* ztl_evtimer_min(ztl_evtimer_t* et)
{
    rbtree_node_t* root;
    root = et->event_timers.root;
    if (root == et->event_timers.sentinel) {
        return NULL;
    }

    return rbtree_min(root, et->event_timers.sentinel);
}

msec_int_t ztl_evtimer_min_ms(ztl_evtimer_t* et, uint64_t currtime)
{
    rbtree_node_t* node;
    node = ztl_evtimer_min(et);
    if (node) {
        if (currtime == 0)
            currtime = et->last_time;
        return (msec_int_t)(node->key - et->last_time);
    }
    return -1;
}

int ztl_evtimer_expire(ztl_evtimer_t* et, uint64_t currtime,
    ztl_evt_handler_pt handler, void* ctx)
{
    rbtree_node_t  *node, *root, *sentinel;

    sentinel = et->event_timers.sentinel;

    et->last_time = currtime;

    for (;; ) {

        root = et->event_timers.root;
        if (root == sentinel) {
            return ZTL_ERR_Empty;
        }

        node = rbtree_min(root, sentinel);

        /* node->key > current_time means not reached */
        if ((msec_int_t)(node->key - et->last_time) > 0) {
            return (int)(node->key - et->last_time);
        }

        //fprintf(stderr, "ztl_expire_event_timer {}", node->udata);

        rbtree_delete(&et->event_timers, node);

        if (handler)
            handler(ctx, node);
    }

    // cannot run here!!
    return -1;
}
