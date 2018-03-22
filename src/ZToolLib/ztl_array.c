#include <string.h>
#include <stdlib.h>

#include "ztl_array.h"


ztl_array_t* ztl_array_create(ztl_pool_t* pool, uint32_t num, size_t eltsize)
{
    ztl_array_t* array;

    if (pool)
        array = (ztl_array_t*)ztl_palloc(pool, sizeof(ztl_array_t));
    else
        array = (ztl_array_t*)malloc(sizeof(ztl_array_t));

    if (array == NULL) {
        return NULL;
    }

    if (ztl_array_init(array, pool, num, eltsize) != 0) {
        return NULL;
    }

    return array;
}


void ztl_array_destroy(ztl_array_t* arr)
{
    ztl_pool_t* pool;

    pool = arr->pool;

    if (pool) {

        if ((uint8_t*)arr->elts + arr->eltsize * arr->nalloc == pool->d.last) {
            pool->d.last -= arr->eltsize * arr->nalloc;
        }

        if ((uint8_t*)arr + sizeof(ztl_array_t) == pool->d.last) {
            pool->d.last = (uint8_t*)arr;
        }
    }
    else {
        if (arr->elts)
            free(arr->elts);
        free(arr);
    }
}


void* ztl_array_push(ztl_array_t* arr)
{
    void        *elt, *new;
    size_t       size;
    ztl_pool_t  *p;

    if (arr->nelts == arr->nalloc) {

        /* the array is full */

        size = arr->eltsize * arr->nalloc;

        p = arr->pool;

        if ((uint8_t*)arr->elts + size == p->d.last
            && p->d.last + arr->eltsize <= p->d.end)
        {
            /*
            * the array allocation is the last in the pool
            * and there is space for new allocation
            */

            p->d.last += arr->eltsize;
            arr->nalloc++;

        }
        else {
            /* allocate a new array */

            new = ztl_palloc(p, 2 * size);
            if (new == NULL) {
                return NULL;
            }

            memcpy(new, arr->elts, size);
            arr->elts = new;
            arr->nalloc *= 2;
        }
    }

    elt = (uint8_t*)arr->elts + arr->nelts * arr->eltsize;
    arr->nelts++;

    return elt;
}


void* ztl_array_push_n(ztl_array_t* arr, uint32_t n)
{
    void        *elt, *new;
    size_t       size;
    uint32_t   nalloc;
    ztl_pool_t  *p;

    size = n * arr->eltsize;

    if (arr->nelts + n > arr->nalloc) {

        /* the array is full */

        p = arr->pool;

        if ((uint8_t*)arr->elts + arr->eltsize * arr->nalloc == p->d.last
            && p->d.last + size <= p->d.end)
        {
            /*
            * the array allocation is the last in the pool
            * and there is space for new allocation
            */

            p->d.last += size;
            arr->nalloc += n;

        }
        else {
            /* allocate a new array */

            nalloc = 2 * ((n >= arr->nalloc) ? n : arr->nalloc);

            new = ztl_palloc(p, nalloc * arr->eltsize);
            if (new == NULL) {
                return NULL;
            }

            memcpy(new, arr->elts, arr->nelts * arr->eltsize);
            arr->elts = new;
            arr->nalloc = nalloc;
        }
    }

    elt = (uint8_t*)arr->elts + arr->eltsize * arr->nelts;
    arr->nelts += n;

    return elt;
}
