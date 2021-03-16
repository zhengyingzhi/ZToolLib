#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ZToolLib/ztl_evloop.h"


int udp_ev_handler(ztl_evloop_t* evloop, ztl_connection_t* conn, int events)
{
    int size, rv;
    uint16_t port;
    char ip[32];
    char buf[1000] = "";
    struct sockaddr_in fromaddr;
    socklen_t addrlen = sizeof(struct sockaddr);
    size = recvfrom(conn->fd, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&fromaddr, &addrlen);

    get_ipport(ip, sizeof(ip) - 1, &port, &fromaddr);
    fprintf(stderr, "udp_ev_handler %s:%d, size=%d,buf=%s\n", ip, port, size, buf);

    if (size > 0) {
        rv = udp_sendex(conn->fd, buf, size, ip, port);
        fprintf(stderr, "udp_ev_handler echo rv:%d\n", rv);
    }

    return 0;
}

void udp_demo(int argc, char* argv[])
{
    int rv;
    sockhandle_t fd;
    ztl_evloop_t* el;

    ztl_evloop_create(&el, 1024);
    ztl_evloop_init(el);
    ztl_evloop_start(el);

    fd = udp_receiver("127.0.0.1", 24680, true);
    rv = ztl_evloop_add(el, fd, ZEV_POLLIN, udp_ev_handler, NULL);
    fprintf(stderr, "ztl_evloop_add rv:%d\n", rv);

    while (1)
    {
#if 0
        ztl_evloop_loop(el, 1000);
#else // another udp echo server demo
        int size;
        char buf[1000] = "";
        struct sockaddr_in fromaddr;
        size = udp_recv(fd, buf, sizeof(buf) - 1, &fromaddr, 1000);
        if (size > 0) {
            rv = udp_send(fd, buf, size, &fromaddr);
            fprintf(stderr, "udp_ev_handler echo rv:%d\n", rv);
        }
#endif//0
    }
}
