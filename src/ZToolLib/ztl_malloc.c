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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <windows.h>
#else
#include <unistd.h>
#endif//_MSC_VER

#include "ztl_atomic.h"
#include "ztl_malloc.h"

/* This function provide us access to the original libc free(). This is useful
 * for instance to free results obtained by backtrace_symbols(). We need
 * to define this function before including zmalloc.h that may shadow the
 * free implementation if we use jemalloc or another non standard allocator. */
#define ztl_libc_free(ptr) free(ptr)

#define ZTL_PORT_LONG   void*

#ifdef ZTL_HAVE_MALLOC_SIZE
#define PREFIX_SIZE (0)
#else
#if defined(__sun) || defined(__sparc) || defined(__sparc__)
#define PREFIX_SIZE (sizeof(PORT_LONGLONG))
#else
#define PREFIX_SIZE (sizeof(size_t))
#endif
#endif

/* Explicitly override malloc/free etc when using tcmalloc. */
#if defined(ZTL_USE_TCMALLOC)
#define malloc(size) tc_malloc(size)
#define calloc(count,size) tc_calloc(count,size)
#define realloc(ptr,size) tc_realloc(ptr,size)
#define free(ptr) tc_free(ptr)
#elif defined(ZTL_USE_JEMALLOC)
#define malloc(size) je_malloc(size)
#define calloc(count,size) je_calloc(count,size)
#define realloc(ptr,size) je_realloc(ptr,size)
#define free(ptr) je_free(ptr)
#elif defined(ZTL_USE_DLMALLOC)
#define malloc(size) g_malloc(size)
#define calloc(count,size) g_calloc(count,size)
#define realloc(ptr,size) g_realloc(ptr,size)
#define free(ptr) g_free(ptr)
#endif//ZTL_USE_XXMALLOC


#define update_zmalloc_stat_alloc(__n) do { \
    uint32_t _n = (__n); \
    if (_n&(sizeof(ZTL_PORT_LONG)-1)) _n += sizeof(ZTL_PORT_LONG)-(_n&(sizeof(ZTL_PORT_LONG)-1)); \
    if (_zmalloc_thread_safe) { \
        ztl_atomic_add(&_used_memory, (_n)); \
    } else { \
        _used_memory += _n; \
    } \
} while(0)

#define update_zmalloc_stat_free(__n) do { \
    uint32_t _n = (__n); \
    if (_n&(sizeof(ZTL_PORT_LONG)-1)) _n += sizeof(ZTL_PORT_LONG)-(_n&(sizeof(ZTL_PORT_LONG)-1)); \
    if (_zmalloc_thread_safe) { \
        ztl_atomic_dec(&_used_memory, (_n)); \
    } else { \
        _used_memory -= _n; \
    } \
} while(0)

static uint32_t _used_memory = 0;
static int      _zmalloc_thread_safe = 0;

static void ztl_malloc_default_oom(uint32_t size) {
    fprintf(stderr, "zmalloc: Out of memory trying to allocate %u bytes\n",  /* %zu -> %Iu */
        size);
    fflush(stderr);
    abort();
}

static void (*ztl_malloc_oom_handler)(uint32_t) = ztl_malloc_default_oom;

void* ztl_malloc(uint32_t size)
{
    void* ptr = malloc(size+PREFIX_SIZE);
    if (!ptr)
        ztl_malloc_oom_handler(size);

#ifdef ZTL_HAVE_MALLOC_SIZE
    update_zmalloc_stat_alloc(zmalloc_size(ptr));
    return ptr;
#else
    *((uint32_t*)ptr) = size;
    update_zmalloc_stat_alloc(size+PREFIX_SIZE);
    return (char*)ptr+PREFIX_SIZE;
#endif//ZTL_HAVE_MALLOC_SIZE
}

void* ztl_calloc(uint32_t size)
{
    void *ptr = calloc(1, size+PREFIX_SIZE);
    if (!ptr)
        ztl_malloc_oom_handler(size);

#ifdef ZTL_HAVE_MALLOC_SIZE
    update_zmalloc_stat_alloc(zmalloc_size(ptr));
    return ptr;
#else
    *((uint32_t*)ptr) = size;
    update_zmalloc_stat_alloc(size+PREFIX_SIZE);
    return (char*)ptr+PREFIX_SIZE;
#endif//ZTL_HAVE_MALLOC_SIZE
}

void* ztl_realloc(void *ptr, uint32_t size)
{
#ifndef ZTL_HAVE_MALLOC_SIZE
    void*   realptr;
#endif//ZTL_HAVE_MALLOC_SIZE

    uint32_t oldsize;
    void*    newptr;

    if (ptr == NULL)
        return ztl_malloc(size);

#ifdef ZTL_HAVE_MALLOC_SIZE
    oldsize = zmalloc_size(ptr);
    newptr = realloc(ptr,size);
    if (!newptr)
        ztl_malloc_oom_handler(size);

    update_zmalloc_stat_free(oldsize);
    update_zmalloc_stat_alloc(ztl_malloc_size(newptr));
    return newptr;
#else
    realptr = (char*)ptr-PREFIX_SIZE;
    oldsize = *((uint32_t*)realptr);
    newptr = realloc(realptr,size+PREFIX_SIZE);
    if (!newptr)
        ztl_malloc_oom_handler(size);

    *((uint32_t*)newptr) = size;
    update_zmalloc_stat_free(oldsize);
    update_zmalloc_stat_alloc(size);
    return (char*)newptr+PREFIX_SIZE;
#endif//ZTL_HAVE_MALLOC_SIZE
}

/* Provide ztl_malloc_size() for systems where this function is not provided by
 * malloc itself, given that in that case we store a header with this
 * information as the first bytes of every allocation. */
#ifndef ZTL_HAVE_MALLOC_SIZE
uint32_t ztl_malloc_size(void *ptr)
{
    void *realptr = (char*)ptr-PREFIX_SIZE;
    size_t size = *((size_t*)realptr);
    /* Assume at least that all the allocations are padded at sizeof(PORT_LONG) by
     * the underlying allocator. */
    if (size&(sizeof(ZTL_PORT_LONG)-1)) size += sizeof(ZTL_PORT_LONG)-(size&(sizeof(ZTL_PORT_LONG)-1));
    return (uint32_t)(size + PREFIX_SIZE);
}
#endif//ZTL_HAVE_MALLOC_SIZE

void ztl_free(void *ptr)
{
    if (ptr == NULL) return;

#ifdef ZTL_HAVE_MALLOC_SIZE
    update_zmalloc_stat_free(ztl_malloc_size(ptr));
    free(ptr);
#else
    void*    realptr;
    uint32_t oldsize;

    realptr = (char*)ptr-PREFIX_SIZE;
    oldsize = *((uint32_t*)realptr);
    update_zmalloc_stat_free(oldsize+PREFIX_SIZE);
    free(realptr);
#endif//ZTL_HAVE_MALLOC_SIZE
}

char *ztl_strdup(const char* str) {
    uint32_t len = (uint32_t)strlen(str) + 1;
    char *p = ztl_malloc(len);

    memcpy(p, str, len);
    return p;
}

uint32_t ztl_malloc_used_memory(void) {
    uint32_t um;

    if (_zmalloc_thread_safe) {
        um = ztl_atomic_add(&_used_memory, 0);
    }
    else {
        um = _used_memory;
    }

    return um;
}

void ztl_malloc_enable_thread_safeness(void) {
    _zmalloc_thread_safe = 1;
}

void ztl_malloc_set_oom_handler(void (*oom_handler)(uint32_t)) {
    ztl_malloc_oom_handler = oom_handler;
}

