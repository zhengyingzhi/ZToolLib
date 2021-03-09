/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 * an easily fast str
 */

#ifndef _ZTL_DSTR_H_INCLUDED_
#define _ZTL_DSTR_H_INCLUDED_

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/* exported types */
typedef char* dstr;

/* exported functions */
dstr   dstr_new_len(const char* str, size_t length);
dstr   dstr_new(const char* str);
void   dstr_free(dstr ds);
size_t dstr_length(const dstr ds);
size_t dstr_avail(const dstr ds);
dstr   dstr_make_room(dstr ds, size_t length);
void   dstr_incr_len(dstr ds, int incr);
dstr   dstr_remove_avail(dstr ds);
size_t dstr_alloc_size(dstr ds);
dstr   dstr_cat_len(dstr ds, const char* str, size_t length);
dstr   dstr_cat(dstr ds, const char* str);
dstr   dstr_cat_vprintf(dstr ds, const char* fmt, va_list ap);
dstr   dstr_cat_printf(dstr ds, const char* fmt, ...);
dstr   dstr_trim(dstr ds, const char* cset);
dstr   dstr_range(dstr ds, size_t start, size_t end);
void   dstr_clear(dstr ds);
dstr*  dstr_split_len(const char* str, size_t length, const char* sep, size_t seplength, int* count);
void   dstr_free_tokens(dstr* tokens, int count);
dstr*  dstr_split_args(const char* line, int* argc);
void   dstr_free_args(dstr* argv, int argc);



#ifdef __cplusplus
}
#endif

#endif//_ZTL_DSTR_H_INCLUDED_
