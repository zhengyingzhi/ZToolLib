/*
 * Copyright(C) Yingzhi Zheng.
 * Copyright(C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_EVENT_LOOP_H_
#define _ZTL_EVENT_LOOP_H_

#include "ztl_network.h"
#include "ztl_evtimer.h"

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/* the exported types */
typedef struct ztl_evloop_st        ztl_evloop_t;
typedef struct ztl_connection_st    ztl_connection_t;

/* the poller method */
typedef enum 
{
    ZTL_EPM_Default     = 0,        // auto use
    ZTL_EPM_Select      = 1,        // user select
    ZTL_EPM_Epoll       = 2,        // only support on linux
    ZTL_EPM_IOCP        = 3         // only support on windows
}ZTL_EV_POLL_METHOD;

/* the socket's events */
typedef enum 
{
    ZEV_POLLIN      = 1,
    ZEV_POLLOUT     = 2,
    ZEV_NEWCONN     = 3,
    ZEV_LOOPONCE    = 4,
    ZEV_TIMEOUT     = 5
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


typedef struct ztl_evconfig_st
{
    uint32_t            thread_num;
    uint32_t            poll_timeout_ms;
    uint16_t            reuse_adddr;
    uint16_t            listen_port;
    uint32_t            listen_addr;
    sockhandle_t        listen_fd;          // we can also set an already listened fd
    ztl_ev_handler_t    handler;
    ztl_evt_handler_pt  timer_handler;
}ztl_evconfig_t;


/* the eported interfaces */

/* create an event loop object
 */
int ztl_evloop_create(ztl_evloop_t** pevloop, ZTL_EV_POLL_METHOD method);

/* init the event loop
 */
int ztl_evloop_init(ztl_evloop_t* evloop, ztl_evconfig_t* config);

/* start an event loop, auto start new threads internal
 */
int ztl_evloop_start(ztl_evloop_t* evloop);

/* stop the event loop */
int ztl_evloop_stop(ztl_evloop_t* evloop);

/* update new event to event loop */
int ztl_evloop_add(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents);
int ztl_evloop_del(ztl_evloop_t* evloop, ztl_connection_t* conn);

/* do once event loop
 */
int ztl_evloop_loop(ztl_evloop_t* evloop, int timeoutMS);

/* release the event loop */
int ztl_evloop_release(ztl_evloop_t* evloop);


/* user data */
void  ztl_evloop_set_usedata(ztl_evloop_t* evloop, void* userdata);
void* ztl_evloop_get_usedata(ztl_evloop_t* evloop);

/* get listen fd of the evloop */
sockhandle_t ztl_evloop_get_listenfd(ztl_evloop_t* evloop);


/* add a timer event */
int ztl_evloop_addtimer(ztl_evloop_t* evloop, ztl_rbtree_node_t* timer, uint32_t timeoutMS);

/* remove the timer, return 0 if success */
int ztl_evloop_deltimer(ztl_evloop_t* evloop, ztl_rbtree_node_t* timer);

/* expire all the timed out timers */
int ztl_evloop_expire(ztl_evloop_t* evloop);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_EVENT_LOOP_H_
