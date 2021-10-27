#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ztl_palloc.h"
#include "ztl_rbtree.h"
#include "ztl_map.h"
#include "ztl_utils.h"


struct cmap_st
{
    rbtree_t        rbtree;
    rbtree_node_t   sentinel;
    rbtree_node_t*  idles;
    ztl_pool_t*     pool;
    int64_t         invalid_value;
    int32_t         size;
    uint32_t        reserve;
    int32_t         access_index;
};


static inline rbtree_node_t* _rbtree_peek(rbtree_t* tree)
{
    return tree->root != tree->sentinel ? tree->root : NULL;
}

static rbtree_node_t* _rbtree_find(rbtree_t *tree,
    rbtree_key_t key)
{
    rbtree_node_t   *root, *sentinel;

    /* a binary tree search */

    root = tree->root;
    sentinel = tree->sentinel;

    while (root && root != sentinel)
    {
        if (root->key == key) {
            return root;
        }

        if (root->key < key) {
            root = root->right;
        }
        else {
            root = root->left;
        }
    }

    return NULL;
}

static void _rbtree_traverse(cmap_t* cmap, rbtree_t *tree, rbtree_node_t* cur,
    cmap_access_pt func, void* context1, int context2)
{
    if (cur && cur != tree->sentinel)
    {
        if (func)
        {
            union_dtype_t d;
            d.ptr = cur->udata;
            func(cmap, context1, context2, cur->key, d.i64);
        }

        if (cur->left)
            _rbtree_traverse(cmap, tree, cur->left, func, context1, context2);
        if (cur->right)
            _rbtree_traverse(cmap, tree, cur->right, func, context1, context2);
    }
}

static void _cmap_array_push(cmap_t* cmap, void* context1, int context2, uint64_t key, int64_t value)
{
    if (cmap->access_index >= context2) {
        return;
    }
    cmap_pair_t* kv_arr, *pkv;
    kv_arr = (cmap_pair_t*)context1;
    pkv = &(kv_arr[cmap->access_index++]);
    pkv->Key = key;
    pkv->Value = value;
}

//////////////////////////////////////////////////////////////////////////
cmap_t* cmap_create(uint32_t reserve)
{
    ztl_pool_t* pool;
    cmap_t*     lmap;

    pool = ztl_create_pool(8192);
    if (!pool) {
        return NULL;
    }

    lmap = (cmap_t*)ztl_pcalloc(pool, sizeof(cmap_t));
    lmap->pool = pool;

    lmap->invalid_value = ZTL_MAP_INVALID_VALUE;
    lmap->reserve = reserve;
    lmap->idles = NULL;
    for (uint32_t i = 0; i < reserve; ++i)
    {
        rbtree_node_t* rbnode;
        rbnode = (rbtree_node_t*)ztl_pcalloc(pool, sizeof(rbtree_node_t));
        rbnode->right = lmap->idles;
        lmap->idles = rbnode;
    }

    rbtree_init(&lmap->rbtree, &lmap->sentinel, rbtree_insert_value);
    cmap_clear(lmap);

    return lmap;
}

void cmap_release(cmap_t* cmap)
{
    ztl_pool_t* pool;
    if (!cmap || !cmap->pool) {
        return;
    }

    pool = cmap->pool;
    ztl_destroy_pool(pool);
}

void cmap_clear(cmap_t* cmap)
{
    rbtree_node_t* rbnode;
    while ((rbnode = _rbtree_peek(&cmap->rbtree)) != NULL)
    {
        rbtree_delete(&cmap->rbtree, rbnode);
    }
    cmap->size = 0;
}

int cmap_size(cmap_t* cmap)
{
    return cmap->size;
}

bool cmap_empty(cmap_t* cmap)
{
    return cmap->size == 0;
}

