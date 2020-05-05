
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ztl_evloop_private.h"
#include "ztl_network.h"
#include "ztl_times.h"
#include "ztl_utils.h"

int ztl_do_recv(ztl_connection_t* conn)
{
    int rv;
    rv = recv(conn->sockfd, conn->rbuf + conn->bytes_recved, conn->rsize - conn->bytes_recved, 0);
    if (rv == 0) {
        conn->disconncted = 1;
    }
    else if (rv > 0) {
        conn->bytes_recved += rv;
    }

    return rv;
}

int ztl_do_send(ztl_connection_t* conn)
{
    int rv;
    rv = send(conn->sockfd, conn->wbuf, conn->wsize, 0);
    if (rv > 0) {
        conn->bytes_sent += rv;
    }
    return rv;
}

ztl_connection_t* ztl_do_accept(ztl_evloop_t* evloop)
{
    ztl_connection_t* newconn;

    sockhandle_t ns;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    newconn = NULL;

    do {
        ns = accept(evloop->listen_conn.sockfd, (struct sockaddr*)&addr, &addrlen);
        if (ns < 0)
        {
            if (is_einterrupt(get_errno())) {
                continue;
            }

            return NULL;
        }
        break;
    } while (true);

    // TCP_NODELAY, RCV_BUFSIZE could be set in cbconn function
    set_nonblock(ns, true);
    set_tcp_keepalive(ns, true);

    // alloc a new connection
    newconn = ztl_new_connection(evloop, ns);
    newconn->port = ntohs(addr.sin_port);
    newconn->addr = addr.sin_addr.s_addr;

#ifdef ZTL_DEBUG
    char addrtext[64];
    inet_ntop(AF_INET, &addr.sin_addr, addrtext, sizeof(addrtext));
    sprintf(addrtext, "%s:%d", addrtext, ntohs(addr.sin_port));
    fprintf(stderr, "connection: new %ld %s -> %p\n", (long)ns, addrtext, newconn);
#endif//ZTL_DEBUG

    return newconn;
}

ztl_connection_t* ztl_new_connection(ztl_evloop_t* evloop, sockhandle_t ns)
{
    ztl_connection_t* conn;
    conn = (ztl_connection_t*)ztl_mp_alloc(evloop->conn_mp);
    memset(conn, 0, sizeof(ztl_connection_t));

    conn->sockfd    = ns;
    conn->evloop    = evloop;
    conn->userdata  = NULL;
    
    conn->recv      = ztl_do_recv;
    conn->send      = ztl_do_send;

    return conn;
}

void ztl_free_connection(ztl_evloop_t* evloop, ztl_connection_t* conn)
{
    if (conn->sockfd != INVALID_SOCKET)
    {
        close_socket(conn->sockfd);
        conn->sockfd = INVALID_SOCKET;
    }

    ztl_mp_free(evloop->conn_mp, conn);
}

void ztl_evloop_update_polltime(ztl_evloop_t* evloop)
{
    evloop->timepoint = get_timestamp();
}
