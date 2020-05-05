#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef _MSC_VER
#include <Windows.h>
#include <process.h>

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#else
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#endif//_MSC_VER

#include "ztl_utils.h"

#include "ztl_times.h"


/* get current date time, including micro second
 */
static void ztl_now(ztl_tm* ptm)
{
#ifdef _MSC_VER
    FILETIME    ftLocal;
    SYSTEMTIME  stLocal;
    GetSystemTimeAsFileTime(&ftLocal);
    FileTimeToSystemTime(&ftLocal, &stLocal);

    int64_t it = ftLocal.dwHighDateTime;
    it = it << 32;
    it |= ftLocal.dwLowDateTime;
    it /= 10; // convert from 100 nano-sec periods to micro-seconds
    it -= (__int64)11644473600000000; //Convert from Windows epoch to Unix epoch
    int microsec = it % 1000000;
    
    ptm->tm_usec    = microsec;
    ptm->tm_sec     = stLocal.wSecond;
    ptm->tm_min     = stLocal.wMinute;
    ptm->tm_hour    = stLocal.wHour + 8;
    ptm->tm_mday    = stLocal.wDay;
    ptm->tm_mon     = stLocal.wMonth;
    ptm->tm_year    = stLocal.wYear;
    ptm->tm_wday    = stLocal.wDayOfWeek;

    if (ptm->tm_hour > 24)
        ptm->tm_hour -= 24;
#else//linux

    struct tm ltm;
    struct timeval tv;
    gettimeofday(&tv, NULL);

#if defined (_POSIX_THREAD_SAFE_FUNCTIONS)
    localtime_r(&tv.tv_sec, &ltm);
#else
    ltm = *localtime(&tv.tv_sec);
    
    ptm->tm_usec    = tv.tv_usec;
    ptm->tm_sec     = ltm.tm_sec;
    ptm->tm_min     = ltm.tm_min;
    ptm->tm_hour    = ltm.tm_hour;
    ptm->tm_mday    = ltm.tm_mday;
    ptm->tm_mon     = ltm.tm_mon + 1;
    ptm->tm_year    = ltm.tm_year + 1900;
    ptm->tm_wday    = ltm.tm_wday;
#endif

#endif//_MSC_VER
}


int64_t get_timestamp()
{
#ifdef _MSC_VER
    return (int64_t)GetTickCount();
#else
    //struct timespec lTime;
    //clock_gettime(CLOCK_REALTIME, &lTime);
    //int64_t lRet = (uint64_t)lTime.tv_sec * 1000 + lTime.tv_nsec / 1000000;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t lRet = (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return lRet;
#endif//_MSC_VER
}


/// get time of day
#ifdef _MSC_VER
void gettimeofday(struct timeval *tp, void* reserve)
{
    int64_t intervals;
    FILETIME ft;

    GetSystemTimeAsFileTime(&ft);

    /*
    * A file time is a 64-bit value that represents the number
    * of 100-nanosecond intervals that have elapsed since
    * January 1, 1601 12:00 A.M. UTC.
    *
    * Between January 1, 1970 (Epoch) and January 1, 1601 there were
    * 134744 days,
    * 11644473600 seconds or
    * 11644473600,000,000,0 100-nanosecond intervals.
    *
    * See also MSKB Q167296.
    */

    intervals = ((__int64)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    intervals -= 116444736000000000;

    tp->tv_sec = (long)(intervals / 10000000);
    tp->tv_usec = (long)((intervals % 10000000) / 10);
}

#endif//_MSC_VER

int ztl_ymd(char* buf, time_t t)
{
    const size_t sz = sizeof("0000-00-00");

    struct tm ltm;
    if (t <= 0)
        t = time(NULL);
    LOCALTIME_S(&ltm, &t);
    return (int)strftime(buf, sz, DATE_FORMAT, &ltm);
}

// 20180102
int ztl_ymd0(char* buf, time_t t)
{
    const size_t sz = sizeof("00000000");

    struct tm ltm;
    if (t <= 0)
        t = time(NULL);
    LOCALTIME_S(&ltm, &t);
    return (int)strftime(buf, sz, "%Y%m%d", &ltm);
}

int ztl_hms(char* buf, time_t t)
{
    const size_t sz = sizeof(TIME_FORMAT);

    struct tm ltm;
    if (t <= 0)
        t = time(NULL);
    LOCALTIME_S(&ltm, &t);
    return (int)strftime(buf, sz, TIME_FORMAT, &ltm);
}

int ztl_hmsu(char* buf)
{
    const size_t sz = sizeof("%02d:%02d:%02d.%06d");

    int len;
    ztl_tm ltm;
    ztl_now(&ltm);

    len = snprintf(buf, sz, "%02d:%02d:%02d.%06d",
        ltm.tm_hour, ltm.tm_min, ltm.tm_sec, ltm.tm_usec);

    return len;
}

int ztl_ymdhms(char* buf, time_t t)
{
    const size_t sz = sizeof("0000-00-00 00:00:00");

    struct tm ltm;
    if (t <= 0)
        t = time(NULL);
    LOCALTIME_S(&ltm, &t);
    return (int)strftime(buf, sz, DATE_TIME_FORMAT, &ltm);
}

int ztl_ymdhmsf(char* buf)
{
    ztl_tm ltm;
    ztl_now(&ltm);

    int len;
    len = sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d.%03d",
        ltm.tm_year, ltm.tm_mon, ltm.tm_mday,
        ltm.tm_hour, ltm.tm_min, ltm.tm_sec,
        ltm.tm_usec / 1000);

    return len;
}

