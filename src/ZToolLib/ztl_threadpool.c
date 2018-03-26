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
#include "ztl_mempool.h"
#include "ztl_atomic.h"
#include "ztl_threads.h"

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#endif//_WIN32

#define ZTL_DEFAULT_CACHE_NUM   256
struct ztl_task_st {
    struct ztl_task_st* next;
    void*               param;
    ztl_dispatch_fn     func;
    ztl_free_fn         afree;
};
typedef struct ztl_task_st ztl_task_t;

struct ztl_thrpool_st {
    uint32_t        thrnum;             // default num threads
    uint32_t        activenum;          // active num threads
    uint32_t        maxsize;            // max task num in task-list
    int32_t         stop;               // running or stop flag
    uint32_t        taskn;              // current task num in job-list
    void*           userdata;           // user data
    ztl_task_t      *head, *tail;       // the task list head and tail
    ztl_mempool_t*      task_pool;      // a lock free cache list
    ztl_thread_mutex_t  lock;           // thread pool lock
    ztl_thread_cond_t   workcond;       // thread condition for task active
    ztl_thread_cond_t   waitcond;       // thread condition for waiting
};


/* append a task to list tail */
static void _append_task(ztl_thrpool_t* tp, ztl_task_t* task)
{
    if (NULL == tp->head) {
        tp->head = task;
    }
    else {
        tp->tail->next = task;
    }
    tp->tail = task;
}

/* get a task from list head */
static ztl_task_t* _get_head_task(ztl_thrpool_t* tp)
{
    ztl_task_t* task = NULL;
    if (tp->head)
    {
        task = tp->head;
        if (task == tp->tail)
            tp->tail = NULL;
        tp->head = tp->head->next;
        ztl_atomic_dec(&tp->taskn, 1);
    }
    return task;
}

// get a task from thread pool's task list
static ztl_task_t* _get_task(ztl_thrpool_t* tp, int timeout)
{
    (void)timeout;

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

    task = _get_head_task(tp);

GET_TASK_END:
    ztl_thread_mutex_unlock(&tp->lock);
    return task;
}

/* release the task object after finished */
static void _release_task(ztl_thrpool_t* tp, ztl_task_t* task)
{
    if (task->afree)
        task->afree(tp, task->param);

    ztl_mp_free(tp->task_pool, task);
}

