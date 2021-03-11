
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ztl_evloop_private.h"
#include "ztl_network.h"
#include "ztl_mem.h"
#include "ztl_times.h"
#include "ztl_utils.h"


int ztl_do_recv(ztl_connection_t* conn)
{
    int rv;
    rv = recv(conn->fd, conn->rbuf + conn->bytes_recved, conn->rsize - conn->bytes_recved, 0);
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
    rv = send(conn->fd, conn->wbuf, conn->wsize, 0);
    if (rv > 0) {
        conn->bytes_sent += rv;
    }
    return rv;
}

ztl_connection_t* ztl_do_accept(ztl_evloop_t* evloop, sockhandle_t listenfd)
{
    ztl_connection_t* newconn;

    sockhandle_t ns;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    newconn = NULL;

    do {
        ns = tcp_accept(listenfd, &addr);
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
    newconn = ztl_connection_new(evloop, ns, addr.sin_addr.s_addr, ntohs(addr.sin_port));
    ztl_connection_save(evloop, newconn);

#ifdef ZTL_DEBUG
    char addrtext[64];
    inet_ntop(AF_INET, &addr.sin_addr, addrtext, sizeof(addrtext));
    sprintf(addrtext, "%s:%d", addrtext, ntohs(addr.sin_port));
    fprintf(stderr, "connection: new %ld %s -> %p\n", (long)ns, addrtext, newconn);
#endif//ZTL_DEBUG

    return newconn;
}


void ztl_evloop_update_polltime(ztl_evloop_t* evloop)
{
    evloop->timepoint = get_timestamp();
}
