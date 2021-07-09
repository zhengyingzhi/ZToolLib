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
    lnode1.data = (void*)0x01;
    ztl_queue_insert_tail(&que, &lnode1.link);
    ZuAssertPtrEquals(zt, ztl_queue_head(&que), ztl_queue_last(&que));
    ZuAssertPtrEquals(zt, ztl_queue_head(&que), &lnode1.link);
    ZuAssertIntEquals(zt, 1, ztl_queue_size(&que));
    test_zque_t* zh1 = ztl_queue_data(ztl_queue_head(&que), test_zque_t, link);
    ZuAssertTrue(zt, zh1->data == (void*)0x01);

    test_zque_t lnode2 = { 0 };
    lnode1.data = (void*)0x02;
    ztl_queue_insert_tail(&que, &lnode2.link);
    ZuAssertPtrEquals(zt, ztl_queue_head(&que), &lnode1.link);
    ZuAssertPtrEquals(zt, ztl_queue_last(&que), &lnode2.link);
    ZuAssertIntEquals(zt, 2, ztl_queue_size(&que));
    test_zque_t* zh2 = ztl_queue_data(ztl_queue_head(&que), test_zque_t, link);
    ZuAssertTrue(zt, zh2->data == (void*)0x02);

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
    ZuAssertIntEquals(zt, 1, ztl_queue_size(&que));
    ZuAssertPtrEquals(zt, ztl_queue_head(&que), &lnode2.link);

    ztl_queue_remove(&lnode2.link);
    ZuAssertIntEquals(zt, 0, ztl_queue_size(&que));
}


void Test_ztl_dlist(ZuTest* zt)
{
    int rv;
    ztl_dlist_t* dl;
    dl = ztl_dlist_create(4, NULL, NULL);

    void* head, *tail;
    for (int64_t i = 1; i <= 6; ++i)
    {
        rv = ztl_dlist_insert_tail(dl, (void*)i);
        ZuAssertIntEquals(zt, 0, rv);
        ZuAssertIntEquals(zt, (int)i, ztl_dlist_size(dl));

        head = ztl_dlist_head(dl);
        tail = ztl_dlist_tail(dl);
        ZuAssertPtrEquals(zt, (void*)0x01, head);
        ZuAssertPtrEquals(zt, (void*)i, tail);
    }

    // search
    ZuAssertTrue(zt, ztl_dlist_have(dl, (void*)0x01));
    ZuAssertTrue(zt, ztl_dlist_have(dl, (void*)0x04));

    // pop front and back
    head = ztl_dlist_pop(dl);
    ZuAssertTrue(zt, (void*)0x01 == head);
    tail = ztl_dlist_pop_back(dl);
    ZuAssertTrue(zt, (void*)0x06 == tail);

    // traverse dlist from head or tail
    int64_t iv = 2;
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
    iter = ztl_dlist_iter_new(dl, ZTL_DLSTART_HEAD);
    do
    {
        void* data = ztl_dlist_next(dl, iter);
        if (data == NULL) {
            ++iv;
            break;
        }

        if (data == (void*)0x03) {
            ZuAssertTrue(zt, true == ztl_dlist_have(dl, (void*)0x03));
            ztl_dlist_erase(dl, iter);
            ZuAssertTrue(zt, false == ztl_dlist_have(dl, (void*)0x03));
        }
        else if (data == (void*)0x05) {
            ZuAssertTrue(zt, true == ztl_dlist_have(dl, (void*)0x05));
            ztl_dlist_erase(dl, iter);
            ZuAssertTrue(zt, false == ztl_dlist_have(dl, (void*)0x05));
        }
    } while (true);
    ztl_dlist_iter_del(dl, iter);
    ZuAssertTrue(zt, ztl_dlist_size(dl) == 2);

    // direction remove expect data
    void* actual;
    actual = ztl_dlist_remove(dl, (void*)0x04);
    ZuAssertTrue(zt, actual == (void*)0x04);
    actual = ztl_dlist_remove(dl, (void*)0x04);
    ZuAssertTrue(zt, actual == NULL);

    // the left data
    ZuAssertTrue(zt, ztl_dlist_size(dl) == 1);
    ZuAssertTrue(zt, ztl_dlist_head(dl) == (void*)0x02);
    ZuAssertTrue(zt, ztl_dlist_tail(dl) == (void*)0x02);

    ztl_dlist_release(dl);
}

