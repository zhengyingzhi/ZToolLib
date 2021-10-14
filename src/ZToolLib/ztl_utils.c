
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <math.h>

#include "ztl_times.h"
#include "ztl_utils.h"

#ifdef _MSC_VER
#include <Windows.h>
#include <process.h>
#include <sys/stat.h>

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#else
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <pthread.h>
#endif//_MSC_VER

/// high perf counter
int64_t query_tick_count()
{
#ifdef _MSC_VER
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
#endif//_MSC_VER
}

int32_t tick_to_us(int64_t tick_beg, int64_t tick_end)
{
#ifdef _MSC_VER
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    if (tick_end == 0)
        return (int32_t)((double)tick_beg / (double)(freq.QuadPart) * 1000000);
    else
        return (int32_t)((double)(tick_end - tick_beg) / (double)(freq.QuadPart) * 1000000);
#else
    if (tick_end == 0)
        return tick_beg;
    return tick_end - tick_beg;
#endif//_MSC_VER
}


/* Return the number of digits of 'v' when converted to string in radix 10.
* See ll2string() for more information. */
uint32_t digits10(uint64_t v)
{
    if (v < 10) return 1;
    if (v < 100) return 2;
    if (v < 1000) return 3;
    if (v < 1000000000000UL) {
        if (v < 100000000UL) {
            if (v < 1000000) {
                if (v < 10000) return 4;
                return 5 + (v >= 100000);
            }
            return 7 + (v >= 10000000UL);
        }
        if (v < 10000000000UL) {
            return 9 + (v >= 1000000000UL);
        }
        return 11 + (v >= 100000000000UL);
    }
    return 12 + digits10(v / 1000000000000UL);
}

int ll2string(char* dst, uint32_t dstlen, int64_t svalue)
{
    static const char digits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    int negative;
    unsigned long long value;

    /* The main loop works with 64bit unsigned integers for simplicity, so
    * we convert the number here and remember if it is negative. */
    /* 在这里做正负号的判断处理 */
    if (svalue < 0) {
        if (svalue != LLONG_MIN) {
            value = -svalue;
        }
        else {
            value = ((unsigned long long) LLONG_MAX) + 1;
        }
        negative = 1;
    }
    else {
        value = svalue;
        negative = 0;
    }

    /* Check length. */
    uint32_t const length = digits10(value) + negative;
    if (length >= dstlen) return 0;

    /* Null term. */
    uint32_t next = length;
    dst[next] = '\0';
    next--;
    while (value >= 100) {
        //做值的换算  
        int const i = (value % 100) << 1;
        value /= 100;
        //i所代表的余数值用digits字符数组中的对应数字代替了  
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
        next -= 2;
    }

    /* Handle last 1-2 digits. */
    if (value < 10) {
        dst[next] = '0' + (uint32_t)value;
    }
    else {
        int i = (uint32_t)value << 1;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }

    /* Add sign. */
    if (negative) dst[0] = '-';
    return length;
}

int64_t atoi_n(const char* data, int len)
{
    int64_t val = 0;
    int isigned = 1;
    int i = 0;
    while (i++ < len) {
        if (*data == ' ') {
            ++data;
            continue;
        }
        break;
    }

    if (*data == '+') {
        isigned = 1;
        ++i;
        ++data;
    }
    else if (*data == '-') {
        isigned = -1;
        ++i;
        ++data;
    }

    while (i++ <= len) {
        if (!isdigit(*data))
            break;

        val = (val * 10) + (*data - '0');
        ++data;
    }
    return val * isigned;
}

