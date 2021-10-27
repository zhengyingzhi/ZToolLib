/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_LOCKS_H_
#define _ZTL_LOCKS_H_

#include <stdint.h>
#include <stdbool.h>

#define ZTL_LOCK_SPIN           2048
#define ZTL_RWLOCK_WLOCK        ((uint32_t)-1)

#ifdef __cplusplus
extern "C" {
#endif

/* =======================================================================
 * spin lock based on a volatile shared address
 */
void ztl_spinlock(volatile uint32_t* lock, uint32_t value, uint32_t spincount);
#define ztl_trylock(lock,val)   (*(lock) == 0 && atomic_cas(lock, 0, val))
#define ztl_unlock(lock)        *(lock) = 0


/* =======================================================================
 * read-write lock based on a volatile shared address
 */
void ztl_rwlock_wlock(volatile uint32_t* lock);
void ztl_rwlock_rlock(volatile uint32_t* lock);
void ztl_rwlock_unlock(volatile uint32_t* lock);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_LOCKS_H_
