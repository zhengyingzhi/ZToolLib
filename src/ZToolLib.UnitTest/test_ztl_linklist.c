#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_linklist.h>

#include <ZToolLib/ztl_dlist.h>


typedef struct test_zque_st
{
    void*       data;
    ztl_queue_t link;
}test_zque_t;

void Test_ztl_linklist(ZuTest* zt)
{
    ztl_queue_t que;
    ztl_queue_init(&que);
    ZuAssertTrue(zt, ztl_queue_empty(&que));

    // insert
    test_zque_t lnode1 = { 0 };
    lnode1.data = (void*)1;
    ztl_queue_insert_tail(&que, &lnode1.link);
    ZuAssertTrue(zt, ztl_queue_head(&que) == ztl_queue_last(&que));
    ZuAssertTrue(zt, ztl_queue_head(&que) == &lnode1.link);
    ZuAssertTrue(zt, 1 == ztl_queue_size(&que));
    test_zque_t* zh1 = ztl_queue_data(ztl_queue_head(&que), test_zque_t, link);
    ZuAssertTrue(zt, zh1->data == (void*)1);

    test_zque_t lnode2 = { 0 };
    lnode1.data = (void*)2;
    ztl_queue_insert_tail(&que, &lnode2.link);
    ZuAssertTrue(zt, ztl_queue_head(&que) == &lnode1.link);
    ZuAssertTrue(zt, ztl_queue_last(&que) == &lnode2.link);
    ZuAssertTrue(zt, 2 == ztl_queue_size(&que));
    test_zque_t* zh2 = ztl_queue_data(ztl_queue_head(&que), test_zque_t, link);
    ZuAssertTrue(zt, zh2->data == (void*)2);

    // traverse
    int size = 0;
    ztl_queue_t* cur;
    cur = ztl_queue_head(&que);
    while (cur != ztl_queue_sentinel(&que))
    {
        ++size;
        cur = ztl_queue_next(cur);
    }
    ZuAssertTrue(zt, 2 == size);

    // remove
    ztl_queue_remove(&lnode1.link);
    ZuAssertTrue(zt, 1 == ztl_queue_size(&que));
    ZuAssertTrue(zt, ztl_queue_head(&que) == &lnode2.link);

    ztl_queue_remove(&lnode2.link);
    ZuAssertTrue(zt, 0 == ztl_queue_size(&que));
}


void Test_ztl_dlist(ZuTest* zt)
{
    int rv;
    ztl_dlist_t* dl;
    dl = ztl_dlist_create(4);

    void* head, *tail;
    for (int i = 1; i <= 6; ++i)
    {
        rv = ztl_dlist_insert_tail(dl, (void*)i);
        ZuAssertTrue(zt, rv == 0);
        ZuAssertTrue(zt, ztl_dlist_size(dl) == i);

        head = ztl_dlist_head(dl);
        tail = ztl_dlist_tail(dl);
        ZuAssertTrue(zt, (void*)1 == head);
        ZuAssertTrue(zt, (void*)i == tail);
    }

    // search
    ZuAssertTrue(zt, ztl_dlist_have(dl, (void*)1));
    ZuAssertTrue(zt, ztl_dlist_have(dl, (void*)4));

    // pop front and back
    head = ztl_dlist_pop(dl);
    ZuAssertTrue(zt, (void*)1 == head);
    tail = ztl_dlist_pop_back(dl);
    ZuAssertTrue(zt, (void*)6 == tail);

    // traverse dlist from head or tail
    int iv = 2;
    ztl_dlist_iterator_t* iter;
    iter = ztl_dlist_iter_new(dl, ZTL_DLSTART_HEAD);
    do 
    {
        void* data = ztl_dlist_next(dl, iter);
        if (data == NULL) {
            --iv;
            break;
        }

        ZuAssertTrue(zt, (void*)iv == data);
        ++iv;
    } while (true);
    ztl_dlist_iter_del(dl, iter);
    ZuAssertTrue(zt, iv == 5);

    iv = 5;
    iter = ztl_dlist_iter_new(dl, ZTL_DLSTART_TAIL);
    do
    {
        void* data = ztl_dlist_next(dl, iter);
        if (data == NULL) {
            ++iv;
            break;
        }

        ZuAssertTrue(zt, (void*)iv == data);
        --iv;
    } while (true);
    ztl_dlist_iter_del(dl, iter);
    ZuAssertTrue(zt, iv == 2);

    // erase by iterator
    bool lrmoved = false;
    iter = ztl_dlist_iter_new(dl, ZTL_DLSTART_HEAD);
    do
    {
        void* data = ztl_dlist_next(dl, iter);
        if (data == NULL) {
            ++iv;
            break;
        }

        if (data == (void*)3) {
            ZuAssertTrue(zt, true == ztl_dlist_have(dl, (void*)3));
            ztl_dlist_erase(dl, iter);
            ZuAssertTrue(zt, false == ztl_dlist_have(dl, (void*)3));
        }
        else if (data == (void*)5) {
            ZuAssertTrue(zt, true == ztl_dlist_have(dl, (void*)5));
            ztl_dlist_erase(dl, iter);
            ZuAssertTrue(zt, false == ztl_dlist_have(dl, (void*)5));
        }
    } while (true);
    ztl_dlist_iter_del(dl, iter);
    ZuAssertTrue(zt, ztl_dlist_size(dl) == 2);

    // direction remove expect data
    void* actual;
    actual = ztl_dlist_remove(dl, (void*)4, NULL);
    ZuAssertTrue(zt, actual == (void*)4);
    actual = ztl_dlist_remove(dl, (void*)4, NULL);
    ZuAssertTrue(zt, actual == NULL);

    // the left data
    ZuAssertTrue(zt, ztl_dlist_size(dl) == 1);
    ZuAssertTrue(zt, ztl_dlist_head(dl) == (void*)2);
    ZuAssertTrue(zt, ztl_dlist_tail(dl) == (void*)2);

    ztl_dlist_release(dl);
}

