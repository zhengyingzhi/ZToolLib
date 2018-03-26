/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_ATOMIC_H_INCLUDE_
#define _ZTL_ATOMIC_H_INCLUDE_

#include <stdint.h>     // uint32_t

#ifdef _MSC_VER

#ifndef _WIN64
#define ztl_cpu_pause()     __asm { pause }
#else
#define ztl_cpu_pause()     
#endif

#define ztl_sched_yield     SwitchToThread
#else

#include <unistd.h>
#include <sched.h>
#define ztl_cpu_pause()     __asm__ ("pause")
//#define ztl_cpu_pause() __asm__ volatile("rep; nop" ::: "memory")

#define ztl_sched_yield     sched_yield
#endif//_MSC_VER


#ifdef _MSC_VER
/* declare the automic operations on win32 platform
 * @brief  atomically adds a_count to the variable pointed by a_ptr
 * @return the value that had previously been in memory
 */
#define ztl_atomic_add(ptr, count)      (InterlockedExchangeAdd((uint32_t*)(ptr), (count)))
#define ztl_atomic_add64(ptr, count)    (InterlockedExchangeAdd64((uint64_t*)(ptr), (count)))

/* @brief  atomically substracts a_count from the variable pointed by a_ptr
 * @return the value that had previously been in memory
 *
 */#define ztl_atomic_dec(ptr, count)      (InterlockedExchangeAdd((uint32_t*)(ptr), -((int32_t)count)))
#define ztl_atomic_dec64(ptr, count)    (InterlockedExchangeAdd64((uint64_t*)(ptr), -((int32_t)count)))

/* @brief  atomically set a_count to the variable pointed by a_ptr
 * @return the value that had previously been in memory
 */
#define ztl_atomic_set(ptr, newVal)     (InterlockedExchange((uint32_t*)(ptr), (newVal)))
#define ztl_atomic_set64(ptr, newVal)   (InterlockedExchange64((uint64_t*)(ptr), (newVal)))

/* @brief  Compare And Swap value
 * @note   If the  current value of *ptr is equal to compVal, then write newVal into *ptr
 * @return true if the comparison is successful and newVal was written
 */
#define ztl_atomic_cas(ptr, compVal, newVal)    ((compVal) == InterlockedCompareExchange((ptr), (newVal), (compVal)))
#define ztl_atomic_cas64(ptr, compVal, newVal)  ((compVal) == InterlockedCompareExchange64((ptr), (newVal), (compVal)))

/* @brief  Compare And Swap pointer
 * @note   If the  current value of ptr is oldPtr, then write newPtr into ptr
 * @return true if the comparison is successful and newVal was written
 */
#define ztl_atomic_casptr(ptr, compPtr, newPtr) ((compPtr) == InterlockedCompareExchangePointer((PVOID*)(ptr), (PVOID*)(newPtr), (PVOID*)(compPtr)))

#else
#ifdef __GNUC__
// Atomic functions in GCC are present from version 4.1.0 on
// http://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html

// Test for GCC >= 4.1.0
#if (__GNUC__ < 4) || \
	((__GNUC__ == 4) && ((__GNUC_MINOR__ < 1) || \
	((__GNUC_MINOR__     == 1) && \
	(__GNUC_PATCHLEVEL__ < 0))) )

#error Atomic built-in functions are only available in GCC in versions >= 4.1.0
#endif // end of check for GCC 4.1.0

/* @brief  atomically adds a_count to the variable pointed by a_ptr
 * @return the value that had previously been in memory
 */
#define ztl_atomic_add(ptr, count)      __sync_fetch_and_add((uint32_t*)(ptr), (count))
#define ztl_atomic_add64(ptr, count)    __sync_fetch_and_add((uint64_t*)(ptr), (count))

/* @brief  atomically substracts a_count from the variable pointed by a_ptr
 * @return the value that had previously been in memory
 */
#define ztl_atomic_dec(ptr, count)      __sync_fetch_and_sub((uint32_t*)(ptr), (count))
#define ztl_atomic_dec64(ptr, count)    __sync_fetch_and_sub((uint64_t*)(ptr), (count))

/* @brief  atomically set a_count to the variable pointed by a_ptr
 * @return the value that had previously been in memory
 */
#define ztl_atomic_set(ptr, newVal)     __sync_lock_test_and_set((uint32_t*)(ptr), (newVal))
#define ztl_atomic_set64(ptr, newVal)   __sync_lock_test_and_set((uint64_t*)(ptr), (newVal))

/* @brief  Compare And Swap
 * @note   If the  current value of *ptr is equal to compVal, then write newVal into *ptr
 * @return true if the comparison is successful and newVal was written
 */
#define ztl_atomic_cas(ptr, compVal, newVal)    __sync_bool_compare_and_swap((ptr), (compVal), (newVal))
#define ztl_atomic_cas64(ptr, compVal, newVal)  __sync_bool_compare_and_swap((ptr), (compVal), (newVal))

/* @brief  Compare And Swap pointer
 * @note   If the  current value of ptr is oldPtr, then write newPtr into ptr
 * @return true if the comparison is successful and newVal was written
 */
#define ztl_atomic_casptr(ptr, compPtr, newPtr) (__sync_bool_compare_and_swap((void**)(ptr), (void*)(newPtr), (void*)(compPtr)))

#else
#error Atomic functions such as __sync_xxx_yyy are not defined for your compiler. Please add them in atomic_ops.h
#endif//__GNUC__

#endif//_MSC_VER


#endif//_ZTL_ATOMIC_H_INCLUDE_
