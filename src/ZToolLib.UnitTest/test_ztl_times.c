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
    printf("time = %s\n", expect_date1);
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
    uint64_t dt2;
    dt2 = ztl_intdatetime();
    ZuAssertTrue(zt, dt2 > 10000000000000ULL);

    // got 20180102201346500 within millisecond
    dt2 = ztl_intdatetimef();
    ZuAssertTrue(zt, dt2 > 10000000000000000ULL);

    ZuAssertIntEquals(zt, 4, ztl_diffday(20210311, 20210315, 0));
    // ztl_diffnow(int endday);

    
    // ztl_str_to_ptime
    ztl_tm_time_t pt;

    ztl_str_to_ptime(&pt, "14:50:05.050", 12);
    ZuAssertIntEquals(zt, 14, pt.hour);    
    ZuAssertIntEquals(zt, 50, pt.minute);
    ZuAssertIntEquals(zt, 05, pt.second);

    //ztl_int_to_ptime
    
    memset(&pt, 0, sizeof(pt));
    ztl_int_to_ptime(&pt,145005,0);
    ZuAssertIntEquals(zt, 14, pt.hour);
    ZuAssertIntEquals(zt, 50, pt.minute);
    ZuAssertIntEquals(zt, 05, pt.second);

    memset(&pt, 0, sizeof(pt));
    ztl_int_to_ptime(&pt, 145005007, 1);
    ZuAssertIntEquals(zt, 14, pt.hour);
    ZuAssertIntEquals(zt, 50, pt.minute);
    ZuAssertIntEquals(zt, 05, pt.second);
    //ztl_str_to_pdate
    ztl_tm_date_t pd;

    ztl_int_to_pdate(&pd, 20210725);
    ZuAssertIntEquals(zt, 2021, pd.year);
    ZuAssertIntEquals(zt, 07, pd.month);
    ZuAssertIntEquals(zt, 25, pd.day); 

    //ztl_tmdt_to_i64
    ztl_tm_dt_t pdt, pdt2;
    pdt.date = pd;
    pdt.time = pt;
    int64_t r = ztl_tmdt_to_i64(&pdt);
    ztl_i64_to_tmdt(&pdt2, r);
    printf("data = %d\n", pdt2.date.year * 10000 + pdt2.date.month * 100 + pdt2.date.day);
    ZuAssertIntEquals(zt, 20210725, pdt2.date.year*10000+ pdt2.date.month*100+ pdt2.date.day);
    ZuAssertIntEquals(zt, 145005, pdt2.time.hour * 10000 + pdt2.time.minute * 100 + pdt2.time.second);

    //ztl_diffday  
    ZuAssertIntEquals(zt, 10, ztl_diffday(20210719, 20210731, 1));
    ZuAssertIntEquals(zt, 7, ztl_diffday(20210718, 20210728, 1));
    ZuAssertIntEquals(zt, 11, ztl_diffday(20210717, 20210728, 0));
    ZuAssertIntEquals(zt, 10, ztl_diffday(20210717, 20210801, 1));

    //ztl_diffnow
    /*ZuAssertIntEquals(zt, 11, ztl_diffnow(20210717,  0));
    ZuAssertIntEquals(zt, 3, ztl_diffnow(20210801, 1));
    ZuAssertIntEquals(zt, 19, ztl_diffnow(20210701, 1));
    ZuAssertIntEquals(zt, 27, ztl_diffnow(20210701, 0));*/

}