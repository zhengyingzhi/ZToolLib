#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ztl_common.h"
#include "ztl_errors.h"
#include "ztl_evloop_private.h"
#include "ztl_network.h"
#include "ztl_tcp_client.h"

extern struct ztl_event_ops selectops;


static int _connected_handler(ztl_tcp_client_t* cli, sockhandle_t fd, void* udata);
static int _read_handler(ztl_tcp_client_t* cli, sockhandle_t fd, void* udata);

static ztl_thread_result_t ZTL_THREAD_CALL _io_thread_func(void * args)
{
    uint64_t            prev_time, curr_time;
    ztl_tcp_client_t*   cli;

    cli = (ztl_tcp_client_t*)args;
    cli->io_thread_id = ztl_thread_self();
    prev_time = get_timestamp() - cli->reconnect_ms;

    if (cli->debug_mode) {
        fprintf(stderr, "_io_thread_func running\n");
    }

    while (cli->running)
    {
        if (!ztl_tcp_client_connected(cli))
        {
            if (cli->reconnect_ms == 0) {
                break;
            }

            curr_time = get_timestamp();
            if (curr_time - prev_time < cli->reconnect_ms) {
                sleepms(1);
                continue;
            }

            ztl_tcp_client_do_connect(cli,
                cli->server_ip, cli->server_port, cli->timeout_ms);

            prev_time = curr_time;

            if (!ztl_tcp_client_connected(cli))
                continue;

            if (cli->on_connect)
                cli->on_connect(cli);
        }

        ztl_tcp_client_poll(cli, cli->timeout_ms);
    }

    if (cli->debug_mode) {
        fprintf(stderr, "_io_thread_func done!\n");
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
int ztl_tcp_client_create(ztl_tcp_client_t** pcli)
{
    ztl_tcp_client_t* cli;

    cli = (ztl_tcp_client_t*)malloc(sizeof(ztl_tcp_client_t));
    memset(cli, 0, sizeof(ztl_tcp_client_t));

    cli->evops          = &selectops;
    cli->evops_ctx      = NULL;
    cli->fd             = INVALID_SOCKET;
    cli->reconnect_ms   = 1000;
    cli->timeout_ms     = 1000;
    cli->io_thread_id   = 0;
    cli->non_block      = 1;
    ztl_thread_mutex_init(&cli->lock, NULL);

    *pcli = cli;
    return 0;
}

int ztl_tcp_client_release(ztl_tcp_client_t* cli)
{
    if (!cli) {
        return -1;
    }

    ztl_tcp_client_stop(cli);

    if (cli->evops_ctx) {
        cli->evops->destroy(cli->evops_ctx);
        cli->evops_ctx = NULL;
    }

    free(cli);

    return 0;
}

int ztl_tcp_client_register(ztl_tcp_client_t* cli, const char* ip, uint16_t port)
{
    strncpy(cli->server_ip, ip, sizeof(cli->server_ip) - 1);
    cli->server_port = port;
    return 0;
}

int ztl_tcp_client_reg_on_connect(ztl_tcp_client_t* cli, ztl_tcp_client_connect_pt handler)
{
    cli->on_connect = handler;
    return 0;
}

int ztl_tcp_client_reg_on_disconnect(ztl_tcp_client_t* cli, ztl_tcp_client_disconnect_pt handler)
{
    cli->on_disconnect = handler;
    return 0;
}

int ztl_tcp_client_reg_on_read(ztl_tcp_client_t* cli, ztl_tcp_client_handler_pt handler)
{
    cli->on_read = handler;
    return 0;
}

int ztl_tcp_client_reg_on_write(ztl_tcp_client_t* cli, ztl_tcp_client_handler_pt handler)
{
    cli->on_write = handler;
    return 0;
}

int ztl_tcp_client_do_connect(ztl_tcp_client_t* cli,
    const char* ip, uint16_t port, int timeout_ms)
{
    int rv;
    sockhandle_t fd;

    if (IS_VALID_SOCKET(cli->fd)) {
        ztl_tcp_client_close_fd(cli);
    }

    fd = create_socket(SOCK_STREAM);
    if (cli->non_block)
        set_nonblock(fd, true);
    if (cli->tcp_nodelay)
        set_tcp_nodelay(fd, true);
    if (cli->snd_buffsize)
        set_snd_buffsize(fd, cli->snd_buffsize);
    if (cli->rcv_buffsize)
        set_rcv_buffsize(fd, cli->rcv_buffsize);
    set_tcp_keepalive(fd, true);

    if (cli->debug_mode) {
        fprintf(stderr, "tcp_client_do_connect %s:%d, sync_mode=%d\n",
            ip, port, cli->sync_mode);
    }

    cli->connect_status = ZTL_CLIENT_STATUS_Connecting;

    if (cli->sync_mode)
        rv = net_connect(fd, ip, port);
    else
        rv = net_connect_nonb(fd, ip, port, timeout_ms);

    if (cli->debug_mode) {
        fprintf(stderr, "tcp_client_do_connect rv=%d, errno=%d\n", rv, get_errno());
    }

    if (rv != 0)
    {
        if (is_wouldblock(get_errno()))
        {
            cli->fd = fd;
            cli->evops->add(cli->evops_ctx, cli->fd, ZEV_POLLIN, 0);
        }
        else
        {
            close_socket(fd);
            fd = INVALID_SOCKET;
            cli->connect_status = ZTL_CLIENT_STATUS_DisConnected;
        }
    }
    else
    {
        if (cli->debug_mode) {
            fprintf(stderr, "tcp_client_do_connect succeed!\n");
        }

        cli->connect_status = ZTL_CLIENT_STATUS_Connected;
        cli->fd = fd;

        cli->evops->add(cli->evops_ctx, cli->fd, ZEV_POLLIN, 0);
    }

    return rv;
}

int ztl_tcp_client_init(ztl_tcp_client_t* cli, bool sync_mode)
{
    int rv;

    if (!cli) {
        return ZTL_ERR_NullType;
    }

    if (cli->inited) {
        return ZTL_ERR_AlreadyInited;
    }

    if (cli->running) {
        return ZTL_ERR_AlreadyRunning;
    }

    if (!cli->server_ip[0] || !cli->server_port) {
        return ZTL_ERR_NotSet;
    }

    if (!cli->evops_ctx) {
        cli->evops->init(&cli->evops_ctx);
        cli->evops->start(cli->evops_ctx);
    }

    cli->inited     = 1;
    cli->running    = 1;
    cli->sync_mode  = sync_mode;
    if (sync_mode)
        cli->non_block = 0;

    if (sync_mode) {
        rv = ztl_tcp_client_do_connect(cli,
            cli->server_ip, cli->server_port, cli->timeout_ms);
    }
    else {
        ztl_thread_create(&cli->thd, NULL, _io_thread_func, cli);
        rv = 0;
    }

    return 0;
}

int ztl_tcp_client_stop(ztl_tcp_client_t* cli)
{
    if (!cli->running) {
        return ZTL_ERR_NotRunning;
    }

    cli->running = 0;

    if (IS_VALID_SOCKET(cli->fd)) {
        ztl_tcp_client_close_fd(cli);
    }

    if (cli->evops_ctx) {
        cli->evops->stop(cli->evops_ctx);
    }

    if (cli->thd) {
        void* retval;
        ztl_thread_join(cli->thd, &retval);
        (void)retval;
        cli->thd = 0;
    }
    return 0;
}

int ztl_tcp_client_close_fd(ztl_tcp_client_t* cli)
{
    if (IS_VALID_SOCKET(cli->fd))
    {
        shutdown_socket(cli->fd, 2);
        close_socket(cli->fd);
        cli->fd = INVALID_SOCKET;
        return 0;
    }
    return ZTL_ERR_BadFD;
}

int ztl_tcp_client_set_debug(ztl_tcp_client_t* cli, int on)
{
    cli->debug_mode = on;
    return 0;
}

int ztl_tcp_client_set_timeout(ztl_tcp_client_t* cli, uint32_t timeout_ms)
{
    cli->timeout_ms = timeout_ms;
    return 0;
}

int ztl_tcp_client_set_reconnect(ztl_tcp_client_t* cli, uint32_t reconnect_ms)
{
    cli->reconnect_ms = reconnect_ms;
    return 0;
}

int ztl_tcp_client_set_nodelay(ztl_tcp_client_t* cli)
{
    cli->tcp_nodelay = 1;
    if (IS_VALID_SOCKET(cli->fd)) {
        return set_tcp_nodelay(cli->fd, 1);
    }
    return 0;
}

int ztl_tcp_client_set_snd_buffsize(ztl_tcp_client_t* cli, uint32_t snd_buffsize)
{
    cli->snd_buffsize = snd_buffsize;
    if (IS_VALID_SOCKET(cli->fd)) {
        return set_snd_buffsize(cli->fd, snd_buffsize);
    }
    return 0;
}

int ztl_tcp_client_set_rcv_buffsize(ztl_tcp_client_t* cli, uint32_t rcv_buffsize)
{
    cli->rcv_buffsize = rcv_buffsize;
    if (IS_VALID_SOCKET(cli->fd)) {
        return set_rcv_buffsize(cli->fd, rcv_buffsize);
    }
    return 0;
}

int ztl_tcp_client_add_fd(ztl_tcp_client_t* cli, sockhandle_t fd, int reqevents)
{
    if (!IS_VALID_SOCKET(fd)) {
        return ZTL_ERR_BadFD;
    }

    if (!cli->inited) {
        return ZTL_ERR_NotInited;
    }

    return cli->evops->add(cli->evops_ctx, fd, reqevents, 0);
}

int ztl_tcp_client_poll(ztl_tcp_client_t* cli, uint32_t timeout_ms)
{
    int i, nevents;
    ztl_fired_event_t   fired_events[8];
    ztl_fired_event_t*  fired_ev;

    nevents = cli->evops->poll(cli->evops_ctx, fired_events, 8, timeout_ms);
    for (i = 0; i < nevents; ++i)
    {
        fired_ev = &fired_events[i];

        if (fired_ev->events & ZEV_POLLIN) {
            _read_handler(cli, fired_ev->fd, cli->udata);
        }

        if (fired_ev->events & ZEV_POLLOUT) {
            if (cli->on_write)
                cli->on_write(cli, fired_ev->fd, cli->udata);
        }
    }
    return nevents;
}

int ztl_tcp_client_recv(ztl_tcp_client_t* cli, char* buf, int len)
{
    int rv;
    if (!ztl_tcp_client_connected(cli)) {
        return ZTL_ERR_NotConnected;
    }

    if (!IS_VALID_SOCKET(cli->fd)) {
        return ZTL_ERR_BadFD;
    }

    rv = recv(cli->fd, buf, len, 0);
    if (rv == 0) {
        cli->connect_status = ZTL_CLIENT_STATUS_DisConnected;
    }
    else if (rv == -1)
    {
        if (is_wouldblock(get_errno()) || is_einterrupt(get_errno())) {
            // not disconnnected
        }
        else {
            cli->connect_status = ZTL_CLIENT_STATUS_DisConnected;
        }
    }

    if (cli->connect_status == ZTL_CLIENT_STATUS_DisConnected) {
        rv = cli->evops->del(cli->evops_ctx, cli->fd, ZEV_POLLIN | ZEV_POLLOUT, 0);
    }

    return rv;
}

int ztl_tcp_client_send(ztl_tcp_client_t* cli, const char* buf, int len)
{
    int rv;
    if (!ztl_tcp_client_connected(cli)) {
        return ZTL_ERR_NotConnected;
    }

    if (!IS_VALID_SOCKET(cli->fd)) {
        return ZTL_ERR_BadFD;
    }

    rv = send(cli->fd, buf, len, 0);
    if (rv == 0)
        return 0;
    if (rv == -1)
    {
        if (is_wouldblock(get_errno()) || is_einterrupt(get_errno())) {
            // not disconnnected
        }
        else {
            cli->connect_status = ZTL_CLIENT_STATUS_DisConnected;
        }
    }

    return rv;
}

static int _connected_handler(ztl_tcp_client_t* cli, sockhandle_t fd, void* udata)
{
    if (cli->debug_mode)
        fprintf(stderr, "_connected_handler\n");

    cli->evops->del(cli->evops_ctx, fd, ZEV_POLLOUT, 0);
    cli->evops->add(cli->evops_ctx, fd, ZEV_POLLIN, 0);

    if (cli->on_connect)
        cli->on_connect(cli);

    return 0;
}

static int _read_handler(ztl_tcp_client_t* cli, sockhandle_t fd, void* udata)
{
    if (cli->on_read)
        cli->on_read(cli, fd, udata);

    return 0;
}
