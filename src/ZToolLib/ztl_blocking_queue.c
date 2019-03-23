#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>

#include "ztl_atomic.h"
#include "lockfreequeue.h"
#include "ztl_threads.h"
#include "ztl_simple_event.h"

#include "ztl_blocking_queue.h"

typedef struct
{
    void*   data;
    int64_t type;
}ztl_bq_data_t;

struct ztl_blocking_queue_st
{
    lfqueue_t*          queue;
    ztl_simevent_t*     event;
    void*               udata;
    volatile uint32_t   waitors;
};


ztl_blocking_queue_t* ztl_bq_create(int quesize)
{
    ztl_blocking_queue_t* zbq = NULL;

    zbq = (ztl_blocking_queue_t*)malloc(sizeof(ztl_blocking_queue_t));
    memset(zbq, 0, sizeof(ztl_blocking_queue_t));

    zbq->queue  = lfqueue_create(quesize, sizeof(ztl_bq_data_t));
    zbq->event  = ztl_simevent_create();
    zbq->udata  = NULL;
    zbq->waitors= 0;

    return zbq;
}

void ztl_bq_release(ztl_blocking_queue_t* zbq)
{
    if (zbq)
    {
        lfqueue_release(zbq->queue);
        ztl_simevent_release(zbq->event);
        free(zbq);
    }
}


int ztl_bq_push(ztl_blocking_queue_t* zbq, void* datap, int64_t datai)
{
    ztl_bq_data_t ldata = { datap, datai };
    if (lfqueue_push_value(zbq->queue, &ldata) == 0)
    {
        if (ztl_atomic_add(&zbq->waitors, 0) > 0)
        {
            ztl_simevent_signal(zbq->event);
        }
        return 0;
    }
    return -1;
}

int ztl_bq_pop(ztl_blocking_queue_t* zbq, int timeoutMS, void** datap, int64_t* datai)
{
    ztl_bq_data_t ldata = { 0, 0 };

    if (timeoutMS < 0)
        timeoutMS = INT_MAX;

    ztl_atomic_add(&zbq->waitors, 1);

    do {
        if (lfqueue_pop_value(zbq->queue, &ldata) == 0)
            break;

        // wait a while 
        ztl_simevent_timedwait(zbq->event, 1);

        --timeoutMS;
    } while (timeoutMS > 0);

    ztl_atomic_dec(&zbq->waitors, 1);

    if (timeoutMS == 0)
    {
        return 0;
    }
    else
    {
        if (datap)
            *datap = ldata.data;
        if (datai)
            *datai = ldata.type;
        return 1;
    }
}

int ztl_bq_size(ztl_blocking_queue_t* zbq)
{
    return lfqueue_size(zbq->queue);
}

bool ztl_bq_empty(ztl_blocking_queue_t* zbq)
{
    return lfqueue_size(zbq->queue) == 0 ? true : false;
}
