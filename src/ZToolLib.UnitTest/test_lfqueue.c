#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_malloc.h>
#include <ZToolLib/lockfreequeue.h>


typedef union {
    void* ptr;
#if defined(_WIN64) || defined(__x86_64__)
    int64_t iv;
#else
    int32_t iv;
#endif
}lfque_union_type_t;


void Test_lfqueu_int(ZuTest* zt)
{
    lfqueue_t* que32;
    int i1, i2;
    int o1, o2;

    que32 = lfqueue_create(4, sizeof(int32_t));
    ZuAssertTrue(zt, lfqueue_empty(que32));

    // once
    i1 = 1;
    lfqueue_push(que32, &i1);
    ZuAssertIntEquals(zt, 1, lfqueue_size(que32));
    lfqueue_pop(que32, &o1);
    ZuAssertIntEquals(zt, 1, o1);

    // again
    i1 = 2;
    lfqueue_push(que32, &i1);
    ZuAssertIntEquals(zt, 1, lfqueue_size(que32));
    i2 = 3;
    lfqueue_push(que32, &i2);
    ZuAssertIntEquals(zt, 2, lfqueue_size(que32));

    lfqueue_pop(que32, &o1);
    lfqueue_pop(que32, &o2);
    ZuAssertIntEquals(zt, 2, o1);
    ZuAssertIntEquals(zt, 3, o2);

    lfqueue_release(que32);
    que32 = NULL;
}

void Test_lfqueu_int64(ZuTest* zt)
{
    lfqueue_t* que64;
    int64_t i1, i2;
    int64_t o1, o2;

    que64 = lfqueue_create(4, sizeof(int64_t));
    ZuAssertTrue(zt, lfqueue_empty(que64));

    // once
    i1 = 1;
    lfqueue_push(que64, &i1);
    ZuAssertIntEquals(zt, 1, lfqueue_size(que64));
    lfqueue_pop(que64, &o1);
    ZuAssertIntEquals(zt, 1, (int)o1);

    // again
    i1 = 2;
    lfqueue_push(que64, &i1);
    ZuAssertIntEquals(zt, 1, lfqueue_size(que64));
    i2 = 3;
    lfqueue_push(que64, &i2);
    ZuAssertIntEquals(zt, 2, lfqueue_size(que64));

    lfqueue_pop(que64, &o1);
    lfqueue_pop(que64, &o2);
    ZuAssertIntEquals(zt, 2, (int)o1);
    ZuAssertIntEquals(zt, 3, (int)o2);

    lfqueue_release(que64);
}

void Test_lfqueue_ptr(ZuTest* zt)
{
#define TEST_QUEUE_SIZE_B 4
    lfqueue_t*  que;
    void*       mem;
    int64_t     memsize;
    int32_t     eltsize = sizeof(void*);
    char*       lpout;

    memsize = lfqueue_memory_size(TEST_QUEUE_SIZE_B, eltsize);
    mem = calloc(1, (size_t)memsize);

    // create a queue by using our memory
    que = lfqueue_create_at_mem(TEST_QUEUE_SIZE_B, eltsize, mem);

    ZuAssertTrue(zt, lfqueue_empty(que));

    char* lp1 = ztl_strdup("hello");
    char* lp2 = ztl_strdup("world");
    char* lp3 = ztl_strdup("ztoollib");

    // push elements into queue (we must pass reference address of each element)
    ZuAssertIntEquals(zt, 0, lfqueue_push(que, &lp1));
    ZuAssertIntEquals(zt, 1, lfqueue_size(que));

    ZuAssertTrue(zt, 0 == lfqueue_pop(que, (void*)&lpout));
    ZuAssertPtrEquals(zt, lp1, lpout);

    ZuAssertIntEquals(zt, 0, lfqueue_push(que, &lp2));
    ZuAssertIntEquals(zt, 0, lfqueue_push(que, &lp3));
    ZuAssertIntEquals(zt, 2, lfqueue_size(que));

    ZuAssertIntEquals(zt, 0, lfqueue_pop(que, (void*)&lpout));
    ZuAssertPtrEquals(zt, lp2, lpout);

    ZuAssertIntEquals(zt, 0, lfqueue_pop(que, (void*)&lpout));
    ZuAssertPtrEquals(zt, lp3, lpout);

    ZuAssertTrue(zt, 0 != lfqueue_pop(que, (void*)&lpout));

    lfqueue_release(que);

    free(mem);
}

