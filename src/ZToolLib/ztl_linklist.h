#ifndef _ZTL_LINKLIST_H_INCLUDED_
#define _ZTL_LINKLIST_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

/*
 * a double linked list
 * user should define a dlist node, which contains a ztl_dlist_t member
 * and use 'ztl_dlist_data' to get the node's address when visit the list
 */

typedef struct ztl_dlist_s  ztl_dlist_t;

struct ztl_dlist_s {
    struct ztl_dlist_s  *prev;
    struct ztl_dlist_s  *next;
};


#define ztl_dlist_init(q)                                                     \
    (q)->prev = q;                                                            \
    (q)->next = q

#define ztl_dlist_empty(h)                                                    \
    (h == (h)->prev)

#define ztl_dlist_insert_head(h, x)                                           \
    (x)->next = (h)->next;                                                    \
    (x)->next->prev = x;                                                      \
    (x)->prev = h;                                                            \
    (h)->next = x

#define ztl_dlist_insert_after   ztl_dlist_insert_head

#define ztl_dlist_insert_tail(h, x)                                           \
    (x)->prev = (h)->prev;                                                    \
    (x)->prev->next = x;                                                      \
    (x)->next = h;                                                            \
    (h)->prev = x

#define ztl_dlist_head(h)                                                     \
    (h)->next

#define ztl_dlist_last(h)                                                     \
    (h)->prev

#define ztl_dlist_sentinel(h)                                                 \
    (h)

#define ztl_dlist_next(q)                                                     \
    (q)->next

#define ztl_dlist_prev(q)                                                     \
    (q)->prev

#if (ZTL_DEBUG)

#define ztl_dlist_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next;                                              \
    (x)->prev = NULL;                                                         \
    (x)->next = NULL

#else

#define ztl_dlist_remove(x)                                                   \
    (x)->next->prev = (x)->prev;                                              \
    (x)->prev->next = (x)->next

#endif//ZTL_DEBUG


#define ztl_dlist_split(h, q, n)                                              \
    (n)->prev = (h)->prev;                                                    \
    (n)->prev->next = n;                                                      \
    (n)->next = q;                                                            \
    (h)->prev = (q)->prev;                                                    \
    (h)->prev->next = h;                                                      \
    (q)->prev = n;

#define ztl_dlist_add(h, n)                                                   \
    (h)->prev->next = (n)->next;                                              \
    (n)->next->prev = (h)->prev;                                              \
    (h)->prev = (n)->prev;                                                    \
    (h)->prev->next = h;

/* the data object which contain the prev & next */
#define ztl_dlist_data(q, type, link)                                         \
    (type *) ((u_char *) q - offsetof(type, link))


/*
 * find the middle queue element if the queue has odd number of elements
 * or the first element of the queue's second part otherwise
 */
ztl_dlist_t* ztl_dlist_middle(ztl_dlist_t* dlist);


/* the stable insertion sort */
void ztl_dlist_sort(ztl_dlist_t *queue,
    int (*cmp)(const ztl_dlist_t *, const ztl_dlist_t *));

#ifdef __cplusplus
}
#endif


#endif//_ZTL_LINKLIST_H_INCLUDED_
