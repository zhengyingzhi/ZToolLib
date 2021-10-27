#include <stdio.h>
#include <string.h>

#include "ztl_utils.h"
#include "ztl_map.h"
#include "ztl_palloc.h"

#include "ztl_fixapi.h"

#define ZTL_FIX_SEP     0x01

struct ztl_fixapi_s
{
    cmap_t*     fixmap;
    ztl_pool_t* pool;
    uint32_t    head_size;
    uint32_t    length;
    uint32_t    capacity;
    char        buffer[ZTL_FIX_BUF_SIZE];
    char*       pbuf;
};

#define _data_len(fixapi)       ((fixapi)->head_size + (fixapi)->length)
#define _kv_data_len(fixapi)    ((fixapi)->length)
#define _avail_size(fixapi)     \
    ((fixapi)->capacity <= _data_len(fixapi) ? fixapi->capacity - _data_len(fixapi) : 0)

#if 0 // not used currently
static ztl_inline void _ztl_insert_item(ztl_fixapi_t* fixapi, fixkey_t id, int64_t value)
{
    rbtree_node_t* pnode;
    pnode = (rbtree_node_t*)ztl_palloc(fixapi->pool, sizeof(rbtree_node_t));
    pnode->key = id;

    union_dtype_t d;
    d.i64 = value;
    pnode->udata = d.ptr;
    cmap_add_ex(fixapi->fixmap, id, pnode);
}
#endif//0

static ztl_inline int64_t _ztl_find_data(ztl_fixapi_t* fixapi, fixkey_t id)
{
    rbtree_node_t* pnode;
    pnode = cmap_find_ex(fixapi->fixmap, id);
    if (pnode) {
        union_dtype_t d;
        d.ptr = pnode->udata;
        return d.i64;
    }
    return ZTL_MAP_INVALID_VALUE;
}

ztl_fixapi_t* ztl_fixapi_create(uint32_t head_size)
{
    ztl_fixapi_t* fixapi;
    fixapi = (ztl_fixapi_t*)malloc(ztl_align(sizeof(ztl_fixapi_t), 8));

    fixapi->fixmap = cmap_create(32);
    fixapi->pool = ztl_create_pool(4096);

    memset(fixapi->buffer, 0, ZTL_FIX_BUF_SIZE);
    fixapi->head_size = head_size;
    fixapi->length = 0;
    fixapi->capacity = ZTL_FIX_BUF_SIZE;

    // default point to buffer
    fixapi->pbuf = fixapi->buffer;

    return fixapi;
}

void ztl_fixapi_release(ztl_fixapi_t* fixapi)
{
    if (fixapi)
    {
        ztl_destroy_pool(fixapi->pool);
        free(fixapi);
    }
}

void ztl_fixapi_clear(ztl_fixapi_t* fixapi)
{
    ztl_reset_pool(fixapi->pool);
    cmap_clear(fixapi->fixmap);
    fixapi->length = 0;
}

char* ztl_fixapi_data(ztl_fixapi_t* fixapi)
{
    return fixapi->pbuf;
}

char* ztl_fixapi_data_kv(ztl_fixapi_t* fixapi)
{
    return fixapi->pbuf + fixapi->head_size;
}

uint32_t ztl_fixapi_length(ztl_fixapi_t* fixapi)
{
    return _data_len(fixapi);
}

uint32_t ztl_fixapi_length_kv(ztl_fixapi_t* fixapi)
{
    return _kv_data_len(fixapi);
}

void ztl_fixapi_setbuffer(ztl_fixapi_t* fixapi, char* buffer, int size)
{
    fixapi->pbuf = buffer;
    fixapi->capacity = size;
}


bool ztl_fixapi_have(ztl_fixapi_t* fixapi, fixkey_t id)
{
    if (cmap_find(fixapi->fixmap, id)) {
        return true;
    }
    return false;
}

