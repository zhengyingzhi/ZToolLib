#ifndef _ZTL_PRODUCER_CONSUMER_H_
#define _ZTL_PRODUCER_CONSUMER_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct ztl_producer_consumer_st ztl_producer_consumer_t;

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

typedef bool(*ztl_pc_handler_pt)(ztl_producer_consumer_t* zpc, void* data);

typedef struct  
{
    void* data;
    ztl_pc_handler_pt handler;
}ztl_pc_data_t;

/* create a producer and consumer model */
ztl_producer_consumer_t* ztl_pc_create(uint32_t quesize);

/* start consumer callback thread */
int ztl_pc_start(ztl_producer_consumer_t* zpc);

/* post data to consumer thread */
int ztl_pc_post(ztl_producer_consumer_t* zpc, void* data, ztl_pc_handler_pt);

/* stop working */
int ztl_pc_stop(ztl_producer_consumer_t* zpc);

/* release the pc object */
void ztl_pc_release(ztl_producer_consumer_t* zpc);

/* set or get user context data */
void  ztl_pc_set_udata(ztl_producer_consumer_t* zpc, void* udata);
void* ztl_pc_get_udata(ztl_producer_consumer_t* zpc);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_PRODUCER_CONSUMER_H_