void print_mem(void* pm, unsigned int size, int nperline)
{
    FILE* fp = stdout;
    if (nperline < 4) nperline = 4;

    fprintf(fp, "base[%p]:\n", pm);
    unsigned char* pc = (unsigned char*)pm;
    unsigned int i = 0;
    while (i < size)
    {
        if (i % nperline == 0)
            fprintf(fp, "[%04d]->", i);
        fprintf(fp, "%02X", *pc++);

        ++i;
        if (i % 4 == 0)
            fprintf(fp, " ");
        if (i % nperline == 0)
            fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}

#if 0
void random_string(char* buf, int size, bool onlyhexchar)
{
    //srand((int)time(NULL) ^ get_proc_id());
    (void)onlyhexchar;

    int i, x;
    for (i = 0; i < size; )
    {
        x = rand() % 'z';
        if ('0' <= x && '9' >= x) {
            buf[i++] = x;
        }
        else if ('a' <= x && 'z' >= x) {
            buf[i++] = x;
        }
        else if ('A' <= x && 'Z' >= x) {
            buf[i++] = x;
        }
        else {
            continue;
        }
    }
}
#else
void random_string(char* buf, int size, bool onlyhexchar)
{
    const char* charset;
    int lAnd;
    if (onlyhexchar) {
        charset = "0123456789abcdef";
        lAnd = 0x0F;
    }
    else {
        charset = "0abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ9";
        lAnd = 0x3F;
    }

    struct timeval ltv;
    gettimeofday(&ltv, NULL);

#ifdef _MSC_VER
    int   lpid = GetCurrentProcessId();
#else
    int   lpid = getpid();
#endif//_MSC_VER
    uint32_t llen = size;
    char* pbuf = buf;

    // Use time and PID to fill the initial array
    if (llen >= sizeof(ltv.tv_usec)) {
        memcpy(pbuf, &ltv.tv_usec, sizeof(ltv.tv_usec));
        llen -= sizeof(ltv.tv_usec);
        pbuf += sizeof(ltv.tv_usec);
    }
    if (llen >= sizeof(ltv.tv_sec)) {
        memcpy(pbuf, &ltv.tv_sec, sizeof(ltv.tv_sec));
        llen -= sizeof(ltv.tv_sec);
        pbuf += sizeof(ltv.tv_sec);
    }
    if (llen >= sizeof(lpid)) {
        memcpy(pbuf, &lpid, sizeof(lpid));
        llen -= sizeof(lpid);
        pbuf += sizeof(lpid);
    }


    // Finally xor it with rand() output, that was already seeded with time at startup
    for (int i = 0; i < size; i++) {
        buf[i] ^= rand();
        buf[i] = charset[buf[i] & lAnd];
    }
}
#endif//0

void lefttrim(char* buf)
{
    if (*buf == '\0')
        return;

    int i;
    int len = (int)strlen(buf);
    for (i = 0; i < len; ++i) {
        if (buf[i] != ' ' && buf[i] != '\r' && buf[i] != '\n' && buf[i] != '\t')
            break;
    }
    if (i > 0)
        strcpy(buf, buf + i);
    return;
}

void righttrim(char* buf)
{
    if (*buf == '\0')
        return;

    int i;
    for (i = (int)strlen(buf) - 1; i >= 0; --i) {
        if (buf[i] != ' ' && buf[i] != '\r' && buf[i] != '\n' && buf[i] != '\t')
            break;
        buf[i] = '\0';
    }
    return;
}

char* remove_char(char* buf, char ch)
{
    char* pcur = buf;
    int w = 0, r = 0;
    for (; buf[r]; ++r)
    {
        if (buf[r] != ch) {
            buf[w++] = buf[r];
        }
    }
    buf[w] = '\0';
    return buf;
}

char* replace_char(char* buf, char old_ch, char new_ch)
{
    char* pcur = buf;
    int w;
    for (w = 0; buf[w]; ++w)
    {
        if (buf[w] == old_ch) {
            buf[w] = new_ch;
        }
    }
    return buf;
}

int64_t parse_size(const char* str, int len)
{
    uint8_t  unit;
    int64_t  size, scale, max;

    unit = str[len - 1];

    switch (unit)
    {
    case 'K':
    case 'k':
        len--;
        max = INT64_MAX / 1024;
        scale = 1024;
        break;

    case 'M':
    case 'm':
        len--;
        max = INT64_MAX / (1024 * 1024);
        scale = 1024 * 1024;
        break;

    case 'G':
    case 'g':
        len--;
        max = INT64_MAX / (1024 * 1024 * 1024);
        scale = 1024 * 1024 * 1024;
        break;

    default:
        max = INT64_MAX;
        scale = 1;
    }

    size = atoi_n(str, len);
    if (size == 0 || size > max) {
        return 0;
    }

    size *= scale;

    return size;
}

uint64_t get_next_power(uint64_t n)
{
    uint64_t i = 2;
    for (;;)
    {
        if (i >= n)
            return i;
        i *= 2;
    }
    return i;
}

int get_cpu_number()
{
    static int ncpu = -1;
    if (ncpu < 0)
    {
#ifdef _WIN32
        SYSTEM_INFO siSys;
        GetSystemInfo(&siSys);
        ncpu = siSys.dwNumberOfProcessors;
#else
        ncpu = sysconf(_SC_NPROCESSORS_CONF);
#endif//_WIN32
    }
    return ncpu;
}

uint32_t get_file_length(const char* filename)
{
    FILE* fp;
    uint32_t sz;

    sz = 0;
    fp = fopen(filename, "r");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        sz = ftell(fp);
        fclose(fp);
    }
    return sz;
}

