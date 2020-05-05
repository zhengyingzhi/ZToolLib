#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ZToolLib/ztl_times.h>
#include <ZToolLib/ztl_unit_test.h>


void Test_ztl_times(ZuTest* zt)
{
    // time_t lTime = 1532653072;
    time_t lTime = time(0);
    struct tm* ptm = localtime(&lTime);
    char expect_date1[32] = "";
    char expect_date2[32] = "";
    int  expect_date_int = 0;
    int  expect_time_int = 0;

    sprintf(expect_date1, "%04d-%02d-%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
    sprintf(expect_date2, "%04d%02d%02d", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday);
    expect_date_int = atoi(expect_date2);
    expect_time_int = ptm->tm_hour * 10000 + ptm->tm_min * 100 + ptm->tm_sec;

    int len = 0, dt;
    char buf[64] = "";

    len = ztl_ymd(buf, 0);
    ZuAssertTrue(zt, 0 == strcmp(buf, expect_date1));
    ZuAssertTrue(zt, len == 10);

    len = ztl_ymd0(buf, 0);
    ZuAssertTrue(zt, 0 == strcmp(buf, expect_date2));
    ZuAssertTrue(zt, len == 8);

    len = ztl_hms(buf, 0);
    //ZuAssertTrue(zt, 0 == strcmp(buf, "09:01:48"));
    ZuAssertTrue(zt, len == 8);

    len = ztl_hmsu(buf);
    //ZuAssertTrue(zt, 0 == strncmp(buf, "09:01:48.", 9));
    ZuAssertTrue(zt, len == 15);

    len = ztl_ymdhms(buf, 0);
    //ZuAssertTrue(zt, 0 == strcmp(buf, "2018-07-27 09:01:48"));
    ZuAssertTrue(zt, len == 19);

    len = ztl_ymdhmsf(buf);
    //ZuAssertTrue(zt, 0 == strncmp(buf, "2018-07-27 09:01:48.", 20));
    ZuAssertTrue(zt, len == 23);

    len = ztl_ymdhmsu(buf);
    //ZuAssertTrue(zt, 0 == strncmp(buf, "2018-07-27 09:01:48.", 20));
    ZuAssertTrue(zt, len == 26);


    // hmsf 20:13:46 -->> 201346
    dt = ztl_hms2inttime("20:13:46");
    ZuAssertTrue(zt, dt == 201346);

    // 20:13:46.500 -->> 201346500
    dt = ztl_hmsf2inttime("20:13:46.500");
    ZuAssertTrue(zt, dt == 201346500);

    // 201346 -->> 20:13:46
    len = ztl_inttime2hms(buf, sizeof(buf), 201346);
    ZuAssertTrue(zt, 0 == strncmp(buf, "20:13:46", 8));
    ZuAssertTrue(zt, len == 8);

    // 201346500 -->> 20:13:46.500
    len = ztl_inttime2hmsf(buf, sizeof(buf), 201346500);
    ZuAssertTrue(zt, 0 == strncmp(buf, "20:13:46.500", 12));
    ZuAssertTrue(zt, len == 12);

    dt = ztl_tointdate(lTime);
    ZuAssertTrue(zt, dt == expect_date_int);

    dt = ztl_tointtime(lTime);
    ZuAssertTrue(zt, dt == expect_time_int);

    // got 201346500
    dt = ztl_tointtimef(lTime);
    //ZuAssertTrue(zt, dt > 10000000);

    // got 20180102201346
    int64_t dt2;
    dt2 = ztl_intdatetime();
    ZuAssertTrue(zt, dt2 > 10000000000000ULL);

    // got 20180102201346500 within millisecond
    dt2 = ztl_intdatetimef();
    ZuAssertTrue(zt, dt2 > 10000000000000000ULL);
}