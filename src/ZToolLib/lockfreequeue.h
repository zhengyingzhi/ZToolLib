/*
* Copyright (C) Yingzhi Zheng
* Copyright (C) zhengyingzhi112@163.com
*/

#ifndef _LOCK_FREE_QUEUE_H_
#define _LOCK_FREE_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

/// exported types
/// a lock free queue based on a circular array
/// supports multiple producers and consumers
typedef struct lfqueue_st lfqueue_t;

/// @brief  create a queue by max queue size
/// @param  quename is mainly used in process shared memory
/// @return the queue object
lfqueue_t* lfqueue_create(unsigned int size, const char* quename);

/// @brief  push an element at the tail of the queue
/// @param  the element to insert in the queue
/// @return 0 if the element was inserted in the queue. -1 if the queue was full
int lfqueue_push(lfqueue_t* que, void* pdata);

/// @brief  pop an element from the queue at the head
/// @param  the element to pop out
/// @return 0 if the element was successfully extracted from the queue. 
///        -1 if the queue was empty
int lfqueue_pop(lfqueue_t* que, void** ppdata);

/// @return the queue size which might be bogus value
int lfqueue_size(lfqueue_t* que);

/// @return true if the queue is empty or not
bool lfqueue_empty(lfqueue_t* que);

/// @brief free the queue
int lfqueue_release(lfqueue_t* que);

#ifdef __cplusplus
}//__cplusplus
#endif

#endif//_LOCK_FREE_QUEUE_H_
