#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ztl_atomic.h"
#include "ztl_dlist.h"
#include "ztl_linklist.h"
#include "ztl_palloc.h"
#include "ztl_threads.h"


typedef struct 
{
    void*               data;
    ztl_queue_t         link;
}dlnode_t;

struct dlist_st
{
    ztl_queue_t         que;
    ztl_queue_t         idles;
    ztl_pool_t*         pool;
    uint32_t            size;
    int               (*cmp)(const void* expect, const void* actual);
    int               (*vfree)(void* val);

    dlist_iter_t*       iter;
    ztl_thread_mutex_t  lock;
    ztl_thread_rwlock_t rwlock;
};


static inline int _dlist_cmp_default(const void* expect, const void* actual)
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

static inline void* _dlist_remove(dlist_t* dl, ztl_queue_t* quelink)
{
    void*           data;
    dlnode_t*   dlnode;

    dlnode = ztl_queue_data(quelink, dlnode_t, link);
    data = dlnode->data;

    --dl->size;
    ztl_queue_remove(quelink);

    ztl_queue_insert_tail(&dl->idles, &dlnode->link);
    return data;
}

static inline dlnode_t* _dlist_dlnode_alloc(dlist_t* dl)
{
    dlnode_t* dlnode;
    ztl_queue_t*  quelink;

    if (!ztl_queue_empty(&dl->idles))
    {
        quelink = ztl_queue_head(&dl->idles);
        dlnode = ztl_queue_data(quelink, dlnode_t, link);
        ztl_queue_remove(quelink);
    }
    else
    {
        dlnode = (dlnode_t*)ztl_pcalloc(dl->pool, sizeof(dlnode_t));
    }
    return dlnode;
}

//////////////////////////////////////////////////////////////////////////
dlist_t* dlist_create(int reserve_nodes,
    int(*cmp)(const void* expect, const void* actual),
    int(*vfree)(void* val))
{
    size_t      nbytes;
    ztl_pool_t* pool;
    dlist_t*    dl;
    ztl_thread_mutexattr_t mattr;

    if (reserve_nodes <= 0) {
        reserve_nodes = DLIST_REVERSE_NODES;
    }

    nbytes = ztl_min(sizeof(dlist_t) + reserve_nodes * sizeof(dlnode_t), 4096);
    pool = ztl_create_pool(ztl_align(nbytes, 64));
    if (!pool) {
        return NULL;
    }

    dl = (dlist_t*)ztl_pcalloc(pool, sizeof(dlist_t));
    dl->pool = pool;

    ztl_queue_init(&dl->que);
    ztl_queue_init(&dl->idles);
    for (int i = 0; i < reserve_nodes; ++i)
    {
        dlnode_t* dlnode;
        dlnode = (dlnode_t*)ztl_pcalloc(dl->pool, sizeof(dlnode_t));
        ztl_queue_insert_tail(&dl->idles, &dlnode->link);
    }
    dl->size    = 0;
    dl->cmp     = cmp ? cmp : _dlist_cmp_default;
    dl->vfree   = vfree;

    dl->iter = (dlist_iter_t*)calloc(1, sizeof(dlist_iter_t));

    ztl_thread_mutexattr_init(&mattr);
    ztl_thread_mutexattr_settype(&mattr, ZTL_THREAD_MUTEX_RECURSIVE_NP);
    ztl_thread_mutex_init(&dl->lock, &mattr);
    ztl_thread_mutexattr_destroy(&mattr);
    ztl_thread_rwlock_init(&dl->rwlock);

    return dl;
}

void dlist_release(dlist_t* dl)
{
    if (!dl || !dl->pool) {
        return;
    }

    ztl_thread_mutex_destroy(&dl->lock);
    ztl_thread_rwlock_destroy(&dl->rwlock);

    ztl_pool_t* pool = dl->pool;
    dlist_clear(dl);
    ztl_destroy_pool(pool);
}

void* dlist_head(dlist_t* dl)
{
    if (dl->size == 0)
        return NULL;

    dlnode_t* dlnode;
    dlnode = ztl_queue_data(ztl_queue_head(&dl->que), dlnode_t, link);
    return dlnode->data;
}

void* dlist_tail(dlist_t* dl)
{
    if (dl->size == 0)
        return NULL;

    dlnode_t* dlnode;
    dlnode = ztl_queue_data(ztl_queue_last(&dl->que), dlnode_t, link);
    return dlnode->data;
}

void dlist_clear(dlist_t* dl)
{
    void* val;
    dlist_iter_t* iter;
    iter = dlist_iter_new(dl, DLSTART_HEAD);
    while ((val = dlist_next(dl, iter)) != NULL)
    {
        if (dl->vfree) {
            dl->vfree(val);
        }
        dlist_erase(dl, iter);
    }
    dlist_iter_del(dl, iter);
}

int dlist_size(dlist_t* dl)
{
    return dl->size;
}

