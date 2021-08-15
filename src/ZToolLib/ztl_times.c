#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

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
    LOCALTIME_S(&tv.tv_sec, &ltm);

    ptm->tm_usec    = tv.tv_usec;
    ptm->tm_sec     = ltm.tm_sec;
    ptm->tm_min     = ltm.tm_min;
    ptm->tm_hour    = ltm.tm_hour;
    ptm->tm_mday    = ltm.tm_mday;
    ptm->tm_mon     = ltm.tm_mon + 1;
    ptm->tm_year    = ltm.tm_year + 1900;
    ptm->tm_wday    = ltm.tm_wday;

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

int64_t get_timestamp_us()
{
    int64_t us;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    us = (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
    return us;
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

typedef LONG(__stdcall* NtDelayExecution)(BOOLEAN Alertable, PLARGE_INTEGER Interval);
static NtDelayExecution g_pNtDelayExec = NULL;

void ztl_sleepus(int us)
{
    LARGE_INTEGER large;
    if (!g_pNtDelayExec) {
        HMODULE hModule = LoadLibraryA("ntdll.dll");
        g_pNtDelayExec = (NtDelayExecution)GetProcAddress(hModule, "NtDelayExecution");
    }
    large.QuadPart = -((LONGLONG)(us * 10000));
    (*g_pNtDelayExec)(TRUE, &large);
}

void ztl_sleepns(int ns)
{
    LARGE_INTEGER large;
    if (!g_pNtDelayExec) {
        HMODULE hModule = LoadLibraryA("ntdll.dll");
        g_pNtDelayExec = (NtDelayExecution)GetProcAddress(hModule, "NtDelayExecution");
    }
    large.QuadPart = -((LONGLONG)(ns * 10));
    (*g_pNtDelayExec)(TRUE, &large);
}

#else

void ztl_sleepns(int ns)
{
    struct timespec lSpec;
    lSpec.tv_sec  = 0;
    lSpec.tv_nsec = 1000 * ns;
    nanosleep(&lSpec, NULL);
}

#endif//_MSC_VER

int ztl_ymd(char* buf, time_t t)
{
    const size_t sz = sizeof("0000-00-00");

    struct tm ltm;
    if (t <= 0)
        t = time(NULL);
    LOCALTIME_S(&t, &ltm);
    return (int)strftime(buf, sz, DATE_FORMAT, &ltm);
}

// 20180102
int ztl_ymd0(char* buf, time_t t)
{
    const size_t sz = sizeof("00000000");

    struct tm ltm;
    if (t <= 0)
        t = time(NULL);
    LOCALTIME_S(&t, &ltm);
    return (int)strftime(buf, sz, "%Y%m%d", &ltm);
}

int ztl_hms(char* buf, time_t t)
{
    const size_t sz = sizeof(TIME_FORMAT);

    struct tm ltm;
    if (t <= 0)
        t = time(NULL);
    LOCALTIME_S(&t, &ltm);
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
    LOCALTIME_S(&t, &ltm);
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

int ztl_hms2int(const char* hms)
{
    // hmsf 20:13:46 -->> 201346
    return atoi(hms) * 10000 + atoi(hms + 3) * 100 + atoi(hms + 6);
}

int ztl_hmsf2int(const char* hmsf)
{
    // hmsf 20:13:46.500 -->> 201346500
    return atoi(hmsf) * 10000000 + atoi(hmsf + 3) * 100000 + 
        atoi(hmsf + 6) * 1000 + atoi(hmsf + 9);
}

int ztl_int2hms(char* hms, int len, int atime)
{
    // hms 201346 -->> 20:13:46
    len = snprintf(hms, len, "%02d:%02d:%02d", atime / 10000,
        (atime % 10000) / 100, atime % 100);
    return len;
}

int ztl_int2hmsf(char* hmsf, int len, int atime)
{
    // FIXME hms 201346500 -->> 20:13:46.500
    len = snprintf(hmsf, len, "%02d:%02d:%02d.%03d", atime / 10000000,
        (atime % 10000000) / 100000, (atime % 100000) / 1000, atime % 1000);
    return len;
}

int ztl_ymd_int(time_t atime)
{
    if (atime == 0)
        atime = time(0);

    // got 20180102
    struct tm ltm;
    LOCALTIME_S(&atime, &ltm);

    return ((ltm.tm_year + 1900) * 10000) + ((ltm.tm_mon + 1) * 100) + ltm.tm_mday;
}

int ztl_hms_int(time_t atime)
{
    if (atime == 0)
        atime = time(0);

    // got 201346 
    struct tm ltm;
    LOCALTIME_S(&atime, &ltm);

    return (ltm.tm_hour * 10000) + (ltm.tm_min * 100) + ltm.tm_sec;
}

int ztl_hmsf_int(time_t atime, int millisec)
{
    // got 201346500
    return ztl_hms_int(atime) * 1000 + millisec;
}

int64_t ztl_ymdhms_int(time_t atime)
{
    if (atime == 0)
        atime = time(0);

    struct tm ltm;
    LOCALTIME_S(&atime, &ltm);

    int64_t ldate = (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
    int64_t ltime = (ltm.tm_hour * 10000) + (ltm.tm_min * 100) + ltm.tm_sec;
    return ldate * 1000000 + ltime;
}

int64_t ztl_ymdhmsf_int(time_t atime, int millisec)
{
    if (atime == 0)
        atime = time(0);

    struct tm ltm;
    LOCALTIME_S(&atime, &ltm);

    int64_t ldate = (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
    int64_t ltime = (ltm.tm_hour * 10000) + (ltm.tm_min * 100) + ltm.tm_sec;
    return ldate * 1000000000 + ltime * 1000 + millisec;
}

void ztl_time_to_ymd_hhmmss(time_t t, int* pdate, int* ptime)
{
    if (t == 0)
        t = time(NULL);

    struct tm ltm;
    LOCALTIME_S(&t, &ltm);
    if (pdate)
        *pdate = (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
    if (ptime)
        *ptime = ltm.tm_hour * 10000 + ltm.tm_min * 100 + ltm.tm_sec;
}

time_t ztl_int_to_time(int date, int time)
{
    struct tm ltm = { 0 };

    if (date > 0)
    {
        ztl_tm_date_t zd;
        ztl_int_pdate(&zd, date);
        ltm.tm_year = zd.year - 1900;
        ltm.tm_mon = zd.month - 1;
        ltm.tm_mday = zd.day;
    }

    if (time > 0)
    {
        ztl_tm_time_t zt;
        ztl_int_ptime(&zt, time, 0);
        ltm.tm_hour = zt.hour;
        ltm.tm_min = zt.minute;
        ltm.tm_sec = zt.second;
    }

    return mktime(&ltm);
}

time_t ztl_str_to_time(const char* date, const char* time)
{
    struct tm ltm = { 0 };

    if (date && date[0])
    {
        ztl_tm_date_t td = { 0 };
        ztl_str_pdate(&td, date, (int)strlen(date));
        ltm.tm_year = td.year - 1900;
        ltm.tm_mon  = td.month - 1;
        ltm.tm_mday = td.day;
    }
    if (time && time[0])
    {
        ztl_tm_time_t tm = { 0 };
        ztl_str_ptime(&tm, time, (int)strlen(time));
        ltm.tm_hour = tm.hour;
        ltm.tm_min = tm.minute;
        ltm.tm_sec = tm.second;
    }
    return mktime(&ltm);
}

int ztl_times_to_dates(int dates[], int dsize, time_t times[], int tsize)
{
    int d = 0, t = 0;
    for (; d < dsize && t < tsize;)
    {
        dates[d++] = ztl_ymd_int(times[t++]);
    }
    return d;
}

time_t ztl_dates_to_times(time_t times[], int tsize, int dates[], int dsize)
{
    int d = 0, t = 0;
    for (; d < dsize && t < tsize;)
    {
        times[d++] = ztl_int_to_time(dates[t++], 0);
    }
    return d;
}

time_t ztl_int_dt_combine(int date, int time)
{
    struct tm ltm = { 0 };
    ltm.tm_year = date / 10000;
    ltm.tm_mon  = date / 100 % 100;
    ltm.tm_mday = date % 100;
    ltm.tm_hour = time / 10000;
    ltm.tm_min  = (time / 100) % 100;
    ltm.tm_sec  = time % 100;
    return mktime(&ltm);
}

time_t ztl_time_dt_combine(time_t tday, time_t ttime)
{
    struct tm dtm = { 0 };
    struct tm ttm = { 0 };
    LOCALTIME_S(&tday, &dtm);
    LOCALTIME_S(&tday, &ttm);
    dtm.tm_hour = ttm.tm_hour;
    dtm.tm_min = ttm.tm_min;
    dtm.tm_sec = ttm.tm_sec;
    return mktime(&dtm);
}

int ztl_str_ptime(ztl_tm_time_t* pt, const char* time_buf, int len)
{
    (void)len;
#if 0
    // 14:50:50.500
    ptm->tm_hour = atoi(buf);
    ptm->tm_min = atoi(buf + 3);
    ptm->tm_sec = atoi(buf + 6);
    // ptm->ms = atoi(buf + 9);
#else
    pt->hour   = (time_buf[0] - '0') * 10 + (time_buf[1] - '0');
    pt->minute = (time_buf[3] - '0') * 10 + (time_buf[4] - '0');
    pt->second = (time_buf[6] - '0') * 10 + (time_buf[7] - '0');

    if (len >= 10)
        return atoi(time_buf + 8);
#endif
    return 0;
}

int ztl_int_ptime(ztl_tm_time_t* pt, int time_int, int have_millisec)
{
    int hhmmss = time_int;
    if (have_millisec)
    {
        hhmmss /= 1000;
        have_millisec = time_int % 1000;
    }

    pt->hour = hhmmss / 10000;
    pt->minute = (hhmmss / 100) % 100;
    pt->second = hhmmss % 100;
    return have_millisec;
}

int ztl_str_pdate(ztl_tm_date_t* pd, const char* date_buf, int len)
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

int ztl_int_pdate(ztl_tm_date_t* pd, int32_t date_int)
{
    pd->year = date_int / 10000;
    pd->month = date_int / 100 % 100;
    pd->day = date_int % 100;
    return 0;
}

int ztl_intdt_to_tm(ztl_tm_dt_t* pdt, int32_t date_int, int32_t time_int, int have_millisec)
{
    ztl_int_pdate(&pdt->date, date_int);
    ztl_int_ptime(&pdt->time, time_int, have_millisec);
    return 0;
}

int ztl_i64_to_tmdt(ztl_tm_dt_t* pdt, int64_t i64dt)
{
    ztl_union_dt_t ud;
    ud.i64 = i64dt;
    pdt->date = ud.dt.date;
    pdt->time = ud.dt.time;
    return 0;
}

int64_t ztl_tmdt_to_i64(const ztl_tm_dt_t* pdt)
{
    ztl_union_dt_t ud;
    ud.dt.date = pdt->date;
    ud.dt.time = pdt->time;
    return ud.i64;
}

int ztl_get_wday(int date)
{
    struct tm ltm = { 0 };
    time_t t = ztl_int_to_time(date, 0);
    LOCALTIME_S(&t, &ltm);
    return ltm.tm_wday;
}

bool ztl_is_weekend(int date)
{
    int wday = ztl_get_wday(date);
    return wday == 0 || wday == 6;
}

int ztl_time_wday(time_t t)
{
    struct tm ltm;
    LOCALTIME_S(&t, &ltm);
    return ltm.tm_wday;
}

bool ztl_time_is_weekend(time_t t)
{
    int wday = ztl_time_wday(t);
    return wday == 0 || wday == 6;
}

int ztl_get_distance_date(int date, int offset)
{
    struct tm ltm = { 0 };
    if (offset == 0) {
        return date;
    }

    time_t t = ztl_int_to_time(date, 0);
    t += offset * SECONDS_PER_DATE;
    LOCALTIME_S(&t, &ltm);
    return (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
}

int ztl_get_prev_date(int date)
{
    struct tm ltm = { 0 };
    ltm.tm_year = date / 10000 - 1900;
    ltm.tm_mon  = date / 100 % 100 - 1;
    ltm.tm_mday = date % 100;

    ltm.tm_mday -= 1;
    if (ltm.tm_mday > 0) {
        return date - 1;
    }

    switch (ltm.tm_mon + 1)
    {
    case 1:
    {
        ltm.tm_year -= 1;
        ltm.tm_mon  = 12 - 1;
        ltm.tm_mday = 31;
        break;
    }
    case 3:
    {
        ltm.tm_mday += 1;
        time_t t = mktime(&ltm) - SECONDS_PER_DATE;
        LOCALTIME_S(&t, &ltm);
        break;
        // 
    }
    case 2:
    case 4:
    case 6:
    case 7:
    case 9:
    case 11:
    case 12:
    {
        ltm.tm_mon -= 1;
        ltm.tm_mday = 31;
        break;
    }
    default:
    {
        ltm.tm_mon -= 1;
        ltm.tm_mday = 30;
        break;
    }
    }//switch

    return (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
}

int ztl_get_next_date(int date)
{
    struct tm ltm = { 0 };
    ltm.tm_year = date / 10000 - 1900;
    ltm.tm_mon  = date / 100 % 100 - 1;
    ltm.tm_mday = date % 100;

    ltm.tm_mday += 1;
    if (ltm.tm_mday < 28) {
        return date + 1;
    }

    switch (ltm.tm_mon + 1)
    {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
    {
        if (ltm.tm_mday < 31)
            return date + 1;
        if (ltm.tm_mon == 11)
        {
            ltm.tm_year += 1;
            ltm.tm_mon = 0;
            ltm.tm_mday = 1;
        }
        else
        {
            ltm.tm_mon += 1;
            ltm.tm_mday = 1;
        }
        break;
    }
    case 4:
    case 6:
    case 9:
    case 11:
    {
        if (ltm.tm_mday < 30)
        {
            return date + 1;
        }
        else
        {
            ltm.tm_mon += 1;
            ltm.tm_mday = 1;
        }
        break;
    }
    case 2:
    {
        ltm.tm_mday -= 1;
        time_t t = mktime(&ltm) + SECONDS_PER_DATE;
        LOCALTIME_S(&t, &ltm);
        break;
    }
    }//switch

    return (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
}

int ztl_date_range(int dates[], int size, int start_date, int end_date, bool exclude_weekend)
{
    int i, n = 0, wday = 0;
    for (i = 0; i < size && start_date <= end_date; ++i)
    {
        if (exclude_weekend)
        {
            wday = ztl_get_wday(start_date);
            if (wday == 0 || wday == 6)
            {
                start_date = ztl_get_distance_date(start_date, 1);
                continue;
            }
        }

        dates[n++] = start_date;
        start_date = ztl_get_distance_date(start_date, 1);
    }
    return n;
}

int ztl_get_distance_time(int time, int offset)
{
    ztl_tm_time_t tm;
    ztl_int_ptime(&tm, time, 0);
    int hour = tm.hour;
    int minute = tm.minute;
    int second = tm.second;

    second += offset;
    if (second > 60)
    {
        minute += second / 60;
        second = second % 60;

        if (minute > 60)
        {
            hour += minute / 60;
            minute = minute % 60;

            if (hour > 24)
                hour = hour % 24;
        }
    }
    else if (second < 0)
    {
        // error
        minute += second / 60 - 1;
        second = 60 + second % 60;

        if (minute < 0)
        {
            hour += minute / 60 - 1;
            minute = 60 + minute % 60;

            if (hour < 0)
                hour = -hour % 24;
        }
    }
    return hour * 10000 + minute * 100 + second;
}

int ztl_minute_range(int minutes[], int size, int start_time, int end_time)
{
    int i = 0, n = 0;
    time_t t;
    struct tm ltm = { 0 };

    t = ztl_int_to_time(0, start_time);

    if (start_time > end_time)
    {
        int end_time_temp = 235900;
        for (;i < size && start_time <= end_time_temp; ++i)
        {
            LOCALTIME_S(&t, &ltm);
            minutes[n++] = ltm.tm_hour * 10000 + ltm.tm_min * 100 + ltm.tm_sec;
            t += 60;
        }

        start_time = 0;
        t = ztl_int_to_time(0, start_time);
    }

    for (;i < size && start_time <= end_time; ++i)
    {
        LOCALTIME_S(&t, &ltm);
        minutes[n++] = ltm.tm_hour * 10000 + ltm.tm_min * 100 + ltm.tm_sec;
        t += 60;
    }
    return n;
}

int ztl_difftime(int t1, int t2, int have_millisec)
{
    if (t1 == t2) {
        return 0;
    }

    ztl_tm_time_t tm1, tm2;
    int millisec1 = ztl_int_ptime(&tm1, t1, have_millisec);
    int millisec2 = ztl_int_ptime(&tm2, t2, have_millisec);

    int sec_diff = tm1.second - tm2.second;
    int min_diff = tm1.minute - tm2.minute;
    int hour_diff = tm1.hour - tm2.hour;

    int diff = hour_diff * 3600 + min_diff * 60 + sec_diff;
    if (have_millisec)
    {
        return diff * 1000 + (millisec1 - millisec2);
    }
    return diff;
}

int ztl_diffday(int startday, int endday, bool exclude_weekend)
{
    time_t t = time(NULL);
    struct tm ls, le;
    int res;

    memset(&ls, 0, sizeof(ls));
    memset(&le, 0, sizeof(le));

    ls.tm_mday = startday % 100;
    ls.tm_mon  = startday / 100 % 100 - 1;
    ls.tm_year = startday / 10000 - 1900;

    le.tm_mday = endday % 100;
    le.tm_mon  = endday / 100 % 100 - 1;
    le.tm_year = endday / 10000 - 1900;

    time_t te = mktime(&le);
    time_t ts = mktime(&ls);
    res = (int)difftime(te, ts) / SECONDS_PER_DATE;

    if (exclude_weekend)
    {
        LOCALTIME_S(&ts, &ls);
        LOCALTIME_S(&te, &le);

        if (le.tm_wday == 0)le.tm_wday = 7;
        if (ls.tm_wday == 0)ls.tm_wday = 7;

        int weknum = (res - le.tm_wday + 1 - (8 - ls.tm_wday)) / 7;
        return weknum * 5 + (6 - (ls.tm_wday <= 5 ? ls.tm_wday : 6)) + (le.tm_wday > 5 ? 5 : (le.tm_wday - 1));
    }

    return res;
}

int ztl_diffnow(int endday, bool exclude_weekend)
{
    time_t t;
    time(&t);
    int today = ztl_ymd_int(t);

    if (today < endday) 
    {
        return ztl_diffday(today, endday, exclude_weekend);
    }
    else if(today > endday)
    {
        return ztl_diffday(endday, today, exclude_weekend);
    }
    else
        return 0;
   
}
