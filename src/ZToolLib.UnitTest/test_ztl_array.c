#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_array.h>

void Test_ztl_array(ZuTest* zt)
{
    ztl_array_t* lArray;
    lArray = ztl_array_create(NULL, 2, sizeof(uint32_t));

    uint32_t    lValue;
    uint32_t*   lpValue;

    // push n value
    lValue = 1;
    ztl_array_push_back(lArray, &lValue);
    ZuAssertTrue(zt, 1 == ztl_array_size(lArray));

    lpValue = ztl_array_push(lArray);
    *lpValue = 2;

    lValue = 3;
    ztl_array_push_back(lArray, &lValue);
    ZuAssertTrue(zt, 3 == ztl_array_size(lArray));

    // access elements
    lpValue = (uint32_t*)ztl_array_at(lArray, 0);
    ZuAssertTrue(zt, 1 == *lpValue);
    lpValue = (uint32_t*)ztl_array_at(lArray, 1);
    ZuAssertTrue(zt, 2 == *lpValue);
    lpValue = (uint32_t*)ztl_array_at(lArray, 2);
    ZuAssertTrue(zt, 3 == *lpValue);

    // pop one
    lpValue = (uint32_t*)ztl_array_pop_back(lArray);
    ZuAssertTrue(zt, 3 == *lpValue);
    ZuAssertTrue(zt, 2 == ztl_array_size(lArray));

    // reserve
    ztl_array_reserve(lArray, 8);
    lpValue = (uint32_t*)ztl_array_at(lArray, 0);
    ZuAssertTrue(zt, 1 == *lpValue);

    // push again
    lpValue = ztl_array_push(lArray);
    *lpValue = 4;
    ZuAssertTrue(zt, 3 == ztl_array_size(lArray));

    lpValue = (uint32_t*)ztl_array_at(lArray, 2);
    ZuAssertTrue(zt, 4 == *lpValue);

    ztl_array_release(lArray);
}

static int test_array_cmp(void* expect, void* actual)
{
    int* pe = (int*)expect;
    int* pa = (int*)actual;
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

void Test_ztl_array2(ZuTest* zt)
{
    ztl_pool_t*  pool;
    pool = ztl_create_pool(4096);

    ztl_array_t* lArray;
    lArray = ztl_array_create(pool, 1024, sizeof(uint64_t));

    uint64_t lCount = 65536;
    for (uint64_t x = 1; x <= lCount; ++x)
    {
        uint64_t* lpValue = ztl_array_push(lArray);
        ZuAssertTrue(zt, lpValue != NULL);

        *lpValue = x;
        ZuAssertTrue(zt, x == ztl_array_size(lArray));
    }

    uint64_t* lpData = (uint64_t*)ztl_array_data(lArray);
    ZuAssertTrue(zt, NULL != lpData);
    ZuAssertTrue(zt, 1 == *lpData);
    ZuAssertTrue(zt, 2 == *(lpData + 1));

    for (uint64_t x = 1; x <= lCount; ++x)
    {
        uint64_t* lpValue = (uint64_t*)ztl_array_at(lArray, x - 1);
        ZuAssertTrue(zt, x == *lpValue);

        if (x % 33 == 0)
            ZuAssertTrue(zt, ztl_array_find(lArray, &x, test_array_cmp) != NULL);
    }

    uint64_t* lpValue = ztl_array_pop_back(lArray);
    ZuAssertTrue(zt, (lCount) == *lpValue);
    ZuAssertTrue(zt, (lCount - 1) == ztl_array_size(lArray));

    uint64_t lNewVal = 11;
    ztl_array_push_back(lArray, &lNewVal);
    uint64_t* lpLast = (uint64_t*)ztl_array_tail(lArray);
    ZuAssertTrue(zt, 11 == *lpLast);


    ztl_array_release(lArray);
}
