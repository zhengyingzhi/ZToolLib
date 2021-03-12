#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ztl_mempool.h"
#include "ztl_rbtree.h"
#include "ztl_map.h"
#include "ztl_utils.h"


struct ztl_map_st
{
    ztl_rbtree_t        rbtree;
    ztl_rbtree_node_t   sentinel;
    ztl_mempool_t*      mp;
    int64_t             invalid_value;
    int32_t             size;
    uint32_t            reserve;
    int32_t             access_index;
};


static ztl_rbtree_node_t* _ztl_rbtree_find(ztl_rbtree_t *tree,
    ztl_rbtree_key_t key)
{
    ztl_rbtree_node_t   *root, *sentinel;

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

static ztl_mempool_t* _ztl_map_create_pool(ztl_map_t* pmap)
{
    return ztl_mp_create(sizeof(ztl_rbtree_node_t), pmap->reserve, true);
}

ztl_map_t* ztl_map_create(uint32_t reserve)
{
    ztl_map_t* lmap;
    lmap = (ztl_map_t*)malloc(sizeof(ztl_map_t));
    memset(lmap, 0, sizeof(ztl_map_t));

    lmap->invalid_value = ZTL_MAP_INVALID_VALUE;

    lmap->reserve = reserve;
    if (reserve > 0) {
        lmap->mp = _ztl_map_create_pool(lmap);
    }
    else {
        lmap->mp = NULL;
    }

    ztl_map_clear(lmap);

    return lmap;
}

void ztl_map_release(ztl_map_t* pmap)
{
    if (pmap->mp) {
        ztl_mp_release(pmap->mp);
    }
    free(pmap);
}

void ztl_map_clear(ztl_map_t* pmap)
{
    ztl_rbtree_init(&pmap->rbtree, &pmap->sentinel, ztl_rbtree_insert_value);
    if (pmap->mp)
    {
        ztl_mp_release(pmap->mp);
        pmap->mp = _ztl_map_create_pool(pmap);
    }
    pmap->size = 0;
}

int ztl_map_size(ztl_map_t* pmap)
{
    return pmap->size;
}

bool ztl_map_empty(ztl_map_t* pmap)
{
    return pmap->size == 0;
}

int ztl_map_add(ztl_map_t* pmap, uint64_t key, int64_t value)
{
    ztl_rbtree_node_t* rbnode;

    if (!pmap->mp) {
        pmap->mp = _ztl_map_create_pool(pmap);
    }

    rbnode = (ztl_rbtree_node_t*)ztl_mp_alloc(pmap->mp);
    memset(rbnode, 0, sizeof(ztl_rbtree_node_t));

    union_dtype_t d;
    d.i64 = value;
    rbnode->udata = d.ptr;

    return ztl_map_add_ex(pmap, key, rbnode);
}

int64_t ztl_map_del(ztl_map_t* pmap, uint64_t key)
{
    int64_t value = pmap->invalid_value;
    ztl_rbtree_node_t* rbnode;

    rbnode = ztl_map_del_ex(pmap, key);
    if (rbnode)
    {
        union_dtype_t d;
        d.ptr = rbnode->udata;
        value = d.i64;
        ztl_mp_free(pmap->mp, rbnode);
    }

    return value;
}

bool ztl_map_count(ztl_map_t* pmap, uint64_t key)
{
    ztl_rbtree_node_t* rbnode;
    rbnode = _ztl_rbtree_find(&pmap->rbtree, key);
    if (rbnode) {
        return true;
    }

    return false;
}

int64_t ztl_map_find(ztl_map_t* pmap, uint64_t key)
{
    ztl_rbtree_node_t* rbnode;
    rbnode = _ztl_rbtree_find(&pmap->rbtree, key);
    if (rbnode) {
        union_dtype_t d;
        d.ptr = rbnode->udata;
        return d.i64;
    }

    return pmap->invalid_value;
}

static void _ztl_rbtree_traverse(ztl_map_t* pmap, ztl_rbtree_t *tree, ztl_rbtree_node_t* cur, 
    ztl_map_access_pt func, void* context1, int context2)
{
    if (cur && cur != tree->sentinel)
    {
        if (func) {
            union_dtype_t d;
            d.ptr = cur->udata;
            func(pmap, context1, context2, cur->key, d.i64);
        }

        if (cur->left)
            _ztl_rbtree_traverse(pmap, tree, cur->left, func, context1, context2);
        if (cur->right)
            _ztl_rbtree_traverse(pmap, tree, cur->right, func, context1, context2);
    }
}

void ztl_map_traverse(ztl_map_t* pmap, ztl_map_access_pt func, void* context1, int context2)
{
    ztl_rbtree_node_t* cur = pmap->rbtree.root;
    _ztl_rbtree_traverse(pmap, &pmap->rbtree, cur, func, context1, context2);
}

static void _ztl_map_array_push(ztl_map_t* pmap, void* context1, int context2, uint64_t key, int64_t value)
{
    if (pmap->access_index >= context2) {
        return;
    }
    ztl_map_pair_t* kv_arr, *pkv;
    kv_arr = (ztl_map_pair_t*)context1;
    pkv = &(kv_arr[pmap->access_index++]);
    pkv->Key = key;
    pkv->Value = value;
}

void ztl_map_to_array(ztl_map_t* pmap, ztl_map_pair_t* kv_array, int arr_size)
{
    pmap->access_index = 0;
    ztl_rbtree_node_t* cur = pmap->rbtree.root;
    ztl_map_traverse(pmap, _ztl_map_array_push, kv_array, arr_size);
}


int ztl_map_add_ex(ztl_map_t* pmap, uint64_t key, ztl_rbtree_node_t* nodeval)
{
    ++pmap->size;
    nodeval->key = key;
    ztl_rbtree_insert(&pmap->rbtree, nodeval);
    return 0;
}

ztl_rbtree_node_t* ztl_map_del_ex(ztl_map_t* pmap, uint64_t key)
{
    ztl_rbtree_node_t* rbnode;
    rbnode = ztl_map_find_ex(pmap, key);

    if (rbnode) {
        ztl_rbtree_delete(&pmap->rbtree, rbnode);
        --pmap->size;
    }
    return rbnode;
}

ztl_rbtree_node_t* ztl_map_find_ex(ztl_map_t* pmap, uint64_t key)
{
    ztl_rbtree_node_t* rbnode;
    rbnode = _ztl_rbtree_find(&pmap->rbtree, key);
    if (rbnode) {
        return rbnode;
    }

    return NULL;
}


//////////////////////////////////////////////////////////////////////////
struct ztl_set_st
{
    ztl_map_t* map;
};

ztl_set_t* ztl_set_create(uint32_t reserve)
{
    ztl_set_t* pset;
    pset = (ztl_set_t*)malloc(sizeof(ztl_set_t));

    pset->map = ztl_map_create(reserve);

    return pset;
}

void ztl_set_release(ztl_set_t* pset)
{
    if (pset) 
    {
        if (pset->map)
            ztl_map_release(pset->map);
        free(pset);
    }
}

void ztl_set_clear(ztl_set_t* pset)
{
    ztl_map_clear(pset->map);
}

int ztl_set_size(ztl_set_t* pset)
{
    return ztl_map_size(pset->map);
}

bool ztl_set_empty(ztl_set_t* pset)
{
    return ztl_map_empty(pset->map);
}

bool ztl_set_count(ztl_set_t* pset, uint64_t key)
{
    return ztl_map_count(pset->map, key);
}

int ztl_set_add(ztl_set_t* pset, uint64_t key)
{
    return ztl_map_add(pset->map, key, 0);
}

int ztl_set_del(ztl_set_t* pset, uint64_t key)
{
    return ztl_map_del(pset->map, key) != pset->map->invalid_value ? 0 : -1;
}

