#ifndef _ZTL_DLIST_H_INCLUDED_
#define _ZTL_DLIST_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/* default reserve queue node number when create dlist */
#define ZTL_DLIST_REVERSE_NODES     1024

/* dlist iterator start direction */
#define ZTL_DLSTART_HEAD            0
#define ZTL_DLSTART_TAIL            1

/* dlist type */
typedef struct ztl_dlist_st ztl_dlist_t;

typedef struct  
{
    void*   nodelink;
    int     direction;
}ztl_dlist_iterator_t;


ztl_dlist_t* ztl_dlist_create(int reserve_nodes,
    int (*cmp)(const void* expect, const void* actual),
    int (*vfree)(void* val));

void ztl_dlist_release(ztl_dlist_t* dl);

void* ztl_dlist_head(ztl_dlist_t* dl);
void* ztl_dlist_tail(ztl_dlist_t* dl);

int ztl_dlist_size(ztl_dlist_t* dl);

int ztl_dlist_insert_head(ztl_dlist_t* dl, void* data);
int ztl_dlist_insert_tail(ztl_dlist_t* dl, void* data);

void* ztl_dlist_pop(ztl_dlist_t* dl);
void* ztl_dlist_pop_back(ztl_dlist_t* dl);

bool ztl_dlist_have(ztl_dlist_t* dl, void* expect);

void* ztl_dlist_search(ztl_dlist_t* dl, void* expect);

void* ztl_dlist_remove(ztl_dlist_t* dl, void* expect);

/* get an iterator of the dlist, direction could be ZTL_DLSTART_HEAD/TAIL */
ztl_dlist_iterator_t* ztl_dlist_iter_new(ztl_dlist_t* dl, int direction);
void ztl_dlist_iter_del(ztl_dlist_t* dl, ztl_dlist_iterator_t* iter);
void* ztl_dlist_next(ztl_dlist_t* dl, ztl_dlist_iterator_t* iter);

/* erase the iter data when traverse dlist */
bool ztl_dlist_erase(ztl_dlist_t* dl, ztl_dlist_iterator_t* iter);

void ztl_dlist_lock(ztl_dlist_t* dl);
void ztl_dlist_unlock(ztl_dlist_t* dl);
void ztl_dlist_rwlock_rdlock(ztl_dlist_t* dl);
void ztl_dlist_rwlock_wrlock(ztl_dlist_t* dl);
void ztl_dlist_rwlock_unlock(ztl_dlist_t* dl);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_DLIST_H_INCLUDED_
