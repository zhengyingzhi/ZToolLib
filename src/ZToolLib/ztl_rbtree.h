/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) zhengyingzhi112@163.com
 */

#ifndef _ZTL_RBTREE_H_INCLUDED_
#define _ZTL_RBTREE_H_INCLUDED_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t  ztl_rbtree_key_t;
typedef int64_t   ztl_rbtree_key_int_t;
typedef uint64_t  ztl_msec_t;
typedef int64_t   ztl_msec_int_t;

typedef struct ztl_rbtree_node_s ztl_rbtree_node_t;

struct ztl_rbtree_node_s {
    ztl_rbtree_key_t      key;
    ztl_rbtree_node_t*    left;
    ztl_rbtree_node_t*    right;
    ztl_rbtree_node_t*    parent;
    void*                 udata;
    uint8_t               color;
    uint8_t               data;
};


typedef struct ztl_rbtree_s  ztl_rbtree_t;

typedef void (*ztl_rbtree_insert_pt) (ztl_rbtree_node_t *root,
    ztl_rbtree_node_t* node, ztl_rbtree_node_t* sentinel);

struct ztl_rbtree_s {
    ztl_rbtree_node_t*     root;
    ztl_rbtree_node_t*     sentinel;
    ztl_rbtree_insert_pt   insert;
};


#define ztl_rbtree_init(tree, s, i)                                           \
    ztl_rbtree_sentinel_init(s);                                              \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s;                                                     \
    (tree)->insert = i


void ztl_rbtree_insert(ztl_rbtree_t *tree, ztl_rbtree_node_t *node);
void ztl_rbtree_delete(ztl_rbtree_t *tree, ztl_rbtree_node_t *node);
void ztl_rbtree_insert_value(ztl_rbtree_node_t *root, ztl_rbtree_node_t *node, ztl_rbtree_node_t *sentinel);
void ztl_rbtree_insert_timer_value(ztl_rbtree_node_t *root, ztl_rbtree_node_t *node, ztl_rbtree_node_t *sentinel);


#define ztl_rbt_red(node)               ((node)->color = 1)
#define ztl_rbt_black(node)             ((node)->color = 0)
#define ztl_rbt_is_red(node)            ((node)->color)
#define ztl_rbt_is_black(node)          (!ztl_rbt_is_red(node))
#define ztl_rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */
#define ztl_rbtree_sentinel_init(node)	ztl_rbt_black(node)


static ztl_rbtree_node_t* ztl_rbtree_min(
    ztl_rbtree_node_t *node, 
	ztl_rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}

#ifdef __cplusplus
}
#endif

#endif /* _ZTL_RBTREE_H_INCLUDED_ */
