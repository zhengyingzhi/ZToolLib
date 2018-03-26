/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_SIMPLE_EVENT_H_
#define _ZTL_SIMPLE_EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/* the exported types */
typedef struct ztl_simevent_st ztl_simevent_t;

/// create a simple event
ztl_simevent_t* ztl_simevent_create();

/// destroy the event object
void ztl_simevent_release(ztl_simevent_t* sev);

/// notify the waitor
void ztl_simevent_signal(ztl_simevent_t* sev);

/// wait next signal
void ztl_simevent_wait(ztl_simevent_t* sev);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_SIMPLE_EVENT_H_
