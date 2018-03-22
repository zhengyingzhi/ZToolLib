#include <stdio.h>

#include "ztl_map.h"


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

int ztl_map_init(ztl_map_t* pmap)
{
    pmap->lock = 0;
    ztl_rbtree_init(&pmap->rbtree, &pmap->sentinel, ztl_rbtree_insert_value);
    return 0;
}

int ztl_map_add(ztl_map_t* pmap, uint64_t key, ztl_rbtree_node_t* nodeval)
{
    nodeval->key = key;
    ztl_rbtree_insert(&pmap->rbtree, nodeval);
    return 0;
}

ztl_rbtree_node_t* ztl_map_del(ztl_map_t* pmap, uint64_t key)
{
    ztl_rbtree_node_t* rbnode;
    rbnode = ztl_map_find(pmap, key);

    if (rbnode) {
        ztl_rbtree_delete(&pmap->rbtree, rbnode);
    }
    return rbnode;
}

ztl_rbtree_node_t* ztl_map_find(ztl_map_t* pmap, uint64_t key)
{
    ztl_rbtree_node_t* rbnode;
    rbnode = _ztl_rbtree_find(&pmap->rbtree, key);
    if (rbnode) {
        return rbnode;
    }

    return NULL;
}
