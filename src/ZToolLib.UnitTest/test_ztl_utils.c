#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

    ZuAssertIntEquals(zt, 5, digits10(10000));
    ZuAssertIntEquals(zt, 7, digits10(1000000));
    ZuAssertIntEquals(zt, 9, digits10(100000000));
    ZuAssertIntEquals(zt, 6, digits10(100000));

    /*ZuAssertIntEquals(zt, 4, read_number_from_file("file.txt"));
    char buf[100] = "";

    read_file_content("file.txt", buf, 100);
    printf("%s\n", buf);*/

    char arr[10] = "12345";
    print_mem(arr, 10, 4);

    int nums[10] = { 0,1,2,3,4,5,6,7,8,9, };
    ZuAssertIntEquals(zt, 5, binary_search(nums, 10, 5));
    ZuAssertIntEquals(zt, 0, binary_search(nums, 10, 0));
    ZuAssertIntEquals(zt, 1, binary_search(nums, 10, 1));

    
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

    remove_char(lBuffer, 'l');
    ZuAssertStrEquals(zt, "heo", lBuffer);

    replace_char(lBuffer, 'e', 'E');
    ZuAssertStrEquals(zt, "hE", lBuffer);
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

    lArrSize = str_delimiter_ex(lBuffer, (int)strlen(lBuffer) - 2, lArr, lArrSize, "|");
    ZuAssertIntEquals(zt, 6, lArrSize);

    ZuAssertTrue(zt, 0 == strncmp("MD001", lArr[0].ptr, lArr[0].len));
    ZuAssertTrue(zt, 0 == strncmp("000001", lArr[1].ptr, lArr[1].len));
    ZuAssertTrue(zt, 0 == strncmp(" ", lArr[2].ptr, lArr[2].len));
    ZuAssertTrue(zt, 0 == strncmp("20:32:10", lArr[5].ptr, lArr[5].len));
}

