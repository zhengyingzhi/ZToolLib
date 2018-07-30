#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_dict.h>

void Test_ztl_dict(ZuTest* zt)
{
    dict* dct;
    dct = dictCreate(NULL, NULL);
    dictRelease(dct);
}
