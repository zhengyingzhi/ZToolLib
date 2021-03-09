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
typedef struct heap_st* heap_t;

/* exported functions */
heap_t  heap_new(int height, size_t offset, int cmp(const void *x, const void *y));
void    heap_free(heap_t *hp);
int     heap_length(heap_t heap);
int     heap_push(heap_t heap, void *elem);
void*   heap_pop(heap_t heap);
void*   heap_remove(heap_t heap, void *elem);
void*   heap_peek(heap_t heap, uint32_t index);
void    heap_lock(heap_t heap);
void    heap_unlock(heap_t heap);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_HEAP_H_INCLUDED_
