#include <math.h>
#include "ztl_event_timer.h"


void ztl_event_timer_init(ztl_event_timer_t* et)
{
    ztl_rbtree_init(&et->event_timers, &et->event_timer_sentinel, 
        ztl_rbtree_insert_timer_value);
}

int ztl_event_timer_add(ztl_event_timer_t* et, ztl_rbtree_node_t* timer, 
    uint32_t aTimeoutMS, int aTimerSet)
{
    ztl_msec_t      key;
    ztl_msec_int_t  diff;

    if (aTimeoutMS == 0) {
        return -1;
    }

    /* Currently no lock, since event timer is only working at IO thread */

    key = et->last_time + aTimeoutMS;

    if (aTimerSet) {

        /*
        * Use a previous timer value if difference between it and a new
        * value is less than EG_TIMER_LAZY_DELAY milliseconds: this allows
        * to minimize the rbtree operations for fast connections.
        */

        diff = (ztl_msec_int_t)(key - timer->key);
        if ((ztl_msec_t)abs(diff) < ZTL_TIMER_INFINITE) {
            return 1;
        }

        ztl_event_timer_del(et, timer);
    }

    timer->key = key;
    ztl_rbtree_insert(&et->event_timers, timer);
    return 0;
}

int ztl_event_timer_del(ztl_event_timer_t* et, ztl_rbtree_node_t* timer)
{
    ztl_rbtree_delete(&et->event_timers, timer);

#if defined(ZTL_DEBUG)
    timer->left = NULL;
    timer->right = NULL;
    timer->parent = NULL;
#endif

    return 0;
}

void ztl_event_timer_expire(ztl_event_timer_t* et, uint64_t currtime, 
    ztl_evt_handler_pt handler, void* ctx)
{
    ztl_rbtree_node_t  *node, *root, *sentinel;

    sentinel = et->event_timers.sentinel;

    et->last_time = currtime;

    for (;; ) {

        root = et->event_timers.root;
        if (root == sentinel) {
            return;
        }

        node = ztl_rbtree_min(root, sentinel);

        /* node->key > ngx_current_time */
        if ((ztl_msec_int_t)(node->key - et->last_time) > 0) {
            break;
        }

        //fprintf(stderr, "ztl_expire_event_timer {}", node->udata);

        ztl_rbtree_delete(&et->event_timers, node);

        if (handler)
            handler(ctx, node);
    }
}
