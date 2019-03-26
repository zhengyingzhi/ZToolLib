#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_vector.h>

void Test_ztl_vector1(ZuTest* zt)
{
    ztl_vector_t* lArray;
    lArray = ztl_vector_create(3, sizeof(uint32_t));

    uint32_t    lValue;

    // push n value
    lValue = 1;
    lArray->push_int(lArray, lValue);
    ZuAssertTrue(zt, 1 == lArray->nelts);

    lValue = 2;
    lArray->push_int(lArray, lValue);
    ZuAssertTrue(zt, 2 == lArray->nelts);

    // access elements
    int* pv = lArray->elts;
    lValue = pv[0];
    ZuAssertTrue(zt, 1 == lValue);
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lArray->reserve(lArray, 5);
    pv = lArray->elts;
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lArray->clear(lArray);
    ZuAssertTrue(zt, 0 == lArray->nelts);

    int count = 10;
    for (int i = 0; i < 10; ++i)
    {
        lArray->push_int(lArray, i);
    }
    ZuAssertTrue(zt, count == lArray->nelts);

    pv = lArray->elts;
    for (int i = 0; i < 10; ++i)
    {
        ZuAssertTrue(zt, i == pv[i]);
    }

    ztl_array_release(lArray);
}


void Test_ztl_vector2(ZuTest* zt)
{
    ztl_vector_t lArray;
    ztl_vector_init(&lArray, 3, sizeof(uint64_t));

    uint32_t    lValue;

    // push n value
    lValue = 1;
    lArray.push_int(&lArray, lValue);
    ZuAssertTrue(zt, 1 == lArray.nelts);

    lValue = 2;
    lArray.push_int(&lArray, lValue);
    ZuAssertTrue(zt, 2 == lArray.nelts);

    // access elements
    int* pv = lArray.elts;
    lValue = pv[0];
    ZuAssertTrue(zt, 1 == lValue);
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lArray.reserve(&lArray, 5);
    pv = lArray.elts;
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lArray.clear(&lArray);
    ZuAssertTrue(zt, 0 == lArray.nelts);

    int count = 10;
    for (int i = 0; i < 10; ++i)
    {
        lArray.push_int(&lArray, i);
    }
    ZuAssertTrue(zt, count == lArray.nelts);

    pv = lArray.elts;
    for (int i = 0; i < 10; ++i)
    {
        ZuAssertTrue(zt, i == pv[i]);
    }

    ztl_array_release(&lArray);
}

typedef struct {
    int iv;
    char name[8];
}vec_elem_t;

void Test_ztl_vector3(ZuTest* zt)
{
    ztl_vector_t lArray;
    ztl_vector_init(&lArray, 3, sizeof(vec_elem_t));

    vec_elem_t lValue = { 0 };

    // push n value
    lValue.iv = 1;
    sprintf(lValue.name, "s%02d", lValue.iv);
    lArray.push_x(&lArray, &lValue);
    ZuAssertTrue(zt, 1 == lArray.nelts);

    lValue.iv = 2;
    sprintf(lValue.name, "s%02d", lValue.iv);
    lArray.push_x(&lArray, &lValue);
    ZuAssertTrue(zt, 2 == lArray.nelts);

    // access elements
    vec_elem_t* pv = lArray.elts;
    lValue = pv[0];
    ZuAssertTrue(zt, 1 == lValue.iv);
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue.iv);

    lArray.reserve(&lArray, 5);
    pv = lArray.elts;
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue.iv);

    lArray.clear(&lArray);
    ZuAssertTrue(zt, 0 == lArray.nelts);

    int count = 10;
    for (int i = 0; i < 10; ++i)
    {
        lValue.iv = i;
        sprintf(lValue.name, "s%02d", lValue.iv);
        lArray.push_x(&lArray, &lValue);
    }
    ZuAssertTrue(zt, count == lArray.nelts);

    pv = lArray.elts;
    for (int i = 0; i < 10; ++i)
    {
        ZuAssertTrue(zt, i == pv[i].iv);
    }

    ztl_array_release(&lArray);
}
