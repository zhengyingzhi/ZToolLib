#ifndef _ZTL_TCP_CLIENT_H_
#define _ZTL_TCP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "ztl_protocol.h"
#include "ztl_threads.h"
#include "ztl_times.h"


#define ZTL_CLIENT_STATUS_DisConnected      0
#define ZTL_CLIENT_STATUS_Connecting        1
#define ZTL_CLIENT_STATUS_Connected         2


/* the client object type */
typedef struct ztl_event_ops        ztl_event_ops_t;
typedef struct ztl_tcp_client_st    ztl_tcp_client_t;

/* the event handler */
typedef void(*ztl_tcp_client_connect_pt)(ztl_tcp_client_t* cli);
typedef void(*ztl_tcp_client_disconnect_pt)(ztl_tcp_client_t* cli, int reason);
typedef int (*ztl_tcp_client_handler_pt)(ztl_tcp_client_t* cli, sockhandle_t fd, void* udata);


struct ztl_tcp_client_st
{
    ztl_event_ops_t *evops;
    void*           evops_ctx;
    void*           udata;
    ztl_thread_t    thd;
    sockhandle_t    fd;
    char            server_ip[32];
    uint16_t        server_port;
    int16_t         inited;
    int32_t         running;
    int32_t         connect_status;
    uint32_t        reconnect_ms;
    uint32_t        timeout_ms;
    uint32_t        snd_buffsize;
    uint32_t        rcv_buffsize;
    uint64_t        connected_time;
    int32_t         debug_mode;

    ztl_tcp_client_connect_pt       on_connect;
    ztl_tcp_client_disconnect_pt    on_disconnect;
    ztl_tcp_client_handler_pt       on_read;
    ztl_tcp_client_handler_pt       on_write;

    ztl_thread_mutex_t              lock;
    int64_t                         io_thread_id;

    uint8_t         sync_mode;
    uint8_t         non_block;
    uint8_t         tcp_nodelay;            /* sock option */
};


/* create a tcp client object
 */
int ztl_tcp_client_create(ztl_tcp_client_t** pcli);

/* release the tcp client object
 */
int ztl_tcp_client_release(ztl_tcp_client_t* cli);

/* register message handler
 */
int ztl_tcp_client_register(ztl_tcp_client_t* cli, const char* ip, uint16_t port);
int ztl_tcp_client_reg_on_connect(ztl_tcp_client_t* cli, ztl_tcp_client_connect_pt handler);
int ztl_tcp_client_reg_on_disconnect(ztl_tcp_client_t* cli, ztl_tcp_client_disconnect_pt handler);
int ztl_tcp_client_reg_on_read(ztl_tcp_client_t* cli, ztl_tcp_client_handler_pt handler);
int ztl_tcp_client_reg_on_write(ztl_tcp_client_t* cli, ztl_tcp_client_handler_pt handler);

/* do connect
 */
int ztl_tcp_client_do_connect(ztl_tcp_client_t* cli, const char* ip, uint16_t port, int timeout_ms);

/* init the tcp client
 * @param sync_mode, whether synchronize connecting
 */
int ztl_tcp_client_init(ztl_tcp_client_t* cli, bool sync_mode);

/* stop the tcp client
 */
int ztl_tcp_client_stop(ztl_tcp_client_t* cli);


/* some common funcs */
int ztl_tcp_client_close_fd(ztl_tcp_client_t* cli);
int ztl_tcp_client_set_debug(ztl_tcp_client_t* cli, int on);
int ztl_tcp_client_set_timeout(ztl_tcp_client_t* cli, uint32_t timeout_ms);
int ztl_tcp_client_set_reconnect(ztl_tcp_client_t* cli, uint32_t reconnect_ms);
int ztl_tcp_client_set_nodelay(ztl_tcp_client_t* cli);
int ztl_tcp_client_set_snd_buffsize(ztl_tcp_client_t* cli, uint32_t snd_buffsize);
int ztl_tcp_client_set_rcv_buffsize(ztl_tcp_client_t* cli, uint32_t rcv_buffsize);

int ztl_tcp_client_add_fd(ztl_tcp_client_t* cli, sockhandle_t fd, int reqevents);
int ztl_tcp_client_poll(ztl_tcp_client_t* cli, uint32_t timeout_ms);
int ztl_tcp_client_recv(ztl_tcp_client_t* cli, char* buf, int len);
int ztl_tcp_client_send(ztl_tcp_client_t* cli, const char* buf, int len);


static inline sockhandle_t ztl_tcp_client_fd(ztl_tcp_client_t* cli) {
    return cli->fd;
}

static inline void* ztl_tcp_client_udata(ztl_tcp_client_t* cli) {
    return cli->udata;
}

static inline int ztl_tcp_client_running(ztl_tcp_client_t* cli) {
    return cli->running;
}

static inline int ztl_tcp_client_inited(ztl_tcp_client_t* cli) {
    return cli->inited;
}

static inline int ztl_tcp_client_connected(ztl_tcp_client_t* cli) {
    return cli->connect_status == ZTL_CLIENT_STATUS_Connected;
}


#ifdef __cplusplus
}
#endif//__cplusplus


#endif//_ZTL_TCP_CLIENT_H_
