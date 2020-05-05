/*
* Copyright (C) Yingzhi Zheng.
* Copyright (C) <zhengyingzhi112@163.com>
*/

#ifndef _ZTL_TIMES_H_
#define _ZTL_TIMES_H_

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
#define ztl_inline inline
extern "C" {
#else
#define ztl_inline 
#endif//__cplusplus

#ifdef __linux__
#define LOCALTIME_S(x,y) localtime_r(y,x)
#else
#define LOCALTIME_S(x,y) localtime_s(x,y)
#endif
#define DATE_FORMAT             "%Y-%m-%d"
#define DATE_FORMAT_CLEAN       "%4d-%02d-%02d"
#define TIME_FORMAT             "%H:%M:%S"
#define DATE_TIME_FORMAT        "%Y-%m-%d %H:%M:%S"
#define DATE_TIME_FORMAT_CLEAN  "%4d-%02d-%02d %02d:%02d:%02d"

#define TIMEZONE_STRING(s)      #s
#define BJ_TZ_OFFSET           +08


typedef struct ztl_tm_s
{
    int tm_usec;  // micro second  - [0 - 999999]
    int tm_sec;   // seconds after the minute - [0, 60] including leap second
    int tm_min;   // minutes after the hour - [0, 59]
    int tm_hour;  // hours since midnight - [0, 23]
    int tm_mday;  // day of the month - [1, 31]
    int tm_mon;   // months since January - [1, 12]
    int tm_year;  // years
    int tm_wday;  // days since Sunday - [0, 6]
    //int tm_yday;  // days since January 1 - [0, 365]
}ztl_tm;


typedef struct {
    uint16_t    year;
    uint8_t     month;
    uint8_t     day;
}ztl_tm_date_t;

typedef union {
    ztl_tm_date_t   date;
    int32_t         id32;
}ztl_union_date_t;

typedef struct {
    uint8_t     hour;
    uint8_t     minute;
    uint8_t     second;
    uint8_t     padding;
}ztl_tm_time_t;

typedef union {
    ztl_tm_time_t   time;
    int32_t         it32;
}ztl_union_time_t;

typedef struct {
    ztl_tm_date_t   date;
    ztl_tm_time_t   time;
}ztl_tm_dt_t;

typedef union {
    int64_t     i64;
    ztl_tm_dt_t dt;
}ztl_union_dt_t;


/// precision to milli-second
int64_t get_timestamp();

/// get time of day
#ifdef _MSC_VER
void gettimeofday(struct timeval *tp, void* reserve);
#else
#include <sys/time.h>
#endif//_MSC_VER


// 2018-01-02
int ztl_ymd(char* buf, time_t t);

// 20180102
int ztl_ymd0(char* buf, time_t t);

// 20:13:46
int ztl_hms(char* buf, time_t t);

// 20:13:46.500123
int ztl_hmsu(char* buf);

// 2018-01-02 20:13:46
int ztl_ymdhms(char* buf, time_t t);

// 2018-01-02 20:13:46.500
int ztl_ymdhmsf(char* buf);

// 2018-01-02 20:13:46.500123
int ztl_ymdhmsu(char* buf);

// hmsf 20:13:46 -->> 201346
int ztl_hms2inttime(const char* hms);

// 20:13:46.500 -->> 201346500
int ztl_hmsf2inttime(const char* hmsf);

// 201346 -->> 20:13:46
int ztl_inttime2hms(char* hms, int len, int atime);

// 201346500 -->> 20:13:46.500
int ztl_inttime2hmsf(char* hmsf, int len, int atime);

// got 20180102
int ztl_tointdate(time_t atime);

// got 201346
int ztl_tointtime(time_t atime);

// got 201346500
int ztl_tointtimef();

// got 20180102201346
int64_t ztl_intdatetime();

// got 20180102201346500 within millisecond
int64_t ztl_intdatetimef();


// 10:35:22 -->> pt
int ztl_str_to_ptime(ztl_tm_time_t* pt, const char* time_buf, int len);
// 103522 -->> pt
int ztl_int_to_ptime(ztl_tm_time_t* pt, int time_int, int have_millisec);

// 20200315 or 2020-03-15 -->> pd
int ztl_str_to_pdate(ztl_tm_date_t* pd, const char* date_buf, int len);
int ztl_int_to_pdate(ztl_tm_date_t* pd, int32_t date_int);

// convert to i64 dt, which could be easily extract to ztl_tm_dt_t
int ztl_intdt_to_tm(ztl_tm_dt_t* pdt, int32_t date_int, int32_t time_int, int have_millisec);
int64_t ztl_tmdt_to_i64(const ztl_tm_dt_t* pdt);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_TIMES_H_
