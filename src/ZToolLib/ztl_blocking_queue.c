#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>

#include "ztl_atomic.h"
#include "ztl_errors.h"
#include "lockfreequeue.h"
#include "ztl_threads.h"
#include "ztl_simple_event.h"

#include "ztl_blocking_queue.h"


struct ztl_blocking_queue_st
{
    lfqueue_t*          queue;
    ztl_simevent_t*     event;
    void*               udata;
    volatile uint32_t   waitors;
};


ztl_blocking_queue_t* ztl_bq_create(uint32_t quesize, uint32_t elemsize)
{
    ztl_blocking_queue_t* zbq = NULL;

    zbq = (ztl_blocking_queue_t*)malloc(sizeof(ztl_blocking_queue_t));
    memset(zbq, 0, sizeof(ztl_blocking_queue_t));

    zbq->queue  = lfqueue_create(quesize, elemsize);
    zbq->event  = ztl_simevent_create();
    zbq->udata  = NULL;
    zbq->waitors= 0;

    return zbq;
}

void ztl_bq_release(ztl_blocking_queue_t* zbq)
{
    if (zbq)
    {
        if (zbq->queue)
            lfqueue_release(zbq->queue);
        if (zbq->event)
            ztl_simevent_release(zbq->event);
        free(zbq);
    }
}


int ztl_bq_push(ztl_blocking_queue_t* zbq, void* datap)
{
    if (lfqueue_push(zbq->queue, datap) == 0)
    {
        if (ztl_atomic_add(&zbq->waitors, 0) > 0) {
            ztl_simevent_signal(zbq->event);
        }
        return 0;
    }
    return ZTL_ERR_QueueFull;
}

int ztl_bq_pop(ztl_blocking_queue_t* zbq, void* datap, int timeout_ms)
{
    int rv;
    if (timeout_ms < 0)
        timeout_ms = INT_MAX;

    ztl_atomic_add(&zbq->waitors, 1);

    do {
        if ((rv = lfqueue_pop(zbq->queue, (void**)datap)) == 0)
            break;

        ztl_simevent_timedwait(zbq->event, 1);

        --timeout_ms;
    } while (timeout_ms > 0);

    ztl_atomic_dec(&zbq->waitors, 1);

    return rv == 0 ? 0 : ZTL_ERR_Timeout;
}

uint32_t ztl_bq_size(ztl_blocking_queue_t* zbq)
{
    return lfqueue_size(zbq->queue);
}

bool ztl_bq_empty(ztl_blocking_queue_t* zbq)
{
    return lfqueue_size(zbq->queue) == 0 ? true : false;
}
