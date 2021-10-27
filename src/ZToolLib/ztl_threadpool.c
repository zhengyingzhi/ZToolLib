/**
 * ztl_threadpool.c
 *
 * This file will contain your implementation of a threadpool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ztl_atomic.h"
#include "ztl_common.h"
#include "ztl_errors.h"
#include "ztl_locks.h"
#include "ztl_palloc.h"
#include "ztl_threads.h"
#include "ztl_threadpool.h"
#include "ztl_times.h"
#include "ztl_utils.h"
#include "lockfreequeue.h"


#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif//_WIN32

#define ZTL_DEFAULT_CACHE_NUM   256
struct ztl_task_st {
    struct ztl_task_st* next;
    void*               arg1;
    void*               arg2;
    ztl_dispatch_fn     func;
    ztl_free_fn         afree1;
    ztl_free_fn         afree2;
};
typedef struct ztl_task_st ztl_task_t;

struct ztl_thrpool_st {
    uint32_t        thrnum;             // default num threads
    uint32_t        activenum;          // active num threads
    uint32_t        maxsize;            // max task num in task-list
    int32_t         stop;               // running or stop flag
    uint32_t        taskn;              // current task num in job-list
    uint32_t        pool_lock;
    void*           userdata;           // user data
    ztl_task_t      *head, *tail;       // the task list head and tail
    lfqueue_t*          idle_tasks;
    ztl_pool_t*         pool;
    ztl_thread_mutex_t  mutex;           // thread pool lock
    ztl_thread_cond_t   workcond;       // thread condition for task active
    ztl_thread_cond_t   waitcond;       // thread condition for waiting
};


static void _empty_dispatch_fn(ztl_thrpool_t* tp, void* arg1, void* arg2)
{
    ZTL_NOTUSED(tp);
    ZTL_NOTUSED(arg1);
    ZTL_NOTUSED(arg2);
}

static ztl_task_t* _new_task(ztl_thrpool_t* tp, ztl_dispatch_fn func,
    void* arg1, void* arg2, ztl_free_fn afree1, ztl_free_fn afree2)
{
    ztl_task_t* task = NULL;
    lfqueue_pop(tp->idle_tasks, (void**)&task);
    if (!task)
    {
        ztl_spinlock(&tp->pool_lock, 1, 2048);
        task = (ztl_task_t*)ztl_palloc(tp->pool, sizeof(ztl_task_t));
        ztl_unlock(&tp->pool_lock);
    }

    task->next = NULL;
    task->arg1 = arg1;
    task->arg2 = arg2;
    task->func = func;
    task->afree1 = afree1;
    task->afree2 = afree2;
    return task;
}

static void _append_task(ztl_thrpool_t* tp, ztl_task_t* task, int priority)
{
    if (!tp->head)
    {
        tp->head = task;
        tp->tail = task;
    }
    else if (priority)
    {
        task->next = tp->head;
        tp->head = task;
        if (!tp->tail)
            tp->tail = task;
    }
    else
    {
        tp->tail->next = task;
        tp->tail = task;
    }
}

static ztl_task_t* _get_head_task(ztl_thrpool_t* tp)
{
    ztl_task_t* task = NULL;
    if (tp->head)
    {
        task = tp->head;
        if (task == tp->tail)
            tp->tail = NULL;
        tp->head = tp->head->next;
        atomic_dec(&tp->taskn, 1);
    }
    return task;
}

static ztl_task_t* _get_task(ztl_thrpool_t* tp, int timeout)
{
    (void)timeout;

    ztl_task_t* task = NULL;

    ztl_thread_mutex_lock(&tp->mutex);
    if (tp->head == NULL)
    {
        ztl_thread_cond_wait(&tp->workcond, &tp->mutex);
    }
    task = _get_head_task(tp);
    ztl_thread_mutex_unlock(&tp->mutex);
    return task;
}

static void _release_task(ztl_thrpool_t* tp, ztl_task_t* task)
{
    if (task->afree1)
        task->afree1(tp, task->arg1);
    if (task->afree2)
        task->afree2(tp, task->arg2);

    if (lfqueue_push(tp->idle_tasks, &task) != 0)
    {
        fprintf(stderr, "push tasks to free queue failed!\n");
    }
}

static ztl_thread_result_t ZTL_THREAD_CALL _thrpool_worker_thread(void* arg)
{
    ztl_thrpool_t* tp = (ztl_thrpool_t*)arg;
    atomic_add(&tp->activenum, 1);

    ztl_task_t* task = NULL;
    while (!tp->stop)
    {
        if ((task = _get_task(tp, 100)) == NULL)
            continue;

        if (task->func)
            task->func(tp, task->arg1, task->arg2);

        _release_task(tp, task);
        task = NULL;
    }

    atomic_dec(&tp->activenum, 1);
    return 0;
}

static int _create_thrpool_worker_thread(ztl_thrpool_t* tp)
{
    if (tp->activenum >= tp->thrnum) {
        return -1;
    }

    ztl_thread_t thr;
    int rv = ztl_thread_create(&thr, NULL, _thrpool_worker_thread, tp);

    return rv;
}

ztl_thrpool_t* ztl_thrpool_create(int threads_num, int max_queue_size)
{
    uint32_t        nbytes;
    ztl_pool_t*     pool;
    ztl_thrpool_t*  tp;

    if (threads_num <= 0 || max_queue_size <= 0)
        return NULL;
    if (threads_num > ZTL_MAX_THR_IN_POOL)
        threads_num = ZTL_MAX_THR_IN_POOL;

    nbytes = sizeof(ztl_thrpool_t) + sizeof(ztl_task_t) * (max_queue_size + 1);
    pool = ztl_create_pool(nbytes);
    if (!pool) {
        return NULL;
    }

    tp = (ztl_thrpool_t*)ztl_pcalloc(pool, sizeof(ztl_thrpool_t));
    tp->activenum   = 0;
    tp->thrnum      = threads_num;
    tp->maxsize     = max_queue_size;
    tp->stop        = 0;
    tp->taskn       = 0;
    tp->userdata    = NULL;
    tp->head        = NULL;
    tp->tail        = NULL;
    tp->pool        = pool;
    tp->pool_lock   = 0;
    tp->idle_tasks  = lfqueue_create(max_queue_size, sizeof(void*));

    ztl_thread_cond_init(&tp->workcond, NULL);
    ztl_thread_cond_init(&tp->waitcond, NULL);
    ztl_thread_mutex_init(&tp->mutex, NULL);

    return tp;
}


int ztl_thrpool_start(ztl_thrpool_t* thpool)
{
    uint32_t n;
    for (n = 0; n < thpool->thrnum; ++n)
    {
        if (0 != _create_thrpool_worker_thread(thpool)) {
            break;
        }
    }

    //int m = 120;
    //while (atomic_add(&tp->activenum, 0) < (uint32_t)n || --m > 0) {
    //    sleepms(1);
    //}

    return 0;
}

int ztl_thrpool_dispatch(ztl_thrpool_t* thpool, ztl_dispatch_fn func,
    void* arg1, void* arg2, ztl_free_fn afree1, ztl_free_fn afree2)
{
    if (thpool->taskn >= thpool->maxsize) {
        return ZTL_ERR_QueueFull;
    }

    uint32_t    old_taskn;
    ztl_task_t* task;

    task = _new_task(thpool, func, arg1, arg2, afree1, afree2);

    ztl_thread_mutex_lock(&thpool->mutex);
    _append_task(thpool, task, 0);
    old_taskn = atomic_add(&thpool->taskn, 1);
    if (old_taskn == 0) {
        ztl_thread_cond_signal(&thpool->workcond);
    }
    ztl_thread_mutex_unlock(&thpool->mutex);
    return 0;
}

int ztl_thrpool_dispatch_priority(ztl_thrpool_t* thpool, ztl_dispatch_fn func,
    void* arg1, void* arg2, ztl_free_fn afree1, ztl_free_fn afree2)
{
    if (thpool->taskn >= thpool->maxsize) {
        return ZTL_ERR_QueueFull;
    }

    uint32_t    old_taskn;
    ztl_task_t* task;

    task = _new_task(thpool, func, arg1, arg2, afree1, afree2);

    ztl_thread_mutex_lock(&thpool->mutex);
    _append_task(thpool, task, 1);
    old_taskn = atomic_add(&thpool->taskn, 1);
    if (old_taskn == 0) {
        ztl_thread_cond_signal(&thpool->workcond);
    }
    ztl_thread_mutex_unlock(&thpool->mutex);
    return 0;
}

int ztl_thrpool_remove(ztl_thrpool_t* thpool, ztl_dispatch_fn func)
{
    ztl_task_t* task, *nexttask;

    nexttask = NULL;

    ztl_thread_mutex_lock(&thpool->mutex);
    task = thpool->head;
    if (NULL == task) {
        goto REMOVE_END;
    }

    nexttask = task->next;
    while (nexttask)
    {
        if (func == nexttask->func)
        {
            atomic_dec(&thpool->taskn, 1);

            task->next = nexttask->next;
            if (nexttask == thpool->tail)
                thpool->tail = task;

            task = nexttask;
            goto REMOVE_END;
        }

        task = nexttask;
        nexttask = nexttask->next;
    }
    task = NULL;

REMOVE_END:
    ztl_thread_mutex_unlock(&thpool->mutex);
    if (task)
    {
        _release_task(thpool, task);
        return 0;
    }

    return ZTL_ERR_NotFound;
}

void ztl_thrpool_set_data(ztl_thrpool_t* thpool, void* userdata)
{
    thpool->userdata = userdata;
}

void* ztl_thrpool_get_data(ztl_thrpool_t* thpool)
{
    return thpool->userdata;
}

int ztl_thrpool_pending(ztl_thrpool_t* thpool)
{
    return atomic_add(&thpool->taskn, 0);
}

int ztl_thrpool_thrnum(ztl_thrpool_t* thpool)
{
    return thpool ? thpool->thrnum : 0;
}

int ztl_thrpool_join(ztl_thrpool_t* thpool, uint32_t timeout_ms)
{
    uint64_t curr_time;
    if (thpool == NULL)
        return -1;

    curr_time = get_timestamp();
    while (atomic_add(&thpool->activenum, 0) > 0)
    {
        ztl_sleepms(1);
        if (get_timestamp() - curr_time >= timeout_ms) {
            return ZTL_ERR_Timeout;
        }
    }
    return 0;
}

int ztl_thrpool_stop(ztl_thrpool_t* thpool)
{
    if (thpool == NULL)
        return -1;

    atomic_set(&thpool->stop, 1);
    for (uint32_t n = 0; n < thpool->thrnum; ++n)
    {
        ztl_thrpool_dispatch(thpool, _empty_dispatch_fn, NULL, NULL, NULL, NULL);
        ztl_thread_cond_signal(&thpool->workcond);
    }
    ztl_thread_cond_broadcast(&thpool->workcond);
    return 0;
}

int ztl_thrpool_release(ztl_thrpool_t* thpool)
{
    if (thpool == NULL)
        return -1;

    if (thpool->stop == 0) {
        ztl_thrpool_stop(thpool);
    }

    ztl_pool_t* pool = thpool->pool;
    ztl_task_t* task;
    while ((task = _get_head_task(thpool)) != NULL)
    {
        if (task->afree1)
            task->afree1(thpool, task->arg1);
        if (task->afree2)
            task->afree2(thpool, task->arg2);
    }
    ztl_thread_mutex_destroy(&thpool->mutex);
    ztl_thread_cond_destroy(&thpool->workcond);

    ztl_destroy_pool(pool);
    return 0;
}
