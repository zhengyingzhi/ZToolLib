#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/lockfreequeue.h>


typedef union {
    void* ptr;
#if defined(_WIN64) || defined(__x86_64__)
    int64_t iv;
#else
    int32_t iv;
#endif
}lfque_union_type_t;

void Test_lfqueue0(ZuTest* zt)
{
    lfqueue_t*  que;

    que = lfqueue_create(4, sizeof(void*));
    ZuAssertTrue(zt, lfqueue_empty(que));

    // push and pop an integer pointer
    lfque_union_type_t ud;
    ud.iv = 1;
    void* p1 = ud.ptr;
    lfqueue_push(que, p1);

    void* p2 = NULL;
    lfqueue_pop(que, &p2);
    ZuAssertTrue(zt, p1 == p2);

    int* p3 = malloc(sizeof(int));
    *p3 = 3;
    lfqueue_push(que, p3);
    lfqueue_pop(que, &p2);
    ZuAssertTrue(zt, p2 == p3);

    lfqueue_release(que);
}

void Test_lfqueue(ZuTest* zt)
{
#define TEST_QUEUE_SIZE_A 4
    lfqueue_t*  que;
    int32_t     rv;

    que = lfqueue_create(TEST_QUEUE_SIZE_A, sizeof(void*));

    ZuAssertTrue(zt, lfqueue_empty(que));

    int data[] = { 1, 2, 3, 4, 5 };
    for (int i = 0; i < TEST_QUEUE_SIZE_A - 1; ++i)
    {
        lfque_union_type_t ud;
        ud.iv = data[i];
        ZuAssertTrue(zt, 0 == lfqueue_push(que, ud.ptr));
    }

    ZuAssertTrue(zt, 3 == lfqueue_size(que));

    int64_t outdata = 0;
    rv = lfqueue_pop(que, (void**)&outdata);
    ZuAssertTrue(zt, rv == 0);
    ZuAssertTrue(zt, outdata == 1);

    lfqueue_pop(que, (void**)&outdata);
    ZuAssertTrue(zt, rv == 0);
    ZuAssertTrue(zt, outdata == 2);

    lfqueue_pop(que, (void**)&outdata);
    ZuAssertTrue(zt, rv == 0);
    ZuAssertTrue(zt, outdata == 3);

    rv = lfqueue_pop(que, (void**)&outdata);
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
    ZuAssertTrue(zt, 0 == lfqueue_push(que, lp1));
    ZuAssertTrue(zt, 1 == lfqueue_size(que));

    void* lpout;
    ZuAssertTrue(zt, 0 == lfqueue_pop(que, &lpout));
    ZuAssertPtrEquals(zt, lp1, lpout);


    ZuAssertTrue(zt, 0 == lfqueue_push(que, lp2));
    ZuAssertTrue(zt, 0 == lfqueue_push(que, lp3));
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

    for (uint32_t i = 1; i <= 3; ++i)
    {
        lfque_union_type_t ud;
        ud.iv = i;
        test_lfpush_data_t ldata = { ud.ptr, ud.ptr };

        // push element into queue, queue internally would do a copy of ldata
        ZuAssertTrue(zt, 0 == lfqueue_push_value(que, &ldata));
        ZuAssertTrue(zt, i == lfqueue_size(que));
    }

    for (int i = 1; i <= 3; ++i)
    {
        test_lfpush_data_t ldata;
        lfque_union_type_t ud;
        ud.iv = i;

        // pop element from queue,
        // !attention, the sizeof of ldata must be >= eltsize (which used to create the queue)
        ZuAssertTrue(zt, 0 == lfqueue_pop_value(que, &ldata));
        ZuAssertPtrEquals(zt, ud.ptr, ldata.handler);
        ZuAssertPtrEquals(zt, ud.ptr, ldata.param);
    }

    test_lfpush_data_t ldata;
    ZuAssertTrue(zt, 0 != lfqueue_pop_value(que, &ldata));
    ZuAssertTrue(zt, lfqueue_empty(que));

    // push & pop again
    lfque_union_type_t ud0, ud1;
    ud0.iv = 10;
    ldata.handler = ud0.ptr;
    ud1.iv = 11;
    ldata.param = ud1.ptr;
    ZuAssertTrue(zt, 0 == lfqueue_push_value(que, &ldata));
    ldata.handler = 0;
    ldata.param = 0;
    ZuAssertTrue(zt, 0 == lfqueue_pop_value(que, &ldata));
    ZuAssertPtrEquals(zt, ud0.ptr, ldata.handler);
    ZuAssertPtrEquals(zt, ud1.ptr, ldata.param);

    lfqueue_release(que);

    free(addr);
}