int ztl_ymdhmsu(char* buf)
{
    ztl_tm ltm;
    ztl_now(&ltm);

    int len;
    len = sprintf(buf, "%4d-%02d-%02d %02d:%02d:%02d.%06d",
        ltm.tm_year, ltm.tm_mon, ltm.tm_mday,
        ltm.tm_hour, ltm.tm_min, ltm.tm_sec,
        ltm.tm_usec);

    return len;
}

int ztl_hms2inttime(const char* hms)
{
    // hmsf 20:13:46 -->> 201346
    return atoi(hms) * 10000 + atoi(hms + 3) * 100 + atoi(hms + 6);
}

int ztl_hmsf2inttime(const char* hmsf)
{
    // hmsf 20:13:46.500 -->> 201346500
    return atoi(hmsf) * 10000000 + atoi(hmsf + 3) * 100000 + 
        atoi(hmsf + 6) * 1000 + atoi(hmsf + 9);
}

int ztl_inttime2hms(char* hms, int len, int atime)
{
    // hms 201346 -->> 20:13:46
    len = snprintf(hms, len, "%02d:%02d:%02d", atime / 10000,
        (atime % 10000) / 100, atime % 100);
    return len;
}

int ztl_inttime2hmsf(char* hmsf, int len, int atime)
{
    // FIXME hms 201346500 -->> 20:13:46.500
    len = snprintf(hmsf, len, "%02d:%02d:%02d.%03d", atime / 10000000,
        (atime % 10000000) / 100000, (atime % 100000) / 1000, atime % 1000);
    return len;
}

int ztl_tointdate(time_t atime)
{
    if (atime == 0)
        atime = time(0);

    // got 20180102
    struct tm ltm;
    LOCALTIME_S(&ltm, &atime);

    return ((ltm.tm_year + 1900) * 10000) + ((ltm.tm_mon + 1) * 100) + ltm.tm_mday;
}

int ztl_tointtime(time_t atime)
{
    if (atime == 0)
        atime = time(0);

    // got 201346 
    struct tm ltm;
    LOCALTIME_S(&ltm, &atime);

    return (ltm.tm_hour * 10000) + (ltm.tm_min * 100) + ltm.tm_sec;
}

int ztl_tointtimef()
{
    // got 201346500
    ztl_tm ltm;
    ztl_now(&ltm);

    return (ltm.tm_hour * 10000000) + (ltm.tm_min * 100000) + ltm.tm_sec * 1000 + ltm.tm_usec;
}

int64_t ztl_intdatetime()
{
    // got 20180102201346
    int64_t ldt;
    ztl_tm ltm;
    ztl_now(&ltm);

    int64_t ldate = (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
    int64_t ltime = (ltm.tm_sec * 10000) + (ltm.tm_min * 100) + ltm.tm_sec;
    ldt = ldate * 1000000 + ltime;
    return ldt;
}

int64_t ztl_intdatetimef()
{
    // got 20180102201346500
    int64_t ldtf;
    ztl_tm ltm;
    ztl_now(&ltm);

    int64_t ldate = (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
    int64_t ltimef = (ltm.tm_sec * 10000000) + (ltm.tm_min * 100000) + ltm.tm_sec * 1000
        + ltm.tm_usec / 1000;
    ldtf = ldate * 1000000000ULL + ltimef;
    return ldtf;
}

int ztl_str_to_ptime(ztl_tm_time_t* pt, const char* time_buf, int len)
{
    (void)len;
#if 0
    // 14:50:50.500
    ptm->tm_hour = atoi(buf);
    ptm->tm_min = atoi(buf + 3);
    ptm->tm_sec = atoi(buf + 6);
    // ptm->ms = atoi(buf + 9);
#else
    pt->hour= (time_buf[0] - '0') * 10 + (time_buf[1] - '0');
    pt->minute = (time_buf[3] - '0') * 10 + (time_buf[4] - '0');
    pt->second = (time_buf[6] - '0') * 10 + (time_buf[7] - '0');
#endif
    return 0;
}

int ztl_int_to_ptime(ztl_tm_time_t* pt, int time_int, int have_millisec)
{
    int hhmmss = time_int;
    if (have_millisec)
        hhmmss /= 1000;

    pt->hour = time_int / 10000;
    pt->minute = (time_int / 100) % 100;
    pt->second = time_int % 100;
    return 0;
}

int ztl_str_to_pdate(ztl_tm_date_t* pd, const char* date_buf, int len)
{
    pd->year = (uint16_t)atoi_n(date_buf, 4);
    if (len == 8) {
        pd->month = (uint8_t)atoi_n(date_buf + 4, 2);
        pd->day = (uint8_t)atoi_n(date_buf + 6, 2);
    }
    else if (len == 10) {
        pd->month = (uint8_t)atoi_n(date_buf + 5, 2);
        pd->day = (uint8_t)atoi_n(date_buf + 8, 2);
    }
    else {
        return -1;
    }
    return 0;
}

int ztl_int_to_pdate(ztl_tm_date_t* pd, int32_t date_int)
{
    pd->year = date_int / 10000;
    pd->month = date_int / 10000 % 100;
    pd->day = date_int % 100;
    return 0;
}

int ztl_intdt_to_tm(ztl_tm_dt_t* pdt, int32_t date_int, int32_t time_int, int have_millisec)
{
    ztl_int_to_pdate(&pdt->date, date_int);
    ztl_int_to_ptime(&pdt->time, time_int, have_millisec);
    return 0;
}

int64_t ztl_tmdt_to_i64(const ztl_tm_dt_t* pdt)
{
    ztl_union_dt_t ud;
    ud.dt.date = pdt->date;
    ud.dt.time = pdt->time;
    return ud.i64;
}