const char* ztl_basename(const char* filepath)
{
    if (!filepath || !filepath[0])
        return NULL;

    int len0 = (int)strlen(filepath);
    int len = len0;
    while (len > 0)
    {
        char ch = filepath[len - 1];
        if (ch == '/' || ch == '\\')
            break;
        len -= 1;
    }
    return filepath + len;
}

int ztl_dirname(char dirname[], int size, const char* filepath)
{
    if (!dirname || size <= 0 || !filepath || !filepath[0])
        return -1;

    int len = (int)strlen(filepath);
    while (len > 0)
    {
        char ch = filepath[len - 1];
        if (ch == '/' || ch == '\\')
        {
            len -= 1;
            while (len > 0)
            {
                ch = filepath[len - 1];
                if (ch == '/' || ch == '\\')
                    len -= 1;
                else
                    break;
            }
            break;
        }
        len -= 1;
    }

    size = ztl_min(size, len);
    strncpy(dirname, filepath, size);
    dirname[size] = '\0';
    return size;
}


bool ztl_startwith(const char* str, const char* needle)
{
    return strncmp(str, needle, strlen(needle)) == 0 ? true : false;
}

bool ztl_endwith(const char* str, const char* needle)
{
    size_t len0 = strlen(str);
    size_t len1 = strlen(needle);
    if (len0 < len1)
        return false;
    return strncmp(str + len0 - len1, needle, len1) == 0 ? true : false;
}

bool ztl_startswith(const char* str, char* needles[], int count)
{
    for (int i = 0; i < count; ++i)
    {
        if (!needles[i])
            break;
        if (strncmp(str, needles[i], strlen(needles[i])) == 0)
            return true;
    }
    return false;
}

bool ztl_endswith(const char* str, char* needles[], int count)
{
    size_t len0 = strlen(str);
    for (int i = 0; i < count; ++i)
    {
        if (!needles[i])
            break;

        size_t len1 = strlen(needles[i]);
        if (len1 > len0)
            continue;

        if (strncmp(str + len0 - len1, needles[i], len1) == 0)
            return true;
    }
    return false;
}


static void print_array(int arr[], int size)
{
    for (int i = 0; i < size; ++i)
    {
        printf("%d ", arr[i]);
    }
    printf("\n");
}

int str_delimiter(char* src, char** arr, int arr_size, char delimiter)
{
    if (!src) {
        return 0;
    }

    char* pur = src;
    int n = 0;
    while (n < arr_size)
    {
        arr[n++] = pur;
        pur = strchr(pur, delimiter);
        if (!pur) {
            break;
        }

        *pur++ = 0x00;
    }
    return n;
}

int str_delimiters(char* src, char** arr, int arr_size, const char* sep)
{
    if (!src) {
        return 0;
    }

    int sep_len = (int)strlen(sep);
    char* pcur = src;
    int n = 0;
    while (n < arr_size)
    {
        arr[n++] = pcur;
        pcur = strstr(pcur, sep);
        if (!pcur) {
            break;
        }

        *pcur = 0x00;
        pcur += sep_len;
    }
    return n;
}

