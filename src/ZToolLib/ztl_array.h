/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_ARRAY_H_INCLUDED_
#define _ZTL_ARRAY_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include "ztl_palloc.h"


typedef struct {
    void*       elts;
    uint32_t    nelts;
    uint32_t    nalloc;
    uint32_t    eltsize   : 30;
    uint32_t    const_obj : 2;
    ztl_pool_t* pool;
}ztl_array_t;

#ifdef __cplusplus
extern "C" {
#endif

/* create a dynamic array
 * @pool the alloc pool, which could be null
 * @num  the initial array size
 * @eltsize each elem size, like sizeof(int)
 * @return the new array object
 */
ztl_array_t* ztl_array_create(ztl_pool_t* pool, uint32_t num, size_t eltsize);

/* init an existed array
 * @pool the alloc pool, which could be null
 * @return 0-success
 */
int ztl_array_init(ztl_array_t* arr, ztl_pool_t* pool, uint32_t num, size_t eltsize);

/* release the array
 */
void ztl_array_release(ztl_array_t* arr);

/* clear the array */
void ztl_array_clear(ztl_array_t* arr);

/* reserve the array */
bool ztl_array_reserve(ztl_array_t* arr, uint32_t reserve_num);

/* push an element at tail */
bool ztl_array_push_back(ztl_array_t* arr, void* elem);
/* pop an element from tail */
void* ztl_array_pop_back(ztl_array_t* arr);

/* get the new pushed element's address */
void* ztl_array_push(ztl_array_t* arr);
void* ztl_array_push_n(ztl_array_t* arr, uint32_t n);

#define ztl_array_at(arr, pos)  ((uint8_t*)arr->elts + pos * arr->eltsize)
#define ztl_array_size(arr)     ((arr)->nelts)
#define ztl_array_isempty(arr)  ((arr)->nelts == 0)

#ifdef __cplusplus
}
#endif

#endif//_ZTL_ARRAY_H_INCLUDED_
