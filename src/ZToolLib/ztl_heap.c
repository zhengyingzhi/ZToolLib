#include <stdlib.h>
#include <string.h>

#define HAVE_MUTEX      1

#include "ztl_common.h"
#include "ztl_mem.h"
#include "ztl_heap.h"
#include "ztl_threads.h"
#include "ztl_utils.h"


struct heap_st
{
    uint32_t    avail, curr;
    void**      h;
    int         offset;
    int         outside_mem;
    int       (*cmp)(const void* x, const void* y);
    
#if HAVE_MUTEX
    ztl_thread_mutex_t  lock;
#endif
};

static inline unsigned left_node(unsigned i)
{
    return i << 1;
}

static inline unsigned right_node(unsigned i)
{
    return (i << 1) + 1;
}

static inline unsigned parent_node(unsigned i)
{
    return i / 2;
}

static inline void* heap_get(heap_t* hp, unsigned long i)
{
    return hp->h[i - 1];
}

static void heap_set(heap_t* hp, unsigned long i, void* elem)
{
    hp->h[i - 1] = elem;
    if (hp->offset >= 0)
    {
        uint32_t* ip = (uint32_t*)elem + hp->offset;
        *ip = i;
    }
}

static uint32_t get_index(heap_t* hp, void* elem)
{
     uint32_t* ip;

    if (hp->offset < 0)
        return 0;
    ip = (uint32_t*)elem + hp->offset;
    return *ip;
}

static void heap_swap(heap_t* hp, uint32_t i, uint32_t j)
{
    void* tmp;

    tmp = heap_get(hp, i);
    heap_set(hp, i, heap_get(hp, j));
    heap_set(hp, j, tmp);
}

static int grow_heap(heap_t* hp)
{
    hp->avail = (hp->avail << 1) + 1;
    if (RESIZE(hp->h, hp->avail * sizeof *hp->h) == NULL)
    {
        hp->curr = hp->avail = 0;
        return -1;
    }
    return 0;
}

static unsigned bubble_up(heap_t* hp, unsigned long i)
{
    while (i > 1 && hp->cmp(heap_get(hp, parent_node(i)), heap_get(hp, i)) < 0)
    {
        heap_swap(hp, parent_node(i), i);
        i = parent_node(i);
    }
    return i;
}

static void max_heapify(heap_t* hp, uint32_t i)
{
    for (;;)
    {
        uint32_t l = left_node(i), r = right_node(i), max;

        max = l <= hp->curr && hp->cmp(heap_get(hp, l), heap_get(hp, i)) > 0
            ? l : i;
        if (r <= hp->curr && hp->cmp(heap_get(hp, r), heap_get(hp, max)) > 0)
            max = r;
        if (max == i)
            break;
        heap_swap(hp, i, max);
        i = max;
    }
}

static void* _heap_remove(heap_t* hp, uint32_t i)
{
    void* ret;

    if (i > hp->curr)
        return NULL;
    ret = heap_get(hp, i);
    heap_set(hp, i, heap_get(hp, hp->curr--));
    i = bubble_up(hp, i);
    max_heapify(hp, i);
    return ret;
}

//////////////////////////////////////////////////////////////////////////
int heap_memory_size()
{
    return ztl_align(sizeof(struct heap_st), 8);
}

heap_t* heap_new(int height, int offset, int cmp(const void* x, const void* y))
{
    heap_t* hp;

    if (height <= 0)
        height = 8;
    if (cmp == NULL)
        return NULL;
    if (NEW(hp) == NULL)
        return NULL;
    heap_new_at_mem(height, offset, cmp, hp);
    hp->outside_mem = 0;
    return hp;
}

heap_t* heap_new_at_mem(int height, int offset, int cmp(const void* x, const void* y), void* mem)
{
    heap_t* hp;
    if (height <= 0)
        height = 8;
    if (cmp == NULL)
        return NULL;

    hp = (heap_t*)mem;
    hp->avail = (height << 1) - 1;
    hp->curr = 0;
    if ((hp->h = CALLOC(1, hp->avail * sizeof *hp->h)) == NULL)
    {
        FREE(hp);
        return NULL;
    }
    hp->offset = offset;
    hp->outside_mem = 1;
    hp->cmp = cmp;

#if HAVE_MUTEX
    ztl_thread_mutexattr_t attr;
    ztl_thread_mutexattr_init(&attr);
    ztl_thread_mutexattr_settype(&attr, ZTL_THREAD_MUTEX_ADAPTIVE_NP);
    ztl_thread_mutex_init(&hp->lock, &attr);
    ztl_thread_mutexattr_destroy(&attr);
#endif//HAVE_MUTEX
    return hp;
}

void heap_free(heap_t* hp)
{
    if (hp == NULL)
        return;
#if HAVE_MUTEX
    ztl_thread_mutex_destroy(&hp->lock);
#endif
    FREE(hp->h);
    FREE(hp);
}

int heap_length(heap_t* hp)
{
    return hp ? (int)hp->curr : 0;
}

/* Return 0 if success, otherwise -1 is returned */
int heap_push(heap_t* hp, void* elem)
{
    if (hp == NULL || elem == NULL)
        return -1;
    if (hp->curr == hp->avail && grow_heap(hp))
        return -1;
    heap_set(hp, ++hp->curr, elem);
    bubble_up(hp, hp->curr);
    return 0;
}

void* heap_pop(heap_t* hp)
{
    return hp ? _heap_remove(hp, 1) : NULL;
}

void* heap_remove(heap_t* hp, void* elem)
{
    unsigned long i;

    if (hp == NULL || elem == NULL)
        return NULL;
    if ((i = get_index(hp, elem)) == 0)
        return NULL;
    return _heap_remove(hp, i);
}

void* heap_peek(heap_t* hp, uint32_t index)
{
    if (hp == NULL)
        return NULL;
    if (hp->curr == 0 || index == 0 || index > hp->curr)
        return NULL;
    return heap_get(hp, index);
}

void heap_lock(heap_t* hp)
{
#if HAVE_MUTEX
    ztl_thread_mutex_lock(&hp->lock);
#endif
}

void heap_unlock(heap_t* hp)
{
#if HAVE_MUTEX
    ztl_thread_mutex_unlock(&hp->lock);
#endif
}

//////////////////////////////////////////////////////////////////////////
int heap_cmp_least_int(const void *x, const void *y)
{
    return (x == y) ? 0 : (x < y ? 1 : -1);
}

int heap_cmp_large_int(const void *x, const void *y)
{
    return (x == y) ? 0 : (x < y ? -1 : 1);
}

int heap_cmp_least_pint32(const void *x, const void *y)
{
    int32_t* px = (int32_t*)x;
    int32_t* py = (int32_t*)y;
    return (*px == *py) ? 0 : (*px < *py ? 1 : -1);
}

int heap_cmp_large_pint32(const void *x, const void *y)
{
    int32_t* px = (int32_t*)x;
    int32_t* py = (int32_t*)y;
    return (*px == *py) ? 0 : (*px < *py ? -1 : 1);
}

int heap_cmp_least_pint64(const void *x, const void *y)
{
    int64_t* px = (int64_t*)x;
    int64_t* py = (int64_t*)y;
    return (*px == *py) ? 0 : (*px < *py ? 1 : -1);
}

int heap_cmp_large_pint64(const void *x, const void *y)
{
    int64_t* px = (int64_t*)x;
    int64_t* py = (int64_t*)y;
    return (*px == *py) ? 0 : (*px < *py ? -1 : 1);
}
