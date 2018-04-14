/*
 * Copyright(C) Yingzhi Zheng.
 * Copyright(C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_EVENT_LOOP_H_
#define _ZTL_EVENT_LOOP_H_

#include "ztl_network.h"
#include "ztl_event_timer.h"

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/* the exported types */
typedef struct ztl_evloop_st        ztl_evloop_t;
typedef struct ztl_connection_st    ztl_connection_t;

/* the socket's events */
typedef enum 
{
    ZEV_POLLIN   = 1,
    ZEV_POLLOUT  = 2,
    ZEV_NEWCONN  = 3,
    ZEV_LOOPONCE = 4
}ZTL_EV_EVENTS;

typedef int(*ztl_recv_t)(ztl_connection_t* conn);
typedef int(*ztl_send_t)(ztl_connection_t* conn);
typedef int(*ztl_ev_handler_t)(ztl_evloop_t* ev, ztl_connection_t* conn, ZTL_EV_EVENTS events);

/* describe one client connection */
struct ztl_connection_st
{
    sockhandle_t    sockfd;         // fd
    uint32_t        addr;           // address
    uint16_t        port;           // port
    uint16_t        reqevents;      // request events

    uint32_t        refcount;
    uint8_t         added;
    uint8_t         closed;
    uint8_t         disconncted;
    uint8_t         padding;

    ztl_evloop_t*   evloop;
    void*           internal;
    void*           userdata;

    char*           rbuf;
    uint32_t        rsize;
    uint32_t        bytes_recved;

    char*           wbuf;
    uint32_t        wsize;
    uint32_t        bytes_sent;

    ztl_ev_handler_t handler;
    ztl_recv_t       recv;
    ztl_send_t       send;
};


typedef struct event_config_st
{
    uint32_t thread_num;
    uint32_t poll_timeout_ms;
    uint32_t listen_addr;
    uint16_t listen_port;
    ztl_ev_handler_t handler;
}event_config_t;


/* the eported interfaces */

/* create an event loop object
 */
ztl_evloop_t* ztl_evloop_create();

/* start an event loop, auto start new threads internal */
int ztl_evloop_start(ztl_evloop_t* evloop, event_config_t* config);

/* stop the event loop */
int ztl_evloop_stop(ztl_evloop_t* evloop);

/* update new event to event loop */
int ztl_evloop_add(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents);
int ztl_evloop_del(ztl_evloop_t* evloop, ztl_connection_t* conn);

/* release the event loop */
int ztl_evloop_release(ztl_evloop_t* evloop);


/* user data */
void  ztl_evloop_set_usedata(ztl_evloop_t* evloop, void* userdata);
void* ztl_evloop_get_usedata(ztl_evloop_t* evloop);

/* get listen fd of the evloop */
sockhandle_t ztl_evloop_get_listenfd(ztl_evloop_t* evloop);


/* add a timer event, return a timerid */
uint32_t evloop_addtimer(ztl_evloop_t* evloop, uint32_t timeoutMS, ztl_evt_handler_pt func, void* arg);

/* remove the timer by the timerid, return 0 if success */
int evloop_deltimer(ztl_evloop_t* evloop, uint32_t timerid);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_EVENT_LOOP_H_
