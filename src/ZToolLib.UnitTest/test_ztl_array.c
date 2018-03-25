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
    ztl_array_pop_back(lArray);
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
