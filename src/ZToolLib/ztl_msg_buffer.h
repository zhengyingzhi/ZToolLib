/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) zhengyingzhi112@163.com
 */

#ifndef _ZTL_MESSAGE_BUFFER_H_
#define _ZTL_MESSAGE_BUFFER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ztl_msg_buffer_st ztl_msg_buffer_t;

struct ztl_msg_buffer_st
{
    ztl_msg_buffer_t* prev;
    ztl_msg_buffer_t* next;
    void      (*cleanup)(ztl_msg_buffer_t*);
    void*       owner;
    void*       udata;
    uint32_t    flags;
    uint32_t    capacity;
    uint32_t    length;
    uint32_t    refcount;
};

ztl_msg_buffer_t* zlt_mb_alloc(uint32_t size);

void zlt_mb_free(ztl_msg_buffer_t* zmb);

uint32_t ztl_mb_addref(ztl_msg_buffer_t* zmb);

uint32_t ztl_mb_decref_release(ztl_msg_buffer_t* zmb);

#define ztl_mb_body(zmb)    (char*)(zmb + 1)

#ifdef __cplusplus
}
#endif

#endif//_ZTL_MESSAGE_BUFFER_H_
