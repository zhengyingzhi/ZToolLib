#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ztl_atomic.h"
#include "lockfreequeue.h"
#include "ztl_threads.h"
#include "ztl_simple_event.h"

#include "ztl_producer_consumer.h"

typedef struct 
{
    ztl_pc_handler_pt handler;
    void*   data;
    int64_t type;
}ztl_pc_data_t;

struct ztl_producer_consumer_st
{
    lfqueue_t*          queue;
    ztl_simevent_t*     event;
    void*               udata;
    ztl_thread_t        thr;
    int32_t             started;
    uint32_t            size;
    volatile uint32_t   count;
};

static bool _ztl_pc_handler_empty(ztl_producer_consumer_t* zpc, int64_t type, void* data)
{
    (void)zpc;
    (void)type;
    (void)data;
    return false;
}

/* the consumer thread routine
 */
static ztl_thread_result_t ZTL_THREAD_CALL _zpc_work_thread(void* arg)
{
    ztl_producer_consumer_t* zpc;
    ztl_pc_data_t pcdata;

    zpc = (ztl_producer_consumer_t*)arg;

    while (true)
    {
        if (lfqueue_pop(zpc->queue, (void**)&pcdata) != 0) {
            ztl_simevent_timedwait(zpc->event, 1);
            continue;
        }
        ztl_atomic_dec(&zpc->count, 1);

        if (pcdata.handler &&
            !pcdata.handler(zpc, pcdata.type, pcdata.data)) {
            break;
        }
    }

    return 0;
}


ztl_producer_consumer_t* ztl_pc_create(unsigned int quesize)
{
    ztl_producer_consumer_t* zpc;
    zpc = (ztl_producer_consumer_t*)malloc(sizeof(ztl_producer_consumer_t));
    memset(zpc, 0, sizeof(ztl_producer_consumer_t));

    zpc->queue = lfqueue_create(quesize, sizeof(ztl_pc_data_t));
    zpc->size  = quesize;
    zpc->event = ztl_simevent_create();

    return zpc;
}

int ztl_pc_start(ztl_producer_consumer_t* zpc)
{
    if (zpc->started) {
        return 1;
    }

    ztl_thread_create(&zpc->thr, NULL, _zpc_work_thread, zpc);

    zpc->started = 1;
    return 0;
}

int ztl_pc_post(ztl_producer_consumer_t* zpc, ztl_pc_handler_pt handler, int64_t type, void* data)
{
    if (!zpc->started) {
        return -2;
    }

    uint32_t        count;
    ztl_pc_data_t   pcdata;
    pcdata.handler  = handler;
    pcdata.type     = type;
    pcdata.data     = data;

    if (0 != lfqueue_push(zpc->queue, &pcdata)) {
        return -1;
    }

    count = ztl_atomic_add(&zpc->count, 1);
    if (count == 0 || count == (uint32_t)-1) {
        ztl_simevent_signal(zpc->event);
    }

    return 0;
}

int ztl_pc_stop(ztl_producer_consumer_t* zpc)
{
    if (!zpc->started) {
        return -1;
    }

    ztl_pc_post(zpc, _ztl_pc_handler_empty, 0, NULL);

    zpc->started = 0;

    if (zpc->thr)
    {
        void* retval;
        ztl_thread_join(zpc->thr, &retval);
        (void)retval;
    }

    return 0;
}

void ztl_pc_release(ztl_producer_consumer_t* zpc)
{
    if (!zpc) {
        return;
    }

    ztl_pc_stop(zpc);

    if (zpc->queue) {
        lfqueue_release(zpc->queue);
    }
    if (zpc->event) {
        ztl_simevent_release(zpc->event);
    }

    free(zpc);
}

int ztl_pc_pending(ztl_producer_consumer_t* zpc)
{
    return ztl_atomic_add(&zpc->count, 0);
}

void  ztl_pc_set_udata(ztl_producer_consumer_t* zpc, void* udata)
{
    zpc->udata = udata;
}

void* ztl_pc_get_udata(ztl_producer_consumer_t* zpc)
{
    return zpc->udata;
}
