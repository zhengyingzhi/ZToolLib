#include <string.h>
#include <stdlib.h>

#include "ztl_utils.h"

#include "ztl_vector.h"



void ztl_vector_clear(ztl_vector_t* vec);
int  ztl_vector_reserve(ztl_vector_t* vec, uint32_t reserve_num);

void ztl_push_char(ztl_vector_t* vec, int8_t val);
void ztl_push_short(ztl_vector_t* vec, int16_t val);
void ztl_push_int(ztl_vector_t* vec, int32_t val);
void ztl_push_int64(ztl_vector_t* vec, int64_t val);
void ztl_push_float(ztl_vector_t* vec, float val);
void ztl_push_double(ztl_vector_t* vec, double val);
void ztl_push_ptr(ztl_vector_t* vec, void* ptr);
void ztl_push_x(ztl_vector_t* vec, void* elem);

static int _ztl_vector_init(ztl_vector_t* array, uint32_t init_num, size_t eltsize)
{
    /*
    * set "array->nelts" before "array->elts", otherwise MSVC thinks
    * that "array->nelts" may be used without having been initialized
    */

    init_num        = ztl_align(init_num, 4);
    array->elts     = (char*)malloc(init_num * eltsize);
    array->nelts    = 0;
    array->nalloc = init_num;
    array->eltsize  = (uint32_t)eltsize;

    array->reserve      = ztl_vector_reserve;
    array->clear        = ztl_vector_clear;
    array->push_char    = ztl_push_char;
    array->push_short   = ztl_push_short;
    array->push_int     = ztl_push_int;
    array->push_int64   = ztl_push_int64;
    array->push_float   = ztl_push_float;
    array->push_double  = ztl_push_double;
    array->push_ptr     = ztl_push_ptr;
    array->push_x       = ztl_push_x;
    return 0;
}


int ztl_vector_init(ztl_vector_t* array, uint32_t init_num, size_t eltsize)
{
    return _ztl_vector_init(array, init_num, eltsize);
}

ztl_vector_t* ztl_vector_create(uint32_t init_num, size_t eltsize)
{
    ztl_vector_t* array;

    array = (ztl_vector_t*)calloc(1, sizeof(ztl_vector_t));
    if (array == NULL) {
        return NULL;
    }

    if (_ztl_vector_init(array, init_num, eltsize) != 0) {
        free(array);
        return NULL;
    }

    return array;
}

void ztl_vector_release(ztl_vector_t* vec)
{
    if (vec->elts) {
        free(vec->elts);
    }
}

void ztl_vector_clear(ztl_vector_t* vec)
{
    vec->nelts = 0;
}

int ztl_vector_reserve(ztl_vector_t* vec, uint32_t reserve_num)
{
    void* enew;
    reserve_num = ztl_align(reserve_num, 4);
    if (vec->nalloc >= reserve_num) {
        return 1;
    }

    if (reserve_num > vec->nalloc)
    {
        enew = realloc(vec->elts, reserve_num * vec->eltsize);
        if (enew == NULL) {
            return -1;
        }

        vec->elts = (char*)enew;
        vec->nalloc = reserve_num;
    }

    return 0;
}


void ztl_push_char(ztl_vector_t* vec, int8_t val)
{
    ZTL_VEC_CHECK_RESERVE(vec);
    int8_t* pv = (int8_t*)vec->elts;
    pv[vec->nelts++] = val;
}

void ztl_push_short(ztl_vector_t* vec, int16_t val)
{
    ZTL_VEC_CHECK_RESERVE(vec);
    int16_t* pv = (int16_t*)vec->elts;
    pv[vec->nelts++] = val;
}

void ztl_push_int(ztl_vector_t* vec, int32_t val)
{
    ZTL_VEC_CHECK_RESERVE(vec);
    int32_t* pv = (int32_t*)vec->elts;
    pv[vec->nelts++] = val;
}

void ztl_push_int64(ztl_vector_t* vec, int64_t val)
{
    ZTL_VEC_CHECK_RESERVE(vec);
    int64_t* pv = (int64_t*)vec->elts;
    pv[vec->nelts++] = val;
}

void ztl_push_float(ztl_vector_t* vec, float val)
{
    ZTL_VEC_CHECK_RESERVE(vec);
    float* pv = (float*)vec->elts;
    pv[vec->nelts++] = val;
}

void ztl_push_double(ztl_vector_t* vec, double val)
{
    ZTL_VEC_CHECK_RESERVE(vec);
    double* pv = (double*)vec->elts;
    pv[vec->nelts++] = val;
}

void ztl_push_ptr(ztl_vector_t* vec, void* val)
{
    ZTL_VEC_CHECK_RESERVE(vec);
    void** pv = (void**)vec->elts;
    pv[vec->nelts++] = val;
}

void ztl_push_x(ztl_vector_t* vec, void* elem)
{
    ZTL_VEC_CHECK_RESERVE(vec);

    char* lpaddr;
    lpaddr = (char*)vec->elts + vec->nelts * vec->eltsize;
    vec->nelts++;

    switch (vec->eltsize)
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
        *(uint32_t*)(lpaddr + 8) = *(uint32_t*)((char*)elem + 8);
        break;
    case 16:
        *(uint64_t*)lpaddr = *(uint64_t*)elem;
        *(uint64_t*)(lpaddr + 8) = *(uint64_t*)((char*)elem + 8);
        break;
    default:
        memcpy(lpaddr, elem, vec->eltsize);
    }

}
