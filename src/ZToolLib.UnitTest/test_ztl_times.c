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
    ZuAssertStrEquals(zt, expect_date1, buf);
    ZuAssertIntEquals(zt, 10, len);

    len = ztl_ymd0(buf, 0);
    ZuAssertTrue(zt, 0 == strcmp(buf, expect_date2));
    ZuAssertIntEquals(zt, 8, len);

    len = ztl_hms(buf, 0);
    //ZuAssertTrue(zt, 0 == strcmp(buf, "09:01:48"));
    ZuAssertIntEquals(zt, 8, len);

    len = ztl_hmsu(buf);
    //ZuAssertTrue(zt, 0 == strncmp(buf, "09:01:48.", 9));
    ZuAssertIntEquals(zt, 15, len);

    len = ztl_ymdhms(buf, 0);
    //ZuAssertTrue(zt, 0 == strcmp(buf, "2018-07-27 09:01:48"));
    ZuAssertIntEquals(zt, 19, len);

    len = ztl_ymdhmsf(buf);
    //ZuAssertTrue(zt, 0 == strncmp(buf, "2018-07-27 09:01:48.", 20));
    ZuAssertIntEquals(zt, 23, len);

    len = ztl_ymdhmsu(buf);
    //ZuAssertTrue(zt, 0 == strncmp(buf, "2018-07-27 09:01:48.", 20));
    ZuAssertIntEquals(zt, 26, len);


    // hmsf 20:13:46 -->> 201346
    dt = ztl_hms2inttime("20:13:46");
    ZuAssertIntEquals(zt, 201346, dt);

    // 20:13:46.500 -->> 201346500
    dt = ztl_hmsf2inttime("20:13:46.500");
    ZuAssertIntEquals(zt, 201346500, dt);

    // 201346 -->> 20:13:46
    len = ztl_inttime2hms(buf, sizeof(buf), 201346);
    ZuAssertTrue(zt, 0 == strncmp(buf, "20:13:46", 8));
    ZuAssertIntEquals(zt, 8, len);

    // 201346500 -->> 20:13:46.500
    len = ztl_inttime2hmsf(buf, sizeof(buf), 201346500);
    ZuAssertTrue(zt, 0 == strncmp(buf, "20:13:46.500", 12));
    ZuAssertIntEquals(zt, 12, len);

    dt = ztl_tointdate(lTime);
    ZuAssertIntEquals(zt, expect_date_int, dt);

    dt = ztl_tointtime(lTime);
    ZuAssertIntEquals(zt, expect_time_int, dt);

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

    ZuAssertIntEquals(zt, 4, ztl_diffday(20210311, 20210315));
    // ztl_diffnow(int endday);
}