/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 * a hash table
 */

#ifndef _ZTL_TABLE_H_INCLUDED_
#define _ZTL_TABLE_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZTL_TABLE_INIT_SIZE     128

/* exported types */
typedef struct table_st*        table_t;
typedef struct table_node_st*   table_node_t;
typedef struct table_iter_st*   table_iter_t;


typedef int     (*cmp_pt)(const void* x, const void* y);
typedef uint64_t(*hash_pt)(const void* key, int size);
typedef void    (*free_pt)(void* p);

/* exported functions */
table_t       table_new(cmp_pt cmp, hash_pt hash, free_pt kfree, free_pt vfree);
void          table_free(table_t* tp);
int           table_size(table_t table);
int           table_length(table_t table);
const void*   table_node_key(table_node_t node);
void*         table_node_value(table_node_t node);
int           table_node_int(table_node_t node);
float         table_node_float(table_node_t node);
double        table_node_double(table_node_t node);
table_iter_t  table_iter_new(table_t table);
table_iter_t  table_iter_safe_new(table_t table);
void          table_iter_free(table_iter_t *tip);
void          table_iter_rewind(table_iter_t iter);
table_node_t  table_next(table_iter_t iter);
int           table_expand(table_t table, unsigned long size);
int           table_resize(table_t table);
void          table_resize_enable(void);
void          table_resize_disable(void);
table_node_t  table_insert_raw(table_t table, const void* key, int keysz);
void          table_set_value(table_node_t node, void* value);
void          table_set_int(table_node_t node, int i);
void          table_set_float(table_node_t node, float f);
void          table_set_double(table_node_t node, double d);
void*         table_insert(table_t table, const void* key, int keysz, void* value);
table_node_t  table_find(table_t table, const void* key, int keysz);
void*         table_get_value(table_t table, const void* key, int keysz);
void*         table_remove(table_t table, const void* key, int keysz);
void          table_clear(table_t table);
int           table_rehash(table_t table, int n);
void          table_rehash_ms(table_t table, int ms);

void          table_lock(table_t table);
void          table_unlock(table_t table);
void          table_rwlock_rdlock(table_t table);
void          table_rwlock_wrlock(table_t table);
void          table_rwlock_unlock(table_t table);


void          table_default_free(void* p);

int           table_default_cmpstr(const void* x, const void* y);
uint64_t      table_default_hashstr(const void* val, int size);
int           table_default_cmpint(const void* x, const void* y);
uint64_t      table_default_hashint(const void* val, int size);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_TABLE_H_INCLUDED_
