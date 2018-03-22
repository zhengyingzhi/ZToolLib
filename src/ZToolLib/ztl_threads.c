#include "ztl_threads.h"

#ifdef _MSC_VER

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

int ztl_thread_mutexattr_init(ztl_thread_mutex_attr_t * mattr)
{
    return 0;
}
int ztl_thread_mutexattr_settype(ztl_thread_mutex_attr_t * mattr, int type)
{
    mattr->type = type;
    return 0;
}
int ztl_thread_mutexattr_destroy(ztl_thread_mutex_attr_t * mattr)
{
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

ztl_thread_t ztl_thread_self()
{
    return GetCurrentThreadId();
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
    attr->stacksize = stacksize;
    return 0;
}

int ztl_thread_create(ztl_thread_t * thr, ztl_thread_attr_t * attr, 
    ztl_thread_func_t myfunc, void * args)
{
    // _beginthreadex returns 0 on an error
    HANDLE h = 0;

    if (NULL != attr) {
        h = (HANDLE)_beginthreadex(NULL, attr->stacksize, myfunc, args, 0, thr);
    }
    else {
        h = (HANDLE)_beginthreadex(NULL, 0, myfunc, args, 0, thr);
    }

    return h > 0 ? 0 : GetLastError();
}

#endif//_MSC_VER