/* -------- setter ---------*/
int ztl_fixapi_set_char(ztl_fixapi_t* fixapi, fixkey_t id, const char val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi),
        _avail_size(fixapi), "%u=%c%c", id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_int16(ztl_fixapi_t* fixapi, fixkey_t id, const uint16_t val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi),
        _avail_size(fixapi), "%u=%d%c", id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_int32(ztl_fixapi_t* fixapi, fixkey_t id, const uint32_t val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi),
        _avail_size(fixapi), "%u=%u%c", id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_int64(ztl_fixapi_t* fixapi, fixkey_t id, const int64_t val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi),
        _avail_size(fixapi), "%u="ZTL_I64_FMT"%c", id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_float(ztl_fixapi_t* fixapi, fixkey_t id, const float val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi),
        _avail_size(fixapi), "%u=%.4f%c", id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_double(ztl_fixapi_t* fixapi, fixkey_t id, const double val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi),
        _avail_size(fixapi), "%u=%.6lf%c", id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_str(ztl_fixapi_t* fixapi, fixkey_t id, const char* val, int len)
{
    if (len <=0)
        len = (int)strlen(val);

    if (_avail_size(fixapi) <= (uint32_t)len) {
        return -1;
    }

    fixapi->length += snprintf(fixapi->pbuf + +_data_len(fixapi),
        _avail_size(fixapi), "%d:", id);
    memcpy(fixapi->pbuf + _data_len(fixapi), val, len);
    fixapi->length += len;
    fixapi->pbuf[fixapi->length] = ZTL_FIX_SEP;
    fixapi->length += 1;
    return 0;
}


/* -------- getter ---------*/
int ztl_fixapi_get_char(ztl_fixapi_t* fixapi, fixkey_t id, char* pval)
{
    char* pdata = (char*)_ztl_find_data(fixapi, id);
    if (pdata) {
        *pval = *pdata;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_int16(ztl_fixapi_t* fixapi, fixkey_t id, uint16_t* pval)
{
    uint16_t* pdata = (uint16_t*)_ztl_find_data(fixapi, id);
    if (pdata) {
        *pval = *pdata;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_int32(ztl_fixapi_t* fixapi, fixkey_t id, uint32_t* pval)
{
    union_dtype_t data;
    data.i64 = _ztl_find_data(fixapi, id);
    if (data.i64 != ZTL_MAP_INVALID_VALUE) {
        *pval = data.i32;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_int64(ztl_fixapi_t* fixapi, fixkey_t id, int64_t* pval)
{
    union_dtype_t data;
    data.i64 = _ztl_find_data(fixapi, id);
    if (data.i64 != ZTL_MAP_INVALID_VALUE) {
        *pval = data.i64;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_float(ztl_fixapi_t* fixapi, fixkey_t id, float* pval)
{
    union_dtype_t data;
    data.i64 = _ztl_find_data(fixapi, id);
    if (data.i64 != ZTL_MAP_INVALID_VALUE) {
        *pval = data.f32;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_double(ztl_fixapi_t* fixapi, fixkey_t id, double* pval)
{
    union_dtype_t data;
    data.i64 = _ztl_find_data(fixapi, id);
    if (data.i64 != ZTL_MAP_INVALID_VALUE) {
        *pval = data.f64;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_str(ztl_fixapi_t* fixapi, fixkey_t id, char* pval, int* plen)
{
    union_dtype_t data;
    data.i64 = _ztl_find_data(fixapi, id);
    if (data.i64 != ZTL_MAP_INVALID_VALUE) {
        *plen = (int)strlen((char*)data.ptr);
        memcpy(pval, data.ptr, *plen);
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_value(ztl_fixapi_t* fixapi, fixkey_t id, void** ppval, int* plen)
{
    union_dtype_t data;
    data.i64 = _ztl_find_data(fixapi, id);
    if (data.i64 != ZTL_MAP_INVALID_VALUE) {
        *ppval = data.ptr;
        *plen = sizeof(data.ptr);
        return 0;
    }
    return -1;
}
