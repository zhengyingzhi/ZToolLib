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
typedef struct ring_st  ring_t;

/* exported functions */
ring_t* ring_new(void);
ring_t* ring_ring(void* x, ...);
void    ring_free(ring_t* ring);
size_t  ring_length(ring_t* ring);
void*   ring_put(ring_t* ring, int i, void* x);
void*   ring_get(ring_t* ring, int i);
void*   ring_add(ring_t* ring, int pos, void* x);
void*   ring_addlo(ring_t* ring, void* x);
void*   ring_addhi(ring_t* ring, void* x);
void*   ring_remove(ring_t* ring, int i);
void*   ring_remlo(ring_t* ring);
void*   ring_remhi(ring_t* ring);
void    ring_rotate(ring_t* ring, int n);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_RING_H_INCLUDED_
