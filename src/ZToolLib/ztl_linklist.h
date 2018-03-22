#ifndef _ZTL_LINKLIST_H_INCLUDED_
#define _ZTL_LINKLIST_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

typedef struct ztl_slist_st ztl_slist_t;

typedef struct ztl_snode_s {
    struct ztl_snode_s* next;
}ztl_snode_t;

typedef struct ztl_dnode_s {
    struct ztl_dnode_s* prev;
    struct ztl_dnode_s* next;
}ztl_dnode_t;

struct ztl_slist_st
{
    ztl_snode_t*    head;
    ztl_snode_t*    tail;
    uint32_t        count;
};

typedef struct ztl_dlist_st ztl_dlist_t;
struct ztl_dlist_st
{
    ztl_dnode_t*    head;
    ztl_dnode_t*    tail;
    uint32_t        count;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * a single linked list
 */



/*
 * a double linked list
 */

#ifdef __cplusplus
}
#endif


#endif//_ZTL_LINKLIST_H_INCLUDED_
