/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_UTILS_H_
#define _ZTL_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#define DOUBLE_ZERO             double(1E-307)
#define IS_DOUBLE_ZERO(D)       ((D) <= DOUBLE_ZERO && (D) >= -DOUBLE_ZERO)
#define STR_ZERO_DOUBLE         "0.0000000000000"

#define ztl_align(d,align)      (((d) + ((align) - 1)) & ~((align) - 1))
#define ztl_align_ptr(p, a)     (uint8_t*) (((uintptr_t) (p) + ((uintptr_t)(a) - 1)) & ~((uintptr_t)(a) - 1))

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


typedef union {
    int64_t     i64;
    double      f64;
    void*       ptr;    // FIXME 4 bytes on 32bit system
    int32_t     i32;
    float       f32;
}union_dtype_t;

/// high perf counter at micro-second
int64_t query_tick_count();
int32_t tick_to_us(int64_t tick_beg, int64_t tick_end);


/// convert a long long value to string, return length
int ll2string(char* dst, uint32_t dstlen, int64_t value);

/// convert previous len data to an integer
int64_t atoi_n(const char* data, int len);

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

/// get file size
uint32_t get_file_length(const char* filename);

/// parse the string ptr into the array by the delemiter charactor like '|', return array size 
int str_delimiter(char* apSrc, char** apRetArr, int aArrSize, char aDelimiter);

typedef struct {
    char* ptr;
    int   len;
}zditem_t;
int str_delimiter_ex(const char* src, int length, zditem_t* retArr, int arrSize, const char* sep);


/// read an integer number from file
int read_number_from_file(const char* filename);

/// read file content
int read_file_content(const char* filename, char buf[], int size);

/// binary search, return the index, return -1 if not find
int binary_search(int arr[], int size, int val);

/// a simple password change algorithm
char* zpassword_change(char* apdata);

/// random related
uint32_t ztl_randseed();
uint32_t ztl_rand(uint32_t* pseed);


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
        case 6: {                                                   \
            *(uint32_t*)dst     = *(uint32_t*)src;                  \
            *(uint16_t*)((char*)dst+4) = *(uint16_t*)((char*)src+4); } \
            break;                                                  \
        case 12: {                                                  \
            *(uint64_t*)dst     = *(uint64_t*)src;                  \
            *(uint32_t*)((char*)dst+8) = *(uint32_t*)((char*)src+8); } \
            break;                                                  \
        case 16: {                                                  \
            *(uint64_t*)dst     = *(uint64_t*)src;                  \
            *(uint64_t*)((char*)dst+8) = *(uint64_t*)((char*)src+8); } \
            break;                                                  \
        case 20: {                                                  \
            *(uint64_t*)dst     = *(uint64_t*)src;                  \
            *(uint64_t*)((char*)dst+8) = *(uint64_t*)((char*)src+8);\
            *(uint32_t*)((char*)dst+16) = *(uint32_t*)((char*)src+16); } \
            break;                                                  \
        case 24: {                                                  \
            *(uint64_t*)dst     = *(uint64_t*)src;                  \
            *(uint64_t*)((char*)dst+8) = *(uint64_t*)((char*)src+8);\
            *(uint64_t*)((char*)dst+16) = *(uint64_t*)((char*)src+16); } \
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
