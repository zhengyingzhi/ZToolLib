#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_vector.h>

void Test_ztl_vector1(ZuTest* zt)
{
    cvector_t* lvec;
    lvec = cvector_create(3, sizeof(uint32_t));

    uint32_t    lValue;

    // push n value
    lValue = 1;
    lvec->push_int(lvec, lValue);
    ZuAssertIntEquals(zt, 1, lvec->nelts);

    lValue = 2;
    lvec->push_int(lvec, lValue);
    ZuAssertIntEquals(zt, 2, lvec->nelts);

    // access elements
    int* pv = (int*)lvec->elts;
    lValue = pv[0];
    ZuAssertIntEquals(zt, 1, lValue);
    lValue = pv[1];
    ZuAssertIntEquals(zt, 2, lValue);

    lvec->reserve(lvec, 5);
    pv = (int*)lvec->elts;
    lValue = pv[1];
    ZuAssertIntEquals(zt, 2, lValue);

    lvec->clear(lvec);
    ZuAssertIntEquals(zt, 0, lvec->nelts);

    int count = 10;
    for (int i = 0; i < count; ++i)
    {
        lvec->push_int(lvec, i);
    }
    ZuAssertIntEquals(zt, (uint32_t)count, lvec->nelts);

    int elems[4] = { 0 };
    elems[0] = 10;
    elems[1] = 11;
    elems[2] = 12;
    elems[3] = 13;
    lvec->reserve(lvec, 32);
    cvector_push_batch(lvec, elems, 4, int);
    count += 4;

    pv = (int*)lvec->elts;
    for (int i = 0; i < count; ++i)
    {
        ZuAssertIntEquals(zt, i, pv[i]);
    }

    int tmp;
    lvec->remove(lvec, 11);
    tmp = 12;
    ZuAssertIntEquals(zt, 11, lvec->index(lvec, &tmp));
    tmp = 6;
    ZuAssertIntEquals(zt, 6, lvec->index(lvec, &tmp));

    cvector_release(lvec);
}


void Test_ztl_vector2(ZuTest* zt)
{
    cvector_t lvec;
    cvector_init(&lvec, 3, sizeof(uint32_t));

    uint32_t    lValue;

    // push n value
    lValue = 1;
    lvec.push_int(&lvec, lValue);
    ZuAssertTrue(zt, 1 == lvec.nelts);

    lValue = 2;
    lvec.push_int  (&lvec, lValue);
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
    for (int i = 0; i < count; ++i)
    {
        lvec.push_int(&lvec, i);
    }
    ZuAssertTrue(zt, (uint32_t)count == lvec.nelts);

    pv = (int*)lvec.elts;
    for (int i = 0; i < count; ++i)
    {
        ZuAssertTrue(zt, i == pv[i]);
    }

    cvector_release(&lvec);
}

typedef struct {
    int iv;
    char name[8];
}vec_elem_t;

void Test_ztl_vector3(ZuTest* zt)
{
    cvector_t lvec;
    cvector_init(&lvec, 3, sizeof(vec_elem_t));

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
    for (int i = 0; i < count; ++i)
    {
        lValue.iv = i;
        sprintf(lValue.name, "s%02d", lValue.iv);
        lvec.push_x(&lvec, &lValue);
    }
    ZuAssertTrue(zt, (uint32_t)count == lvec.nelts);

    pv = (vec_elem_t*)lvec.elts;
    for (int i = 0; i < count; ++i)
    {
        ZuAssertTrue(zt, i == pv[i].iv);
    }

    cvector_release(&lvec);
}

//test of ztl_push_ptr
void Test_ztl_vector4(ZuTest* zt)
{
    cvector_t lvec;
    cvector_init(&lvec, 3, sizeof(void*));

    int*    lValue;
    int     arr[] = { 1,2,3 };

    // push n value
    lvec.push_ptr(&lvec, &arr[0]);
    ZuAssertTrue(zt, 1 == lvec.nelts);

    lvec.push_ptr(&lvec, &arr[1]);
    ZuAssertTrue(zt, 2 == lvec.nelts);
    // access elements

    uint32_t** pv;
    pv = (uint32_t**)lvec.elts;
    lValue =pv[0];
    ZuAssertTrue(zt, 1 == *lValue);

    lValue = pv[1];
    ZuAssertTrue(zt, 2 == *lValue);

    lvec.reserve(&lvec, 5);
    pv = (uint32_t**)lvec.elts;
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == *lValue);

    lvec.clear(&lvec);
    ZuAssertTrue(zt, 0 == lvec.nelts);

    int count = 10;
    uint32_t num = 4294967290;
    for (int i = 0; i < count; ++i)
    {
        int32_t* lValue = (int*)malloc(sizeof(int32_t));
        *lValue = num++;
        lvec.push_ptr(&lvec, lValue);
    }
    ZuAssertTrue(zt, (uint32_t)count == lvec.nelts);

    num = 4294967290;
    int** pv2 = (int**)lvec.elts;
    for (int i = 0; i < count; ++i)
    {
        lValue = pv2[i];
        ZuAssertIntEquals(zt, num++, *lValue);
    }

    cvector_release(&lvec);
}

//test of ztl_push_int64
void Test_ztl_vector5(ZuTest* zt)
{
    cvector_t* lvec;
    lvec = cvector_create(3, sizeof(uint64_t));

    uint64_t    lValue;

    // push n value
    lValue = 1;
    lvec->push_int64(lvec, lValue);
    ZuAssertTrue(zt, 1 == lvec->nelts);

    lValue = 2;
    lvec->push_int64(lvec, lValue);
    ZuAssertTrue(zt, 2 == lvec->nelts);

    // access elements
    int64_t* pv;
    pv = (int64_t*)lvec->elts;
    lValue = pv[0];
    ZuAssertTrue(zt, 1 == lValue);
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lvec->reserve(lvec, 5);
    pv = (int64_t*)lvec->elts;
    lValue = pv[1];
    ZuAssertTrue(zt, 2 == lValue);

    lvec->clear(lvec);
    ZuAssertTrue(zt, 0 == lvec->nelts);

    int count = 10;
    for (int i = 0; i < count; ++i)
    {
        lvec->push_int64(lvec, (int64_t)i);
    }
    ZuAssertTrue(zt, (int64_t)count == lvec->nelts);

    pv = (int64_t*)lvec->elts;
    for (int i = 0; i < count; ++i)
    {
        ZuAssertTrue(zt, (int64_t)i == pv[i]);
    }

    cvector_release(lvec);
}

void Test_ztl_vector6(ZuTest* zt)
{
    cvector_t* lvec;
    lvec = cvector_create(3, sizeof(int));

    int    lValue;

    // push n value
    lValue = 1;
    lvec->push_x(lvec, &lValue);
    ZuAssertTrue(zt, 1 == lvec->nelts);

    lValue = 2;
    lvec->push_x(lvec, &lValue);
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
    for (int i = 0; i < count; ++i)
    {
        lvec->push_x(lvec, &i);
    }
    ZuAssertTrue(zt, (uint32_t)count == lvec->nelts);

    pv = (int*)lvec->elts;
    for (int i = 0; i < count; ++i)
    {
        ZuAssertTrue(zt, i == pv[i]);
    }

    cvector_release(lvec);
}