#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "ztl_buffer.h"


void* _ztl_buffer_default_alloc(void* ctxdata, void* oldaddr, uint32_t size)
{
    (void)ctxdata;
    if (oldaddr)
        oldaddr = realloc(oldaddr, size);
    else
        oldaddr = malloc(size);
    return oldaddr;
}
void _ztl_buffer_default_dealloc(void* ctxdata, void* addr)
{
    (void)ctxdata;
    if (addr) {
        free(addr);
    }
}


void ztl_buffer_init(ztl_buffer_t* zbuf)
{
    zbuf->data      = NULL;
    zbuf->size      = 0;
    zbuf->capacity  = 0;
    zbuf->const_addr= 0;
    zbuf->alloc     = _ztl_buffer_default_alloc;
    zbuf->dealloc   = _ztl_buffer_default_dealloc;
    zbuf->ctx       = zbuf;
}

void ztl_buffer_init_byaddr(ztl_buffer_t* zbuf, void* addr, uint32_t capacity)
{
    zbuf->data      = addr;
    zbuf->size      = 0;
    zbuf->capacity  = capacity;
    zbuf->const_addr= 1;

    zbuf->alloc     = _ztl_buffer_default_alloc;
    zbuf->dealloc   = _ztl_buffer_default_dealloc;
    zbuf->ctx       = zbuf;
}


void ztl_buffer_set_alloc_func(ztl_buffer_t* zbuf, 
    ztl_buffer_alloc_pt alloc_func,
    ztl_buffer_dealloc_pt dealloc_func, void* ctxdata)
{
    if (zbuf && alloc_func) {
        zbuf->alloc   = alloc_func;
        zbuf->dealloc = dealloc_func;
        zbuf->ctx     = ctxdata;
    }
}

void ztl_buffer_release(ztl_buffer_t* zbuf)
{
    if (!zbuf->const_addr) {
        zbuf->dealloc(zbuf->ctx, zbuf->data);
    }
}

bool ztl_buffer_reserve(ztl_buffer_t* zbuf, uint32_t capacity)
{
    void* data;

    if (zbuf->const_addr) {
        return false;
    }

    if (capacity > zbuf->capacity)
    {
        //roundup
        {
            capacity--;
            capacity |= capacity >> 1;
            capacity |= capacity >> 2;
            capacity |= capacity >> 4;
            capacity |= capacity >> 8;
            capacity |= capacity >> 16;
#if (__x86_64__ || _WIN64)
            capacity |= capacity >> 32;
#endif
            capacity++;
        }

        data = zbuf->alloc(zbuf->ctx, zbuf->data, capacity);
        if (data == NULL) {
            return false;
        }

        zbuf->data = data;
        zbuf->capacity = capacity;
    }

    return true;
}

bool ztl_buffer_compact(ztl_buffer_t* zbuf)
{
    void* data;

    if (zbuf->const_addr) {
        return false;
    }

    if (zbuf->capacity > zbuf->size)
    {
        data = zbuf->alloc(zbuf->ctx, zbuf->data, zbuf->size);
        if (zbuf->size)
            assert(data != NULL);
        zbuf->data = data;
        zbuf->capacity = zbuf->size;
    }

    return true;
}


void ztl_buffer_append(ztl_buffer_t* zbuf, void* adata, uint32_t asize)
{
    ztl_buffer_reserve(zbuf, zbuf->size + asize);
    memcpy((char *)zbuf->data + zbuf->size, adata, asize);
    zbuf->size += asize;
}

void ztl_buffer_insert(ztl_buffer_t* zbuf, uint32_t pos, void* adata, uint32_t asize)
{
    ztl_buffer_reserve(zbuf, zbuf->size + asize);
    if (pos < zbuf->size)
        memmove((char *)zbuf->data + pos + asize, (char *)zbuf->data + pos, zbuf->size - pos);
    memcpy((char *)zbuf->data + pos, adata, asize);
    zbuf->size += asize;
}


void ztl_buffer_fill(ztl_buffer_t* zbuf, uint32_t pos, uint32_t count, void* adata, uint32_t asize)
{
    size_t i;

    ztl_buffer_reserve(zbuf, zbuf->size + (count * asize));
    if (pos < zbuf->size)
        memmove((char *)zbuf->data + pos + (count * asize), (char *)zbuf->data + pos, zbuf->size - pos);

    for (i = 0; i < count; i++)
        memcpy((char *)zbuf->data + pos + (i * asize), adata, asize);
    zbuf->size += count * asize;
}

void ztl_buffer_erase(ztl_buffer_t* zbuf, uint32_t pos, uint32_t size)
{
    memmove((char *)zbuf->data + pos, (char *)zbuf->data + pos + size, zbuf->size - pos - size);
    zbuf->size -= size;
}

