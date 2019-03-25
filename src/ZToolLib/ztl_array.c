#include <string.h>
#include <stdlib.h>

#include "ztl_array.h"


int _ztl_array_init(ztl_array_t* array, ztl_pool_t* pool, uint32_t num, size_t eltsize)
{
    /*
    * set "array->nelts" before "array->elts", otherwise MSVC thinks
    * that "array->nelts" may be used without having been initialized
    */

    array->nelts    = 0;
    array->nalloc   = num;
    array->eltsize  = (uint32_t)eltsize;
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


ztl_array_t* ztl_array_create(ztl_pool_t* pool, uint32_t num, size_t eltsize)
{
    ztl_array_t* array;

    if (pool)
        array = (ztl_array_t*)ztl_pcalloc(pool, sizeof(ztl_array_t));
    else
        array = (ztl_array_t*)calloc(1, sizeof(ztl_array_t));

    if (array == NULL) {
        return NULL;
    }

    array->const_obj = 0;

    if (_ztl_array_init(array, pool, num, eltsize) != 0) {
        if (!pool)
            free(array);
        return NULL;
    }

    return array;
}

int ztl_array_init(ztl_array_t* array, ztl_pool_t* pool, uint32_t num, size_t eltsize)
{
    array->const_obj = 1;

    return _ztl_array_init(array, pool, num, eltsize);
}

void ztl_array_release(ztl_array_t* arr)
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

        /* the array object was not malloced from os */
        if (!arr->const_obj)
            free(arr);
    }
}

void ztl_array_clear(ztl_array_t* arr)
{
    if (arr->pool)
    {
        ztl_pool_t* pool = arr->pool;
        if ((uint8_t*)arr->elts + arr->eltsize * arr->nalloc == pool->d.last) {
            pool->d.last -= arr->eltsize * arr->nalloc;
        }

        if ((uint8_t*)arr + sizeof(ztl_array_t) == pool->d.last) {
            pool->d.last = (uint8_t*)arr;
        }
    }
    arr->nelts = 0;
}

bool ztl_array_reserve(ztl_array_t* arr, uint32_t reserve_num)
{
    void        *enew;
    size_t      size;

    if (reserve_num > arr->nalloc)
    {
        ztl_pool_t* pool;
        pool = arr->pool;

        size = arr->eltsize * arr->nalloc;

        if (pool)
        {
            if ((uint8_t*)arr->elts + reserve_num == pool->d.last
                && pool->d.last + arr->eltsize <= pool->d.end)
            {
                /*
                * the array allocation is the last in the pool
                * and there is space for new allocation
                */

                pool->d.last += arr->eltsize;
                arr->nalloc++;

            }
            else {
                /* allocate a new array */

                enew = ztl_palloc(pool, reserve_num * arr->eltsize);
                if (enew == NULL) {
                    return false;
                }

                memcpy(enew, arr->elts, size);
                arr->elts = enew;
                arr->nalloc = reserve_num;
            }
        }
        else
        {
            enew = realloc(arr->elts, reserve_num * arr->eltsize);
            if (enew == NULL) {
                return false;
            }

            arr->elts = enew;
            arr->nalloc = reserve_num;
        }
    }

    return true;
}

