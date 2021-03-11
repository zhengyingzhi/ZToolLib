#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ztl_evloop.h"
#include "ztl_network.h"
#include "ztl_tcp_server.h"


static int _read_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, int events)
{
    int     rv, size;
    char    buf[1000];
    ztl_tcp_server_t* tcpsvr;
    fprintf(stderr, "_read_handler\n");

    tcpsvr = (ztl_tcp_server_t*)ztl_evloop_get_usedata(evloop);

    for (;;)
    {
        if (conn->bytes_recved == 0 && conn->rbuf == NULL) {
            // buffer could be from memory pool
            conn->rbuf = (char*)malloc(1024);
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

        if (size == 0)
        {
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
        }
        break;
    }

    return 0;
}

static int _write_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, int events)
{
    fprintf(stderr, "_write_handler\n");
    ztl_evloop_del(evloop, conn->fd, ZEV_POLLOUT);
    ztl_evloop_add(evloop, conn->fd, ZEV_POLLIN, _read_handler, conn->userdata);
}

static int _accept_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, int events)
{
    sockhandle_t        ns;
    ztl_tcp_server_t*    tcpsvr;
    struct sockaddr_in  from_addr;
    char                from_ip[32];
    uint16_t            from_port;

    tcpsvr = (ztl_tcp_server_t*)ztl_evloop_get_usedata(evloop);

    for (;;)
    {
        ns = tcp_accept(tcpsvr->listenfd, &from_addr);
        if (ns == INVALID_SOCKET || ns == 0) {
            return -1;
        }

        inetaddr_to_string(from_ip, sizeof(from_ip) - 1, from_addr.sin_addr.s_addr);
        from_port = ntohs(from_addr.sin_port);
        fprintf(stderr, "_accept ns=%d, from=%s:%d\n", (int)ns, from_ip, from_port);

        // TCP_NODELAY, RCV_BUFSIZE could be set in cbconn function
        set_nonblock(ns, true);
        set_tcp_keepalive(ns, true);

        // add-in
        ztl_evloop_add(evloop, ns, ZEV_POLLIN, _read_handler, tcpsvr);
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
    while (tcpsvr->running)
    {
        ztl_evloop_loop(tcpsvr->evloop, timeout_ms);
    }

    fprintf(stderr, "tcp_server_thread done!\n");
    return 0;
}

//////////////////////////////////////////////////////////////////////////
int ztl_tcp_server_create(ztl_tcp_server_t** ptcpsvr)
{
    ztl_tcp_server_t* tcpsvr;

    tcpsvr = (ztl_tcp_server_t*)malloc(sizeof(ztl_tcp_server_t));
    memset(tcpsvr, 0, sizeof(ztl_tcp_server_t));

    *ptcpsvr = tcpsvr;
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
    sockhandle_t    listenfd;
    ztl_evloop_t*   evloop;

    if (tcpsvr->inited) {
        return -1;
    }
    tcpsvr->inited = 1;

    rv = ztl_evloop_create(&evloop, 1024);
    if (rv != 0) {
        return rv;
    }

    memcpy(&tcpsvr->svrconf, config, sizeof(ztl_tcp_server_config_t));

    if (config->listen_fd == 0 || config->listen_fd == INVALID_SOCKET)
    {
        listenfd = create_socket(SOCK_STREAM);
        set_nonblock(listenfd, 1);
        set_tcp_nodelay(listenfd, 1);
        rv = tcp_listen(listenfd, config->listen_ip, config->listen_port, config->reuse_addr, 512);
        if (rv != 0)
        {
            fprintf(stderr, "ztl_tcp_server_init listen failed at %s:%d\n",
                    config->listen_ip, config->listen_port);
            close_socket(listenfd);
            return rv;
        }
        tcpsvr->listenfd = listenfd;
    }
    else {
        tcpsvr->listenfd = config->listen_fd;
    }

    ztl_evloop_set_usedata(evloop, tcpsvr);

    rv = ztl_evloop_init(evloop);

    tcpsvr->listenfd = listenfd;
    tcpsvr->evloop = evloop;
    return rv;
}

int ztl_tcp_server_start(ztl_tcp_server_t* tcpsvr)
{
    if (tcpsvr->running) {
        return 1;
    }
    tcpsvr->running = 1;

    if (!tcpsvr->evloop)
    {
        return -1;
    }

    ztl_evloop_add(tcpsvr->evloop, tcpsvr->listenfd, ZEV_POLLIN, _accept_handler, tcpsvr);
    ztl_evloop_start(tcpsvr->evloop);

    ztl_thread_create(&tcpsvr->thd, NULL, _tcp_server_thread_func, tcpsvr);

    return 0;
}

int ztl_tcp_server_stop(ztl_tcp_server_t* tcpsvr)
{
    if (!tcpsvr->running) {
        return 0;
    }

    tcpsvr->running = 0;
    if (tcpsvr->evloop) {
        return ztl_evloop_stop(tcpsvr->evloop);
    }
    return 0;
}
