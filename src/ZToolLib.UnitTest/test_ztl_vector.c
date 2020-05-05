#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_vector.h>

void Test_ztl_vector1(ZuTest* zt)
{
    ztl_vector_t* lvec;
    lvec = ztl_vector_create(3, sizeof(uint32_t));

    uint32_t    lValue;

    // push n value
    lValue = 1;
    lvec->push_int(lvec, lValue);
    ZuAssertTrue(zt, 1 == lvec->nelts);

    lValue = 2;
    lvec->push_int(lvec, lValue);
    ZuAssertTrue(zt, 2 == lvec->nelts);

    // access elements
    int* pv = (int*)lvec->elts;
    lValue = pv[0];
    ZuAssertTrue(zt, 1 == lValue);
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lvec->reserve(lvec, 5);
    pv = (int*)lvec->elts;
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lvec->clear(lvec);
    ZuAssertTrue(zt, 0 == lvec->nelts);

    int count = 10;
    for (int i = 0; i < 10; ++i)
    {
        lvec->push_int(lvec, i);
    }
    ZuAssertTrue(zt, count == lvec->nelts);

    pv = (int*)lvec->elts;
    for (int i = 0; i < 10; ++i)
    {
        ZuAssertTrue(zt, i == pv[i]);
    }

    ztl_vector_release(lvec);
}


void Test_ztl_vector2(ZuTest* zt)
{
    ztl_vector_t lvec;
    ztl_vector_init(&lvec, 3, sizeof(uint64_t));

    uint32_t    lValue;

    // push n value
    lValue = 1;
    lvec.push_int(&lvec, lValue);
    ZuAssertTrue(zt, 1 == lvec.nelts);

    lValue = 2;
    lvec.push_int(&lvec, lValue);
    ZuAssertTrue(zt, 2 == lvec.nelts);

    // access elements
    int* pv;
    pv = (int*)lvec.elts;
    lValue = pv[0];
    ZuAssertTrue(zt, 1 == lValue);
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lvec.reserve(&lvec, 5);
    pv = (int*)lvec.elts;
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lvec.clear(&lvec);
    ZuAssertTrue(zt, 0 == lvec.nelts);

    int count = 10;
    for (int i = 0; i < 10; ++i)
    {
        lvec.push_int(&lvec, i);
    }
    ZuAssertTrue(zt, count == lvec.nelts);

    pv = (int*)lvec.elts;
    for (int i = 0; i < 10; ++i)
    {
        ZuAssertTrue(zt, i == pv[i]);
    }

    ztl_vector_release(&lvec);
}

typedef struct {
    int iv;
    char name[8];
}vec_elem_t;

void Test_ztl_vector3(ZuTest* zt)
{
    ztl_vector_t lvec;
    ztl_vector_init(&lvec, 3, sizeof(vec_elem_t));

    vec_elem_t lValue = { 0 };

    // push n value
    lValue.iv = 1;
    sprintf(lValue.name, "s%02d", lValue.iv);
    lvec.push_x(&lvec, &lValue);
    ZuAssertTrue(zt, 1 == lvec.nelts);

    lValue.iv = 2;
    sprintf(lValue.name, "s%02d", lValue.iv);
    lvec.push_x(&lvec, &lValue);
    ZuAssertTrue(zt, 2 == lvec.nelts);

    // access elements
    vec_elem_t* pv;
    pv = (vec_elem_t*)lvec.elts;
    lValue = pv[0];
    ZuAssertTrue(zt, 1 == lValue.iv);
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue.iv);

    lvec.reserve(&lvec, 5);
    pv = (vec_elem_t*)lvec.elts;
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue.iv);

    lvec.clear(&lvec);
    ZuAssertTrue(zt, 0 == lvec.nelts);

    int count = 10;
    for (int i = 0; i < 10; ++i)
    {
        lValue.iv = i;
        sprintf(lValue.name, "s%02d", lValue.iv);
        lvec.push_x(&lvec, &lValue);
    }
    ZuAssertTrue(zt, count == lvec.nelts);

    pv = (vec_elem_t*)lvec.elts;
    for (int i = 0; i < 10; ++i)
    {
        ZuAssertTrue(zt, i == pv[i].iv);
    }

    ztl_vector_release(&lvec);
}