bool ztl_array_push_back(ztl_array_t* arr, void* elem)
{
    char* lpaddr;
    lpaddr = (char*)ztl_array_push(arr);
    if (!lpaddr) {
        return false;
    }

    switch (arr->eltsize)
    {
    case 1:
        *(uint8_t*)lpaddr = *(uint8_t*)elem;
        break;
    case 2:
        *(uint16_t*)lpaddr = *(uint16_t*)elem;
        break;
    case 3:
        *(uint16_t*)lpaddr = *(uint16_t*)elem;
        *(uint8_t*)(lpaddr + 2) = *(uint8_t*)((char*)elem + 2);
        break;
    case 4:
        *(uint32_t*)lpaddr = *(uint32_t*)elem;
        break;
    case 5:
        *(uint32_t*)lpaddr = *(uint32_t*)elem;
        *(uint8_t*)(lpaddr + 4) = *(uint8_t*)((char*)elem + 4);
        break;
    case 6:
        *(uint32_t*)lpaddr = *(uint32_t*)elem;
        *(uint16_t*)(lpaddr + 4) = *(uint16_t*)((char*)elem + 4);
        break;
    case 7:
        *(uint32_t*)lpaddr = *(uint32_t*)elem;
        *(uint16_t*)(lpaddr + 4) = *(uint16_t*)((char*)elem + 4);
        *(uint8_t*)(lpaddr + 6) = *(uint8_t*)((char*)elem + 6);
        break;
    case 8:
        *(uint64_t*)lpaddr = *(uint64_t*)elem;
        break;
    case 12:
        *(uint64_t*)lpaddr = *(uint64_t*)elem;
        *(uint32_t*)(lpaddr + 8)= *(uint32_t*)((char*)elem + 8);
        break;
    case 16:
        *(uint64_t*)lpaddr = *(uint64_t*)elem;
        *(uint64_t*)(lpaddr + 8) = *(uint64_t*)((char*)elem + 8);
        break;
    default:
        memcpy(lpaddr, elem, arr->eltsize);
    }

    return true;
}

void* ztl_array_pop_back(ztl_array_t* arr)
{
    void* elem = NULL;
    if (arr->nelts > 0) {
        arr->nelts--;
        elem = (uint8_t*)arr->elts + arr->nelts * arr->eltsize;
    }

    return elem;
}

void* ztl_array_push(ztl_array_t* arr)
{
    void *elt;

    if (arr->nelts == arr->nalloc) {

        /* the array is full */
        
        if (!ztl_array_reserve(arr, arr->nalloc << 1)) {
            return NULL;
        }
    }

    elt = (uint8_t*)arr->elts + arr->nelts * arr->eltsize;
    arr->nelts++;

    return elt;
}


void* ztl_array_push_n(ztl_array_t* arr, uint32_t n)
{
    void *elt;

    if (arr->nelts + n > arr->nalloc) {

        /* the array is full */

        if (!ztl_array_reserve(arr, arr->nalloc << 1)) {
            return NULL;
        }
    }

    elt = (uint8_t*)arr->elts + arr->eltsize * arr->nelts;
    arr->nelts += n;

    return elt;
}


static inline int _ztl_array_cmp(void* expect, void* actual)
{
    if (expect == actual) {
        return 0;
    }
    else if (expect < actual) {
        return -1;
    }
    else {
        return 1;
    }
}

void* ztl_array_find(ztl_array_t* arr, void* expect, int(*cmp)(void* expect, void* actual))
{
    if (!cmp)
        cmp = _ztl_array_cmp;

    void* actual;

    for (uint32_t x = 0; x < ztl_array_size(arr); ++x)
    {
        actual = ztl_array_at(arr, x);
        if (cmp(expect, actual) == 0) {
            break;
        }
        actual = NULL;
    }

    return actual;
}

void ztl_array_remove_value(ztl_array_t* arr, void* value, int(*cmp)(void* value, void* actual))
{
    if (!cmp)
        cmp = _ztl_array_cmp;

    uint32_t arrsize;
    char* actual;

    arrsize = ztl_array_size(arr);

    for (uint32_t x = 0; x < arrsize; ++x)
    {
        actual = ztl_array_at(arr, x);
        if (cmp(value, actual) == 0)
        {
            if (x != arrsize - 1)
            {
                memmove(actual, actual + arr->eltsize, (arrsize - x - 1) * arr->eltsize);
            }

            arr->nelts -= 1;
            break;
        }
        actual = NULL;
    }
}

void* ztl_array_remove_index(ztl_array_t* arr, uint32_t index)
{
    char* addr = NULL;
    if (index > arr->nelts) {
        return NULL;
    }

    addr = ztl_array_at(arr, index);
    if (index != arr->nelts - 1)
    {
        memmove(addr, addr + arr->eltsize, (arr->nelts - index - 1) * arr->eltsize);
    }

    arr->nelts -= 1;
    return arr;
}

void ztl_array_foreach(ztl_array_t* arr, void* udata, void(*fn)(void* value, void* udata))
{
    for (uint32_t x = 0; x < ztl_array_size(arr); ++x)
    {
        fn(ztl_array_at(arr, x), udata);
    }
}