int str_delimiter_ex(const char* src, int length, zditem_t* ret_arr, int arr_size, const char* sep)
{
    int sep_len = (int)strlen(sep);
    int index = 0;
    const char* sentinel = src + length;
    while (index < arr_size)
    {
        char* end;
        if (sep_len == 1)
            end = strchr(src, *sep);
        else
            end = strstr(src, sep);

        if (end == NULL || end >= sentinel)
        {
            // last item
            if (sentinel - src > 0)
            {
                ret_arr[index].ptr = (char*)src;
                ret_arr[index].len = (int)(sentinel - src);
                index++;
            }
            break;
        }

        ret_arr[index].ptr = (char*)src;
        ret_arr[index].len = (int)(end - src);
        index++;

        // next pos
        src = end + sep_len;
    }

    return index;
}

int read_number_from_file(const char* filename)
{
    int lValue = -1;
    FILE* fp;
    fp = fopen(filename, "r");
    if (fp) {
        (void)fscanf(fp, "%d", &lValue);
        fclose(fp);
    }
    return lValue;
}

int read_file_content(const char* filename, char buf[], int size)
{
    int lValue = -1;
    FILE* fp;
    fp = fopen(filename, "r");
    if (fp)
    {
        lValue = (int)fread(buf, size, 1, fp);
        fclose(fp);
    }
    return lValue;
}

int binary_search(int arr[], int size, int val)
{
    //print_array(arr, size);
    int low = 0, middle = 0, high = size;
    while (low < high)
    {
        middle = (low + high) >> 1;
        if (val == arr[middle]) {
            return middle;
        }
        else if (val < arr[middle]) {
            high = middle;
        }
        else {
            low = middle + 1;
        }
    }
    return -1;
}

int binary_search_i64(int64_t arr[], int size, int64_t val)
{
    int low = 0, middle = 0, high = size;
    while (low < high)
    {
        middle = (low + high) >> 1;
        if (val == arr[middle]) {
            return middle;
        }
        else if (val < arr[middle]) {
            high = middle;
        }
        else {
            low = middle + 1;
        }
    }
    return -1;
}

int binary_search_dbl(double arr[], int size, double val)
{
    int low = 0, middle = 0, high = size;
    while (low < high)
    {
        middle = (low + high) >> 1;
        if (DBL_EQ(val, arr[middle], DBL_EPSILON_E6)) {
            return middle;
        }
        else if (val < arr[middle]) {
            high = middle;
        }
        else {
            low = middle + 1;
        }
    }
    return -1;
}

#if 0//implement by cursive
int binary_search2(int arr[], int low, int high, int val)
{
    int middle = (low + high) >> 1;
    if (low > high) {
        return -1;
    }

    if (val == arr[middle]) {
        return middle;
    }
    if (val < arr[middle]) {
        return binary_search2(arr, low, middle - 1, val);
    }
    if (val > arr[middle]) {
        return binary_search2(arr, middle + 1, high, val);
    }
}
#endif

uint32_t ztl_randseed()
{
    uint32_t lSeed;
#ifdef _MSC_VER
    DWORD64 x = __rdtsc();
    lSeed = (uint32_t)x + (uint32_t)(x >> 32);
#else
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
    lSeed = lo + hi;
#endif

    return lSeed;
}

uint32_t ztl_rand(uint32_t* pseed)
{
    *pseed = (214013 * (*pseed) + 2531011);
    return (*pseed >> 16) & 0x7FFF;
}

double ztl_round(double x, int precision)
{
    double m;
    switch (precision)
    {
    case 0:     m = 1;              break;
    case 1:     m = 10;             break;
    case 2:     m = 100;            break;
    case 3:     m = 1000;           break;
    case 4:     m = 10000;          break;
    case 5:     m = 100000;         break;
    case 6:     m = 1000000;        break;
    case 7:     m = 10000000;       break;
    case 8:     m = 100000000;      break;
    case 9:     m = 1000000000;     break;
    case 10:    m = 10000000000;    break;
    case 11:    m = 100000000000;   break;
    case 12:    m = 1000000000000;  break;
    default:
        return x;
    }

    return round(x * m) / m;
}
