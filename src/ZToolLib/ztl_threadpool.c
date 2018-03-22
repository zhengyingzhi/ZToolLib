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
struct ztl_job_st{
    struct ztl_job_st*  next;
    ztl_dispatch_fn	    func;
    void*		        arg;
    ztl_free_fn		    afree;
};
typedef struct ztl_job_st* ztl_job_t;

struct ztl_thrpool_st {
    uint32_t        thrnum;				// defaul num threads
    uint32_t        maxthrnum;			// max num threads
    uint32_t        activenum;			// active num thread
    uint32_t        maxsize;			// max job num in job-list
    int32_t         stop;				// running or stop flag
    int32_t         flags;				// reserved varable
    uint32_t        jobnum;				// current job num in job-list
    void*           userdata;
    ztl_job_t       head, tail;		// the job list head and tail
    ztl_mempool_t*      cachejobs;	    // a lock free cache list
    ztl_thread_mutex_t  lock;		    // thread pool lock
    ztl_thread_cond_t   workcond;	    // thread condition for job active
    ztl_thread_cond_t   waitcond;	    // thread condition for waiting
};

static void _append_job(ztl_thrpool_t* tp, ztl_job_t job)
{
    if (NULL == tp->head)
    {
        tp->head = job;
        tp->tail = job;
    }
    else
    {
        tp->tail->next = job;
    }
    tp->tail = job;
}

static ztl_job_t _get_head_job(ztl_thrpool_t* tp)
{
    ztl_job_t job = NULL;
    if (tp->head != NULL)
    {
        job = tp->head;
        if (job == tp->tail)
            tp->tail = NULL;
        tp->head = tp->head->next;
        ztl_atomic_dec(&tp->jobnum, 1);
    }
    return job;
}

/// get a job from thread pool's job list
static ztl_job_t _get_job(ztl_thrpool_t* tp, int timeout)
{
    ztl_job_t job = NULL;

    ztl_thread_mutex_lock(&tp->lock);
    if (tp->head == NULL)
    {
        ztl_thread_cond_wait(&tp->workcond, &tp->lock);
        if (tp->head == NULL)
        {
            goto GET_JOB_END;
        }
    }

    // get head job
    job = _get_head_job(tp);

GET_JOB_END:
    ztl_thread_mutex_unlock(&tp->lock);
    return job;
}

static void _release_job(ztl_thrpool_t* tp, ztl_job_t job)
{
    if (job->afree)
        job->afree(tp, job->arg);

    // append the job to cache list if
    ztl_mp_free(tp->cachejobs, job);
}

/// thread pool's worker thread
static ztl_thread_result_t ZTL_THREAD_CALL _thrpool_worker_thread(void* arg)
{
    ztl_thrpool_t* tp = (ztl_thrpool_t*)arg;
    ztl_atomic_add(&tp->activenum, 1);

    ztl_job_t job = NULL;
    while (tp->stop == 0)
    {
        if ((job = _get_job(tp, 100)) == NULL)
            continue;

        // execute acutal job
        if (job->func)
            job->func(tp, job->arg);

        _release_job(tp, job);
        job = NULL;
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
    tp->jobnum      = 0;
    tp->userdata    = NULL;
    tp->head        = NULL;
    tp->tail        = NULL;
    tp->cachejobs   = ztl_mp_create(sizeof(struct ztl_job_st), ZTL_MAX_CACHE_NUM);

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
int ztl_thrpool_dispatch(ztl_thrpool_t* thpool, ztl_dispatch_fn func, void* arg, ztl_free_fn afree)
{
    if (thpool == NULL)
        return -1;
    if (thpool->jobnum >= thpool->maxsize)
        return -2;

    ztl_job_t job = (ztl_job_t)ztl_mp_alloc(thpool->cachejobs);
    job->func = func;
    job->arg = arg;
    job->afree = afree;
    job->next = NULL;

    // apend the job at the tail of job list
    ztl_thread_mutex_lock(&thpool->lock);
    _append_job(thpool, job);
    uint32_t lOldJobN = ztl_atomic_add(&thpool->jobnum, 1);
    ztl_thread_mutex_unlock(&thpool->lock);

    if (lOldJobN == 0)
    {
        ztl_thread_cond_signal(&thpool->workcond);
    }
    else if (lOldJobN > (thpool->thrnum << 1) &&
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

    ztl_job_t job, nextJob;

    ztl_thread_mutex_lock(&thpool->lock);
    job = thpool->head;
    if (NULL == job) {
        goto REMOVE_END;
    }

    // if the first job
    if ((NULL == cmp_func && job->arg == arg) || (NULL != cmp_func && cmp_func(job->arg, arg)))
    {
#ifdef _DEBUG
        fprintf(stdout, "thrpool_remove1: remove %p(%d)\n", arg, *(int*)arg);
#endif
        ztl_atomic_dec(&thpool->jobnum, 1);

        if (NULL == job->next)
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

    nextJob = job->next;
    while (nextJob)
    {
        if ((NULL == cmp_func && nextJob->arg == arg) || (NULL != cmp_func && cmp_func(nextJob->arg, arg)))
        {
#ifdef _DEBUG
            fprintf(stdout, "thrpool_remove2: remove %p(%d)\n", arg, *(int*)arg);
#endif
            ztl_atomic_dec(&thpool->jobnum, 1);

            job->next = nextJob->next;
            if (nextJob == thpool->tail)
                thpool->tail = job;
            break;
        }

        job = nextJob;
        nextJob = nextJob->next;
    }

    job = nextJob;

REMOVE_END:
    ztl_thread_mutex_unlock(&thpool->lock);
    if (job)
    {
        _release_job(thpool, job);
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
int ztl_thrpool_jobnum(ztl_thrpool_t* thpool)
{
    if (NULL == thpool)
        return 0;
    return thpool->jobnum;
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
    ztl_job_t job = NULL;
    // free all the objects in the job list
    while ((job = _get_head_job(thpool)) != NULL)
    {
        if (job->afree)
            job->afree(thpool, job->arg);

        ztl_mp_free(thpool->cachejobs, job);
    }
    ztl_thread_mutex_unlock(&thpool->lock);

    ztl_mp_destroy(thpool->cachejobs);

    // free the thread pool
    ztl_thread_mutex_destroy(&thpool->lock);
    ztl_thread_cond_destroy(&thpool->workcond);
    free(thpool);
    return 0;
}

