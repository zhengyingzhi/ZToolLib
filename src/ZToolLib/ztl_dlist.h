/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 * a easily double linked list
 */

#ifndef _ZTL_DLIST_H_INCLUDED_
#define _ZTL_DLIST_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/* default reserve queue node number when create dlist */
#define ZTL_DLIST_REVERSE_NODES     1024

#define ZTL_DLIST_ITER_BACK         0
#define ZTL_DLIST_ITER_PREV         1


typedef struct ztl_dlist_st ztl_dlist_t;

typedef struct  
{
    void*   node;
    int     direction;
}ztl_dlist_iterator_t;

typedef void*(*ztl_dlist_alloc_pt)(void* ctx, void* oldaddr, uint32_t size);
typedef void(*ztl_dlist_dealloc_pt)(void* ctx, void* addr);

ztl_dlist_t* ztl_dlist_create(int reserve_nodes);

void ztl_dlist_release(ztl_dlist_t* dl);

void* ztl_dlist_head(ztl_dlist_t* dl);
void* ztl_dlist_tail(ztl_dlist_t* dl);

int ztl_dlist_size(ztl_dlist_t* dl);

int ztl_dlist_insert_head(ztl_dlist_t* dl, void* data);
int ztl_dlist_insert_tail(ztl_dlist_t* dl, void* data);

void* ztl_dlist_pop(ztl_dlist_t* dl);
void* ztl_dlist_pop_back(ztl_dlist_t* dl);

ztl_dlist_iterator_t* ztl_dlist_iter_new(ztl_dlist_t* dl, int direction);
void ztl_dlist_iter_del(ztl_dlist_t* dl, ztl_dlist_iterator_t* iter);
void* ztl_dlist_next(ztl_dlist_t* dl, ztl_dlist_iterator_t* iter);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_DLIST_H_INCLUDED_
