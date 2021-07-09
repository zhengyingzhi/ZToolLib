#include "ztl_threads.h"

#ifndef _MSC_VER

int ztl_thread_rwlock_init(ztl_thread_rwlock_t* rwlock)
{
    pthread_rwlockattr_t rwattr;
    pthread_rwlockattr_init(&rwattr);
    pthread_rwlockattr_setkind_np(&rwattr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    pthread_rwlock_init((pthread_rwlock_t*)rwlock, &rwattr);
    pthread_rwlockattr_destroy(&rwattr);
    return 0;
}

#else

int ztl_thread_mutex_init(ztl_thread_mutex_t * mutex, void * attr)
{
    InitializeCriticalSection(mutex);
    return 0;
}

int ztl_thread_mutex_destroy(ztl_thread_mutex_t * mutex)
{
    DeleteCriticalSection(mutex);
    return 0;
}

int ztl_thread_mutex_lock(ztl_thread_mutex_t * mutex)
{
    EnterCriticalSection(mutex);
    return 0;
}

int ztl_thread_mutex_unlock(ztl_thread_mutex_t * mutex)
{
    LeaveCriticalSection(mutex);
    return 0;
}

int ztl_thread_mutexattr_init(ztl_thread_mutexattr_t* mattr)
{
    return 0;
}

int ztl_thread_mutexattr_settype(ztl_thread_mutexattr_t* mattr, int type)
{
    mattr->type = type;
    return 0;
}

int ztl_thread_mutexattr_destroy(ztl_thread_mutexattr_t* mattr)
{
    return 0;
}

int ztl_thread_rwlock_init(ztl_thread_rwlock_t* rwlock)
{
    InitializeCriticalSection(rwlock);
    return 0;
}

int ztl_thread_rwlock_rdlock(ztl_thread_rwlock_t* rwlock)
{
    EnterCriticalSection(rwlock);
    return 0;
}

int ztl_thread_rwlock_wrlock(ztl_thread_rwlock_t* rwlock)
{
    EnterCriticalSection(rwlock);
    return 0;
}

int ztl_thread_rwlock_unlock(ztl_thread_rwlock_t* rwlock)
{
    LeaveCriticalSection(rwlock);
    return 0;
}

int ztl_thread_cond_init(ztl_thread_cond_t * cond, void * attr)
{
    *cond = CreateEvent(NULL, FALSE, FALSE, NULL);
    return NULL == *cond ? GetLastError() : 0;
}

int ztl_thread_cond_destroy(ztl_thread_cond_t * cond)
{
    int rv = CloseHandle(*cond);
    return 0 == rv ? GetLastError() : 0;
}

/// Caller must be holding the lock, like pthread_cond_wait
int ztl_thread_cond_wait(ztl_thread_cond_t * cond, ztl_thread_mutex_t * mutex)
{
    int rv = 0;
    ztl_thread_mutex_unlock(mutex);
    rv = WaitForSingleObject(*cond, INFINITE);
    ztl_thread_mutex_lock(mutex);
    return WAIT_OBJECT_0 == rv ? 0 : GetLastError();
}

int ztl_thread_cond_signal(ztl_thread_cond_t * cond)
{
    int rv = SetEvent(*cond);
    return 0 == rv ? GetLastError() : 0;
}

int ztl_thread_cond_broadcast(ztl_thread_cond_t * cond)
{
    int rv = SetEvent(*cond);
    return 0 == rv ? GetLastError() : 0;
}

unsigned int ztl_thread_self()
{
    return (unsigned int)GetCurrentThreadId();
}

int ztl_thread_attr_init(ztl_thread_attr_t * attr)
{
    memset(attr, 0, sizeof(ztl_thread_attr_t));
    return 0;
}

int ztl_thread_attr_destroy(ztl_thread_attr_t * attr)
{
    return 0;
}

int ztl_thread_attr_setdetachstate(ztl_thread_attr_t * attr, int detachstate)
{
    attr->detachstate = detachstate;
    return 0;
}

int ztl_thread_attr_setstacksize(ztl_thread_attr_t * attr, size_t stacksize)
{
    attr->stacksize = (int)stacksize;
    return 0;
}

int ztl_thread_create(ztl_thread_t * thr, ztl_thread_attr_t * attr, 
    ztl_thread_func_t myfunc, void * args)
{
    // _beginthreadex returns 0 on an error
    HANDLE h = 0;
    unsigned int lthr;

    if (NULL != attr) {
        h = (HANDLE)_beginthreadex(NULL, attr->stacksize, myfunc, args, 0, &lthr);
    }
    else {
        h = (HANDLE)_beginthreadex(NULL, 0, myfunc, args, 0, &lthr);
    }

    if (h) {
        *thr = h;
        return 0;
    }
    return GetLastError();
}

int ztl_thread_join(ztl_thread_t thr, void **retval)
{
    (void)retval;
    if (thr) {
        WaitForSingleObject(thr, INFINITE);
    }
    return 0;
}


#endif//_MSC_VER
