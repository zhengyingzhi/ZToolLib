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


typedef struct cvector_s    cvector_t;
struct cvector_s {
    char*       elts;
    uint32_t    nelts;
    uint32_t    nalloc;
    uint32_t    eltsize;
    int32_t     flag;
    int  (*reserve)(cvector_t* vec, uint32_t reserve_num);
    void (*clear)(cvector_t* vec);
    int  (*index)(cvector_t* vec, void* pelem);
    int  (*remove)(cvector_t* vec, int idx);
    void (*push_char)(cvector_t* vec, int8_t val);
    void (*push_short)(cvector_t* vec, int16_t val);
    void (*push_int)(cvector_t* vec, int32_t val);
    void (*push_int64)(cvector_t* vec, int64_t val);
    void (*push_float)(cvector_t* vec, float val);
    void (*push_double)(cvector_t* vec, double val);
    void (*push_ptr)(cvector_t* vec, void* ptr);
    void (*push_x)(cvector_t* vec, void* elem);
};

/* init a dynamic array (cpp style)
 * example: cvector_init(&vec, 32, sizeof(double));
 */
int cvector_init(cvector_t* vec, uint32_t init_num, size_t eltsize);

/* create a dynamic array (cpp style)
 * @num  the initial array size
 * @eltsize each elem size, like sizeof(int)
 * @return the new array object
 */
cvector_t* cvector_create(uint32_t init_num, size_t eltsize);

/* release the array
 * vec: the object returned by cvector_create
 */
void cvector_release(cvector_t* vec);


#define cvector_avail(vec)           ((vec)->nalloc - (vec)->nelts)
#define cvector_size(vec)            (vec)->nelts
#define cvector_datas(vec, type_t)   (type_t*)((vec)->elts)

#define cvector_push_batch(vec, src, count, type_t)                             \
    {                                                                           \
        type_t* _dst = (type_t*)((vec)->elts + (vec)->nelts * (vec)->eltsize);  \
        for (int _i = 0; _i < (int)count; ++_i) {                               \
            _dst[_i] = src[_i];                                                 \
        }                                                                       \
        (vec)->nelts += count;                                                  \
    }

#define cvector_push(vec, val, type_t)                                          \
    do {                                                                        \
        if ((vec)->nelts == (vec)->nalloc) {                                    \
            (vec)->reserve((vec), vec->nalloc << 1);                            \
        }                                                                       \
        type_t* _pv = (type_t*)((vec)->elts);                                   \
        _pv[(vec)->nelts++] = val;                                              \
        break;                                                                  \
    } while (true);


#define CVEC_CHECK_RESERVE(vec)                             \
    do {                                                    \
        if (vec->nelts == vec->nalloc) {                    \
            vec->reserve(vec, vec->nalloc << 1);            \
        }                                                   \
    } while (0);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_VECTOR_H_INCLUDED_
