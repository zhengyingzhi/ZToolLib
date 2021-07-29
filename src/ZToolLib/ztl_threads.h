#ifndef _ZTL_THREADS_H_
#define _ZTL_THREADS_H_

#ifndef _MSC_VER

/// linux pthread

#include <pthread.h>
#include <unistd.h>

typedef void *                          ztl_thread_result_t;
typedef pthread_mutex_t                 ztl_thread_mutex_t;
typedef pthread_cond_t                  ztl_thread_cond_t;
typedef pthread_t                       ztl_thread_t;
typedef pthread_attr_t                  ztl_thread_attr_t;
typedef pthread_rwlock_t                ztl_thread_rwlock_t;

/// ztl thread mutex lock
#define ztl_thread_mutex_init           pthread_mutex_init
#define ztl_thread_mutex_destroy        pthread_mutex_destroy
int ztl_thread_rwlock_init(ztl_thread_rwlock_t* rwlock);
int ztl_thread_rwlock_destroy(ztl_thread_rwlock_t* rwlock);
#define ztl_thread_rwlock_rdlock        pthread_rwlock_rdlock
#define ztl_thread_rwlock_wrlock        pthread_rwlock_wrlock
#define ztl_thread_rwlock_unlock        pthread_rwlock_unlock

#define ztl_thread_mutex_lock           pthread_mutex_lock
#define ztl_thread_mutex_unlock         pthread_mutex_unlock

#define ztl_thread_mutexattr_t          pthread_mutexattr_t
#define ztl_thread_mutexattr_init       pthread_mutexattr_init
#define ztl_thread_mutexattr_settype    pthread_mutexattr_settype
#define ztl_thread_mutexattr_destroy    pthread_mutexattr_destroy
#define ZTL_THREAD_MUTEX_ADAPTIVE_NP    PTHREAD_MUTEX_ADAPTIVE_NP

#define ztl_thread_cond_init            pthread_cond_init
#define ztl_thread_cond_destroy         pthread_cond_destroy
#define ztl_thread_cond_wait            pthread_cond_wait
#define ztl_thread_cond_signal          pthread_cond_signal
#define ztl_thread_cond_broadcast       pthread_cond_broadcast

#define ztl_thread_attr_init            pthread_attr_init
#define ztl_thread_attr_destroy         pthread_attr_destroy
#define ztl_thread_attr_setdetachstate  pthread_attr_setdetachstate
#define ZTL_THREAD_CREATE_DETACHED      PTHREAD_CREATE_DETACHED
#define ztl_thread_attr_setstacksize    pthread_attr_setstacksize

int ztl_gettid();
#define ztl_thread_self                 pthread_self
#define ztl_thread_create               pthread_create
#define ztl_thread_join                 pthread_join

#define ZTL_THREAD_CALL
typedef ztl_thread_result_t (ZTL_THREAD_CALL* ztl_thread_func_t)(void* args);

#else ///////////////////////////////////////////////////////////////////////

/// windows thread
#include <windows.h>
#include <process.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void*               ztl_thread_t;
typedef unsigned            ztl_thread_result_t;

#define ZTL_THREAD_CALL __stdcall
typedef ztl_thread_result_t (ZTL_THREAD_CALL* ztl_thread_func_t )( void * args );

typedef CRITICAL_SECTION    ztl_thread_mutex_t;
typedef CRITICAL_SECTION    ztl_thread_rwlock_t;
typedef HANDLE              ztl_thread_cond_t;

typedef struct tag_thread_attr {
    int stacksize;
    int detachstate;
}ztl_thread_attr_t;

typedef struct tag_thread_mutex_attr {
    int type;
}ztl_thread_mutexattr_t;

#define ZTL_THREAD_CREATE_DETACHED  1

/// thread mutex lock
int ztl_thread_mutex_init(ztl_thread_mutex_t * mutex, void * attr );
int ztl_thread_mutex_destroy(ztl_thread_mutex_t * mutex );
int ztl_thread_mutex_lock(ztl_thread_mutex_t * mutex );
int ztl_thread_mutex_unlock(ztl_thread_mutex_t * mutex );

int ztl_thread_rwlock_init(ztl_thread_rwlock_t* rwlock);
int ztl_thread_rwlock_rdlock(ztl_thread_rwlock_t* rwlock);
int ztl_thread_rwlock_wrlock(ztl_thread_rwlock_t* rwlock);
int ztl_thread_rwlock_unlock(ztl_thread_rwlock_t* rwlock);
int ztl_thread_rwlock_destroy(ztl_thread_rwlock_t* rwlock);

int ztl_thread_mutexattr_init(ztl_thread_mutexattr_t* mattr);
int ztl_thread_mutexattr_settype(ztl_thread_mutexattr_t* mattr, int type);
int ztl_thread_mutexattr_destroy(ztl_thread_mutexattr_t* mattr);
#define ZTL_THREAD_MUTEX_ADAPTIVE_NP 0

int ztl_thread_cond_init(ztl_thread_cond_t * cond, void * attr );
int ztl_thread_cond_destroy(ztl_thread_cond_t * cond );
int ztl_thread_cond_wait(ztl_thread_cond_t * cond, ztl_thread_mutex_t * mutex );
int ztl_thread_cond_signal(ztl_thread_cond_t * cond );
int ztl_thread_cond_broadcast(ztl_thread_cond_t * cond );

int ztl_thread_attr_init(ztl_thread_attr_t * attr );
int ztl_thread_attr_destroy(ztl_thread_attr_t * attr );
int ztl_thread_attr_setdetachstate(ztl_thread_attr_t * attr, int detachstate );
int ztl_thread_attr_setstacksize(ztl_thread_attr_t * attr, size_t stacksize );

/// create a new thread, return 0 if success
int ztl_thread_create(ztl_thread_t * thr, ztl_thread_attr_t * attr, ztl_thread_func_t myfunc, void * args );

int ztl_thread_join(ztl_thread_t thr, void **retval);

/// get thread id
int ztl_getpid();
int ztl_gettid();
int ztl_thread_self();


#ifdef __cplusplus
}
#endif

#endif//_MSC_VER

#endif//_ZTL_THREADS_H_
