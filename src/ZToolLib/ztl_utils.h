/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_UTILS_H_
#define _ZTL_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#define DOUBLE_ZERO             double(1E-307)
#define IS_DOUBLE_ZERO(D)       (D <= DOUBLE_ZERO && D >= -DOUBLE_ZERO)
#define STR_ZERO_DOUBLE         "0.0000000000000"

#define ztl_align(d,align)      (((d) + (align - 1)) & ~(align - 1))
#define ztl_align_ptr(p, a)     (uint8_t*) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define ztl_min(a,b)            (a) < (b) ? (a) : (b)
#define ztl_max(a,b)            (a) > (b) ? (a) : (b)

#ifdef __cplusplus
#define ztl_inline inline
extern "C" {
#else
#define ztl_inline 
#endif//__cplusplus

#ifndef _MSC_VER
static void DebugBreak() {}
#endif//_MSC_VER

#if defined(_DEBUG) || defined(DEBUG)
#define ztl_assert(condition)  do { if(!(condition)){ \
    fprintf(stderr, "ZTL Assertion failed: %s @ %s::%s (%d)\n", \
    #condition , __FILE__, __FUNCTION__, __LINE__); DebugBreak();} } while(0)
#else
#define ztl_assert(condition) 
#endif//DEBUG


/// high perf counter at micro-second
int64_t query_tick_count();
int32_t tick_to_us(int64_t aTickCountBeg, int64_t aTickCountEnd);

/// precision to milli-second
int64_t get_timestamp();


/// get current date, return length, fmtDelimiter like '-', '/', return length
int current_date(char* chDate, int nLen, char fmtDelimiter);

/// get current time, like 14:23:50.187940, return length
int current_time(char* chTime, int nLen, bool bMicroSec);

/// get current date time like 2017/12/12 14:23:50.187940, return length
int cur_date_time(char* chBuf, int nLen, char fmtDelimiter, bool bMicroSec);

/// get time of day
#ifdef _WIN32
void gettimeofday(struct timeval *tp, void* reserve);
#else
#include <sys/time.h>
#endif//_WIN32

/// convert a long long value to string, return length
int ll2string(char* dst, uint32_t dstlen, int64_t value);

/// convert previous len data to an integer
int64_t atoi_n(const char* pszData, int len);

/// print memory by hex
void print_mem(void* pm, unsigned int size, int nperline);

/// generate random string
void random_string(char* buf, int size, bool onlyhexchar);

/// trim left/right blank chars
void lefttrim(char* buf);
void righttrim(char* buf);

/// parse string within k,K,m,M to numeric, 2K ->> 2048
int64_t parse_size(const char* str, int len);

/// get cpu core number
int get_cpu_number();

/// parse the string ptr into the array by the delemiter charactor like '|', return array size 
int str_delimiter(char* apSrc, char** apRetArr, int aArrSize, char aDelimiter);

/// read an integer number from file
int read_number_from_file(const char* apfile);

/// binary search, return the index, return -1 if not find
int binary_search(int arr[], int size, int val);

/// a simple password change algorithm
char* zpassword_change(char* apdata);


union zudi {
    double f;
    uint64_t i;
};
#define encode_double(src,dst) do { \
    union zudi lud; \
    lud.f = src;    \
    dst = lud.i;    \
    } while (0);

#define decode_double(src,dst) do { \
    union zudi lud; \
    lud.i = src;    \
    dst = lud.f;    \
    } while (0);


/* a fixed size fast memory copy, carefully use!
 */
#define ztlncpy(dst,src,size)                                       \
    do {                                                            \
        switch (size) {                                             \
        case 1: *(uint8_t*) dst = *(uint8_t*)src;   break;          \
        case 2: *(uint16_t*)dst = *(uint16_t*)src;  break;          \
        case 4: *(uint32_t*)dst = *(uint32_t*)src;  break;          \
        case 8: *(uint64_t*)dst = *(uint64_t*)src;  break;          \
        case 12:                                                    \
            *(uint64_t*)dst     = *(uint64_t*)src;                  \
            *(uint32_t*)((char*)dst+8) = *(uint32_t*)((char*)src+8);\
            break;                                                  \
        case 16:                                                    \
            *(uint64_t*)dst     = *(uint64_t*)src;                  \
            *(uint64_t*)((char*)dst+8) = *(uint64_t*)((char*)src+8);\
            break;                                                  \
        default:                                                    \
            memcpy(dst,src,size);                                   \
        }                                                           \
    } while (0);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
template<typename T>
void zswap(T& ax, T& ay)
{
    T lt = ax;
    ax = ay;
    ay = lt;
}
#else
#define zswap(x,y)  \
    do {\
        x = x ^ y;  \
        y = x ^ y;  \
        x = x ^ y;  \
    } while (0);
#endif//__cplusplus

#endif//_ZTL_UTILS_H_
