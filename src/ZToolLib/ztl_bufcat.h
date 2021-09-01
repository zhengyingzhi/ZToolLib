/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 * a simple string cat utils
 */

#ifndef _ZTL_BUFCAT_H_
#define _ZTL_BUFCAT_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* exported types */
typedef struct bufcat_s
{
    char*   buf;
    size_t  len;
    size_t  capicity;
    int     alloced;
    int     sep_len;
    const char* sep;
}bufcat_t;

/* Init sc by buf & capicity */
void bufcat_init(bufcat_t* bc, char buf[], int capicity, const char* sep);
void bufcat_free(bufcat_t* bc);

int  bufcat_str_len(bufcat_t* bc, const char* str, int len);
int  bufcat_str(bufcat_t* bc, const char* str);
int  bufcat_int(bufcat_t* bc, int val);
int  bufcat_int64(bufcat_t* bc, int64_t val);
int  bufcat_double(bufcat_t* bc, double val, int precision);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_BUFCAT_H_
