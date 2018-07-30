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

/// precision to milli-second
int64_t get_timestamp();

/// get time of day
#ifdef _WIN32
void gettimeofday(struct timeval *tp, void* reserve);
#else
#include <sys/time.h>
#endif//_WIN32


// 2018-01-02
int ztl_ymd(char* buf);

// 20180102
int ztl_ymd0(char* buf);

// 20:13:46
int ztl_hms(char* buf);

// 20:13:46.500
int ztl_hmsu(char* buf);

// 2018-01-02 20:13:46
int ztl_ymdhms(char* buf);

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


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_TIMES_H_
