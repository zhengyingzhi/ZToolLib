#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ZToolLib/ztl_common.h"
#include "ZToolLib/ztl_errors.h"
#include "ZToolLib/ztl_network.h"
#include "ZToolLib/ztl_tcp_server.h"
#include "ZToolLib/ztl_times.h"
#include "ZToolLib/ztl_utils.h"


// TODO: 再实现一个行情转发系统
/* 2个 connection to server，2个 listen on 8500 & 8501
 * 1. 向mdgw发起登录请求，并保持心跳
 * 2. 从mdgw接收行情，并转发给client端，没有客户端则直接抛弃数据包
 * 3. 接收client端的登录请求，并返回应答（大端模式数据），且要操持心跳
 * 4. 
 */

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
    ztl_evloop_add(evloop, conn->fd, ZEV_POLLIN, _read_handler, conn->userdata1);

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


void trans_md_demo(int argc, char* argv[])
{}

int trans_server_demo(int argc, char* argv[])
{
    int rv;
    sockhandle_t    listenfd, conn_fd;
    ztl_evloop_t*   evloop;
    int reuseaddr = 1, tcp_nodelay = 1;
    const char* listen_ip = "";
    const char* server_ip = "39.104.95.144";
    uint16_t listen_port = 13579;
    uint16_t server_port = 5001;

    listenfd = tcp_listen_ex(listen_ip, listen_port, reuseaddr, tcp_nodelay);
    if (!IS_VALID_SOCKET(listenfd))
    {
        fprintf(stderr, "ztl_tcp_server_init listen failed at %s:%d\n",
            listen_ip, listen_port);
        return ZTL_ERR_BadFD;
    }

    rv = ztl_evloop_create(&evloop, 1024);
    if (rv != 0) {
        return rv;
    }

    rv = ztl_evloop_init(evloop);
    fprintf(stderr, "ztl_evloop_init rv=%d\n", rv);

    ztl_evloop_add(evloop, listenfd, ZEV_POLLIN, _accept_handler, NULL);

    conn_fd = create_socket(SOCK_STREAM);
    set_nonblock(conn_fd, 1);
    set_tcp_nodelay(conn_fd, 1);
    net_connect_nonb(conn_fd, server_ip, server_port, 3000);

    fprintf(stderr, "trans_server_demo end!\n");
    return 0;
}
