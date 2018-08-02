#ifndef _ZTL_MAP_H_INCLUDE_
#define _ZTL_MAP_H_INCLUDE_

#include <stdbool.h>
#include <stdint.h>

#include "ztl_rbtree.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct ztl_map_st ztl_map_t;

/* init a map which is implemented by rb-tree,
 * if reserve > 0, it will internally pre-reserve memory space
 */
ztl_map_t* ztl_map_create(uint32_t reserve);

/* release the map object
 */
void ztl_map_release(ztl_map_t* pmap);

/* clear the map
 */
void ztl_map_clear(ztl_map_t* pmap);

/* return size
 */
int ztl_map_size(ztl_map_t* pmap);

/* is empty or not
 */
bool ztl_map_empty(ztl_map_t* pmap);


/* add a key-value into map
 * note: only allowed use ztl_map_del to delete elem
 */
int ztl_map_add(ztl_map_t* pmap, uint64_t key, void* value);

/* del the key-value from map
 * return the value of the key
 */
void* ztl_map_del(ztl_map_t* pmap, uint64_t key);

/* find the value by the key
 */
void* ztl_map_find(ztl_map_t* pmap, uint64_t key);


//////////////////////////////////////////////////////////////////////////
/* for quick insert key-value into map by using rbtree_node             */
/* nodeval is create by user
 * note: only allowed use ztl_map_del_ex to delete elem
 */
int ztl_map_add_ex(ztl_map_t* pmap, uint64_t key, ztl_rbtree_node_t* nodeval);

/* del the key-value from map
* return the ztl_rbtree_node_t value of the key
*/
ztl_rbtree_node_t* ztl_map_del_ex(ztl_map_t* pmap, uint64_t key);

/* find the value by the key
*/
ztl_rbtree_node_t* ztl_map_find_ex(ztl_map_t* pmap, uint64_t key);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_MAP_H_INCLUDE_
