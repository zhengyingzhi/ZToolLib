#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_array.h>

void Test_ztl_array_int32(ZuTest* zt)
{
    ztl_array_t* lArray;
    lArray = ztl_array_create(NULL, 2, sizeof(uint32_t));

    uint32_t    lValue;
    uint32_t*   lpValue;

    // push n value
    lValue = 1;
    ztl_array_push_back(lArray, &lValue);
    ZuAssertIntEquals(zt, 1, ztl_array_size(lArray));

    lpValue = ztl_array_push(lArray);
    *lpValue = 2;

    lValue = 3;
    ztl_array_push_back(lArray, &lValue);
    ZuAssertIntEquals(zt, 3, ztl_array_size(lArray));

    // access elements
    lpValue = (uint32_t*)ztl_array_at(lArray, 0);
    ZuAssertIntEquals(zt, 1, *lpValue);
    lpValue = (uint32_t*)ztl_array_at(lArray, 1);
    ZuAssertIntEquals(zt, 2, *lpValue);
    lpValue = (uint32_t*)ztl_array_at(lArray, 2);
    ZuAssertIntEquals(zt, 3, *lpValue);

    // pop one
    lpValue = (uint32_t*)ztl_array_pop_back(lArray);
    ZuAssertIntEquals(zt, 3, *lpValue);
    ZuAssertIntEquals(zt, 2, ztl_array_size(lArray));

    // reserve
    ztl_array_reserve(lArray, 8);
    lpValue = (uint32_t*)ztl_array_at(lArray, 0);
    ZuAssertIntEquals(zt, 1, *lpValue);

    // push again
    lpValue = ztl_array_push(lArray);
    *lpValue = 4;
    ZuAssertIntEquals(zt, 3, ztl_array_size(lArray));

    lpValue = (uint32_t*)ztl_array_at(lArray, 2);
    ZuAssertIntEquals(zt, 4, *lpValue);

    ztl_array_release(lArray);
}

static int test_array_cmp(void* expect, void* actual)
{
    uint64_t* pe = (uint64_t*)expect;
    uint64_t* pa = (uint64_t*)actual;
    if (*pe == *pa) {
        return 0;
    }
    else if (*pe < *pa) {
        return -1;
    }
    else {
        return 1;
    }
}

void Test_ztl_array_int64(ZuTest* zt)
{
    ztl_pool_t*  pool;
    pool = ztl_create_pool(4096);

    ztl_array_t* lArray;
    lArray = ztl_array_create(pool, 1024, sizeof(uint64_t));

    uint64_t lCount = 65536;
    for (uint64_t x = 1; x <= lCount; ++x)
    {
        uint64_t* lpValue = ztl_array_push(lArray);
        ZuAssertPtrNotNull(zt, lpValue);

        *lpValue = x;
        ZuAssertIntEquals(zt, (int)x, ztl_array_size(lArray));
    }

    uint64_t* lpData = (uint64_t*)ztl_array_data(lArray);
    ZuAssertPtrNotNull(zt, lpData);
    ZuAssertInt64Equals(zt, 1, *lpData);
    ZuAssertInt64Equals(zt, 2, *(lpData + 1));

    for (uint64_t x = 1; x <= lCount; ++x)
    {
        uint64_t* lpValue = (uint64_t*)ztl_array_at(lArray, x - 1);
        ZuAssertInt64Equals(zt, x, *lpValue);

        if (x % 33 == 0)
            ZuAssertTrue(zt, ztl_array_find(lArray, &x, test_array_cmp) != NULL);
    }

    uint64_t* lpValue = ztl_array_pop_back(lArray);
    ZuAssertInt64Equals(zt, (lCount), *lpValue);
    ZuAssertInt64Equals(zt, (lCount - 1), ztl_array_size(lArray));

    uint64_t lNewVal = 11;
    ztl_array_push_back(lArray, &lNewVal);
    uint64_t* lpLast = (uint64_t*)ztl_array_tail(lArray);
    ZuAssertInt64Equals(zt, 11, *lpLast);


    ztl_array_release(lArray);
}


void Test_ztl_array_ptr(ZuTest* zt)
{
    ztl_pool_t*  pool;
    pool = ztl_create_pool(4096);

    ztl_array_t* lArray;
    lArray = ztl_array_create(pool, 1024, sizeof(void*));

    int values[] = { 1, 2, 3 };
    int* p1 = malloc(sizeof(uint32_t));
    *p1 = values[0];
    int* p2 = malloc(sizeof(uint32_t));
    *p2 = values[1];
    int* p3 = malloc(sizeof(uint32_t));
    *p3 = values[2];

    /* ztl_array_push_back() pass in the elem's address
     * ztl_array_push(), return the dst address, here we put pointer into the address
     * ztl_array_at2() return the elem's value, here value is pointer
     */

    ztl_array_push_back(lArray, &p1);
    // ztl_array_push_back(lArray, &p2);
    void** lpdst = (void**)ztl_array_push(lArray);
    *lpdst = p2;
    ztl_array_push_back(lArray, &p3);

    int* ptr;
    ptr = ztl_array_at2(lArray, 0);
    ZuAssertPtrEquals(zt, p1, ptr);

    ptr = ztl_array_at2(lArray, 1);
    ZuAssertPtrEquals(zt, p2, ptr);

    ptr = ztl_array_at2(lArray, 2);
    ZuAssertPtrEquals(zt, p3, ptr);

    ztl_array_release(lArray);
}

typedef struct
{
    int valint;
    double valdouble;
}array_test_data_t;

void Test_ztl_array_more(ZuTest* zt)
{
    ztl_pool_t*  pool;
    pool = ztl_create_pool(4096);

    ztl_array_t* lArray;
    lArray = ztl_array_create(pool, 1024, sizeof(array_test_data_t));

    array_test_data_t data1 = { 1, 2.0 };
    array_test_data_t data2 = { 2, 3.0 };

    ztl_array_push_back(lArray, &data1);
    array_test_data_t* lpdst = ztl_array_push(lArray);
    *lpdst = data2;

    array_test_data_t* ptr;
    ptr = (array_test_data_t*)ztl_array_at(lArray, 0);
    ZuAssertIntEquals(zt, data1.valint, ptr->valint);
    ZuAssertDblEquals(zt, data1.valdouble, ptr->valdouble, 3);

    ptr = (array_test_data_t*)ztl_array_at(lArray, 1);
    ZuAssertIntEquals(zt, data2.valint, ptr->valint);
    ZuAssertDblEquals(zt, data2.valdouble, ptr->valdouble, 3);

    ztl_array_release(lArray);
}
