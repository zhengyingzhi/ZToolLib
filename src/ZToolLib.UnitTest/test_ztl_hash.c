#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_hash.h>
#include <ZToolLib/ztl_md5.h>


// static int str_cmp(const void *x, const void *y) {
//     return strcmp((char *)x, (char *)y);
// }

static uint64_t _hashpjw(const void* key) {
    return (uint64_t)ztl_hashpjw(key);
}


void Test_ztl_hash(ZuTest* zt)
{
    char data[64] = "hello c";
    uint32_t h32;
    uint64_t h64;

    h32 = ztl_murmur_hash2((unsigned char*)data, (uint32_t)strlen(data));
    ZuAssertTrue(zt, h32 != 0);

    h64 = ztl_murmur_hash2_64((unsigned char*)data, (uint32_t)strlen(data),
        (uint64_t)time(NULL));
    ZuAssertTrue(zt, h64 != 0);

    h64 = _hashpjw(data);
    ZuAssertTrue(zt, h64 != 0);
    h32 = ztl_hashdjb2(data);
    ZuAssertTrue(zt, h32 != 0);
}

void Test_ztl_md5(ZuTest* zt)
{
    unsigned char data1[32] = "hello c";
    unsigned char data2[32] = "hello cpp";
    unsigned char result1[32] = "";
    unsigned char result2[32] = "";
    ztl_md5_t ctx;
    ztl_md5_init(&ctx);
    ztl_md5_update(&ctx, data1, strlen((char*)data1));
    ztl_md5_final(result1, &ctx);

    ztl_md5_update(&ctx, data2, strlen((char*)data2));
    ztl_md5_final(result2, &ctx);

    ZuAssertTrue(zt, 0 != strcmp((char*)result1, (char*)result2));
}
