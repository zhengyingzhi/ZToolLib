#include <stdio.h>

#include "ztl_rbtree.h"


/*
 * The red-black tree code is based on the algorithm described in
 * the "Introduction to Algorithms" by Cormen, Leiserson and Rivest.
 */


static void ztl_rbtree_left_rotate(ztl_rbtree_node_t **root,
    ztl_rbtree_node_t *sentinel, ztl_rbtree_node_t *node);
static void ztl_rbtree_right_rotate(ztl_rbtree_node_t **root,
    ztl_rbtree_node_t *sentinel, ztl_rbtree_node_t *node);


void ztl_rbtree_insert(ztl_rbtree_t *tree, ztl_rbtree_node_t *node)
{
    ztl_rbtree_node_t  **root, *temp, *sentinel;

    /* a binary tree insert */

    root = (ztl_rbtree_node_t **) &tree->root;
    sentinel = tree->sentinel;

    if (*root == sentinel) {
        node->parent = NULL;
        node->left = sentinel;
        node->right = sentinel;
        ztl_rbt_black(node);
        *root = node;

        return;
    }

    tree->insert(*root, node, sentinel);

    /* re-balance tree */

    while (node != *root && ztl_rbt_is_red(node->parent)) {

        if (node->parent == node->parent->parent->left) {
            temp = node->parent->parent->right;

            if (ztl_rbt_is_red(temp)) {
                ztl_rbt_black(node->parent);
                ztl_rbt_black(temp);
                ztl_rbt_red(node->parent->parent);
                node = node->parent->parent;

            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    ztl_rbtree_left_rotate(root, sentinel, node);
                }

                ztl_rbt_black(node->parent);
                ztl_rbt_red(node->parent->parent);
                ztl_rbtree_right_rotate(root, sentinel, node->parent->parent);
            }

        } else {
            temp = node->parent->parent->left;

            if (ztl_rbt_is_red(temp)) {
                ztl_rbt_black(node->parent);
                ztl_rbt_black(temp);
                ztl_rbt_red(node->parent->parent);
                node = node->parent->parent;

            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    ztl_rbtree_right_rotate(root, sentinel, node);
                }

                ztl_rbt_black(node->parent);
                ztl_rbt_red(node->parent->parent);
                ztl_rbtree_left_rotate(root, sentinel, node->parent->parent);
            }
        }
    }

    ztl_rbt_black(*root);
}