int dlist_insert_head(dlist_t* dl, void* data)
{
    dlnode_t* dlnode;
    dlnode = _dlist_dlnode_alloc(dl);
    if (!dlnode) {
        return -1;
    }

    dlnode->data = data;
    ztl_queue_insert_head(&dl->que, &dlnode->link);
    ++dl->size;
    return 0;
}

int dlist_insert_tail(dlist_t* dl, void* data)
{
    dlnode_t* dlnode;
    dlnode = _dlist_dlnode_alloc(dl);
    if (!dlnode) {
        return -1;
    }

    dlnode->data = data;
    ztl_queue_insert_tail(&dl->que, &dlnode->link);
    ++dl->size;
    return 0;
}

void* dlist_pop(dlist_t* dl)
{
    if (dl->size == 0) {
        return NULL;
    }

    ztl_queue_t* quelink;
    quelink = ztl_queue_head(&dl->que);
    return _dlist_remove(dl, quelink);
}

void* dlist_pop_back(dlist_t* dl)
{
    if (dl->size == 0) {
        return NULL;
    }

    ztl_queue_t* quelink;
    quelink = ztl_queue_last(&dl->que);
    return _dlist_remove(dl, quelink);
}

bool dlist_have(dlist_t* dl, void* expect)
{
    void* actual;
    actual = dlist_search(dl, expect, DLSTART_HEAD);
    return actual ? true : false;
}

void* dlist_search(dlist_t* dl, void* expect, int DLSTART_direction)
{
    void* actual;
    dlist_iter_t* iter;

    iter = dlist_iter_new(dl, DLSTART_direction);
    while ((actual = dlist_next(dl, iter)) != NULL)
    {
        if (dl->cmp(expect, actual) == 0) {
            break;
        }
    }
    dlist_iter_del(dl, iter);

    return actual;
}

void* dlist_remove(dlist_t* dl, void* expect)
{
    void* actual;
    dlist_iter_t* iter;

    iter = dlist_iter_new(dl, DLSTART_HEAD);
    while ((actual = dlist_next(dl, iter)) != NULL)
    {
        if (dl->cmp(expect, actual) == 0)
        {
            dlist_erase(dl, iter);
            break;
        }
    }
    dlist_iter_del(dl, iter);

    return actual;
}

dlist_iter_t* dlist_iter_new(dlist_t* dl, int DLSTART_direction)
{
    dlist_iter_t* iter;
    iter = dl->iter;
    if (!atomic_casptr(&dl->iter, iter, NULL) || !iter)
    {
        iter = (dlist_iter_t*)malloc(sizeof(dlist_iter_t));
    }

    if (DLSTART_direction == DLSTART_HEAD)
        iter->nodelink = ztl_queue_head(&dl->que);
    else
        iter->nodelink = ztl_queue_last(&dl->que);
    iter->data = NULL;
    iter->dlist = dl;
    iter->direction = DLSTART_direction;
    return iter;
}

void dlist_iter_del(dlist_t* dl, dlist_iter_t* iter)
{
    if (!atomic_casptr(&dl->iter, NULL, iter))
    {
        free(iter);
    }
}

void* dlist_next(dlist_t* dl, dlist_iter_t* iter)
{
    dlnode_t*   dlnode;
    ztl_queue_t*    current;

    current = (ztl_queue_t*)iter->nodelink;
    if (current != ztl_queue_sentinel(&dl->que))
    {
        // retrieve current data, and move iterator to next
        dlnode = ztl_queue_data(current, dlnode_t, link);
        iter->data = dlnode->data;

        if (iter->direction == DLSTART_HEAD) {
            iter->nodelink = ztl_queue_next(current);
        }
        else {
            iter->nodelink = ztl_queue_prev(current);
        }
    }
    else
    {
        iter->data = NULL;
    }

    return iter->data;
}

bool dlist_erase(dlist_t* dl, dlist_iter_t* iter)
{
    ztl_queue_t* current;
    current = (ztl_queue_t*)iter->nodelink;
    ztl_queue_t* oldlink;
    if (iter->direction == DLSTART_HEAD)
        oldlink = ztl_queue_prev(current);
    else
        oldlink = ztl_queue_next(current);

    if (oldlink == ztl_queue_sentinel(&dl->que)) {
        return false;
    }

    _dlist_remove(dl, oldlink);
    return true;
}

void dlist_lock(dlist_t* dl)
{
    if (dl == NULL)
        return;

    ztl_thread_mutex_lock(&dl->lock);
}

void dlist_unlock(dlist_t* dl)
{
    if (dl == NULL)
        return;
    ztl_thread_mutex_unlock(&dl->lock);
}

void dlist_rwlock_rdlock(dlist_t* dl)
{
    if (dl == NULL)
        return;
    ztl_thread_rwlock_rdlock(&dl->rwlock);
}

void dlist_rwlock_wrlock(dlist_t* dl)
{
    if (dl == NULL)
        return;
    ztl_thread_rwlock_wrlock(&dl->rwlock);
}

void dlist_rwlock_unlock(dlist_t* dl)
{
    if (dl == NULL)
        return;
    ztl_thread_rwlock_unlock(&dl->rwlock);
}
