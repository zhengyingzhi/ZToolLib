#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ztl_palloc.h"

static void* ztl_palloc_small(ztl_pool_t *pool, size_t size, uint32_t align);
static void* ztl_palloc_block(ztl_pool_t *pool, size_t size);
static void* ztl_palloc_large(ztl_pool_t *pool, size_t size);

#define ztl_free  free
#define ztl_memalign(alignment, size)  malloc(size)

ztl_pool_t* ztl_create_pool(size_t size)
{
    ztl_pool_t*  p;

    p = (ztl_pool_t*)ztl_memalign(ZTL_POOL_ALIGNMENT, size);
    if (p == NULL) {
        return NULL;
    }

    p->d.last = (uint8_t *) p + sizeof(ztl_pool_t);
    p->d.end = (uint8_t *) p + size;
    p->d.next = NULL;
    p->d.failed = 0;

    size = size - sizeof(ztl_pool_t);
    p->max = (size < ZTL_MAX_ALLOC_FROM_POOL) ? size : ZTL_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->large = NULL;
    p->cleanup = NULL;

    return p;
}


void ztl_destroy_pool(ztl_pool_t *pool)
{
    ztl_pool_t          *p, *n;
    ztl_pool_large_t    *l;
    ztl_pool_cleanup_t  *c;

    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            c->handler(c->data);
        }
    }

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ztl_free(l->alloc);
        }
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ztl_free(p);

        if (n == NULL) {
            break;
        }
    }
}


void ztl_reset_pool(ztl_pool_t *pool)
{
    ztl_pool_t        *p;
    ztl_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ztl_free(l->alloc);
        }
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (uint8_t*)p + sizeof(ztl_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->large = NULL;
}


void* ztl_palloc(ztl_pool_t *pool, size_t size)
{
#if !(ZTL_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ztl_palloc_small(pool, size, 1);
    }
#endif

    return ztl_palloc_large(pool, size);
}


void* ztl_pnalloc(ztl_pool_t *pool, size_t size)
{
#if !(ZTL_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ztl_palloc_small(pool, size, 0);
    }
#endif

    return ztl_palloc_large(pool, size);
}


static void* ztl_palloc_small(ztl_pool_t *pool, size_t size, uint32_t align)
{
    uint8_t    *m;
    ztl_pool_t  *p;

    p = pool->current;

    do {
        m = p->d.last;

        if (align) {
            m = ztl_align_ptr(m, ZTL_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return ztl_palloc_block(pool, size);
}


static void* ztl_palloc_block(ztl_pool_t *pool, size_t size)
{
    uint8_t     *m;
    size_t       psize;
    ztl_pool_t   *p, *newp;

    psize = (size_t) (pool->d.end - (uint8_t*)pool);

    m = (uint8_t*)ztl_memalign(ZTL_POOL_ALIGNMENT, psize);
    if (m == NULL) {
        return NULL;
    }

    newp = (ztl_pool_t *)m;

    newp->d.end = m + psize;
    newp->d.next = NULL;
    newp->d.failed = 0;

    m += sizeof(ztl_pool_data_t);
    m = ztl_align_ptr(m, ZTL_ALIGNMENT);
    newp->d.last = m + size;

    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }

    p->d.next = newp;

    return m;
}


static void* ztl_palloc_large(ztl_pool_t *pool, size_t size)
{
    void              *p;
    uint32_t           n;
    ztl_pool_large_t  *large;

    p = malloc(size);
    if (p == NULL) {
        return NULL;
    }

    n = 0;

    for (large = pool->large; large; large = large->next) {
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = (ztl_pool_large_t*)ztl_palloc_small(pool, sizeof(ztl_pool_large_t), 1);
    if (large == NULL) {
        ztl_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


void* ztl_pmemalign(ztl_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ztl_pool_large_t  *large;

	(void)alignment;
    p = ztl_memalign(alignment, size);
    if (p == NULL) {
        return NULL;
    }

    large = (ztl_pool_large_t*)ztl_palloc_small(pool, sizeof(ztl_pool_large_t), 1);
    if (large == NULL) {
        ztl_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}


uint32_t ztl_pfree(ztl_pool_t *pool, void *p)
{
    ztl_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (p == l->alloc) {
            ztl_free(l->alloc);
            l->alloc = NULL;

            return 0;
        }
    }

    return 2;
}


void* ztl_pcalloc(ztl_pool_t *pool, size_t size)
{
    void *p;

    p = ztl_palloc(pool, size);
    if (p) {
        memset(p, 0, size);
    }

    return p;
}


ztl_pool_cleanup_t* ztl_pool_cleanup_add(ztl_pool_t *p, size_t size)
{
    ztl_pool_cleanup_t  *c;

    c = (ztl_pool_cleanup_t*)ztl_palloc(p, sizeof(ztl_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ztl_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    return c;
}

