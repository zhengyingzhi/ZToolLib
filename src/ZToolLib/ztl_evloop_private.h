/*
 * Copyright(C) Yingzhi Zheng.
 * Copyright(C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_EVENT_LOOP_PRIVATE_H_
#define _ZTL_EVENT_LOOP_PRIVATE_H_

#include <stdint.h>

#include "ztl_network.h"
#include "ztl_evloop.h"
#include "ztl_mempool.h"

#define ZTL_DEF_POLL_TIMEOUT_MS     200
#define ZTL_DEF_CONN_INIT_COUNT     256

typedef struct ztl_event_ops ztl_event_ops_t;

struct ztl_evloop_st
{
    sockhandle_t        listenfd;
    unsigned short      listen_port;
    unsigned int        listen_addr;

    sockhandle_t        pipefds[2];
    sockhandle_t        timeoutMS;
    int                 thrnum;
    int                 running;
    volatile uint32_t   nexited;
    uint32_t            looponce_flag;
    ztl_event_timer_t   timers;
    ztl_mempool_t*      conn_mp;

    ztl_ev_handler_t    handler;
    void*               userdata;

    void*               ctx;
    ztl_event_ops_t*    evsel;
};

struct ztl_event_ops
{
    int(*init)(ztl_evloop_t* evloop);

    int(*start)(ztl_evloop_t* evloop);

    int(*add)(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS reqevents);
    int(*del)(ztl_evloop_t* evloop, ztl_connection_t* conn);

    int(*stop)(ztl_evloop_t* evloop);

    int(*destroy)(ztl_evloop_t* evloop);

    const char* name;
};


/* some common native helpers */
int ztl_do_recv(ztl_connection_t* conn);

int ztl_do_send(ztl_connection_t* conn);

ztl_connection_t* ztl_do_accept(ztl_evloop_t* evloop);

void ztl_free_connection(ztl_evloop_t* evloop, ztl_connection_t* conn);
ztl_connection_t* ztl_new_connection(ztl_evloop_t* evloop, sockhandle_t sockfd);



#endif//_ZTL_EVENT_LOOP_PRIVATE_H_
