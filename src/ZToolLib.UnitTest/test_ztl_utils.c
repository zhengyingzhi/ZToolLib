#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_utils.h>

void Test_ztl_util(ZuTest* zt)
{
    int lValue = 5;
    lValue = ztl_align(lValue, 8);
    ZuAssertIntEquals(zt, 8, lValue);

    lValue = 1024;
    lValue = ztl_align(lValue, 4096);
    ZuAssertIntEquals(zt, 4096, lValue);


    int a = 1, b = 2;
    ZuAssertIntEquals(zt, 1, ztl_min(a, b));
    ZuAssertIntEquals(zt, 2, ztl_max(a, b));
}

void Test_ztl_ll2string(ZuTest* zt)
{
    char lBuffer[128] = "";
    uint32_t lLength = 0;
    int64_t lValue;

    lValue = 0;
    lLength = ll2string(lBuffer, sizeof(lBuffer) - 1, lValue);
    ZuAssertStrEquals(zt, "0", lBuffer);
    ZuAssertIntEquals(zt, 1, lLength);

    lValue = 123;
    lLength = ll2string(lBuffer, sizeof(lBuffer) - 1, lValue);
    ZuAssertStrEquals(zt, "123", lBuffer);
    ZuAssertIntEquals(zt, 3, lLength);

    lValue = -110;
    lLength = ll2string(lBuffer, sizeof(lBuffer) - 1, lValue);
    ZuAssertStrEquals(zt, "-110", lBuffer);
    ZuAssertIntEquals(zt, 4, lLength);
}

void Test_ztl_atoin(ZuTest* zt)
{
    char lBuffer[128] = "";
    int64_t lValue;

    strcpy(lBuffer, "0123");
    lValue = atoi_n(lBuffer, 1);
    ZuAssertIntEquals(zt, 0, (int32_t)lValue);

    lValue = atoi_n(lBuffer, 2);
    ZuAssertIntEquals(zt, 1, (int32_t)lValue);

    lValue = atoi_n(lBuffer, 3);
    ZuAssertIntEquals(zt, 12, (int32_t)lValue);

    lValue = atoi_n(lBuffer, 4);
    ZuAssertIntEquals(zt, 123, (int32_t)lValue);


    strcpy(lBuffer, " -110");
    lValue = atoi_n(lBuffer, 3);
    ZuAssertIntEquals(zt, -1, (int32_t)lValue);

    lValue = atoi_n(lBuffer, 5);
    ZuAssertIntEquals(zt, -110, (int32_t)lValue);
}


void Test_ztl_trim(ZuTest* zt)
{
    char lBuffer[128] = "";
    strcpy(lBuffer, "  hello\t ");
    lefttrim(lBuffer);
    ZuAssertStrEquals(zt, "hello\t ", lBuffer);

    righttrim(lBuffer);
    ZuAssertStrEquals(zt, "hello", lBuffer);
}

void Test_ztl_parse_size(ZuTest* zt)
{
    int64_t lValue;
    const char* lpstr;

    lpstr = "1001";
    lValue = parse_size(lpstr, 4);
    ZuAssertIntEquals(zt, 1001, (int32_t)lValue);

    lpstr = "20k";
    lValue = parse_size(lpstr, 3);
    ZuAssertIntEquals(zt, 20 * 1024, (int32_t)lValue);

    lpstr = "3M";
    lValue = parse_size(lpstr, 2);
    ZuAssertIntEquals(zt, 3 * 1024 * 1024, (int32_t)lValue);

    lpstr = "1024m";
    lValue = parse_size(lpstr, 5);
    ZuAssertIntEquals(zt, 1 * 1024 * 1024 * 1024, (uint32_t)lValue);

    lpstr = "a2K";
    lValue = parse_size(lpstr, 3);
    ZuAssertIntEquals(zt, 0, (int32_t)lValue);
}

void Test_ztl_strdelimiter(ZuTest* zt)
{
    char  lBuffer[128] = "MD001|000001| || 100.0|20:32:10";
    zditem_t lArr[16] = { 0 };
    int   lArrSize = 16;

    lArrSize = str_delimiter_ex(lBuffer, strlen(lBuffer) - 2, lArr, lArrSize, "|");
    ZuAssertIntEquals(zt, 6, lArrSize);

    ZuAssertTrue(zt, 0 == strncmp("MD001", lArr[0].ptr, lArr[0].len));
    ZuAssertTrue(zt, 0 == strncmp("000001", lArr[1].ptr, lArr[1].len));
    ZuAssertTrue(zt, 0 == strncmp(" ", lArr[2].ptr, lArr[2].len));
    ZuAssertTrue(zt, 0 == strncmp("20:32:10", lArr[5].ptr, lArr[5].len));
}

void Test_ztl_ztlncpy(ZuTest* zt)
{
    int64_t data = 0;
    void* psrc = (void*)0x01;
    ztlncpy(&data, &psrc, sizeof(void*));
    ZuAssertTrue(zt, 1 == data);

    struct st_data {
        int val;
        void* ptr;
    };
}
