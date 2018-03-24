#include "ztl_linklist.h"


ztl_dlist_t* ztl_dlist_middle(ztl_dlist_t *queue)
{
    ztl_dlist_t  *middle, *next;

    middle = ztl_dlist_head(queue);

    if (middle == ztl_dlist_last(queue)) {
        return middle;
    }

    next = ztl_dlist_head(queue);

    for ( ;; ) {
        middle = ztl_dlist_next(middle);

        next = ztl_dlist_next(next);

        if (next == ztl_dlist_last(queue)) {
            return middle;
        }

        next = ztl_dlist_next(next);

        if (next == ztl_dlist_last(queue)) {
            return middle;
        }
    }
}


void ztl_dlist_sort(ztl_dlist_t *queue,
    int(*cmp)(const ztl_dlist_t *, const ztl_dlist_t *))
{
    ztl_dlist_t  *q, *prev, *next;

    q = ztl_dlist_head(queue);

    if (q == ztl_dlist_last(queue)) {
        return;
    }

    for (q = ztl_dlist_next(q); q != ztl_dlist_sentinel(queue); q = next) {

        prev = ztl_dlist_prev(q);
        next = ztl_dlist_next(q);

        ztl_dlist_remove(q);

        do {
            if (cmp(prev, q) <= 0) {
                break;
            }

            prev = ztl_dlist_prev(prev);

        } while (prev != ztl_dlist_sentinel(queue));

        ztl_dlist_insert_after(prev, q);
    }
}

