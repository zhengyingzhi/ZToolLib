#include <stdlib.h>
#include <string.h>

#include "ztl_common.h"
#include "ztl_mem.h"
#include "ztl_utils.h"


void* mem_alloc(size_t nbytes, const char* file, int line)
{
    ZTL_NOTUSED(file);
    ZTL_NOTUSED(line);

    if (nbytes < 1)
        return NULL;
    return malloc(nbytes);
}

void* mem_calloc(size_t count, size_t nbytes, const char* file, int line)
{
    ZTL_NOTUSED(file);
    ZTL_NOTUSED(line);

    if (count < 1 || nbytes < 1)
        return NULL;
    return calloc(count, nbytes);
}

void* mem_posixalign(size_t alignment, size_t nbytes, const char* file, int line)
{
    void* p;
    ZTL_NOTUSED(file);
    ZTL_NOTUSED(line);

    if (nbytes < 1)
        return NULL;

#ifdef _MSC_VER
    p = malloc(ztl_align(nbytes, 8));
#else
    if (posix_memalign(&p, alignment, nbytes) != 0)
        return NULL;
#endif//_MSC_VER
    return p;
}

void mem_free(void* p, const char* file, int line)
{
    ZTL_NOTUSED(file);
    ZTL_NOTUSED(line);

    if (p) {
        free(p);
    }
}

void* mem_resize(void* p, size_t nbytes, const char* file, int line)
{
    ZTL_NOTUSED(file);
    ZTL_NOTUSED(line);

    if (nbytes < 1)
        return NULL;
    return realloc(p, nbytes);
}

char* mem_strndup(const char* str, size_t length)
{
    char* p;
    if (!str) {
        return NULL;
    }

    if ((p = malloc(length + 1)) == NULL)
        return NULL;
    memcpy(p, str, length);
    p[length] = '\0';
    return p;
}

char* mem_strdup(const char* str)
{
    return mem_strndup(str, strlen(str));
}


