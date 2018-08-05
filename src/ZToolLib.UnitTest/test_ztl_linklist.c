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
    ztl_dlist_t* dl;
    dl = ztl_dlist_create(4);


    ztl_dlist_release(dl);
}

