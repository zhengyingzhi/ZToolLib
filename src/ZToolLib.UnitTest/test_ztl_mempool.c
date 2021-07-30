#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_mempool.h>

void Test_ztl_mempool(ZuTest* zt)
{
    typedef struct  
    {
        char    name[12];
        int     age;
        float   score;
    }mptest_t;

    int  init_count = 2;
    bool auto_expand = true;
    ztl_mempool_t* mp;
    mp = ztl_mp_create(sizeof(mptest_t), init_count, auto_expand);

    // the entity size is aligned to 8 bytes internally
    ZuAssertTrue(zt, sizeof(mptest_t) <= ztl_mp_entity_size(mp));

    mptest_t* lptest1 = (mptest_t*)ztl_mp_alloc(mp);
    ZuAssertTrue(zt, NULL != lptest1);
    memset(lptest1, 0, sizeof(mptest_t));
    strcpy(lptest1->name, "111");
    lptest1->age = 1;
    lptest1->score = 6.5;
    ZuAssertTrue(zt, 1 == ztl_mp_exposed(mp));

    mptest_t* lptest2 = (mptest_t*)ztl_mp_alloc(mp);
    ZuAssertTrue(zt, NULL != lptest2);
    memset(lptest1, 0, sizeof(mptest_t));
    strcpy(lptest2->name, "222");
    lptest2->age = 2;
    lptest2->score = 7.5;

    mptest_t* lptest3 = (mptest_t*)ztl_mp_alloc(mp);
    ZuAssertTrue(zt, NULL != lptest3);
    memset(lptest3, 0, sizeof(mptest_t));
    strcpy(lptest3->name, "333");
    lptest3->age = 3;
    lptest3->score = 8.5;
    ZuAssertTrue(zt, 3 == ztl_mp_exposed(mp));

    // free
    ztl_mp_free(mp, lptest2);
    ZuAssertTrue(zt, 2 == ztl_mp_exposed(mp));
    ztl_mp_free(mp, lptest3);
    ZuAssertTrue(zt, 1 == ztl_mp_exposed(mp));

    // alloc again
    mptest_t* lptest4 = (mptest_t*)ztl_mp_alloc(mp);
    ZuAssertTrue(zt, NULL != lptest4);
    ZuAssertTrue(zt, 2 == ztl_mp_exposed(mp));

    ztl_mp_release(mp);
}
