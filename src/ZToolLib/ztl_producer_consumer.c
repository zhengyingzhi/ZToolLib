#include <stdlib.h>
#include <string.h>

#include "ztl_threads.h"
#include "lockfreequeue.h"
#include "ztl_simple_event.h"

#include "ztl_producer_consumer.h"


struct ztl_producer_consumer_st
{
    lfqueue_t*          queue;
    ztl_simevent_t*     event;
    void*               udata;
    uint32_t            size;
    volatile uint32_t   count;
};


ztl_producer_consumer_t* ztl_pc_create(uint32_t quesize)
{
    ztl_producer_consumer_t* zpc;
    zpc = (ztl_producer_consumer_t*)malloc(sizeof(ztl_producer_consumer_t));
    return zpc;
}
