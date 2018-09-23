/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_BLOCKING_QUEUE_H_
#define _ZTL_BLOCKING_QUEUE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* exported types */

/* a lock free queue based on a circular array
 * supports multiple producers and multiple consumers
 */
typedef struct ztl_blocking_queue_st ztl_blocking_queue_t;


/* create a blocking queue for multiple threads,
 * implemented by a fixed size lock free queue
 */
ztl_blocking_queue_t* ztl_bq_create(int quesize);

/* release the blocking queue
 */
void ztl_bq_release(ztl_blocking_queue_t* zbq);

/* try push data into queue, and notify waitors
 * return 0 if success
 */
int ztl_bq_push(ztl_blocking_queue_t* zbq, void* datap, int64_t datai);

/* try pop data from queue with timeout
 * return 1 if success, 0 if timeout, other if error
 */
int ztl_bq_pop(ztl_blocking_queue_t* zbq, int timeoutMS, void** datap, int64_t* datai);

/* return current pending data in queue (not the queue's capacity)
 */
int ztl_bq_size(ztl_blocking_queue_t* zbq);

/* return true if no pending data
 */
bool ztl_bq_empty(ztl_blocking_queue_t* zbq);


#endif//_ZTL_BLOCKING_QUEUE_H_
