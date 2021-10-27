#include <string.h>
#include <stdlib.h>

#include "ztl_utils.h"

#include "ztl_vector.h"



void cvector_clear(cvector_t* vec);
int  cvector_reserve(cvector_t* vec, uint32_t reserve_num);

int  cvector_index(cvector_t* vec, void* pelem);
int  cvector_remove(cvector_t* vec, int index);

void cvector_push_char(cvector_t* vec, int8_t val);
void cvector_push_short(cvector_t* vec, int16_t val);
void cvector_push_int(cvector_t* vec, int32_t val);
void cvector_push_int64(cvector_t* vec, int64_t val);
void cvector_push_float(cvector_t* vec, float val);
void cvector_push_double(cvector_t* vec, double val);
void cvector_push_ptr(cvector_t* vec, void* ptr);
void cvector_push_x(cvector_t* vec, void* elem);

static int _cvector_init(cvector_t* vec, uint32_t init_num, size_t eltsize)
{
    /*
    * set "array->nelts" before "array->elts", otherwise MSVC thinks
    * that "array->nelts" may be used without having been initialized
    */

    init_num        = ztl_align(init_num, 4);
    vec->elts       = (char*)malloc(ztl_align(init_num * eltsize, 64));
    vec->nelts      = 0;
    vec->nalloc     = init_num;
    vec->eltsize    = (uint32_t)eltsize;
    vec->flag       = 0;

    vec->reserve    = cvector_reserve;
    vec->clear      = cvector_clear;
    vec->index      = cvector_index;
    vec->remove     = cvector_remove;
    vec->push_char  = cvector_push_char;
    vec->push_short = cvector_push_short;
    vec->push_int   = cvector_push_int;
    vec->push_int64 = cvector_push_int64;
    vec->push_float = cvector_push_float;
    vec->push_double= cvector_push_double;
    vec->push_ptr   = cvector_push_ptr;
    vec->push_x     = cvector_push_x;
    return 0;
}


int cvector_init(cvector_t* vec, uint32_t init_num, size_t eltsize)
{
    return _cvector_init(vec, init_num, eltsize);
}

cvector_t* cvector_create(uint32_t init_num, size_t eltsize)
{
    cvector_t* vec;

    vec = (cvector_t*)calloc(1, sizeof(cvector_t));
    if (vec == NULL) {
        return NULL;
    }

    if (_cvector_init(vec, init_num, eltsize) != 0)
    {
        free(vec);
        return NULL;
    }
    vec->flag = 1;

    return vec;
}

void cvector_release(cvector_t* vec)
{
    if (!vec) {
        return;
    }

    if (vec->elts)
    {
        free(vec->elts);
        vec->elts = NULL;
    }
    vec->nelts = 0;

    if (vec->flag)
        free(vec);
}

int cvector_reserve(cvector_t* vec, uint32_t reserve_num)
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

void cvector_clear(cvector_t* vec)
{
    vec->nelts = 0;
}

int cvector_index(cvector_t* vec, void* pelem)
{
    int index = 0;
    char* pcur = vec->elts;
    while (index < (int)vec->nelts)
    {
        if (memcmp(pcur, pelem, vec->eltsize) == 0) {
            return index;
        }
        pcur += vec->eltsize;
        ++index;
    }
    return -1;
}

int cvector_remove(cvector_t* vec, int idx)
{
    if(idx < 0 || idx >= (int)vec->nelts) {
        return -1;
    }

    vec->nelts -= 1;
    if (idx < (int)vec->nelts - 1)
    {
        memmove(vec->elts + vec->eltsize * idx, vec->elts + vec->eltsize * (idx + 1),
            vec->nelts * vec->eltsize);
    }
    return 0;
}


void cvector_push_char(cvector_t* vec, int8_t val)
{
    CVEC_CHECK_RESERVE(vec);
    int8_t* pv = (int8_t*)vec->elts;
    pv[vec->nelts++] = val;
}

void cvector_push_short(cvector_t* vec, int16_t val)
{
    CVEC_CHECK_RESERVE(vec);
    int16_t* pv = (int16_t*)vec->elts;
    pv[vec->nelts++] = val;
}

void cvector_push_int(cvector_t* vec, int32_t val)
{
    CVEC_CHECK_RESERVE(vec);
    int32_t* pv = (int32_t*)vec->elts;
    pv[vec->nelts++] = val;
}

void cvector_push_int64(cvector_t* vec, int64_t val)
{
    CVEC_CHECK_RESERVE(vec);
    int64_t* pv = (int64_t*)vec->elts;
    pv[vec->nelts++] = val;
}

void cvector_push_float(cvector_t* vec, float val)
{
    CVEC_CHECK_RESERVE(vec);
    float* pv = (float*)vec->elts;
    pv[vec->nelts++] = val;
}

void cvector_push_double(cvector_t* vec, double val)
{
    CVEC_CHECK_RESERVE(vec);
    double* pv = (double*)vec->elts;
    pv[vec->nelts++] = val;
}

void cvector_push_ptr(cvector_t* vec, void* val)
{
    CVEC_CHECK_RESERVE(vec);
    void** pv = (void**)vec->elts;
    pv[vec->nelts++] = val;
}

void cvector_push_x(cvector_t* vec, void* elem)
{
    CVEC_CHECK_RESERVE(vec);

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
