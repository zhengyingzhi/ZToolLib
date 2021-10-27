#ifndef _ZTL_DLIST_H_INCLUDED_
#define _ZTL_DLIST_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/* default reserve queue node number when create dlist */
#define DLIST_REVERSE_NODES         1024

/* dlist iterator start direction */
#define DLSTART_HEAD                0
#define DLSTART_TAIL                1

/* dlist type */
typedef struct dlist_st         dlist_t;
typedef struct dlist_iter_st    dlist_iter_t;

struct dlist_iter_st
{
    void*       nodelink;
    void*       data;
    dlist_t*    dlist;
    int         direction;
};


dlist_t* dlist_create(int reserve_nodes,
    int (*cmp)(const void* expect, const void* actual),
    int (*vfree)(void* val));

void  dlist_release(dlist_t* dl);
void* dlist_head(dlist_t* dl);
void* dlist_tail(dlist_t* dl);
void  dlist_clear(dlist_t* dl);
int   dlist_size(dlist_t* dl);

int   dlist_insert_head(dlist_t* dl, void* data);
int   dlist_insert_tail(dlist_t* dl, void* data);
void* dlist_pop(dlist_t* dl);
void* dlist_pop_back(dlist_t* dl);
bool  dlist_have(dlist_t* dl, void* expect);

/* search the expect value in the dlist, by direction DLSTART_HEAD(0)/TAIL(1) */
void* dlist_search(dlist_t* dl, void* expect, int DLSTART_direction);

/* try remove the expect value in the dlist */
void* dlist_remove(dlist_t* dl, void* expect);

/* get an iterator of the dlist, direction could be DLSTART_HEAD(0)/TAIL(1) */
dlist_iter_t* dlist_iter_new(dlist_t* dl, int DLSTART_direction);
void dlist_iter_del(dlist_t* dl, dlist_iter_t* iter);
void* dlist_next(dlist_t* dl, dlist_iter_t* iter);

/* erase the iter data when traverse dlist */
bool dlist_erase(dlist_t* dl, dlist_iter_t* iter);

void dlist_lock(dlist_t* dl);
void dlist_unlock(dlist_t* dl);
void dlist_rwlock_rdlock(dlist_t* dl);
void dlist_rwlock_wrlock(dlist_t* dl);
void dlist_rwlock_unlock(dlist_t* dl);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_DLIST_H_INCLUDED_
