#ifndef _ZTL_ARRAY_H_INCLUDED_
#define _ZTL_ARRAY_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include "ztl_palloc.h"


typedef struct {
    void*       elts;
    uint32_t    nelts;
    size_t      eltsize;
    uint32_t    nalloc;
    ztl_pool_t* pool;
}ztl_array_t;

#ifdef __cplusplus
extern "C" {
#endif

ztl_array_t* ztl_array_create(ztl_pool_t* pool, uint32_t num, size_t eltsize);
void  ztl_array_destroy(ztl_array_t* arr);
void* ztl_array_push(ztl_array_t* arr);
void* ztl_array_push_n(ztl_array_t* arr, uint32_t n);

#define ztl_array_at(arr, pos)  ((uint8_t*)arr->elts + arr->nelts * arr->eltsize)
#define ztl_array_size(arr)     ((arr)->nelts)
#define ztl_array_isempty(arr)  ((arr)->nelts == 0)

static int ztl_array_init(ztl_array_t* array, ztl_pool_t* pool, uint32_t num, size_t eltsize)
{
    /*
    * set "array->nelts" before "array->elts", otherwise MSVC thinks
    * that "array->nelts" may be used without having been initialized
    */

    array->nelts    = 0;
    array->eltsize  = eltsize;
    array->nalloc   = num;
    array->pool     = pool;

    if (pool)
        array->elts = ztl_palloc(pool, num * eltsize);
    else
        array->elts = malloc(num * eltsize);

    if (array->elts == NULL) {
        return -1;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif//_ZTL_ARRAY_H_INCLUDED_