int cmap_add(cmap_t* cmap, uint64_t key, int64_t value)
{
    rbtree_node_t* rbnode;

    if (cmap->idles)
    {
        rbnode = cmap->idles;
        cmap->idles = cmap->idles->right;

        rbnode->left = NULL;
        rbnode->right = NULL;
        rbnode->parent = NULL;
    }
    else
    {
        rbnode = (rbtree_node_t*)ztl_pcalloc(cmap->pool, sizeof(rbtree_node_t));
    }

    union_dtype_t d;
    d.i64 = value;
    rbnode->udata = d.ptr;

    return cmap_add_ex(cmap, key, rbnode);
}

int64_t cmap_del(cmap_t* cmap, uint64_t key)
{
    int64_t value = cmap->invalid_value;
    rbtree_node_t* rbnode;

    rbnode = cmap_del_ex(cmap, key);
    if (rbnode)
    {
        union_dtype_t d;
        d.ptr = rbnode->udata;
        value = d.i64;

        rbnode->right = cmap->idles;
        cmap->idles = rbnode;
    }

    return value;
}

bool cmap_count(cmap_t* cmap, uint64_t key)
{
    rbtree_node_t* rbnode;
    rbnode = _rbtree_find(&cmap->rbtree, key);
    return rbnode ? true : false;
}

int64_t cmap_find(cmap_t* cmap, uint64_t key)
{
    rbtree_node_t* rbnode;
    rbnode = _rbtree_find(&cmap->rbtree, key);
    if (rbnode)
    {
        union_dtype_t d;
        d.ptr = rbnode->udata;
        return d.i64;
    }

    return cmap->invalid_value;
}

void cmap_traverse(cmap_t* cmap, cmap_access_pt func, void* context1, int context2)
{
    rbtree_node_t* cur = cmap->rbtree.root;
    _rbtree_traverse(cmap, &cmap->rbtree, cur, func, context1, context2);
}

void cmap_to_array(cmap_t* cmap, cmap_pair_t* kv_array, int arr_size)
{
    cmap->access_index = 0;
    // rbtree_node_t* cur = cmap->rbtree.root;
    cmap_traverse(cmap, _cmap_array_push, kv_array, arr_size);
}

int cmap_add_ex(cmap_t* cmap, uint64_t key, rbtree_node_t* nodeval)
{
    ++cmap->size;
    nodeval->key = key;
    rbtree_insert(&cmap->rbtree, nodeval);
    return 0;
}

rbtree_node_t* cmap_del_ex(cmap_t* cmap, uint64_t key)
{
    rbtree_node_t* rbnode;
    rbnode = cmap_find_ex(cmap, key);

    if (rbnode)
    {
        rbtree_delete(&cmap->rbtree, rbnode);
        --cmap->size;
    }
    return rbnode;
}

rbtree_node_t* cmap_find_ex(cmap_t* cmap, uint64_t key)
{
    rbtree_node_t* rbnode;
    rbnode = _rbtree_find(&cmap->rbtree, key);
    return rbnode;
}

//////////////////////////////////////////////////////////////////////////
struct cset_st
{
    cmap_t* cmap;
};

cset_t* cset_create(uint32_t reserve)
{
    cset_t* cset;
    cset = (cset_t*)malloc(sizeof(cset_t));
    cset->cmap = cmap_create(reserve);
    return cset;
}

void cset_release(cset_t* cset)
{
    if (cset) 
    {
        if (cset->cmap)
            cmap_release(cset->cmap);
        free(cset);
    }
}

void cset_clear(cset_t* cset)
{
    cmap_clear(cset->cmap);
}

int cset_size(cset_t* cset)
{
    return cmap_size(cset->cmap);
}

bool cset_empty(cset_t* cset)
{
    return cmap_empty(cset->cmap);
}

bool cset_count(cset_t* cset, uint64_t key)
{
    return cmap_count(cset->cmap, key);
}

int cset_add(cset_t* cset, uint64_t key)
{
    return cmap_add(cset->cmap, key, 0);
}

int cset_del(cset_t* cset, uint64_t key)
{
    return cmap_del(cset->cmap, key) != cset->cmap->invalid_value ? 0 : -1;
}

