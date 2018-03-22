#ifndef _ZTL_MAP_H_INCLUDE_
#define _ZTL_MAP_H_INCLUDE_

#include "ztl_rbtree.h"

typedef struct ztl_map_st
{
    volatile uint32_t   lock;
    ztl_rbtree_t        rbtree;
    ztl_rbtree_node_t   sentinel;
}ztl_map_t;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * init the map
 */
int ztl_map_init(ztl_map_t* pmap);

/// add key-value to map
int ztl_map_add(ztl_map_t* pmap, uint64_t key, ztl_rbtree_node_t* nodeval);

/// delete the key-value from map, and return the delete key
ztl_rbtree_node_t* ztl_map_del(ztl_map_t* pmap, uint64_t key);

/// find the value by the key
ztl_rbtree_node_t* ztl_map_find(ztl_map_t* pmap, uint64_t key);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_MAP_H_INCLUDE_
