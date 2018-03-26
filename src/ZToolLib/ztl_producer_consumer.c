#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ztl_atomic.h"
#include "lockfreequeue.h"
#include "ztl_mempool.h"
#include "ztl_threads.h"
#include "ztl_simple_event.h"

#include "ztl_producer_consumer.h"

#define ZTL_DEFAULT_PCDATA_NUM   1024

typedef struct 
{
    ztl_pc_handler_pt handler;
    void* data;
}ztl_pc_data_t;

struct ztl_producer_consumer_st
{
    lfqueue_t*          queue;
    ztl_simevent_t*     event;
    ztl_mempool_t*      pool;
    void*               udata;
    ztl_thread_t        thr;
    int32_t             started;
    uint32_t            size;
    volatile uint32_t   count;
};

static bool _ztl_pc_handler_empty(ztl_producer_consumer_t* zpc, void* data)
{
    (void)zpc;
    (void)data;
    return false;
}

static ztl_thread_result_t ZTL_THREAD_CALL _zpc_work_thread(void* arg)
{
    ztl_producer_consumer_t* zpc;
    ztl_pc_data_t pcdata;

    zpc = (ztl_producer_consumer_t*)arg;

    while (true)
    {
        //pcdata = NULL;
        if (0 != lfqueue_pop(zpc->queue, &pcdata)) {
            ztl_simevent_wait(zpc->event);
            continue;
        }
        ztl_atomic_dec(&zpc->count, 1);

        if (!pcdata.handler(zpc, pcdata.data)) {
            break;
        }

        //ztl_mp_free(zpc->pool, pcdata);
    }

    return 0;
}


ztl_producer_consumer_t* ztl_pc_create(uint32_t quesize)
{
    ztl_producer_consumer_t* zpc;
    zpc = (ztl_producer_consumer_t*)malloc(sizeof(ztl_producer_consumer_t));
    memset(zpc, 0, sizeof(ztl_producer_consumer_t));

    //zpc->queue = lfqueue_create(quesize, sizeof(void*));
    zpc->queue = lfqueue_create(quesize, sizeof(ztl_pc_data_t));
    zpc->size  = quesize;

    zpc->event = ztl_simevent_create();

    zpc->pool = ztl_mp_create(sizeof(ztl_pc_data_t), ZTL_DEFAULT_PCDATA_NUM, 1);
    if (zpc->pool == NULL) {
        free(zpc);
        zpc = NULL;
    }

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

int ztl_pc_post(ztl_producer_consumer_t* zpc, ztl_pc_handler_pt handler, void* data)
{
    if (!handler) {
        return -1;
    }
    if (!zpc->started) {
        return -2;
    }

    //TODO: test
#if 0
    ztl_pc_data_t* pcdata;
    pcdata = (ztl_pc_data_t*)ztl_mp_alloc(zpc->pool);
    pcdata->data    = data;
    pcdata->handler = handler;
#else
    ztl_pc_data_t pcdata;
    pcdata.data    = data;
    pcdata.handler = handler;
#endif

    lfqueue_push(zpc->queue, &pcdata);
    if (ztl_atomic_add(&zpc->count, 1) == 0) {
        ztl_simevent_signal(zpc->event);
    }

    return 0;
}

int ztl_pc_stop(ztl_producer_consumer_t* zpc)
{
    if (!zpc->started) {
        return -1;
    }

    ztl_pc_post(zpc, _ztl_pc_handler_empty, NULL);

    zpc->started = 0;

    if (zpc->thr) {
        void* retval;
        ztl_thread_join(zpc->thr, &retval);
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
    if (zpc->pool) {
        ztl_mp_release(zpc->pool);
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
