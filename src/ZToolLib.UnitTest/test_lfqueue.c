#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/lockfreequeue.h>


void Test_lfqueue(ZuTest* zt)
{
#define TEST_QUEUE_SIZE_A 4
    lfqueue_t*  que;
    int32_t     eltsize = sizeof(int);
    int32_t     rv;

    que = lfqueue_create(TEST_QUEUE_SIZE_A, sizeof(void*));

    ZuAssertTrue(zt, lfqueue_empty(que));

    int data[] = { 1, 2, 3, 4, 5 };
    for (int i = 0; i < TEST_QUEUE_SIZE_A - 1; ++i)
    {
        ZuAssertTrue(zt, 0 == lfqueue_push(que, &data[i]));
    }

    ZuAssertTrue(zt, 3 == lfqueue_size(que));

    int64_t outdata = 0;
    rv = lfqueue_pop(que, &outdata);
    ZuAssertTrue(zt, rv == 0);
    ZuAssertTrue(zt, outdata == 1);

    lfqueue_pop(que, &outdata);
    ZuAssertTrue(zt, rv == 0);
    ZuAssertTrue(zt, outdata == 2);

    lfqueue_pop(que, &outdata);
    ZuAssertTrue(zt, rv == 0);
    ZuAssertTrue(zt, outdata == 3);

    rv = lfqueue_pop(que, &outdata);
    ZuAssertTrue(zt, rv != 0);

    lfqueue_release(que);
}

void Test_lfqueue2(ZuTest* zt)
{
#define TEST_QUEUE_SIZE_B 4
    lfqueue_t*  que;
    void*       addr;
    int64_t     memsize;
    int32_t     eltsize = sizeof(void*);

    memsize = lfqueue_memory_size(TEST_QUEUE_SIZE_B, eltsize);
    addr = calloc(1, (size_t)memsize);

    // create a queue by using our memory
    que = lfqueue_create2(TEST_QUEUE_SIZE_B, eltsize, addr, memsize);

    ZuAssertTrue(zt, lfqueue_empty(que));

    void* lp1 = (void*)0x01;
    void* lp2 = (void*)0x02;
    void* lp3 = (void*)0x03;

    // push elements into queue (we must pass reference address of each element)
    ZuAssertTrue(zt, 0 == lfqueue_push(que, &lp1));
    ZuAssertTrue(zt, 1 == lfqueue_size(que));

    void* lpout;
    ZuAssertTrue(zt, 0 == lfqueue_pop(que, &lpout));
    ZuAssertPtrEquals(zt, lp1, lpout);


    ZuAssertTrue(zt, 0 == lfqueue_push(que, &lp2));
    ZuAssertTrue(zt, 0 == lfqueue_push(que, &lp3));
    ZuAssertTrue(zt, 2 == lfqueue_size(que));

    ZuAssertTrue(zt, 0 == lfqueue_pop(que, &lpout));
    ZuAssertPtrEquals(zt, lp2, lpout);

    ZuAssertTrue(zt, 0 == lfqueue_pop(que, &lpout));
    ZuAssertPtrEquals(zt, lp3, lpout);

    ZuAssertTrue(zt, 0 != lfqueue_pop(que, &lpout));

    lfqueue_release(que);

    free(addr);
}


void Test_lfqueue3(ZuTest* zt)
{
#define TEST_QUEUE_SIZE_C 4
    typedef struct 
    {
        void* handler;
        void* param;
    }test_lfpush_data_t;

    lfqueue_t*  que;
    void*       addr;
    int64_t     memsize;
    int32_t     eltsize = sizeof(test_lfpush_data_t);

    memsize = lfqueue_memory_size(TEST_QUEUE_SIZE_C, eltsize);
    addr = calloc(1, (size_t)memsize);

    // create a queue by using our memory
    que = lfqueue_create2(TEST_QUEUE_SIZE_C, eltsize, addr, memsize);

    ZuAssertTrue(zt, lfqueue_empty(que));

    for (int i = 1; i <= 3; ++i)
    {
        test_lfpush_data_t ldata = { (void*)i, (void*)i };

        // push element into queue, queue internally would do a copy of ldata
        ZuAssertTrue(zt, 0 == lfqueue_push(que, &ldata));
        ZuAssertTrue(zt, i == lfqueue_size(que));
    }

    for (int i = 1; i <= 3; ++i)
    {
        test_lfpush_data_t ldata;

        // pop element from queue,
        // !attention, the sizeof of ldata must be >= eltsize (which used to create the queue)
        ZuAssertTrue(zt, 0 == lfqueue_pop(que, &ldata));
        ZuAssertPtrEquals(zt, (void*)i, ldata.handler);
        ZuAssertPtrEquals(zt, (void*)i, ldata.param);
    }

    test_lfpush_data_t ldata;
    ZuAssertTrue(zt, 0 != lfqueue_pop(que, &ldata));
    ZuAssertTrue(zt, lfqueue_empty(que));

    // push & pop again
    ldata.handler = (void*)10;
    ldata.param = (void*)11;
    ZuAssertTrue(zt, 0 == lfqueue_push(que, &ldata));
    ldata.handler = 0;
    ldata.param = 0;
    ZuAssertTrue(zt, 0 == lfqueue_pop(que, &ldata));
    ZuAssertPtrEquals(zt, (void*)10, ldata.handler);
    ZuAssertPtrEquals(zt, (void*)11, ldata.param);

    lfqueue_release(que);

    free(addr);
}
