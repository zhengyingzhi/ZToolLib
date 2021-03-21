#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ztl_array.h"
#include "ztl_atomic.h"
#include "ztl_errors.h"
#include "ztl_palloc.h"
#include "ztl_producer_consumer.h"
#include "ztl_event_dispatcher.h"


typedef ztl_producer_consumer_t ztl_pccore_t;

typedef struct {
    ztl_evd_handler_pt  handler;
    void*               ctx;
}ztl_evd_data_t;

struct ztl_event_dispatcher_st {
    ztl_pccore_t*       zpccore;
    void*               udata;
    ztl_pool_t*         pool;
    ztl_array_t*        evtalbe[ZS_EVENT_DISPATCHER_MAX_EVID];
};


static bool _ztl_pc_handler(ztl_producer_consumer_t* zpc, int64_t type, void* data)
{
    uint32_t evtype = (uint32_t)type;

    ztl_event_dispatcher_t* zevd;
    zevd = ztl_pc_get_udata(zpc);

    ztl_evd_do_callback(zevd, evtype, data);

    return true;
}



ztl_event_dispatcher_t* ztl_evd_create(unsigned int quesize)
{
    ztl_event_dispatcher_t* zevd;
    ztl_pool_t* pool;
    
    pool = ztl_create_pool(4096);
    zevd = (ztl_event_dispatcher_t*)ztl_pcalloc(pool, sizeof(ztl_event_dispatcher_t));

    zevd->pool = pool;
    zevd->zpccore = ztl_pc_create(quesize);
    ztl_pc_set_udata(zevd->zpccore, zevd);

    return zevd;
}

int ztl_evd_start(ztl_event_dispatcher_t* zevd)
{
    if (zevd->zpccore)
        return ztl_pc_start(zevd->zpccore);
    return 0;
}

int ztl_evd_register(ztl_event_dispatcher_t* zevd, 
    void* ctx, uint32_t evtype, ztl_evd_handler_pt handler)
{
    ztl_array_t*    events;
    ztl_evd_data_t* evobj;

    if (evtype >= ZS_EVENT_DISPATCHER_MAX_EVID) {
        return ZTL_ERR_OutOfEVID;
    }

    events = zevd->evtalbe[evtype];
    if (!events) {
        events = (ztl_array_t*)ztl_pcalloc(zevd->pool, sizeof(ztl_array_t));
        ztl_array_init(events, zevd->pool, 8, sizeof(ztl_evd_data_t));
        zevd->evtalbe[evtype] = events;
    }

    evobj = (ztl_evd_data_t*)ztl_array_push(events);
    evobj->handler = handler;
    evobj->ctx = ctx;

    return 0;
}

int ztl_evd_post(ztl_event_dispatcher_t* zevd, uint32_t evtype, void* evdata)
{
    int rv;
    if (evtype >= ZS_EVENT_DISPATCHER_MAX_EVID) {
        return ZTL_ERR_OutOfEVID;
    }

    if (zevd->zpccore) {
        rv = ztl_pc_post(zevd->zpccore, _ztl_pc_handler, evtype, evdata);
        if (rv == -1)
            rv = ZTL_ERR_QueueFull;
        else if (rv == -2)
            rv = ZTL_ERR_NotStartThread;
    }
    else {
        rv = ztl_evd_do_callback(zevd, evtype, evdata);
    }

    return rv;
}

int ztl_evd_do_callback(ztl_event_dispatcher_t* zevd, uint32_t evtype, void* evdata)
{
    if (evtype >= ZS_EVENT_DISPATCHER_MAX_EVID) {
        return ZTL_ERR_OutOfEVID;
    }

    ztl_evd_data_t* evobj;
    ztl_array_t*    events;
    events = zevd->evtalbe[evtype];
    if (!events) {
        return ZTL_ERR_NotRegistered;
    }

    for (uint32_t i = 0; i < ztl_array_size(events); ++i)
    {
        evobj = (ztl_evd_data_t*)ztl_array_at(events, i);
        if (evobj->handler)
            evobj->handler(zevd, evobj->ctx, evtype, evdata);
    }

    return 0;
}

int ztl_evd_stop(ztl_event_dispatcher_t* zevd)
{
    if (zevd->zpccore) {
        return ztl_pc_stop(zevd->zpccore);
    }

    return 0;
}

void ztl_evd_release(ztl_event_dispatcher_t* zevd)
{
    if (!zevd) {
        return;
    }

    ztl_evd_stop(zevd);

    ztl_pc_release(zevd->zpccore);
    zevd->zpccore = NULL;

    if (zevd->pool) {
        ztl_destroy_pool(zevd->pool);
    }

}

void  ztl_evd_set_udata(ztl_event_dispatcher_t* zevd, void* udata)
{
    zevd->udata = udata;
}

void* ztl_evd_get_udata(ztl_event_dispatcher_t* zevd)
{
    return zevd->udata;
}