/// thread pool's worker thread
static ztl_thread_result_t ZTL_THREAD_CALL _thrpool_worker_thread(void* arg)
{
    ztl_thrpool_t* tp = (ztl_thrpool_t*)arg;
    ztl_atomic_add(&tp->activenum, 1);

    ztl_task_t* task = NULL;
    while (!tp->stop)
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

static int _create_worker_thread(ztl_thrpool_t* tp)
{
    if (tp->activenum >= tp->thrnum)
    {
        return -1;
    }

    ztl_thread_t thr;
    int rv = ztl_thread_create(&thr, NULL, _thrpool_worker_thread, tp);

    return rv;
}

ztl_thrpool_t* ztl_thrpool_create(int threads_num, int max_queue_size)
{
    if (threads_num <= 0 || max_queue_size <= 0)
        return NULL;
    if (threads_num > ZTL_MAX_THR_IN_POOL)
        threads_num = ZTL_MAX_THR_IN_POOL;

    ztl_thrpool_t* tp;
    tp = (ztl_thrpool_t*)malloc(sizeof(ztl_thrpool_t));
    tp->activenum   = 0;
    tp->thrnum      = threads_num;
    tp->maxsize     = max_queue_size;
    tp->stop        = 0;
    tp->taskn       = 0;
    tp->userdata    = NULL;
    tp->head        = NULL;
    tp->tail        = NULL;
    tp->task_pool   = ztl_mp_create(sizeof(ztl_task_t), ZTL_DEFAULT_CACHE_NUM, 1);

    ztl_thread_cond_init(&tp->workcond, NULL);
    ztl_thread_cond_init(&tp->waitcond, NULL);
    ztl_thread_mutex_init(&tp->lock, NULL);

    int n;
    for (n = 0; n < threads_num; ++n)
    {
        if (0 != _create_worker_thread(tp)) {
            break;
        }
    }

    //int m = 120;
    //while (ztl_atomic_add(&tp->activenum, 0) < (uint32_t)n || --m > 0) {
    //    sleepms(1);
    //}

    return tp;
}

/// dispatch a runnable task to the thread pool with argument "arg"
int ztl_thrpool_dispatch(ztl_thrpool_t* thpool, ztl_dispatch_fn func, void* param, ztl_free_fn afree)
{
    if (thpool->taskn >= thpool->maxsize) {
        return -1;
    }

    uint32_t    lOldTaskN;
    ztl_task_t* task;

    task = (ztl_task_t*)ztl_mp_alloc(thpool->task_pool);
    task->next  = NULL;
    task->param = param;
    task->func  = func;
    task->afree = afree;

    // apend the task at the tail of task list, and try wakeup worker
    ztl_thread_mutex_lock(&thpool->lock);
    _append_task(thpool, task);
    lOldTaskN = ztl_atomic_add(&thpool->taskn, 1);
    ztl_thread_mutex_unlock(&thpool->lock);

    if (lOldTaskN == 0) {
        ztl_thread_cond_signal(&thpool->workcond);
    }

    return 0;
}

int ztl_thrpool_remove(ztl_thrpool_t* thpool, ztl_compare_fn cmp_func, void* param)
{
    if (NULL == thpool) {
        return -1;
    }

    ztl_task_t* task, *nexttask;

    nexttask = NULL;

    ztl_thread_mutex_lock(&thpool->lock);
    task = thpool->head;
    if (NULL == task) {
        goto REMOVE_END;
    }

    // if the first task
    if ((NULL == cmp_func && task->param == param) ||
        (NULL != cmp_func && cmp_func(task->param, param)))
    {
        ztl_atomic_dec(&thpool->taskn, 1);

        if (task->next  == NULL) {
            thpool->head = NULL;
            thpool->tail = NULL;
        }
        else {
            thpool->head = thpool->head->next;
        }

        goto REMOVE_END;
    }

    // traverse the task list
    nexttask = task->next;
    while (nexttask)
    {
        if ((NULL == cmp_func && nexttask->param == param) ||
            (NULL != cmp_func && cmp_func(nexttask->param, param)))
        {
            ztl_atomic_dec(&thpool->taskn, 1);

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
    ztl_thread_mutex_unlock(&thpool->lock);
    if (task) {
        _release_task(thpool, task);
        return 0;
    }

    return -1;
}

void ztl_thrpool_set_data(ztl_thrpool_t* thpool, void* userdata)
{
    thpool->userdata = userdata;
}

void* ztl_thrpool_get_data(ztl_thrpool_t* thpool)
{
    return thpool->userdata;
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

int ztl_thrpool_pending(ztl_thrpool_t* thpool)
{
    return ztl_atomic_add(&thpool->taskn, 0);
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

int ztl_thrpool_release(ztl_thrpool_t* thpool)
{
    if (thpool == NULL)
        return -1;

    if (thpool->stop == 0) {
        ztl_thrpool_stop(thpool);
    }

    ztl_task_t* task;
    ztl_thread_mutex_lock(&thpool->lock);
    while ((task = _get_head_task(thpool)) != NULL)
    {
        if (task->afree)
            task->afree(thpool, task->param);

        ztl_mp_free(thpool->task_pool, task);
    }
    ztl_thread_mutex_unlock(&thpool->lock);

    if (thpool->task_pool) {
        ztl_mp_release(thpool->task_pool);
    }

    ztl_thread_mutex_destroy(&thpool->lock);
    ztl_thread_cond_destroy(&thpool->workcond);

    free(thpool);
    return 0;
}

