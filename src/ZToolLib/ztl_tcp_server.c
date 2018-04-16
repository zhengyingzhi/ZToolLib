#include <stdlib.h>
#include <string.h>

#include "ztl_evloop.h"
#include "ztl_tcp_server.h"

struct ztl_tcp_server_st
{
    ztl_evloop_t*           evloop;
    ztl_evconfig_t          evconf;
    ztl_tcp_server_config_t svrconf;

    int32_t     inited;
    uint32_t    running;
};


static int _ztl_ev_handler(ztl_evloop_t* ev, ztl_connection_t* conn, ZTL_EV_EVENTS events);

int ztl_tcp_server_create(ztl_tcp_server_t** ptcpsvr)
{
    ztl_tcp_server_t* tcpsvr;

    tcpsvr = (ztl_tcp_server_t*)malloc(sizeof(ztl_tcp_server_t));
    memset(tcpsvr, 0, sizeof(ztl_tcp_server_t));

    return 0;
}

int ztl_tcp_server_release(ztl_tcp_server_t* tcpsvr)
{
    if (!tcpsvr) {
        return -1;
    }

    if (tcpsvr->evloop) {
        ztl_evloop_release(tcpsvr->evloop);
    }

    free(tcpsvr);

    return 0;
}

int ztl_tcp_server_init(ztl_tcp_server_t* tcpsvr, ztl_tcp_server_config_t* config)
{
    int rv;
    ztl_evloop_t*   evloop;
    ztl_evconfig_t* evconf;

    if (tcpsvr->inited) {
        return -1;
    }
    tcpsvr->inited = 1;

    memcpy(&tcpsvr->svrconf, config, sizeof(ztl_tcp_server_config_t));

    evconf = &tcpsvr->evconf;
    evconf->listen_addr = string_to_inetaddr(config->listen_ip);
    evconf->listen_port = config->listen_port;

    rv = ztl_evloop_create(&evloop, ZTL_EPM_Default);
    if (rv != 0) {
        return rv;
    }

    ztl_evloop_set_usedata(evloop, tcpsvr);

    rv = ztl_evloop_init(evloop, evconf);

    tcpsvr->evloop = evloop;
    return rv;
}

int ztl_tcp_server_start(ztl_tcp_server_t* tcpsvr)
{
    tcpsvr->running = 1;

    if (tcpsvr->evloop) {
        ztl_evloop_start(tcpsvr->evloop);
    }

    return 0;
}

int ztl_tcp_server_stop(ztl_tcp_server_t* tcpsvr)
{
    if (tcpsvr->running == 0) {
        return 0;
    }

    tcpsvr->running = 0;
    if (tcpsvr->evloop) {
        ztl_evloop_stop(tcpsvr->evloop);
    }
    return 0;
}



//////////////////////////////////////////////////////////////////////////
typedef struct  
{
    ztl_connection_t*   conn;
    ztl_tcp_server_t*   tcpsvr;
    char                buf[32];
}ztl_server_conn_t;
static int _ztl_ev_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, ZTL_EV_EVENTS events)
{
    ztl_server_conn_t*  svrconn;
    ztl_tcp_server_t*   tcpsvr;
    tcpsvr = (ztl_tcp_server_t*)ztl_evloop_get_usedata(evloop);

    if (events == ZEV_NEWCONN)
    {
        // new connection
        svrconn = (ztl_server_conn_t*)malloc(sizeof(ztl_server_conn_t));

        svrconn->conn   = conn;
        svrconn->tcpsvr = tcpsvr;

        conn->userdata  = svrconn;
    }
    else if (events == ZEV_POLLIN)
    {
        // read event
        svrconn = (ztl_server_conn_t*)conn->userdata;

        if (conn->bytes_recved == 0 && conn->rbuf == NULL) {
            // buffer could be from memory pool
            conn->rbuf = (char*)malloc(1014);
            conn->rsize = 1024;
        }
        else {
            // todo: check whether got a whole packet
        }

    }
    else if (events == ZEV_POLLOUT)
    {
        // wriet event
    }

    return 0;
}
