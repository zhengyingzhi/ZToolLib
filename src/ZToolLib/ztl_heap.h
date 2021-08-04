/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_RING_H_INCLUDED_
#define _ZTL_RING_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* exported types */
typedef struct heap_st  heap_t;

/* exported functions */

/* The 'offset' parameter is optional, but must be provided to be able to use heap_remove().
 * If heap_remove() will not be used, then a negative value can be provided.
 * cmp: return 1 if x < y when a least heap, return -1 if x < y when a largest heap
 */
heap_t* heap_new(int height, size_t offset, int cmp(const void *x, const void *y));
void    heap_free(heap_t* hp);
int     heap_length(heap_t* hp);
int     heap_push(heap_t* hp, void* elem);
void*   heap_pop(heap_t* hp);
void*   heap_remove(heap_t* hp, void* elem);

/* Peek the top elem, and 'index' starts with 1 */
void*   heap_peek(heap_t* hp, uint32_t index);
void    heap_lock(heap_t* hp);

void    heap_unlock(heap_t* hp);

/* the default cmp func 
 * heap_int_cmp_least return  1 if (x < y), else 0 if x == y
 * heap_int_cmp_large return -1 if (x < y), else 0 if x == y
 */
int     heap_cmp_least_int(const void *x, const void *y);
int     heap_cmp_large_int(const void *x, const void *y);
int     heap_cmp_least_pint32(const void *x, const void *y);
int     heap_cmp_large_pint32(const void *x, const void *y);
int     heap_cmp_least_pint64(const void *x, const void *y);
int     heap_cmp_large_pint64(const void *x, const void *y);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_HEAP_H_INCLUDED_
