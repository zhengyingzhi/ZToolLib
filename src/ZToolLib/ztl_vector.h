/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 * a cpp style vector implemented by pure c
 */

#ifndef _ZTL_VECTOR_H_INCLUDED_
#define _ZTL_VECTOR_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ztl_vector_s ztl_vector_t;
struct ztl_vector_s {
    char*       elts;
    uint32_t    nelts;
    uint32_t    nalloc;
    uint32_t    eltsize;
    int  (*reserve)(ztl_vector_t* vec, uint32_t reserve_num);
    void (*clear)(ztl_vector_t* vec);
    void (*push_char)(ztl_vector_t* vec, int8_t val);
    void (*push_short)(ztl_vector_t* vec, int16_t val);
    void (*push_int)(ztl_vector_t* vec, int32_t val);
    void (*push_int64)(ztl_vector_t* vec, int64_t val);
    void (*push_float)(ztl_vector_t* vec, float val);
    void (*push_double)(ztl_vector_t* vec, double val);
    void (*push_ptr)(ztl_vector_t* vec, void* ptr);
    void (*push_x)(ztl_vector_t* vec, void* elem);
};

/* init a dynamic array (cpp style)
 */
int ztl_vector_init(ztl_vector_t* vec, uint32_t init_num, size_t eltsize);

/* create a dynamic array (cpp style)
 * @num  the initial array size
 * @eltsize each elem size, like sizeof(int)
 * @return the new array object
 */
ztl_vector_t* ztl_vector_create(uint32_t init_num, size_t eltsize);

/* release the array
 * vec: the object returned by ztl_vector_create
 */
void ztl_vector_release(ztl_vector_t* vec);


#define ZTL_VEC_CHECK_RESERVE(vec) \
    do { \
        if (vec->nelts == vec->nalloc) { \
            vec->reserve(vec, vec->nalloc << 1); \
        } \
    } while (false);

#define ztl_vector_push(vec, val, type_t) \
    do { \
        if (vec->nelts == vec->nalloc) { \
            vec->reserve(vec, vec->nalloc << 1); \
        } \
        type_t* pv = (type_t*)vec->elts; \
        pv[vec->nelts++] = val; \
    } while (true);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_VECTOR_H_INCLUDED_
