/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_PRODUCER_CONSUMER_H_
#define _ZTL_PRODUCER_CONSUMER_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/* the expored types */
typedef struct ztl_producer_consumer_st ztl_producer_consumer_t;

/* the consumer callback handler,
 * @note, if return false, the consumer thread would break
 */
typedef bool(*ztl_pc_handler_pt)(ztl_producer_consumer_t* zpc, void* data);


/* create a producer and consumer model object within a fixed-size queue
 */
ztl_producer_consumer_t* ztl_pc_create(unsigned int quesize);

/* start consumer callback thread
 */
int ztl_pc_start(ztl_producer_consumer_t* zpc);

/* post data to consumer thread
 * @return -1 if queue is full, -2 if the not start working thread
 */
int ztl_pc_post(ztl_producer_consumer_t* zpc, ztl_pc_handler_pt handler, void* data);

/* stop working thread
 */
int ztl_pc_stop(ztl_producer_consumer_t* zpc);

/* release the pc object 
 */
void ztl_pc_release(ztl_producer_consumer_t* zpc);

/* return the pending count in the queue to process
 */
int ztl_pc_pending(ztl_producer_consumer_t* zpc);

/* set or get user context data
 */
void  ztl_pc_set_udata(ztl_producer_consumer_t* zpc, void* udata);
void* ztl_pc_get_udata(ztl_producer_consumer_t* zpc);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_PRODUCER_CONSUMER_H_
