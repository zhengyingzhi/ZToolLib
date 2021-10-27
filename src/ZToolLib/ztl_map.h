#ifndef _ZTL_MAP_H_INCLUDE_
#define _ZTL_MAP_H_INCLUDE_

#include <stdbool.h>
#include <stdint.h>

#include "ztl_rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif


#define ZTL_MAP_INVALID_VALUE       (-1)


typedef uint64_t    cmap_key_t;
typedef uint64_t    cmap_value_t;

typedef struct
{
    cmap_key_t      Key;
    cmap_value_t    Value;
}cmap_pair_t;

/* a simple map base on rbtree implement by C */
typedef struct cmap_st cmap_t;

typedef void (*cmap_access_pt)(cmap_t* cmap, void* context1, int context2, uint64_t key, int64_t value);

/* init a map which is implemented by rb-tree,
 * if reserve > 0, it will internally pre-reserve memory space
 */
cmap_t* cmap_create(uint32_t reserve);
void cmap_release(cmap_t* cmap);
void cmap_clear(cmap_t* cmap);
int  cmap_size(cmap_t* cmap);
bool cmap_empty(cmap_t* cmap);

/* add a key-value into map
 * note: only allowed use cmap_del to delete elem
 */
int cmap_add(cmap_t* cmap, uint64_t key, int64_t value);

/* del the key-value from map
 * return the value of the key
 */
int64_t cmap_del(cmap_t* cmap, uint64_t key);

/* have the value by the key
 */
bool cmap_count(cmap_t* cmap, uint64_t key);

/* find the value by the key
 */
int64_t cmap_find(cmap_t* cmap, uint64_t key);

/* traverse all elems of the map
 */
void cmap_traverse(cmap_t* cmap, cmap_access_pt func, void* context1, int context2);

/* convert to array
 */
void cmap_to_array(cmap_t* cmap, cmap_pair_t* kv_array, int arr_size);

//////////////////////////////////////////////////////////////////////////
/* for quick insert key-value into map by using rbtree_node             */
/* nodeval is create by user
 * note: only allowed use cmap_del_ex to delete elem
 */
int cmap_add_ex(cmap_t* cmap, uint64_t key, rbtree_node_t* nodeval);

/* del the key-value from map
 * return the rbtree_node_t value of the key
 */
rbtree_node_t* cmap_del_ex(cmap_t* cmap, uint64_t key);

/* find the value by the key
 */
rbtree_node_t* cmap_find_ex(cmap_t* cmap, uint64_t key);


//////////////////////////////////////////////////////////////////////////

typedef struct cset_st cset_t;
cset_t* cset_create(uint32_t reserve);
void cset_release(cset_t* cset);
void cset_clear(cset_t* cset);
int  cset_size(cset_t* cset);
bool cset_empty(cset_t* cset);
bool cset_count(cset_t* cset, uint64_t key);
int  cset_add(cset_t* cset, uint64_t key);
int  cset_del(cset_t* cset, uint64_t key);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_MAP_H_INCLUDE_
