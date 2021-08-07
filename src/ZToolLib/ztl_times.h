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

#if defined(__linux__) || defined(_POSIX_THREAD_SAFE_FUNCTIONS)
#define LOCALTIME_S(t,ltm)      localtime_r(t,ltm)
#else
#define LOCALTIME_S(t,ltm)      localtime_s(ltm,t)
#endif
#define DATE_FORMAT             "%Y-%m-%d"
#define DATE_FORMAT_CLEAN       "%4d-%02d-%02d"
#define TIME_FORMAT             "%H:%M:%S"
#define DATE_TIME_FORMAT        "%Y-%m-%d %H:%M:%S"
#define DATE_TIME_FORMAT_CLEAN  "%4d-%02d-%02d %02d:%02d:%02d"

#define SECONDS_PER_DATE        (24 * 60 * 60)  // 86400
#define SECONDS_HALF_DATE       (12 * 60 * 60)  // 43200
#define SECONDS_PER_HOUR        (60 * 60)       // 3600

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
int64_t get_timestamp_us();

#ifdef _MSC_VER
/// get time of day
void gettimeofday(struct timeval *tp, void* reserve);

/// try sleep ms/us/ns
#define sleepms(x)                      Sleep(x)
#define ztl_sleepms(x)                  Sleep(x)
void ztl_sleepus(int us);
void ztl_sleepns(int ns);

#else
#include <sys/time.h>
#include <sys/unistd.h>

/// try sleep ms/us/ns
#define sleepms(x)                      usleep((x)*1000)
#define ztl_sleepms(x)                  usleep((x)*1000)
#define ztl_sleepus(x)                  usleep(x)
void ztl_sleepns(int ns);

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
int ztl_hms2int(const char* hms);

// 20:13:46.500 -->> 201346500
int ztl_hmsf2int(const char* hmsf);

// 201346 -->> 20:13:46
int ztl_int2hms(char* hms, int len, int atime);

// 201346500 -->> 20:13:46.500
int ztl_int2hmsf(char* hmsf, int len, int atime);

// got 20180102
int ztl_ymd_int(time_t atime);

// got 201346
int ztl_hms_int(time_t atime);

// got 201346500
int ztl_hmsf_int(time_t atime, int millisec);

// got 20180102201346
int64_t ztl_ymdhms_int(time_t atime);

// got 20180102201346500 within millisecond
int64_t ztl_ymdhmsf_int(time_t atime, int millisec);

// got time_t by 20180102 & 201346
time_t ztl_int_to_time(int date, int time);
time_t ztl_str_to_time(const char* date, const char* time);

// 10:35:22 -->> pt
int ztl_str_ptime(ztl_tm_time_t* pt, const char* time_buf, int len);
// 103522 -->> pt
int ztl_int_ptime(ztl_tm_time_t* pt, int time_int, int have_millisec);

// 20200315 or 2020-03-15 -->> pd
int ztl_str_pdate(ztl_tm_date_t* pd, const char* date_buf, int len);
int ztl_int_pdate(ztl_tm_date_t* pd, int32_t date_int);

// convert to i64 dt, which could be easily extract to ztl_tm_dt_t
int ztl_intdt_to_tm(ztl_tm_dt_t* pdt, int32_t date_int, int32_t time_int, int have_millisec);
int ztl_i64_to_tmdt(ztl_tm_dt_t* pdt, int64_t i64dt);
int64_t ztl_tmdt_to_i64(const ztl_tm_dt_t* pdt);

// get weekday (tm_wday 0-Sunday, 6-Saturday)
int  ztl_get_wday(int date);
bool ztl_is_weekend(int date);
int  ztl_time_wday(time_t t);
bool ztl_time_is_weekend(time_t t);

// get previous(offset < 0) or next(offset > 0) date by offset date
int ztl_get_distance_date(int date, int offset);
int ztl_get_prev_date(int date);
int ztl_get_next_date(int date);
int ztl_date_range(int dates[], int size, int start_date, int end_date, bool exclude_weekend);

// get preivous(offset < 0) or next(offset > 0) time by offset seconds
int ztl_get_distance_time(int time, int offset);
int ztl_minute_range(int minutes[], int size, int start_time, int end_time);

// calc time distance, return t1-t2 total seconds or milliseconds
int ztl_difftime(int t1, int t2, int have_millisec);

// calc day distance
int ztl_diffday(int startday, int endday, bool exclude_weekend);
int ztl_diffnow(int endday, bool exclude_weekend);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_TIMES_H_
