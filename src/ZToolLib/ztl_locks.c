#include "ztl_locks.h"
#include "ztl_atomic.h"

#ifdef _MSC_VER
#include <windows.h>
#endif//_MSC_VER

//////////////////////////////////////////////////////////////////////////
/* spin lock for mutex access */
void ztl_spinlock(volatile uint32_t* lock, uint32_t value, uint32_t spincount)
{
    uint32_t i, n;

    for (;; ) {

        if (*lock == 0 && ztl_atomic_cas(lock, 0, value)) {
            return;
        }

            for (n = 1; n < spincount; n <<= 1) {

                for (i = 0; i < n; i++) {
                    ztl_cpu_pause();
                }

                if (*lock == 0 && ztl_atomic_cas(lock, 0, value)) {
                    return;
                }
            }

        ztl_sched_yield();
    }
}


//////////////////////////////////////////////////////////////////////////
/* spin lock for read-write access */
void ztl_rwlock_wlock(volatile uint32_t* lock)
{
    uint32_t  i, n;

    for (;; ) {

        if (*lock == 0 && ztl_atomic_cas(lock, 0, ZTL_RWLOCK_WLOCK)) {
            return;
        }


        for (n = 1; n < ZTL_LOCK_SPIN; n <<= 1) {

            for (i = 0; i < n; i++) {
                ztl_cpu_pause();
            }

            if (*lock == 0
                && ztl_atomic_cas(lock, 0, ZTL_RWLOCK_WLOCK))
            {
                return;
            }
        }

        ztl_sched_yield();
    }
}

void ztl_rwlock_rlock(volatile uint32_t* lock)
{
    uint32_t            i, n;
    volatile uint32_t   readers;

    for (;; ) {
        readers = *lock;

        if (readers != ZTL_RWLOCK_WLOCK
            && ztl_atomic_cas(lock, readers, readers + 1))
        {
            return;
        }


        for (n = 1; n < ZTL_LOCK_SPIN; n <<= 1) {

            for (i = 0; i < n; i++) {
                ztl_cpu_pause();
            }

            readers = *lock;

            if (readers != ZTL_RWLOCK_WLOCK
                && ztl_atomic_cas(lock, readers, readers + 1))
            {
                return;
            }
        }

        ztl_sched_yield();
    }
}

void ztl_rwlock_unlock(volatile uint32_t* lock)
{
    volatile uint32_t readers;

    readers = *lock;

    if (readers == ZTL_RWLOCK_WLOCK) {
        *lock = 0;
        return;
    }

    for (;; ) {

        if (ztl_atomic_cas(lock, readers, readers - 1)) {
            return;
        }

        readers = *lock;
    }
}

