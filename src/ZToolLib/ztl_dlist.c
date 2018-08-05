#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ztl_mempool.h"

#include "ztl_linklist.h"

#include "ztl_dlist.h"


typedef struct 
{
    void*                   data;
    ztl_queue_t             link;
}ztl_dlnode_t;

struct ztl_dlist_st
{
    ztl_queue_t             que;
    ztl_mempool_t*          mp;
    uint32_t                size;
    uint32_t                lock;
    ztl_dlist_iterator_t*   iter;
};


ztl_dlist_t* ztl_dlist_create(int reserve_nodes)
{
    ztl_dlist_t* dl;
    dl = (ztl_dlist_t*)malloc(sizeof(ztl_dlist_t));
    memset(dl, 0, sizeof(*dl));

    if (reserve_nodes <= 0) {
        reserve_nodes = ZTL_DLIST_REVERSE_NODES;
    }

    ztl_queue_init(&dl->que);
    dl->mp      = ztl_mp_create(sizeof(ztl_dlnode_t), reserve_nodes, true);
    dl->size    = 0;
    dl->lock    = 0;
    dl->iter    = (ztl_dlist_iterator_t*)malloc(sizeof(ztl_dlist_iterator_t));

    return dl;
}

void ztl_dlist_release(ztl_dlist_t* dl)
{
    ztl_mp_release(dl->mp);
    if (dl->iter) {
        free(dl->iter);
    }
    free(dl);
}

void* ztl_dlist_head(ztl_dlist_t* dl)
{
    if (dl->size == 0)
        return NULL;
    return ztl_queue_data(ztl_queue_head(&dl->que), ztl_dlnode_t, link);
}

void* ztl_dlist_tail(ztl_dlist_t* dl)
{
    if (dl->size == 0)
        return NULL;
    return ztl_queue_data(ztl_queue_last(&dl->que), ztl_dlnode_t, link);
}

int ztl_dlist_size(ztl_dlist_t* dl)
{
    return dl->size;
}

int ztl_dlist_insert_head(ztl_dlist_t* dl, void* data)
{
    ztl_dlnode_t* dlnode;
    dlnode = (ztl_dlnode_t*)ztl_mp_alloc(dl->mp);
    if (!dlnode) {
        return -1;
    }
    dlnode->data = data;

    ztl_queue_insert_head(&dl->que, &dlnode->link);
    ++dl->size;
    return 0;
}

int ztl_dlist_insert_tail(ztl_dlist_t* dl, void* data)
{
    ztl_dlnode_t* dlnode;
    dlnode = (ztl_dlnode_t*)ztl_mp_alloc(dl->mp);
    if (!dlnode) {
        return -1;
    }
    dlnode->data = data;

    ztl_queue_insert_tail(&dl->que, &dlnode->link);
    ++dl->size;
    return 0;
}

void* ztl_dlist_pop(ztl_dlist_t* dl)
{
    if (dl->size == 0) {
        return NULL;
    }

    void* data;
    ztl_queue_t* nodelink;
    nodelink = ztl_queue_head(&dl->que);
    data = ztl_queue_data(nodelink, ztl_dlnode_t, link);

    --dl->size;
    ztl_queue_remove(nodelink);
    return data;
}

void* ztl_dlist_pop_back(ztl_dlist_t* dl)
{
    if (dl->size == 0) {
        return NULL;
    }

    void* data;
    ztl_queue_t* nodelink;
    nodelink = ztl_queue_last(&dl->que);
    data = ztl_queue_data(nodelink, ztl_dlnode_t, link);

    --dl->size;
    ztl_queue_remove(nodelink);
    return data;
}



ztl_dlist_iterator_t* ztl_dlist_iter_new(ztl_dlist_t* dl, int direction)
{
    ztl_dlist_iterator_t* iter;
    if (dl->iter) {
        iter = dl->iter;
        dl->iter = NULL;
    }
    else {
        iter = (ztl_dlist_iterator_t*)malloc(sizeof(ztl_dlist_iterator_t));
    }

    iter->direction = direction;
    iter->node = ztl_queue_head(&dl->que);
    return iter;
}

void ztl_dlist_iter_del(ztl_dlist_t* dl, ztl_dlist_iterator_t* iter)
{
    if (dl->iter) {
        free(iter);
    }
    else {
        dl->iter = iter;
    }
}

void* ztl_dlist_next(ztl_dlist_t* dl, ztl_dlist_iterator_t* iter)
{
    void* data = NULL;
    ztl_queue_t* itn = (ztl_queue_t*)iter->node;
    if (ztl_queue_sentinel(&dl->que) != itn)
    {
        ztl_dlnode_t* dlnode = ztl_queue_data(&dl->que, ztl_dlnode_t, link);
        data = dlnode;
        if (iter->direction == ZTL_DLIST_ITER_BACK) {
            iter->node = ztl_queue_next(itn);
        }
        else {
            iter->node = ztl_queue_prev(itn);
        }
    }
    return data;
}
