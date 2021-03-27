#ifndef _ZTL_EVENT_LOOP_H_
#define _ZTL_EVENT_LOOP_H_

#include "ztl_network.h"
#include "ztl_evtimer.h"

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#define ZTL_DEFAULT_EVLOOP_SIZE     1024

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
    ZEV_NONE        = 0,
    ZEV_POLLIN      = 1,
    ZEV_POLLOUT     = 2,
    ZEV_NEWCONN     = 4,
    ZEV_LOOPONCE    = 8,
    ZEV_TIMEOUT     = 16
}ZTL_EV_EVENTS;

typedef int(*ztl_recv_t)(ztl_connection_t* conn);
typedef int(*ztl_send_t)(ztl_connection_t* conn);

typedef int(*ztl_ev_handler_t)(ztl_evloop_t* evloop, ztl_connection_t* conn, int events);
typedef int(*ztl_timer_handler_t)(ztl_evloop_t* evloop, uint64_t timer_id, void* udata);
typedef int(*ztl_timer_finalizer_t)(ztl_evloop_t* evloop, uint64_t timer_id, void* udata);


/* describe one client connection */
struct ztl_connection_st
{
    sockhandle_t    fd;             // socket fd
    uint32_t        addr;           // address
    uint16_t        port;           // port
    uint16_t        events;         // request events

    uint32_t        refcount;
    uint8_t         added;
    uint8_t         closed;
    uint8_t         disconncted;
    uint8_t         padding;

    ztl_evloop_t*   evloop;
    void*           internal;
    void*           userdata;       // for upper app, framework would not use it

    char*           rbuf;
    uint32_t        rsize;
    uint32_t        bytes_recved;

    char*           wbuf;
    uint32_t        wsize;
    uint32_t        bytes_sent;

    ztl_ev_handler_t read_handler;
    ztl_ev_handler_t write_handler;
    ztl_recv_t       recv;
    ztl_send_t       send;

    // free the object if the last refcount
    int            (*free)(ztl_connection_t*);
};


/* the eported interfaces */

/* create an event loop object
 */
int ztl_evloop_create(ztl_evloop_t** pevloop, int size);

/* init the event loop
 */
int ztl_evloop_init(ztl_evloop_t* evloop);

/* start an event loop, auto start new threads internal
 */
int ztl_evloop_start(ztl_evloop_t* evloop);

/* stop the event loop */
int ztl_evloop_stop(ztl_evloop_t* evloop);

/* create pipes for evloop */
int ztl_evloop_make_pipes(ztl_evloop_t* evloop);

/* update new event to event loop,
 * internally will create a new connection object for the fd if not exists
 */
int ztl_evloop_add(ztl_evloop_t* evloop, sockhandle_t fd, int reqevents,
                   ztl_ev_handler_t handler, void* udata);

/* delelte event from event loop,
 * you need manually free the connection object of the fd
 */
int ztl_evloop_del(ztl_evloop_t* evloop, sockhandle_t fd, int delevents);

/* do once event loop
 */
int ztl_evloop_looponce(ztl_evloop_t* evloop, int timeout_ms);
int ztl_evloop_loop(ztl_evloop_t* evloop, int timeout_ms);

/* release the event loop */
int ztl_evloop_release(ztl_evloop_t* evloop);

/* user data */
void  ztl_evloop_set_usedata(ztl_evloop_t* evloop, void* userdata);
void* ztl_evloop_get_usedata(ztl_evloop_t* evloop);

/* some evloop helpers */
ztl_connection_t* ztl_connection_new(ztl_evloop_t* evloop, sockhandle_t fd,
    uint32_t fd_addr, uint16_t fd_port);
ztl_connection_t* ztl_connection_find(ztl_evloop_t* evloop, sockhandle_t fd);
ztl_connection_t* ztl_connection_remove(ztl_evloop_t* evloop, sockhandle_t fd);
int ztl_connection_save(ztl_evloop_t* evloop, ztl_connection_t* conn);

/* thread lock if needed */
void ztl_evloop_lock(ztl_evloop_t* evloop);
void ztl_evloop_unlock(ztl_evloop_t* evloop);

/* update timestamp for evloop */
void ztl_evloop_timestamp_update(ztl_evloop_t* evloop);
uint64_t ztl_evloop_timestamp_get(ztl_evloop_t* evloop);

/* add a timer event
 * @return the timer_id, 0 is failed
 */
uint64_t ztl_evloop_addtimer(ztl_evloop_t* evloop, uint32_t timeout_ms,
    ztl_timer_handler_t handler,
    ztl_timer_finalizer_t finalizer, void* udata);

/* remove the timer,
 * @return 0 if deleted success
 */
int ztl_evloop_deltimer(ztl_evloop_t* evloop, uint64_t timer_id);

/* expire all the timed out timers
 * @return the nearest time ms left, -1 means none
 * @brief not thread safe!
 */
int ztl_evloop_expire(ztl_evloop_t* evloop, uint64_t currtime);

/* get the pending timers count
 */
uint32_t ztl_evloop_timer_count(ztl_evloop_t* evloop);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_EVENT_LOOP_H_
