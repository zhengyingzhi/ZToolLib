#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_utils.h>
#include <ZToolLib/ztl_dict.h>

static uint64_t _intDictHashFunc(const void *key)
{
    return dictGenHashFunction(key, sizeof(uint32_t));
}

static int _intCompareFunc(void* privdata, const void* key1,
    const void* key2)
{
    uint64_t k1 = (uint64_t)key1;
    uint64_t k2 = (uint64_t)key2;
    return k1 == k2;
}

static dictType _intDictType = {
    _intDictHashFunc,
    NULL,
    NULL,
    _intCompareFunc,
    NULL,
    NULL
};

void Test_ztl_dict(ZuTest* zt)
{
    uint32_t seed = ztl_randseed();

    dict* dct;
    dct = dictCreate(&_intDictType, NULL);
    ZuAssertTrue(zt, 0 == dictSize(dct));

    int count = 10;
    int arr[32] = { 0 };
    for (int i = 0; i < count; ++i)
    {
        uint32_t num = ztl_rand(&seed);
        arr[i] = num;

        dictAdd(dct, &arr[i], (void*)arr[i]);
        ZuAssertTrue(zt, (i + 1) == dictSize(dct));
    }

    for (int i = 0; i < count; ++i)
    {
        dictEntry* de;
        de = dictFind(dct, &arr[i]);
        ZuAssertTrue(zt, (void*)arr[i] == de->v.val);
    }

    dictRelease(dct);
}
