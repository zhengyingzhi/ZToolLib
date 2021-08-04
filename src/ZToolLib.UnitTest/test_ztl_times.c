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

void Test_ztl_times2(ZuTest* zt)
{
    ZuAssertIntEquals(zt, 20210730, ztl_get_distance_date(20210731, -1));
    ZuAssertIntEquals(zt, 20210801, ztl_get_distance_date(20210731, 1));

    ZuAssertIntEquals(zt, 20201215, ztl_get_prev_date(20201216));
    ZuAssertIntEquals(zt, 20201216, ztl_get_next_date(20201215));
    ZuAssertIntEquals(zt, 20201231, ztl_get_prev_date(20210101));
    ZuAssertIntEquals(zt, 20210101, ztl_get_next_date(20201231));
    ZuAssertIntEquals(zt, 20200229, ztl_get_prev_date(20200301));
    ZuAssertIntEquals(zt, 20200301, ztl_get_next_date(20200229));
    ZuAssertIntEquals(zt, 20210228, ztl_get_prev_date(20210301));
    ZuAssertIntEquals(zt, 20210301, ztl_get_next_date(20210228));
    ZuAssertIntEquals(zt, 20210531, ztl_get_prev_date(20210601));
    ZuAssertIntEquals(zt, 20210601, ztl_get_next_date(20210531));
    ZuAssertIntEquals(zt, 20210930, ztl_get_prev_date(20211001));
    ZuAssertIntEquals(zt, 20211001, ztl_get_next_date(20210930));

    ZuAssertIntEquals(zt, 6, ztl_get_wday(20210731));
    ZuAssertIntEquals(zt, 3, ztl_get_wday(20210804));

    int dates1[32], dates2[32];
    int len1 = ztl_date_range(dates1, 32, 20210726, 20210806, true);
    int len2 = ztl_date_range(dates2, 32, 20210726, 20210806, false);
    ZuAssertIntEquals(zt, 10, len1);
    ZuAssertIntEquals(zt, 12, len2);
    ZuAssertIntEquals(zt, 20210726, dates1[0]);
    ZuAssertIntEquals(zt, 20210727, dates1[1]);
    ZuAssertIntEquals(zt, 20210802, dates1[5]);
    ZuAssertIntEquals(zt, 20210726, dates2[0]);
    ZuAssertIntEquals(zt, 20210727, dates2[1]);
    ZuAssertIntEquals(zt, 20210731, dates2[5]);

    ZuAssertIntEquals(zt, 100030, ztl_get_distance_time(100000, 30));
    ZuAssertIntEquals(zt, 100115, ztl_get_distance_time(100000, 75));
    ZuAssertIntEquals(zt, 95940,  ztl_get_distance_time(100000, -20));
    ZuAssertIntEquals(zt, 95855,  ztl_get_distance_time(100000, -65));

    ZuAssertIntEquals(zt, -30, ztl_difftime(100000, 100030, 0));
    ZuAssertIntEquals(zt, -75, ztl_difftime(100000, 100115, 0));
    ZuAssertIntEquals(zt, 30, ztl_difftime(100030, 100000, 0));
    ZuAssertIntEquals(zt, 75, ztl_difftime(100115, 100000, 0));

    ZuAssertIntEquals(zt, -61, ztl_difftime(113001, 113102, 0));
    ZuAssertIntEquals(zt, 61, ztl_difftime(113102, 113001, 0));
    ZuAssertIntEquals(zt, -20, ztl_difftime(120152, 120212, 0));
    ZuAssertIntEquals(zt, 20, ztl_difftime(120212, 120152, 0));

    ZuAssertIntEquals(zt, 29600, ztl_difftime(113000200, 112930600, 1));
    ZuAssertIntEquals(zt, -900, ztl_difftime(113000200, 113001100, 1));
    ZuAssertIntEquals(zt, -61400, ztl_difftime(113001200, 113102600, 1));
    ZuAssertIntEquals(zt, 60600, ztl_difftime(113102200, 113001600, 1));
    ZuAssertIntEquals(zt, -20400, ztl_difftime(120152200, 120212600, 1));
    ZuAssertIntEquals(zt, 19600, ztl_difftime(120212200, 120152600, 1));

    ZuAssertIntEquals(zt, 86275, ztl_difftime(235910, 115, 0));
    ZuAssertIntEquals(zt, -86342, ztl_difftime(8, 235910, 0));
}
