/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_THREAD_POOL_H_
#define _ZTL_THREAD_POOL_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/// maximum number of threads allowed in a pool
#define ZTL_MAX_THR_IN_POOL 128

/// the exported types
typedef struct ztl_thrpool_st ztl_thrpool_t;

/// "dispatch_fn" declares a typed function pointer.
typedef void (*ztl_dispatch_fn)(ztl_thrpool_t* , void* );

/// "free_fn" declares a typed of cleanup function pointer
typedef void (*ztl_free_fn)(ztl_thrpool_t* , void* );

/// "compare_fn" declares a typed of comparable func when remove a task
typedef bool (*ztl_compare_fn)(void*, void* );


/* thrpool_create creates a fixed-sized number threads & queue size
 * if the function succeeds, it returns a (non-NULL)
 * "threadpool", else it return NULL.
 */
ztl_thrpool_t* ztl_thrpool_create(int threads_num, int max_queue_size);

/* @brief   dispatch a new task to the thread pool with argument 'param'
 * @param   afree: to free the param after task finished
 * @return  0 success, -1 the task queue is full
 */
int ztl_thrpool_dispatch(ztl_thrpool_t* thpool, ztl_dispatch_fn func, void* param, ztl_free_fn afree);

/* @brief   remove the queued task from thread pool by passed compare func and argument "param"
 */
int ztl_thrpool_remove(ztl_thrpool_t* thpool, ztl_compare_fn cmp_func, void* param);

/* @brief   set or get user data related this thrpool
 */
void  ztl_thrpool_set_data(ztl_thrpool_t* thpool, void* userdata);
void* ztl_thrpool_get_data(ztl_thrpool_t* thpool);

/* @brief   get current pending tasks count in queue
 */
int ztl_thrpool_pending(ztl_thrpool_t* thpool);

/* @return  working threads number
 */
int ztl_thrpool_thrnum(ztl_thrpool_t* thpool);

/* @brief   wait all threads exit
 */
int ztl_thrpool_join(ztl_thrpool_t* thpool);

/* @brief   thrpool stop all working threads
 */
int ztl_thrpool_stop(ztl_thrpool_t* thpool);

/* @brief   destroy the threadpool, causing all threads in it to commit suicide,
 * and then frees all the memory associated with the thread pool
 */
int ztl_thrpool_release(ztl_thrpool_t* thpool);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_THREAD_POOL_H_
