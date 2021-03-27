
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ztl_evloop_private.h"
#include "ztl_network.h"
#include "ztl_mem.h"
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
    ztl_connection_t*   newconn;
    sockhandle_t        ns;
    struct sockaddr_in  addr;

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


ztl_timer_event_t* ztl_timer_node_new(ztl_evloop_t* evloop)
{
    ztl_timer_event_t* node;

    if (evloop->idle_timers) {
        node = evloop->idle_timers;
        evloop->idle_timers = node->next;
        node->prev = NULL;
        node->next = NULL;
    }
    else {
        node = (ztl_timer_event_t*)ALLOC(sizeof(ztl_timer_event_t));
        memset(node, 0, sizeof(ztl_timer_event_t));
    }
    return node;
}

ztl_timer_event_t* ztl_timer_node_find(ztl_evloop_t* evloop, uint64_t timer_id)
{
    ztl_timer_event_t* node;
    node = evloop->work_timers;
    while (node)
    {
        if (node->timer_id == timer_id) {
            return node;
        }
        node = node->next;
    }
    return node;
}

int ztl_timer_node_save(ztl_evloop_t* evloop, ztl_timer_event_t* new_node)
{
    ztl_timer_event_t* next_node;
    if (!evloop->work_timers) {
        new_node->prev = new_node->next = NULL;
        evloop->work_timers = new_node;
    }
    else {
        next_node = evloop->work_timers->next;
        new_node->next = next_node;
        evloop->work_timers->next = new_node;
        new_node->prev = evloop->work_timers;
        if (next_node)
            next_node->prev = new_node;
    }

    return 0;
}

int ztl_timer_node_remove(ztl_evloop_t* evloop, ztl_timer_event_t* node)
{
    if (node->next) {
        node->next->prev = node->prev;
    }
    if (node->prev) {
        node->prev->next = node->next;
    }
    if (evloop->work_timers == node) {
        if (node->prev)
            evloop->work_timers = node->prev;
        else
            evloop->work_timers = node->next;
    }

    node->prev = NULL;
    node->next = evloop->idle_timers;
    evloop->idle_timers = node;
    return 0;
}

int ztl_timer_node_free_all(ztl_evloop_t* evloop)
{
    int n;
    ztl_timer_event_t* node;

    n = 0;
    while (evloop->idle_timers)
    {
        node = evloop->idle_timers;
        evloop->idle_timers = node->next;
        FREE(node);
        ++n;
    }

    while (evloop->work_timers)
    {
        node = evloop->work_timers;
        evloop->work_timers = node->next;
        if (node->handler)
            node->handler(evloop, node->timer_id, node->udata);
        if (node->finalizer)
            node->finalizer(evloop, node->timer_id, node->udata);
        FREE(node);
        ++n;
    }

    evloop->idle_timers = NULL;
    evloop->work_timers = NULL;
    return n;
}
