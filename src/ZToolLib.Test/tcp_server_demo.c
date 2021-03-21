#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ZToolLib/ztl_network.h"
#include "ZToolLib/ztl_tcp_server.h"
#include "ZToolLib/ztl_times.h"



static int timer_handler(ztl_evloop_t* evloop, uint64_t timer_id, void* udata)
{
    fprintf(stderr, "timer_handler id=%d, udata=%p\n", (int)timer_id, udata);
    return 0;
}
static int timer_finalizer(ztl_evloop_t* evloop, uint64_t timer_id, void* udata)
{
    // fprintf(stderr, "timer_finalizer id=%d, udata=%p\n", (int)timer_id, udata);
    return 0;
}

static int _tcp_server_ns(ztl_tcp_server_t* tcpsvr, sockhandle_t fd)
{
    fprintf(stderr, "_tcp_server_ns fd=%d\n", (int)fd);
    return 0;
}

static int _tcp_server_event(ztl_tcp_server_t* tcpsvr,
    uint32_t type, void* msg, uint32_t length)
{
    fprintf(stderr, "_tcp_server_event type=%d, msg=%s, length=%d\n",
        type, (char*)msg, length);
    return 0;
}


void tcp_server_demo(int argc, char* argv[])
{
    int rv, times;
    ztl_tcp_server_t*       tcpsvr;
    ztl_tcp_server_config_t lconfig;
    memset(&lconfig, 0, sizeof(ztl_tcp_server_config_t));

    strcpy(lconfig.listen_ip, "127.0.0.1");
    lconfig.listen_port = 13579;
    lconfig.reuse_addr = 1;
    lconfig.poll_timeout = 1000;
    lconfig.newconn_handler = _tcp_server_ns;
    lconfig.event_handler = _tcp_server_event;

    tcpsvr = NULL;
    ztl_tcp_server_create(&tcpsvr);
    ztl_tcp_server_init(tcpsvr, &lconfig);
    ztl_tcp_server_start_no_thread(tcpsvr);

    times = 12;
    while (times--)
    {
        // sleepms(1000);

        rv = ztl_evloop_expire(tcpsvr->evloop, get_timestamp());
        ztl_evloop_looponce(tcpsvr->evloop, 1200);

        // sleepms(50);
        ztl_evloop_addtimer(tcpsvr->evloop, 1500, timer_handler, timer_finalizer, (void*)150);
        ztl_evloop_addtimer(tcpsvr->evloop, 1000, timer_handler, timer_finalizer, (void*)100);
        if (times == 8 || times == 6)
            ztl_evloop_addtimer(tcpsvr->evloop, 60, timer_handler, timer_finalizer, (void*)60);
        fprintf(stderr, "added timer ms_left=%d!\n", rv);
    }

    ztl_tcp_server_release(tcpsvr);
    fprintf(stderr, "test_tcp_sever end!");
}
