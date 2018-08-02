#include <stdio.h>
#include <string.h>

#include "ztl_utils.h"
#include "ztl_map.h"
#include "ztl_palloc.h"

#include "ztl_fixapi.h"

#define ZTL_FIX_SEP     0x01

struct ztl_fixapi_s
{
    ztl_map_t*  fixmap;
    ztl_pool_t* pool;
    char        buffer[ZTL_FIX_BUF_SIZE];
    uint32_t    length;
    uint32_t    capacity;
    char*       pbuf;
};

#define _avail_size(fixapi) (fixapi->capacity <= fixapi->length ? fixapi->capacity - fixapi->length : 0)
#define _data_len(fixapi)   (fixapi->length)

static ztl_inline void _ztl_insert_item(ztl_fixapi_t* fixapi, fixkey_t id, void* value)
{
    ztl_rbtree_node_t* pnode;
    pnode = (ztl_rbtree_node_t*)ztl_palloc(fixapi->pool, sizeof(ztl_rbtree_node_t));
    pnode->key = id;
    pnode->udata = (void*)value;
    ztl_map_add_ex(fixapi->fixmap, id, pnode);
}

static ztl_inline void* _ztl_find_data(ztl_fixapi_t* fixapi, fixkey_t id)
{
    ztl_rbtree_node_t* pnode;
    pnode = ztl_map_find_ex(fixapi->fixmap, id);
    return pnode ? pnode->udata : NULL;
}

ztl_fixapi_t* ztl_fixapi_create()
{
    ztl_fixapi_t* fixapi;
    fixapi = (ztl_fixapi_t*)malloc(ztl_align(sizeof(ztl_fixapi_t), 8));

    fixapi->fixmap = ztl_map_create(0);
    fixapi->pool = ztl_create_pool(4096);

    memset(fixapi->buffer, 0, ZTL_FIX_BUF_SIZE);
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
    ztl_map_clear(fixapi->fixmap);
    fixapi->length = 0;
}

char* ztl_fixapi_data(ztl_fixapi_t* fixapi)
{
    return fixapi->pbuf;
}

int ztl_fixapi_length(ztl_fixapi_t* fixapi)
{
    return fixapi->length;
}

void ztl_fixapi_setbuffer(ztl_fixapi_t* fixapi, char* buffer, int size)
{
    fixapi->pbuf = buffer;
    fixapi->capacity = size;
}


bool ztl_fixapi_have(ztl_fixapi_t* fixapi, fixkey_t id)
{
    if (ztl_map_find(fixapi->fixmap, id)) {
        return true;
    }
    return false;
}

/* -------- setter ---------*/
int ztl_fixapi_set_char(ztl_fixapi_t* fixapi, fixkey_t id, const char val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi), _avail_size(fixapi), "%u=%c%c",
        id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_int16(ztl_fixapi_t* fixapi, fixkey_t id, const uint16_t val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi), _avail_size(fixapi), "%u=%d%c",
        id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_int32(ztl_fixapi_t* fixapi, fixkey_t id, const uint32_t val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi), _avail_size(fixapi), "%u=%u%c",
        id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_int64(ztl_fixapi_t* fixapi, fixkey_t id, const int64_t val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi), _avail_size(fixapi), "%u=%lld%c",
        id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_float(ztl_fixapi_t* fixapi, fixkey_t id, const float val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi), _avail_size(fixapi), "%u=%f%c",
        id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_double(ztl_fixapi_t* fixapi, fixkey_t id, const double val)
{
    fixapi->length += snprintf(fixapi->pbuf + _data_len(fixapi), _avail_size(fixapi), "%u=%lf%c",
        id, val, ZTL_FIX_SEP);
    return 0;
}

int ztl_fixapi_set_str(ztl_fixapi_t* fixapi, fixkey_t id, const char* val, int len)
{
    if (len <=0)
        len = strlen(val);

    if (_avail_size(fixapi) <= (uint32_t)len) {
        return -1;
    }

    fixapi->length += snprintf(fixapi->pbuf + +_data_len(fixapi), _avail_size(fixapi), "%d:", id);
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
    if (pdata)
    {
        *pval = (char)pdata;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_int16(ztl_fixapi_t* fixapi, fixkey_t id, uint16_t* pval)
{
    uint16_t* pdata = (uint16_t*)_ztl_find_data(fixapi, id);
    if (pdata)
    {
        *pval = (uint16_t)pdata;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_int32(ztl_fixapi_t* fixapi, fixkey_t id, uint32_t* pval)
{
    uint32_t* pdata = (uint32_t*)_ztl_find_data(fixapi, id);
    if (pdata)
    {
        *pval = (uint32_t)pdata;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_int64(ztl_fixapi_t* fixapi, fixkey_t id, int64_t* pval)
{
    int64_t* pdata = (int64_t*)_ztl_find_data(fixapi, id);
    if (pdata)
    {
        *pval = (int64_t)pdata;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_float(ztl_fixapi_t* fixapi, fixkey_t id, float* pval)
{
    float* pdata = (float*)_ztl_find_data(fixapi, id);
    if (pdata)
    {
        *pval = (float)(uint32_t)pdata;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_double(ztl_fixapi_t* fixapi, fixkey_t id, double* pval)
{
    void* pdata = _ztl_find_data(fixapi, id);
    if (pdata)
    {
        *pval = (double)(int64_t)pdata;
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_str(ztl_fixapi_t* fixapi, fixkey_t id, char* pval, int* plen)
{
    char* pdata = (char*)_ztl_find_data(fixapi, id);
    if (pdata)
    {
        *plen = strlen(pdata);
        memcpy(pval, pdata, *plen);
        return 0;
    }
    return -1;
}

int ztl_fixapi_get_value(ztl_fixapi_t* fixapi, fixkey_t id, void** ppval, int* plen)
{
    return -1;
}


