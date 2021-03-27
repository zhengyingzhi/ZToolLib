#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ztl_common.h"
#include "ztl_errors.h"
#include "ztl_evloop.h"
#include "ztl_network.h"
#include "ztl_tcp_server.h"
#include "ztl_utils.h"


#define ZTL_DEFAULT_NS_TIMEOUT_MS   15000

static int _read_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, int events)
{
    ZTL_NOTUSED(events);

    int     rv, size;
    char    buf[1000];
    // ztl_tcp_server_t* tcpsvr;
    fprintf(stderr, "_read_handler\n");

    // tcpsvr = (ztl_tcp_server_t*)ztl_evloop_get_usedata(evloop);

    for (;;)
    {
        if (conn->bytes_recved == 0 && conn->rbuf == NULL) {
            // buffer could be from memory pool
            conn->rbuf = (char*)malloc(1024);
            conn->rbuf[0] = '\0';
            conn->rsize = 1023;
        }
        else {
            // todo: check whether got a whole packet
        }

        memset(buf, 0, sizeof(buf) - 1);
        conn->bytes_recved = 0;

        size = conn->recv(conn);
        conn->rbuf[size] = '\0';  // FIXME
        fprintf(stderr, "read_handler fd=%d, size=%d, buf=%s\n", (int)conn->fd, size, conn->rbuf);

        // use conn->rbuf,bytesrecved

        if (size == 0 || (size < 0 && !is_wouldblock(get_errno())))
        {
            // fprintf(stderr, "read_handler fd=%d broken!\n", (int)conn->fd);
            ztl_evloop_del(evloop, conn->fd, ZEV_POLLIN | ZEV_POLLOUT);

            ztl_connection_remove(evloop, conn->fd);
            conn->free(conn);
            conn = NULL;
            return 0;
        }
        else
        {
            // echo
            conn->wbuf = conn->rbuf;
            conn->wsize = size;
            rv = conn->send(conn);
            fprintf(stderr, "read_handler echo rv:%d\n", rv);
        }
        break;
    }

    return 0;
}

static int _write_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, int events)
{
    ZTL_NOTUSED(events);
    fprintf(stderr, "_write_handler\n");
    ztl_evloop_del(evloop, conn->fd, ZEV_POLLOUT);
    ztl_evloop_add(evloop, conn->fd, ZEV_POLLIN, _read_handler, conn->userdata);

    return 0;
}

static int _timeout_handler(ztl_evloop_t* evloop, uint64_t timer_id, void* udata)
{
    fprintf(stderr, "_timeout_handler\n");
    sockhandle_t fd;
    union_dtype_t d;
    d.ptr = udata;
    fd = d.i32;
    shutdown_socket(fd, 2);
    return 0;
}

static int _accept_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, int events)
{
    ZTL_NOTUSED(conn);
    ZTL_NOTUSED(events);

    sockhandle_t        ns;
    ztl_tcp_server_t*   tcpsvr;
    struct sockaddr_in  from_addr;
    char                from_ip[32];
    uint16_t            from_port;

    tcpsvr = (ztl_tcp_server_t*)ztl_evloop_get_usedata(evloop);

    for (;;)
    {
        ns = tcp_accept(tcpsvr->listenfd, &from_addr);
        if (!IS_VALID_SOCKET(ns)) {
            return ZTL_ERR_BadFD;
        }

        get_ipport(from_ip, sizeof(from_ip) - 1, &from_port, &from_addr);
        fprintf(stderr, "_accept ns=%d, from=%s:%d\n", (int)ns, from_ip, from_port);

        // TCP_NODELAY, RCV_BUFSIZE could be set in cbconn function
        set_nonblock(ns, true);
        set_tcp_keepalive(ns, true);

        ztl_evloop_add(evloop, ns, ZEV_POLLIN, _read_handler, tcpsvr);

        union_dtype_t d;
        d.i32 = ns;
        ztl_evloop_addtimer(evloop, ZTL_DEFAULT_NS_TIMEOUT_MS, _timeout_handler,
            NULL, d.ptr);
    }

    return 0;
}