void Test_lfqueue_mem0(ZuTest* zt)
{
#define TEST_QUEUE_SIZE_C 4
    typedef struct 
    {
        void* pdata;
        int64_t idata;
    }test_lfpush_data_t;

    lfqueue_t*  que;
    void*       mem;
    int64_t     memsize;
    int32_t     eltsize = sizeof(void*);
    test_lfpush_data_t *lpdata, *lpdata2;

    memsize = lfqueue_memory_size(TEST_QUEUE_SIZE_C, eltsize);
    mem = calloc(1, (size_t)memsize);

    // create a queue by using our memory
    que = lfqueue_create_at_mem(TEST_QUEUE_SIZE_C, eltsize, mem);

    ZuAssertTrue(zt, lfqueue_empty(que));

    for (uint32_t i = 1; i <= 3; ++i)
    {
        lpdata = (test_lfpush_data_t*)malloc(sizeof(test_lfpush_data_t));
        lfque_union_type_t d;
        d.iv = i;
        lpdata->pdata = d.ptr;
        lpdata->idata = i * 10;

        ZuAssertIntEquals(zt, 0, lfqueue_push(que, &lpdata));
        ZuAssertIntEquals(zt, i, lfqueue_size(que));
    }

    for (int i = 1; i <= 3; ++i)
    {
        lpdata = NULL;
        lfque_union_type_t d;
        d.iv = i;
        ZuAssertIntEquals(zt, 0, lfqueue_pop(que, (void**)&lpdata));
        ZuAssertPtrEquals(zt, d.ptr, lpdata->pdata);
        ZuAssertIntEquals(zt, i*10, (int)lpdata->idata);
        free(lpdata);
    }

    ZuAssertTrue(zt, 0 != lfqueue_pop(que, (void*)&lpdata));
    ZuAssertTrue(zt, lfqueue_empty(que));

    // push & pop again
    lpdata = (test_lfpush_data_t*)malloc(sizeof(test_lfpush_data_t));
    lpdata->pdata = (void*)0xFFAA;
    lpdata->idata = 1234;
    ZuAssertIntEquals(zt, 0, lfqueue_push(que, &lpdata));
    ZuAssertIntEquals(zt, 1, lfqueue_size(que));

    lfqueue_pop(que, (void*)&lpdata2);
    ZuAssertPtrEquals(zt, lpdata, lpdata2);
    free(lpdata2);

    lfqueue_release(que);

    free(mem);
}

void Test_lfqueue_mem1(ZuTest* zt)
{
#define TEST_QUEUE_SIZE_C 4
    typedef struct
    {
        void* pdata;
        int64_t idata;
    }test_lfpush_data_t;

    lfqueue_t*  que;
    int32_t     eltsize = sizeof(test_lfpush_data_t);
    test_lfpush_data_t ldata, ldata2;

    que = lfqueue_create(TEST_QUEUE_SIZE_C, eltsize);

    ZuAssertTrue(zt, lfqueue_empty(que));

    for (uint32_t i = 1; i <= 3; ++i)
    {
        lfque_union_type_t d;
        d.iv = i;
        ldata.pdata = d.ptr;
        ldata.idata = i * 10;

        // push element into queue, queue internally would do a copy of ldata
        ZuAssertIntEquals(zt, 0, lfqueue_push(que, &ldata));
        ZuAssertIntEquals(zt, i, lfqueue_size(que));
    }

    for (int i = 1; i <= 3; ++i)
    {
        ZuAssertIntEquals(zt, 0, lfqueue_pop(que, &ldata));
        lfque_union_type_t d;
        d.iv = i;
        ZuAssertPtrEquals(zt, d.ptr, ldata.pdata);
        ZuAssertIntEquals(zt, i * 10, (int)ldata.idata);
    }

    ZuAssertTrue(zt, 0 != lfqueue_pop(que, &ldata2));
    ZuAssertTrue(zt, lfqueue_empty(que));

    // push & pop again
    ldata.pdata = (void*)0xFFAA;
    ldata.idata = 1234;
    ZuAssertIntEquals(zt, 0, lfqueue_push(que, &ldata));
    ZuAssertIntEquals(zt, 1, lfqueue_size(que));

    lfqueue_pop(que, &ldata2);
    ZuAssertPtrEquals(zt, ldata.pdata, ldata2.pdata);
    ZuAssertIntEquals(zt, (int)ldata.idata, (int)ldata2.idata);

    lfqueue_release(que);
}
