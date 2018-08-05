#include "ztl_linklist.h"


ztl_queue_t* ztl_queue_middle(ztl_queue_t *queue)
{
    ztl_queue_t  *middle, *next;

    middle = ztl_queue_head(queue);

    if (middle == ztl_queue_last(queue)) {
        return middle;
    }

    next = ztl_queue_head(queue);

    for ( ;; ) {
        middle = ztl_queue_next(middle);

        next = ztl_queue_next(next);

        if (next == ztl_queue_last(queue)) {
            return middle;
        }

        next = ztl_queue_next(next);

        if (next == ztl_queue_last(queue)) {
            return middle;
        }
    }
}


void ztl_queue_sort(ztl_queue_t *queue,
    int(*cmp)(const ztl_queue_t *, const ztl_queue_t *))
{
    ztl_queue_t  *q, *prev, *next;

    q = ztl_queue_head(queue);

    if (q == ztl_queue_last(queue)) {
        return;
    }

    for (q = ztl_queue_next(q); q != ztl_queue_sentinel(queue); q = next) {

        prev = ztl_queue_prev(q);
        next = ztl_queue_next(q);

        ztl_queue_remove(q);

        do {
            if (cmp(prev, q) <= 0) {
                break;
            }

            prev = ztl_queue_prev(prev);

        } while (prev != ztl_queue_sentinel(queue));

        ztl_queue_insert_after(prev, q);
    }
}

int ztl_queue_size(ztl_queue_t* queue)
{
    int size;
    ztl_queue_t  *q;

    size = 0;
    q = ztl_queue_head(queue);

    for (; q != ztl_queue_sentinel(queue); ) {
        ++size;
        q = ztl_queue_next(q);
    }

    return size;
}