static ztl_thread_result_t ZTL_THREAD_CALL _tcp_server_thread_func(void * args)
{
    int timeout_ms;
    ztl_tcp_server_t* tcpsvr;
    tcpsvr = (ztl_tcp_server_t*)args;
    timeout_ms = tcpsvr->svrconf.poll_timeout;

    fprintf(stderr, "tcp_server_thread running\n");
    tcpsvr->running = 1;
    ztl_evloop_loop(tcpsvr->evloop, timeout_ms);
    tcpsvr->running = 0;

    fprintf(stderr, "tcp_server_thread done!\n");
    return 0;
}

//////////////////////////////////////////////////////////////////////////
int ztl_tcp_server_create(ztl_tcp_server_t** ptcpsvr)
{
    ztl_tcp_server_t* tcpsvr;

    tcpsvr = (ztl_tcp_server_t*)malloc(sizeof(ztl_tcp_server_t));
    if (!tcpsvr) {
        return ZTL_ERR_AllocFailed;
    }
    memset(tcpsvr, 0, sizeof(ztl_tcp_server_t));

    *ptcpsvr = tcpsvr;
    return 0;
}

int ztl_tcp_server_release(ztl_tcp_server_t* tcpsvr)
{
    if (!tcpsvr) {
        return ZTL_ERR_NullType;
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
    sockhandle_t    listenfd;
    ztl_evloop_t*   evloop;

    if (tcpsvr->inited) {
        return ZTL_ERR_NotInited;
    }
    tcpsvr->inited = 1;

    rv = ztl_evloop_create(&evloop, 1024);
    if (rv != 0) {
        return rv;
    }

    memcpy(&tcpsvr->svrconf, config, sizeof(ztl_tcp_server_config_t));

    if (!IS_VALID_SOCKET(config->listen_fd))
    {
        listenfd = tcp_listen_ex(config->listen_ip, config->listen_port,
            config->reuse_addr, config->tcp_nodelay);
        if (!IS_VALID_SOCKET(listenfd))
        {
            fprintf(stderr, "ztl_tcp_server_init listen failed at %s:%d\n",
                    config->listen_ip, config->listen_port);
            return ZTL_ERR_BadFD;
        }
        tcpsvr->listenfd = listenfd;
    }
    else {
        tcpsvr->listenfd = config->listen_fd;
    }

    ztl_evloop_set_usedata(evloop, tcpsvr);

    rv = ztl_evloop_init(evloop);

    tcpsvr->evloop = evloop;
    return rv;
}

int ztl_tcp_server_start(ztl_tcp_server_t* tcpsvr)
{
    if (tcpsvr->running) {
        return ZTL_ERR_AlreadyRunning;
    }
    tcpsvr->running = 1;

    if (!tcpsvr->evloop) {
        return ZTL_ERR_NotCreated;
    }

    ztl_evloop_add(tcpsvr->evloop, tcpsvr->listenfd,
        ZEV_POLLIN, _accept_handler, tcpsvr);
    ztl_evloop_start(tcpsvr->evloop);

    ztl_thread_create(&tcpsvr->thd, NULL, _tcp_server_thread_func, tcpsvr);

    return 0;
}

int ztl_tcp_server_start_no_thread(ztl_tcp_server_t* tcpsvr)
{
    if (tcpsvr->running) {
        return ZTL_ERR_AlreadyRunning;
    }
    tcpsvr->running = 1;

    if (!tcpsvr->evloop) {
        return ZTL_ERR_NotCreated;
    }

    ztl_evloop_add(tcpsvr->evloop, tcpsvr->listenfd,
        ZEV_POLLIN, _accept_handler, tcpsvr);
    ztl_evloop_start(tcpsvr->evloop);

    tcpsvr->thd = 0;

    return 0;
}

int ztl_tcp_server_stop(ztl_tcp_server_t* tcpsvr)
{
    if (!tcpsvr->running) {
        return ZTL_ERR_NotRunning;
    }

    tcpsvr->running = 0;
    if (tcpsvr->evloop) {
        return ztl_evloop_stop(tcpsvr->evloop);
    }
    return 0;
}
