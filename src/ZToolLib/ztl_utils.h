#ifndef _ZTL_UTILS_H_
#define _ZTL_UTILS_H_

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#define DOUBLE_ZERO             double(1E-307)
#define IS_DOUBLE_ZERO(D)       ((D) <= DOUBLE_ZERO && (D) >= -DOUBLE_ZERO)
#define STR_ZERO_DOUBLE         "0.0000000000000"

#define DBL_EPSILON_E6          (1.0e-6)
#define DBL_EPSILON_E8          (1.0e-8)
#define DBL_EPSILON_E12         (1.0e-12)

#define DBL_EQ(d1, d2, ep)      (fabs((d1) - (d2)) < (ep))
#define DBL_GE(d1, d2, ep)      ((d1) >= ((d2) - (ep)))
#define DBL_GT(d1, d2, ep)      ((d1) >= ((d2) + (ep)))

#define ztl_align(d,align)      (((d) + ((align) - 1)) & ~((align) - 1))
#define ztl_align_ptr(p, a)     (uint8_t*) (((uintptr_t) (p) + ((uintptr_t)(a) - 1)) & ~((uintptr_t)(a) - 1))

#define ztl_min(a,b)            (a) < (b) ? (a) : (b)
#define ztl_max(a,b)            (a) > (b) ? (a) : (b)
#define ztl_new_val(p,type,val) {(p) = (type*)malloc(sizeof(type)); *((type*)(p))=val;}
#define _atoi_2(d)              (((d)[0] - '0') * 10 + (d)[1] - '0')



#ifdef __cplusplus
#define ztl_inline inline
extern "C" {
#else
#define ztl_inline 
#endif//__cplusplus

#ifdef _MSC_VER
#define ZTL_I64_FMT     "%lld"
#define ztl_stricmp     _stricmp

#else /* linux platfrom */

#define ZTL_I64_FMT     "%ld"
// static void DebugBreak() {}
#define ztl_stricmp     strcasecmp

#endif//_MSC_VER

#if defined(_DEBUG) || defined(DEBUG)
#define ztl_assert(condition)  do { if(!(condition)){ \
    fprintf(stderr, "ZTL Assertion failed: %s @ %s::%s (%d)\n", \
    #condition , __FILE__, __FUNCTION__, __LINE__); DebugBreak();} } while(0)
#else
#define ztl_assert(condition) 
#endif//DEBUG

#if defined(_WIN64) || defined(__x86_64__)
#define ZTL_X64 1
#define ZTL_X86 0
#else
#define ZTL_X64 0
#define ZTL_X86 1
#endif//X64


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

/// int len
uint32_t digits10(uint64_t v);

/// convert a long long value to string, return length
int ll2string(char* dst, uint32_t dstlen, int64_t value);

/// convert previous len data to an integer
int64_t atoi_n(const char* data, int len);

/// print memory by hex
void print_mem(void* pm, unsigned int size, int nperline);

/// generate random string
void random_string(char* buf, int size, bool onlyhexchar);

/// trim left/right blank chars
void  lefttrim(char* buf);
void  righttrim(char* buf);
char* remove_char(char* buf, char ch);
char* replace_char(char* buf, char old_ch, char new_ch);

/// parse string within k,K,m,M to numeric, 2K ->> 2048
int64_t parse_size(const char* str, int len);

/// get next power number of 2
uint64_t get_next_power(uint64_t n);

/// get cpu core number
int get_cpu_number();

/// get file size
uint32_t get_file_length(const char* filename);

/// get basename and dirname
const char* ztl_basename(const char* filepath);
int ztl_dirname(char dirname[], int size, const char* filepath);

/// parse the string ptr into the array by the delemiter charactor like '|', return array size 
int str_delimiter(char* src, char** arr, int arr_size, char delimiter);
int str_delimiters(char* src, char** arr, int arr_size, const char* sep);

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
int binary_search_i64(int64_t arr[], int size, int64_t val);
int binary_search_dbl(double arr[], int size, double val);

/// random related
uint32_t ztl_randseed();
uint32_t ztl_rand(uint32_t* pseed);

/// round a double val, like round(1.2346, 3) -->> 1.2350
double ztl_round(double x, int precision);


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
#define _UNIT_COPY(type,dst,src)    *((type*)(dst)) = *((type*)(src))
#define ztlncpy(dst,src,size)                                           \
    {                                                                   \
        switch (size) {                                                 \
        case 1: _UNIT_COPY(uint8_t,  dst, src);     break;              \
        case 2: _UNIT_COPY(uint16_t, dst, src);     break;              \
        case 4: _UNIT_COPY(uint32_t, dst, src);     break;              \
        case 8: _UNIT_COPY(uint64_t, dst, src);     break;              \
        case 6: {                                                       \
            _UNIT_COPY(uint32_t, dst, src);                             \
            _UNIT_COPY(uint16_t, ((char*)dst+4), ((char*)src+4)); }     \
            break;                                                      \
        case 12: {                                                      \
            _UNIT_COPY(uint64_t, dst, src);                             \
            _UNIT_COPY(uint32_t, ((char*)dst+8), ((char*)src+8)); }     \
            break;                                                      \
        case 16: {                                                      \
            _UNIT_COPY(uint64_t, dst, src);                             \
            _UNIT_COPY(uint64_t, ((char*)dst+8), ((char*)src+8)); }     \
            break;                                                      \
        case 20: {                                                      \
            _UNIT_COPY(uint64_t, dst, src);                             \
            _UNIT_COPY(uint64_t, ((char*)dst+8), ((char*)src+8));       \
            _UNIT_COPY(uint32_t, ((char*)dst+16), ((char*)src+16));}    \
            break;                                                      \
        case 24: {                                                      \
            _UNIT_COPY(uint64_t, dst, src);                             \
            _UNIT_COPY(uint64_t, ((char*)dst+8), ((char*)src+8));       \
            _UNIT_COPY(uint64_t, ((char*)dst+16), ((char*)src+16));}    \
            break;                                                      \
        default:                                                        \
            memcpy(dst,src,size);  break;                               \
        }                                                               \
    }


#define STR2LOWER(str) \
    do { \
        char *p = str; \
        while (*p) { \
            *p = tolower(*p); \
            ++p; \
        } \
    } while (0);

#define STR2UPPER(str) \
    do { \
        char *p = str; \
        while (*p) { \
            *p = toupper(*p); \
            ++p; \
        } \
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
