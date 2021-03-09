#include <stdlib.h>
#include <string.h>

#if _MSC_VER
#include <WinSock2.h>
#define HAVE_PTHREAD    0
#else
#define HAVE_PTHREAD    1
#endif//_MSC_VER

#include "ztl_common.h"
#include "ztl_mem.h"
#include "ztl_heap.h"

struct heap_st {
    uint32_t    avail, curr;
    void**      h;
    size_t      offset;
    int       (*cmp)(const void* x, const void* y);
#if HAVE_PTHREAD
    pthread_mutex_t	lock;
#else
    CRITICAL_SECTION lock;
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

static inline void* heap_get(heap_t heap, unsigned long i)
{
    return heap->h[i - 1];
}

static void heap_set(heap_t heap, unsigned long i, void* elem)
{
    heap->h[i - 1] = elem;
    if (heap->offset >= 0)
    {
        uint32_t* ip = (uint32_t*)elem + heap->offset;
        *ip = i;
    }
}

static uint32_t get_index(heap_t heap, void* elem)
{
     uint32_t* ip;

    if (heap->offset < 0)
        return 0;
    ip = (uint32_t*)elem + heap->offset;
    return *ip;
}

static void heap_swap(heap_t heap, uint32_t i, uint32_t j)
{
    void* tmp;

    tmp = heap_get(heap, i);
    heap_set(heap, i, heap_get(heap, j));
    heap_set(heap, j, tmp);
}

static int grow_heap(heap_t heap)
{
    heap->avail = (heap->avail << 1) + 1;
    if (RESIZE(heap->h, heap->avail * sizeof *heap->h) == NULL)
    {
        heap->curr = heap->avail = 0;
        return -1;
    }
    return 0;
}

static unsigned bubble_up(heap_t heap, unsigned long i)
{
    while (i > 1 && heap->cmp(heap_get(heap, parent_node(i)), heap_get(heap, i)) < 0)
    {
        heap_swap(heap, parent_node(i), i);
        i = parent_node(i);
    }
    return i;
}

static void max_heapify(heap_t heap, uint32_t i)
{
    for (;;)
    {
        uint32_t l = left_node(i), r = right_node(i), max;

        max = l <= heap->curr && heap->cmp(heap_get(heap, l), heap_get(heap, i)) > 0
            ? l : i;
        if (r <= heap->curr && heap->cmp(heap_get(heap, r), heap_get(heap, max)) > 0)
            max = r;
        if (max == i)
            break;
        heap_swap(heap, i, max);
        i = max;
    }
}

static void* _heap_remove(heap_t heap, uint32_t i)
{
    void* ret;

    if (i > heap->curr)
        return NULL;
    ret = heap_get(heap, i);
    heap_set(heap, i, heap_get(heap, heap->curr--));
    i = bubble_up(heap, i);
    max_heapify(heap, i);
    return ret;
}

/* The 'offset' parameter is optional, but must be provided to be able to use heap_remove().
* If heap_remove() will not be used, then a negative value can be provided.
*/
heap_t heap_new(int height, size_t offset, int cmp(const void* x, const void* y))
{
    heap_t heap;
#if HAVE_PTHREAD
    pthread_mutexattr_t attr;
#endif//HAVE_PTHREAD

    if (height <= 0)
        height = 8;
    if (cmp == NULL)
        return NULL;
    if (NEW(heap) == NULL)
        return NULL;
    heap->avail = (height << 1) - 1;
    heap->curr = 0;
    if ((heap->h = CALLOC(1, heap->avail * sizeof *heap->h)) == NULL)
    {
        FREE(heap);
        return NULL;
    }
    heap->offset = offset;
    heap->cmp = cmp;

#if HAVE_PTHREAD
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP);
    pthread_mutex_init(&heap->lock, &attr);
    pthread_mutexattr_destroy(&attr);
#else
    InitializeCriticalSection(&heap->lock);
#endif//HAVE_PTHREAD
    return heap;
}

void heap_free(heap_t *hp)
{
    if (hp == NULL || *hp == NULL)
        return;
#if HAVE_PTHREAD
    pthread_mutex_destroy(&(*hp)->lock);
#else
    DeleteCriticalSection(&(*hp)->lock);
#endif
    FREE((*hp)->h);
    FREE(*hp);
}

int heap_length(heap_t heap)
{
    if (heap == NULL)
        return 0;
    return (int)heap->curr;
}

/* Return 0 if success, otherwise -1 is returned */
int heap_push(heap_t heap, void* elem)
{
    if (heap == NULL || elem == NULL)
        return -1;
    if (heap->curr == heap->avail && grow_heap(heap))
        return -1;
    heap_set(heap, ++heap->curr, elem);
    bubble_up(heap, heap->curr);
    return 0;
}

void* heap_pop(heap_t heap)
{
    if (heap == NULL)
        return NULL;
    return _heap_remove(heap, 1);
}

void* heap_remove(heap_t heap, void* elem)
{
    unsigned long i;

    if (heap == NULL || elem == NULL)
        return NULL;
    if ((i = get_index(heap, elem)) == 0)
        return NULL;
    return _heap_remove(heap, i);
}

void* heap_peek(heap_t heap, uint32_t index)
{
    if (heap == NULL)
        return NULL;
    if (heap->curr == 0 || index == 0 || index > heap->curr)
        return NULL;
    return heap_get(heap, index);
}

void heap_lock(heap_t heap)
{
    if (heap == NULL)
        return;
#if HAVE_PTHREAD
    pthread_mutex_lock(&heap->lock);
#else
    EnterCriticalSection(&heap->lock);
#endif
}

void heap_unlock(heap_t heap)
{
    if (heap == NULL)
        return;
#if HAVE_PTHREAD
    pthread_mutex_unlock(&heap->lock);
#else
    LeaveCriticalSection(&heap->lock);
#endif
}