void ztl_rbtree_insert_value(ztl_rbtree_node_t *temp, 
    ztl_rbtree_node_t *node,
    ztl_rbtree_node_t *sentinel)
{
    ztl_rbtree_node_t  **p;

    for ( ;; ) {

        p = (node->key < temp->key) ? &temp->left : &temp->right;

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ztl_rbt_red(node);
}


void ztl_rbtree_insert_timer_value(ztl_rbtree_node_t *temp, 
    ztl_rbtree_node_t *node,
    ztl_rbtree_node_t *sentinel)
{
    ztl_rbtree_node_t  **p;

    for ( ;; ) {

        /*
         * Timer values
         * 1) are spread in small range, usually several minutes,
         * 2) and overflow each 49 days, if milliseconds are stored in 32 bits.
         * The comparison takes into account that overflow.
         */

        /*  node->key < temp->key */

        p = ((ztl_rbtree_key_int_t) (node->key - temp->key) < 0)
            ? &temp->left : &temp->right;

        if (*p == sentinel) {
            break;
        }

        temp = *p;
    }

    *p = node;
    node->parent = temp;
    node->left = sentinel;
    node->right = sentinel;
    ztl_rbt_red(node);
}


void
ztl_rbtree_delete(ztl_rbtree_t *tree, ztl_rbtree_node_t *node)
{
    uint32_t            red;
    ztl_rbtree_node_t  **root, *sentinel, *subst, *temp, *w;

    /* a binary tree delete */

    root = (ztl_rbtree_node_t **) &tree->root;
    sentinel = tree->sentinel;

    if (node->left == sentinel) {
        temp = node->right;
        subst = node;

    } else if (node->right == sentinel) {
        temp = node->left;
        subst = node;

    } else {
        subst = ztl_rbtree_min(node->right, sentinel);

        if (subst->left != sentinel) {
            temp = subst->left;
        } else {
            temp = subst->right;
        }
    }

    if (subst == *root) {
        *root = temp;
        ztl_rbt_black(temp);

        /* DEBUG stuff */
        node->left = NULL;
        node->right = NULL;
        node->parent = NULL;
        node->key = 0;

        return;
    }

    red = ztl_rbt_is_red(subst);

    if (subst == subst->parent->left) {
        subst->parent->left = temp;

    } else {
        subst->parent->right = temp;
    }

    if (subst == node) {

        temp->parent = subst->parent;

    } else {

        if (subst->parent == node) {
            temp->parent = subst;

        } else {
            temp->parent = subst->parent;
        }

        subst->left = node->left;
        subst->right = node->right;
        subst->parent = node->parent;
        ztl_rbt_copy_color(subst, node);

        if (node == *root) {
            *root = subst;

        } else {
            if (node == node->parent->left) {
                node->parent->left = subst;
            } else {
                node->parent->right = subst;
            }
        }

        if (subst->left != sentinel) {
            subst->left->parent = subst;
        }

        if (subst->right != sentinel) {
            subst->right->parent = subst;
        }
    }

    /* DEBUG stuff */
    node->left = NULL;
    node->right = NULL;
    node->parent = NULL;
    node->key = 0;

    if (red) {
        return;
    }

    /* a delete fixup */

    while (temp != *root && ztl_rbt_is_black(temp)) {

        if (temp == temp->parent->left) {
            w = temp->parent->right;

            if (ztl_rbt_is_red(w)) {
                ztl_rbt_black(w);
                ztl_rbt_red(temp->parent);
                ztl_rbtree_left_rotate(root, sentinel, temp->parent);
                w = temp->parent->right;
            }

            if (ztl_rbt_is_black(w->left) && ztl_rbt_is_black(w->right)) {
                ztl_rbt_red(w);
                temp = temp->parent;

            } else {
                if (ztl_rbt_is_black(w->right)) {
                    ztl_rbt_black(w->left);
                    ztl_rbt_red(w);
                    ztl_rbtree_right_rotate(root, sentinel, w);
                    w = temp->parent->right;
                }

                ztl_rbt_copy_color(w, temp->parent);
                ztl_rbt_black(temp->parent);
                ztl_rbt_black(w->right);
                ztl_rbtree_left_rotate(root, sentinel, temp->parent);
                temp = *root;
            }

        } else {
            w = temp->parent->left;

            if (ztl_rbt_is_red(w)) {
                ztl_rbt_black(w);
                ztl_rbt_red(temp->parent);
                ztl_rbtree_right_rotate(root, sentinel, temp->parent);
                w = temp->parent->left;
            }

            if (ztl_rbt_is_black(w->left) && ztl_rbt_is_black(w->right)) {
                ztl_rbt_red(w);
                temp = temp->parent;

            } else {
                if (ztl_rbt_is_black(w->left)) {
                    ztl_rbt_black(w->right);
                    ztl_rbt_red(w);
                    ztl_rbtree_left_rotate(root, sentinel, w);
                    w = temp->parent->left;
                }

                ztl_rbt_copy_color(w, temp->parent);
                ztl_rbt_black(temp->parent);
                ztl_rbt_black(w->left);
                ztl_rbtree_right_rotate(root, sentinel, temp->parent);
                temp = *root;
            }
        }
    }

    ztl_rbt_black(temp);
}


ztl_rbtree_node_t* ztl_rbtree_min(ztl_rbtree_node_t *node, ztl_rbtree_node_t *sentinel)
{
    while (node->left != sentinel) {
        node = node->left;
    }

    return node;
}

static void ztl_rbtree_left_rotate(
	ztl_rbtree_node_t **root, 
	ztl_rbtree_node_t *sentinel,
    ztl_rbtree_node_t *node)
{
    ztl_rbtree_node_t  *temp;

    temp = node->right;
    node->right = temp->left;

    if (temp->left != sentinel) {
        temp->left->parent = node;
    }

    temp->parent = node->parent;

    if (node == *root) {
        *root = temp;

    } else if (node == node->parent->left) {
        node->parent->left = temp;

    } else {
        node->parent->right = temp;
    }

    temp->left = node;
    node->parent = temp;
}


static void ztl_rbtree_right_rotate(
	ztl_rbtree_node_t **root, 
	ztl_rbtree_node_t *sentinel,
    ztl_rbtree_node_t *node)
{
    ztl_rbtree_node_t  *temp;

    temp = node->left;
    node->left = temp->right;

    if (temp->right != sentinel) {
        temp->right->parent = node;
    }

    temp->parent = node->parent;

    if (node == *root) {
        *root = temp;

    } else if (node == node->parent->right) {
        node->parent->right = temp;

    } else {
        node->parent->left = temp;
    }

    temp->right = node;
    node->parent = temp;
}

