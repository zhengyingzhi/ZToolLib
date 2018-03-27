/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_BUFFER_H_INCLUDED_
#define _ZTL_BUFFER_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

typedef struct ztl_buffer_st ztl_buffer_t;

typedef void*(*ztl_buffer_alloc_pt)(void* ctx, void* oldaddr, uint32_t size);
typedef void (*ztl_buffer_dealloc_pt)(void* ctx, void* addr);

struct ztl_buffer_st
{
    char*       data;
    uint32_t    size;
    uint32_t    capacity    : 31;
    uint32_t    const_addr  : 1;
    ztl_buffer_alloc_pt     alloc;
    ztl_buffer_dealloc_pt   dealloc;
    void*                   ctx;
};

#ifdef __cplusplus
extern "C" {
#endif

/* int a buffer */
void ztl_buffer_init(ztl_buffer_t* zbuf);

/* also we could specify outside addr */
void ztl_buffer_init_byaddr(ztl_buffer_t* zbuf, void* addr, uint32_t capacity);

/* set outside alloc func ptr */
void ztl_buffer_set_alloc_func(ztl_buffer_t* zbuf, ztl_buffer_alloc_pt alloc_func, 
    ztl_buffer_dealloc_pt dealloc_func, void* ctxdata);

/* release the buffer if data is alloced from os */
void ztl_buffer_release(ztl_buffer_t* zbuf);

bool ztl_buffer_reserve(ztl_buffer_t* zbuf, uint32_t capacity);
bool ztl_buffer_compact(ztl_buffer_t* zbuf);

/* insert data */
void ztl_buffer_append(ztl_buffer_t* zbuf, void* adata, uint32_t asize);
void ztl_buffer_insert(ztl_buffer_t* zbuf, uint32_t pos, void* adata, uint32_t asize);
void ztl_buffer_fill(ztl_buffer_t* zbuf, uint32_t pos, uint32_t count, void* adata, uint32_t asize);
void ztl_buffer_erase(ztl_buffer_t* zbuf, uint32_t pos, uint32_t size);

/* clear size */
#define ztl_buffer_clear(zbuf)      (zbuf)->size = 0

#define ztl_buffer_empty(zbuf)      (zbuf)->size == 0
#define ztl_buffer_size(zbuf)       (zbuf)->size
#define ztl_buffer_capacity(zbuf)   (zbuf)->capacity

/* element access */
#define ztl_buffer_data(zbuf)       (zbuf)->data


#ifdef __cplusplus
}
#endif


#endif//_ZTL_BUFFER_H_INCLUDED_
