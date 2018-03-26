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


/// thrpool_create creates a fixed-sized thread
/// pool.  If the function succeeds, it returns a (non-NULL)
/// "threadpool", else it returns NULL.
ztl_thrpool_t* ztl_thrpool_create(int threads_num, int max_queue_size);

/// dispatch a new task to the thread pool with argument 'arg'
/// return 0 success, -1 the task queue is full
int ztl_thrpool_dispatch(ztl_thrpool_t* thpool, ztl_dispatch_fn func, void* param, ztl_free_fn afree);

/// remove the queued job from thread pool by passed func and argument "arg"
int ztl_thrpool_remove(ztl_thrpool_t* thpool, ztl_compare_fn cmp_func, void* param);

/// set or get user data related this thrpool
void  ztl_thrpool_set_data(ztl_thrpool_t* thpool, void* userdata);
void* ztl_thrpool_get_data(ztl_thrpool_t* thpool);

/// get current tasks count in queue
int ztl_thrpool_tasknum(ztl_thrpool_t* thpool);

/// return working threads number
int ztl_thrpool_thrnum(ztl_thrpool_t* thpool);

/// return the pending tasks in the queue to process
int ztl_thrpool_pending(ztl_thrpool_t* thpool);

/// wait all threads exit
int ztl_thrpool_join(ztl_thrpool_t* thpool);

/// thread pool stop
int ztl_thrpool_stop(ztl_thrpool_t* thpool);

/// destroy the threadpool, causing all threads in it to commit suicide,
/// and then frees all the memory associated with the thread pool
int ztl_thrpool_release(ztl_thrpool_t* thpool);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_THREAD_POOL_H_