void Test_ztl_ztlncpy(ZuTest* zt)
{
    int8_t  res8 = -1, i8 = 8;
    int16_t res16 = -1, i16 = 16;
    int32_t res32 = -1, i32 = 32;
    int64_t res64 = -1, i64 = 64;
    ztlncpy(&res8, &i8, sizeof(int8_t));
    ztlncpy(&res16, &i16, sizeof(int16_t));
    ztlncpy(&res32, &i32, sizeof(int32_t));
    ztlncpy(&res64, &i64, sizeof(int64_t));

    ZuAssertIntEquals(zt, i8, res8);
    ZuAssertIntEquals(zt, i16, res16);
    ZuAssertIntEquals(zt, i32, res32);
    ZuAssertInt64Equals(zt, i64, res64);

    // 6 bytes
#pragma pack(push, 1)
    struct st_data_6 {
        int32_t i32;
        int16_t i16;
    };
#pragma pack(pop)
    ZuAssertIntEquals(zt, 6, sizeof(struct st_data_6));
    struct st_data_6 resdata_6 = { 0 };
    struct st_data_6 srcdata_6 = { 0 };
    srcdata_6.i32 = 32;
    srcdata_6.i16 = 16;
    ztlncpy(&resdata_6, &srcdata_6, sizeof(struct st_data_6));
    ZuAssertIntEquals(zt, 32, resdata_6.i32);
    ZuAssertIntEquals(zt, 16, resdata_6.i16);

    // 12 bytes
    struct st_data_12 {
        int i32_1;
        int i32_2;
        int i32_3;
    };
    ZuAssertIntEquals(zt, 12, sizeof(struct st_data_12));
    struct st_data_12 resdata_12 = { 0 };
    struct st_data_12 srcdata_12 = { 0 };
    srcdata_12.i32_1 = 321;
    srcdata_12.i32_2 = 322;
    srcdata_12.i32_3 = 323;
    ztlncpy(&resdata_12, &srcdata_12, sizeof(struct st_data_12));
    ZuAssertIntEquals(zt, 321, resdata_12.i32_1);
    ZuAssertIntEquals(zt, 322, resdata_12.i32_2);
    ZuAssertIntEquals(zt, 323, resdata_12.i32_3);

    // 16 bytes
    struct st_data_16 {
        int64_t i64_1;
        int64_t i64_2;
    };
    ZuAssertIntEquals(zt, 16, sizeof(struct st_data_16));
    struct st_data_16 resdata_16 = { 0 };
    struct st_data_16 srcdata_16 = { 0 };
    srcdata_16.i64_1 = 641;
    srcdata_16.i64_2 = 642;
    ztlncpy(&resdata_16, &srcdata_16, sizeof(struct st_data_16));
    ZuAssertInt64Equals(zt, 641, resdata_16.i64_1);
    ZuAssertInt64Equals(zt, 642, resdata_16.i64_2);

    // 20 bytes
#pragma pack(push, 1)
    struct st_data_20 {
        int64_t i64_1;
        int64_t i64_2;
        int32_t i32;
    };
#pragma pack(pop)
    ZuAssertIntEquals(zt, 20, sizeof(struct st_data_20));
    struct st_data_20 resdata_20 = { 0 };
    struct st_data_20 srcdata_20 = { 0 };
    srcdata_20.i64_1 = 641;
    srcdata_20.i64_2 = 642;
    srcdata_20.i32   = 32;
    ztlncpy(&resdata_20, &srcdata_20, sizeof(struct st_data_20));
    ZuAssertInt64Equals(zt, 641, resdata_20.i64_1);
    ZuAssertInt64Equals(zt, 642, resdata_20.i64_2);
    ZuAssertIntEquals(zt, 32, resdata_20.i32);

    // 24 bytes
    struct st_data_24 {
        int64_t i64_1;
        int64_t i64_2;
        int64_t i64_3;
    };
    ZuAssertIntEquals(zt, 24, sizeof(struct st_data_24));
    struct st_data_24 resdata_24 = { 0 };
    struct st_data_24 srcdata_24 = { 0 };
    srcdata_24.i64_1 = 641;
    srcdata_24.i64_2 = 642;
    srcdata_24.i64_3 = 643;
    ztlncpy(&resdata_24, &srcdata_24, sizeof(struct st_data_24));
    ZuAssertInt64Equals(zt, 641, resdata_24.i64_1);
    ZuAssertInt64Equals(zt, 642, resdata_24.i64_2);
    ZuAssertInt64Equals(zt, 643, resdata_24.i64_3);

    // multi bytes
    char lpsrc[64] = "hello world, i love this game";
    char lpdst[64] = "";
    ztlncpy(lpdst, lpsrc, (int)strlen(lpsrc));
    ZuAssertStrEquals(zt, lpsrc, lpdst);
}

void Test_ztl_round(ZuTest* zt)
{
    double d1, d2;
    d1 = 11.2344;
    d2 = 11.5646;
    ZuAssertTrue(zt, DBL_EQ(ztl_round(d1, 0), 11.0, DBL_EPSILON_E6));
    ZuAssertTrue(zt, DBL_EQ(ztl_round(d2, 0), 12.0, DBL_EPSILON_E6));

    ZuAssertTrue(zt, DBL_EQ(ztl_round(d1, 1), 11.20, DBL_EPSILON_E6));
    ZuAssertTrue(zt, DBL_EQ(ztl_round(d2, 1), 11.60, DBL_EPSILON_E6));

    ZuAssertTrue(zt, DBL_EQ(ztl_round(d1, 2), 11.23, DBL_EPSILON_E6));
    ZuAssertTrue(zt, DBL_EQ(ztl_round(d2, 2), 11.56, DBL_EPSILON_E6));

    ZuAssertTrue(zt, DBL_EQ(ztl_round(d1, 3), 11.234, DBL_EPSILON_E6));
    ZuAssertTrue(zt, DBL_EQ(ztl_round(d2, 3), 11.565, DBL_EPSILON_E6));
}
