/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_EVENT_DISPATCHER_H_
#define _ZTL_EVENT_DISPATCHER_H_

#include <stdbool.h>
#include <stdint.h>


#define ZS_EVENT_DISPATCHER_MAX_QUEUED  65536
#define ZS_EVENT_DISPATCHER_MAX_EVID    128

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/* the expored types */
typedef struct ztl_event_dispatcher_st ztl_event_dispatcher_t;

/* the callback handler
 */
typedef bool(*ztl_evd_handler_pt)(ztl_event_dispatcher_t* zevd, void* ctx, uint32_t evtype, void* evdata);


/* create a producer and consumer model object within a fixed-size queue
 */
ztl_event_dispatcher_t* ztl_evd_create(unsigned int quesize);

/* start consumer callback thread
 */
int ztl_evd_start(ztl_event_dispatcher_t* zevd);

/* register a handler into table by the type
 * note: we could register multiple handle for one event type
 */
int ztl_evd_register(ztl_event_dispatcher_t* zevd, void* ctx, uint32_t evtype, ztl_evd_handler_pt handler);

/* post data to process
 * @return -1 if queue is full, -2 if the not start working thread
 */
int ztl_evd_post(ztl_event_dispatcher_t* zevd, uint32_t evtype, void* evdata);


/* do callback by the passed in type and data
 */
int ztl_evd_do_callback(ztl_event_dispatcher_t* zevd, uint32_t evtype, void* evdata);

/* stop working thread
 */
int ztl_evd_stop(ztl_event_dispatcher_t* zevd);

/* release the object 
 */
void ztl_evd_release(ztl_event_dispatcher_t* zevd);

/* set or get user context data
 */
void  ztl_evd_set_udata(ztl_event_dispatcher_t* zevd, void* udata);
void* ztl_evd_get_udata(ztl_event_dispatcher_t* zevd);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_EVENT_DISPATCHER_H_
