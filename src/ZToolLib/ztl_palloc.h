#ifndef _ZTL_PALLOC_H_INCLUDED_
#define _ZTL_PALLOC_H_INCLUDED_

#include <string.h>
#include <stdlib.h>
#include "ztl_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* default pagesize  */
#define ztl_pagesize            4096

/*
 * ztl_MAX_ALLOC_FROM_POOL should be (ztl_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define ZTL_MAX_ALLOC_FROM_POOL  (ztl_pagesize - 1)

#define ZTL_DEFAULT_POOL_SIZE    (16 * 1024)

#define ZTL_ALIGNMENT            sizeof(unsigned long) 

#define ZTL_POOL_ALIGNMENT       16
#define ZTL_MIN_POOL_SIZE                                                   \
    ztl_align((sizeof(ztl_pool_t) + 2 * sizeof(ztl_pool_large_t)),          \
              ZTL_POOL_ALIGNMENT)

typedef struct ztl_pool_s        ztl_pool_t;

typedef struct ztl_pool_large_s  ztl_pool_large_t;

typedef void (*ztl_pool_cleanup_pt)(void *data);

typedef struct ztl_pool_cleanup_s  ztl_pool_cleanup_t;


struct ztl_pool_cleanup_s {
    ztl_pool_cleanup_pt      handler;
    void*                   data;
    ztl_pool_cleanup_t*      next;
};


struct ztl_pool_large_s {
    ztl_pool_large_t     *next;
    void                *alloc;
};


typedef struct {
    uint8_t				*last;
    uint8_t             *end;
    ztl_pool_t           *next;
    uint32_t             failed;
} ztl_pool_data_t;

struct ztl_pool_s {
    ztl_pool_data_t       d;
    size_t               max;
    ztl_pool_t           *current;
    ztl_pool_large_t     *large;
    ztl_pool_cleanup_t   *cleanup;
};


ztl_pool_t *ztl_create_pool(size_t size);
void ztl_destroy_pool(ztl_pool_t *pool);
void ztl_reset_pool(ztl_pool_t *pool);

void *ztl_palloc(ztl_pool_t *pool, size_t size);
void *ztl_pnalloc(ztl_pool_t *pool, size_t size);
void *ztl_pcalloc(ztl_pool_t *pool, size_t size);
void *ztl_pmemalign(ztl_pool_t *pool, size_t size, size_t alignment);
uint32_t ztl_pfree(ztl_pool_t *pool, void *p);


ztl_pool_cleanup_t *ztl_pool_cleanup_add(ztl_pool_t *p, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _ZTL_PALLOC_H_INCLUDED_ */
