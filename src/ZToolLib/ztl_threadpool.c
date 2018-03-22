/**
 * ztl_threadpool.c
 *
 * This file will contain your implementation of a threadpool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "ztl_threadpool.h"
#include "ztl_threads.h"
#include "ztl_mempool.h"
#include "ztl_atomic.h"

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif//WIN32

#define ZTL_MAX_CACHE_NUM   256
struct ztl_task_st{
    struct ztl_task_st* next;
    ztl_dispatch_fn     func;
    void*               param;
    ztl_free_fn         afree;
};
typedef struct ztl_task_st ztl_task_t;

struct ztl_thrpool_st {
    uint32_t        thrnum;             // default num threads
    uint32_t        maxthrnum;          // max num threads
    uint32_t        activenum;          // active num threads
    uint32_t        maxsize;            // max task num in task-list
    int32_t         stop;               //running or stop flag
    int32_t         flags;              // reserved variable
    uint32_t        taskn;              // current task num in job-list
    void*           userdata;
    ztl_task_t      *head, *tail;       // the task list head and tail
    ztl_mempool_t*      task_pool;      // a lock free cache list
    ztl_thread_mutex_t  lock;           // thread pool lock
    ztl_thread_cond_t   workcond;       // thread condition for job active
    ztl_thread_cond_t   waitcond;       // thread condition for waiting
};

static void _append_task(ztl_thrpool_t* tp, ztl_task_t* task)
{
    if (NULL == tp->head) {
        tp->head = task;
        tp->tail = task;
    }
    else {
        tp->tail->next = task;
    }
    tp->tail = task;
}

static ztl_task_t* _get_head_task(ztl_thrpool_t* tp)
{
    ztl_task_t* task = NULL;
    if (tp->head != NULL)
    {
        task = tp->head;
        if (task == tp->tail)
            tp->tail = NULL;
        tp->head = tp->head->next;
        ztl_atomic_dec(&tp->taskn, 1);
    }
    return task;
}

/// get a task from thread pool's task list
static ztl_task_t* _get_task(ztl_thrpool_t* tp, int timeout)
{
    ztl_task_t* task = NULL;

    ztl_thread_mutex_lock(&tp->lock);
    if (tp->head == NULL)
    {
        ztl_thread_cond_wait(&tp->workcond, &tp->lock);
        if (tp->head == NULL)
        {
            goto GET_TASK_END;
        }
    }

    // get head task
    task = _get_head_task(tp);

GET_TASK_END:
    ztl_thread_mutex_unlock(&tp->lock);
    return task;
}

static void _release_task(ztl_thrpool_t* tp, ztl_task_t* task)
{
    if (task->afree)
        task->afree(tp, task->param);

    // append the task to cache list if
    ztl_mp_free(tp->task_pool, task);
}

/// thread pool's worker thread
static ztl_thread_result_t ZTL_THREAD_CALL _thrpool_worker_thread(void* arg)
{
    ztl_thrpool_t* tp = (ztl_thrpool_t*)arg;
    ztl_atomic_add(&tp->activenum, 1);

    ztl_task_t* task = NULL;
    while (tp->stop == 0)
    {
        if ((task = _get_task(tp, 100)) == NULL)
            continue;

        // execute actual task
        if (task->func)
            task->func(tp, task->param);

        _release_task(tp, task);
        task = NULL;
    }

    ztl_atomic_dec(&tp->activenum, 1);
    return 0;
}

/// create multi threads
static int _create_worker_thread(ztl_thrpool_t* tp)
{
    if (ztl_atomic_add(&tp->thrnum, 1) >= tp->maxthrnum)
    {
        ztl_atomic_dec(&tp->thrnum, 1);
        return -1;
    }

    ztl_thread_t thr;
    int rv = ztl_thread_create(&thr, NULL, _thrpool_worker_thread, tp);
    if (rv != 0)
    {
        ztl_atomic_dec(&tp->thrnum, 1);
    }

#ifdef _DEBUG
    fprintf(stdout, "create a thread [%u]\n", thr);
#endif
    return rv;
}

/// create a thread pool
ztl_thrpool_t* ztl_thrpool_create(int min_threads_num, int max_threads_num, int max_queue_size)
{
    if (min_threads_num <= 0 || min_threads_num > max_threads_num || max_queue_size <= 0)
        return NULL;
    if (max_threads_num > ZTL_MAX_THR_IN_POOL)
        max_threads_num = ZTL_MAX_THR_IN_POOL;

    ztl_thrpool_t* tp;
    tp = (ztl_thrpool_t*)malloc(sizeof(ztl_thrpool_t));
    tp->maxthrnum   = max_threads_num;
    tp->activenum   = 0;
    tp->thrnum      = 0;
    tp->maxsize     = max_queue_size;
    tp->stop        = 0;
    tp->flags       = 0;
    tp->taskn       = 0;
    tp->userdata    = NULL;
    tp->head        = NULL;
    tp->tail        = NULL;
    tp->task_pool   = ztl_mp_create(sizeof(ztl_task_t), ZTL_MAX_CACHE_NUM);

    ztl_thread_cond_init(&tp->workcond, NULL);
    ztl_thread_cond_init(&tp->waitcond, NULL);
    ztl_thread_mutex_init(&tp->lock, NULL);

    // create num_threads thread
    int n;
    for (n = 0; n < min_threads_num; ++n) {
        if (0 != _create_worker_thread(tp))
            break;;
    }

    int m = 120;
    while (ztl_atomic_add(&tp->activenum, 0) < (uint32_t)n || --m > 0) {
        sleepms(1);
    }

    return tp;
}

/// dispatch a runnable task to the thread pool with argument "arg"
int ztl_thrpool_dispatch(ztl_thrpool_t* thpool, ztl_dispatch_fn func, void* param, ztl_free_fn afree)
{
    if (thpool == NULL)
        return -1;
    if (thpool->taskn >= thpool->maxsize)
        return -2;

    ztl_task_t* task;
    task = (ztl_task_t*)ztl_mp_alloc(thpool->task_pool);
    task->next  = NULL;
    task->func  = func;
    task->param = param;
    task->afree = afree;

    // apend the job at the tail of job list
    ztl_thread_mutex_lock(&thpool->lock);
    _append_task(thpool, task);
    uint32_t lOldTaskN = ztl_atomic_add(&thpool->taskn, 1);
    ztl_thread_mutex_unlock(&thpool->lock);

    if (lOldTaskN == 0)
    {
        ztl_thread_cond_signal(&thpool->workcond);
    }
    else if (lOldTaskN > (thpool->thrnum << 1) &&
        ztl_atomic_add(&thpool->thrnum, 0) < thpool->maxthrnum)
    {
        _create_worker_thread(thpool);
    }
    return 0;
}

/// remove threa queued task from thread pool by argument "arg"
int ztl_thrpool_remove(ztl_thrpool_t* thpool, ztl_dispatch_fn func, ztl_compare_fn cmp_func, void* arg)
{
    if (NULL == thpool) {
        return -1;
    }

    ztl_task_t* task, *nexttask;

    ztl_thread_mutex_lock(&thpool->lock);
    task = thpool->head;
    if (NULL == task) {
        goto REMOVE_END;
    }

    // if the first job
    if ((NULL == cmp_func && task->param == arg)
        || (NULL != cmp_func && cmp_func(task->param, arg)))
    {
#if defined(_DEBUG) || defined(DEBUG)
        fprintf(stdout, "thrpool_remove1: remove %p(%d)\n", arg, *(int*)arg);
#endif//debug
        ztl_atomic_dec(&thpool->taskn, 1);

        if (NULL == task->next)
        {
            thpool->head = NULL;
            thpool->tail = NULL;
        }
        else
        {
            thpool->head = thpool->head->next;
        }
        goto REMOVE_END;
    }

    nexttask = task->next;
    while (nexttask)
    {
        if ((NULL == cmp_func && nexttask->param == arg) || 
            (NULL != cmp_func && cmp_func(nexttask->param, arg)))
        {
#ifdef _DEBUG
            fprintf(stdout, "thrpool_remove2: remove %p(%d)\n", arg, *(int*)arg);
#endif
            ztl_atomic_dec(&thpool->taskn, 1);

            task->next = nexttask->next;
            if (nexttask == thpool->tail)
                thpool->tail = task;
            break;
        }

        task = nexttask;
        nexttask = nexttask->next;
    }

    task = nexttask;

REMOVE_END:
    ztl_thread_mutex_unlock(&thpool->lock);
    if (nexttask)
    {
        _release_task(thpool, nexttask);
        return 0;
    }
    return -1;
}

void ztl_thrpool_set_data(ztl_thrpool_t* thpool, void* userdata)
{
    if (thpool)
        thpool->userdata = userdata;
}

void* ztl_thrpool_get_data(ztl_thrpool_t* thpool)
{
    if (thpool)
        return thpool->userdata;
    return NULL;
}

/// get current task count
int ztl_thrpool_tasknum(ztl_thrpool_t* thpool)
{
    if (NULL == thpool)
        return 0;
    return thpool->taskn;
}

int ztl_thrpool_thrnum(ztl_thrpool_t* thpool)
{
    if (NULL == thpool)
        return 0;
    return thpool->thrnum;
}

/// wait all thread exit
int ztl_thrpool_join(ztl_thrpool_t* thpool)
{
    if (thpool == NULL)
        return -1;

    while (ztl_atomic_add(&thpool->activenum, 0) > 0) {
        sleepms(1);
    }
    return 0;
}

/// thread pool stop
int ztl_thrpool_stop(ztl_thrpool_t* thpool)
{
    if (thpool == NULL)
        return -1;

    ztl_atomic_set(&thpool->stop, 1);
    for (uint32_t n = 0; n < thpool->thrnum; ++n)
    {
        ztl_thrpool_dispatch(thpool, NULL, NULL, NULL);
        ztl_thread_cond_signal(&thpool->workcond);
    }
    ztl_thread_cond_broadcast(&thpool->workcond);

    sleepms(1);
    return 0;
}

/// destroy the threadpool, causing all threads in it to commit suicide,
/// and then frees all the memory associated with the thread pool
int ztl_thrpool_destroy(ztl_thrpool_t* thpool)
{
    if (thpool == NULL)
        return -1;

    if (thpool->stop == 0)
    {
        ztl_thrpool_stop(thpool);
    }
    sleepms(1);

    ztl_thread_mutex_lock(&thpool->lock);
    ztl_task_t* task = NULL;
    // free all the objects in the job list
    while ((task = _get_head_task(thpool)) != NULL)
    {
        if (task->afree)
            task->afree(thpool, task->param);

        ztl_mp_free(thpool->task_pool, task);
    }
    ztl_thread_mutex_unlock(&thpool->lock);

    ztl_mp_destroy(thpool->task_pool);

    // free the thread pool
    ztl_thread_mutex_destroy(&thpool->lock);
    ztl_thread_cond_destroy(&thpool->workcond);
    free(thpool);
    return 0;
}

