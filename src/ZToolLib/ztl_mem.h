/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_MEM_H_INCLUDED_
#define _ZTL_MEM_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/* exported functions */
void* mem_alloc(size_t nbytes, const char* file, int line);
void* mem_calloc(size_t count, size_t nbytes, const char* file, int line);
void* mem_posixalign(size_t alignment, size_t nbytes, const char* file, int line);
void  mem_free(void* p, const char* file, int line);
void* mem_resize(void* p, size_t nbytes, const char* file, int line);
char* mem_strndup(const char* str, size_t length);
char* mem_strdup(const char* str);

    /* exported macros */
#define ALLOC(nbytes) mem_alloc(nbytes, __FILE__, __LINE__)
#define CALLOC(count, nbytes) mem_calloc(count, nbytes, __FILE__, __LINE__)
#define POSIXALIGN(alignment, nbytes) mem_posixalign(alignment, nbytes, __FILE__, __LINE__)
#define NEW(p) (p = ALLOC(sizeof *(p)))
#define NEW0(p) (p = CALLOC(1, sizeof *(p)))
#define FREE(p) ((void)(mem_free(p, __FILE__, __LINE__), p = 0))
#define RESIZE(p, nbytes) (p = mem_resize(p, nbytes, __FILE__, __LINE__))


#ifdef __cplusplus
}
#endif

#endif//_ZTL_MEM_H_INCLUDED_
