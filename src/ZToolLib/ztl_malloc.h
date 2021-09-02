/* zmalloc - total amount of allocated memory aware version of malloc()
 *
 * Copyright (c) 2009-2010, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ZTL_MALLOC_H_INCLUDE_
#define _ZTL_MALLOC_H_INCLUDE_

#include <stdio.h>
#include <stdint.h>

/* Double expansion needed for stringification of macro values. */
#define __xstr(s)   __str(s)
#define __str(s)    #s

#if defined(ZTL_USE_TCMALLOC)
#define ZTL_MALLOC_LIB ("tcmalloc-" __xstr(TC_VERSION_MAJOR) "." __xstr(TC_VERSION_MINOR))
#include <google/tcmalloc.h>
#if (TC_VERSION_MAJOR == 1 && TC_VERSION_MINOR >= 6) || (TC_VERSION_MAJOR > 1)
#define ZTL_HAVE_MALLOC_SIZE 1
#define ztl_malloc_size(p) tc_malloc_size(p)
#else
#error "Newer version of tcmalloc required"
#endif//ZTL_USE_TCMALLOC

#elif defined(ZTL_USE_JEMALLOC)
#define ZTL_MALLOC_LIB ("jemalloc-" __xstr(JEMALLOC_VERSION_MAJOR) "." __xstr(JEMALLOC_VERSION_MINOR) "." __xstr(JEMALLOC_VERSION_BUGFIX))
#include <jemalloc/jemalloc.h>
#if (JEMALLOC_VERSION_MAJOR == 2 && JEMALLOC_VERSION_MINOR >= 1) || (JEMALLOC_VERSION_MAJOR > 2)
#define ZTL_HAVE_MALLOC_SIZE 1
#define ztl_malloc_size(p) je_malloc_usable_size(p)
#else
#error "Newer version of jemalloc required"
#endif//ZTL_USE_JEMALLOC

#elif defined(__APPLE__)
#include <malloc/malloc.h>
#define ZTL_HAVE_MALLOC_SIZE 1
#define ztl_malloc_size(p) malloc_size(p)

#elif defined(ZTL_USE_DLMALLOC)
#include <dlmalloc.h>
#define ZTL_MALLOC_LIB ("dlmalloc-" __xstr(2) "." __xstr(8) )
#define ZTL_HAVE_MALLOC_SIZE 1
#define ztl_malloc_size(p)  g_msize(p)
#endif

#ifndef ZTL_MALLOC_LIB
#define ZTL_MALLOC_LIB "libc"
#endif


void* ztl_malloc(size_t size);
void* ztl_calloc(size_t size);
void* ztl_realloc(void* ptr, size_t size);
void  ztl_free(void* ptr);
char* ztl_strdup(const char* str);
char* ztl_strndup(const void* str, size_t size);

size_t ztl_malloc_used_memory(void);
void ztl_malloc_enable_thread_safeness(void);
void ztl_malloc_set_oom_handler(void (*oom_handler)(size_t));

#ifndef ZTL_HAVE_MALLOC_SIZE
size_t ztl_malloc_size(void *ptr);
#endif

#endif /* _ZTL_MALLOC_H_INCLUDE_ */
