#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_crypt.h>
#include <ZToolLib/ztl_base64.h>
#include <ZToolLib/ztl_utils.h>

void Test_ztl_base64(ZuTest* zt)
{
    char key[16] = "";
    random_string(key, sizeof(key) - 1, false);

    char b64[64] = "";
    uint32_t bLen64 = sizeof(b64);
    ztl_base64_encode(key, (uint32_t)strlen(key), b64, &bLen64);

    char rawdata[64] = "";
    uint32_t rawlen = sizeof(rawdata);
    ztl_base64_decode(b64, bLen64, rawdata, &rawlen);

    ZuAssertTrue(zt, (uint32_t)strlen(key) == rawlen);
    ZuAssertTrue(zt, strncmp(key, rawdata, rawlen) == 0);

    // base64 encode a struct
    struct testStruct
    {
        char buf[12];
        int  age;
        double score;
        char male;
        short flag;
    };
    fprintf(stderr, "sizeof testStruect:%d\n", (int)sizeof(struct testStruct));
    struct testStruct ts = { 0 };
    strcpy(ts.buf, "helloworld");
    ts.age = 30;
    ts.score = 7.5;
    ts.male = 1;
    ts.flag = 2;
    memset(b64, 0, sizeof(b64));
    bLen64 = sizeof(b64);
    ztl_base64_encode((const char*)&ts, sizeof(ts), b64, &bLen64);

    char ts_decode[sizeof(struct testStruct) + 1] = "";
    rawlen = sizeof(ts_decode);
    // ztl_base64_decode(b64, bLen64, (char*)&ts2, &rawlen);    // error maybe callstack corrupt
    ztl_base64_decode(b64, bLen64, ts_decode, &rawlen);

    struct testStruct* ts2 = (struct testStruct*)ts_decode;
    ZuAssertStrEquals(zt, ts2->buf, ts.buf);
    ZuAssertTrue(zt, ts2->age == ts.age);
    ZuAssertTrue(zt, ts2->male == ts.male);
    ZuAssertTrue(zt, ts2->flag == ts.flag);
    ZuAssertTrue(zt, ts2->score == ts.score);
}

void Test_ztl_encrypt(ZuTest* zt)
{
    char key[16] = "qhH0BZvAaY3m1fS";
    //random_string(key, sizeof(key) - 1, false);

    char rawdata[64] = "hello world, 123";
    char encryptdata[64] = "";
    int  encryptlen = sizeof(encryptdata);
    ztl_aes_encrypt(key, rawdata, (int)strlen(rawdata), encryptdata, &encryptlen);
    ZuAssertTrue(zt, strlen(encryptdata) > 0);
    ZuAssertTrue(zt, encryptlen > 0);

    char rawdata2[64] = "";
    int  rawlen2 = sizeof(rawdata2);
    ztl_aes_decrypt(key, encryptdata, (int)strlen(encryptdata), rawdata2, &rawlen2);
    ZuAssertTrue(zt, 0 == strcmp(rawdata, rawdata2));
    ZuAssertTrue(zt, (int)strlen(rawdata) == rawlen2);
}