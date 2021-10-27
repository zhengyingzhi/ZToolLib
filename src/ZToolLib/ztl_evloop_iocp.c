#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "ztl_evloop_private.h"
#include "ztl_mempool.h"
#include "ztl_evtimer.h"
#include "ztl_network.h"
#include "ztl_atomic.h"
#include "ztl_utils.h"

/* the file only support on Windows */
#ifdef _WIN32

#include <process.h>
#include <mswsock.h>
#include <guiddef.h>

#pragma comment(lib, "mswsock.lib")

#define _ZTL_MAX_POST_ACCEPT        1
#define _ZTL_IODATA_BUFF_SIZE       16
#define _ZTL_DEF_IOCP_DATA_COUNT    256

#define ZTL_THE_CTX(evloop)         ((iocp_ctx_t*)evloop->evops_ctx)

/* the addr of overlapped for post event and get queued iocp */
typedef struct per_io_data
{
    OVERLAPPED  ovlp;               // must be the first member
    WSABUF      wsaBuf;             //
    char        buf[_ZTL_IODATA_BUFF_SIZE];
    int         nRecvedBytes;
    int         optType;            //operation type for read/write/accept
    SOCKET      sockfd;
#define IO_OPT_READ   1
#define IO_OPT_WRITE  2
#define IO_OPT_ACCEPT 3
}per_io_data_t;

typedef struct per_sock_contex
{
    SOCKET         m_Socket;
    SOCKADDR_IN    m_ClientAddr;
    per_io_data_t* m_pIOData;
}per_sock_contex_t;

typedef struct iocp_ctx_st
{
    HANDLE         hIocp;
    ztl_mempool_t* iodata_mp;

    // AcceptEx and GetAcceptExSockaddrs function pointer
    LPFN_ACCEPTEX             pfnAcceptEx;
    LPFN_GETACCEPTEXSOCKADDRS pfnGetAcceptExSockAddrs;
}iocp_ctx_t;


/// work thread entry
static unsigned __stdcall iocp_loop_entry(void* arg);
/// accept thread when not use AcceptEx
static unsigned __stdcall iocp_accept_entry(void* arg);

static int iocp_init(ztl_evloop_t* evloop);
static int iocp_start(ztl_evloop_t* evloop);
static int iocp_add(ztl_evloop_t* evloop, sockhandle_t fd, int reqevents, int flags);
static int iocp_del(ztl_evloop_t* evloop, sockhandle_t fd, int delevents, int flags);
static int iocp_poll(ztl_evloop_t* evloop, ztl_fired_event_t* fires, int size, int ms);
static int iocp_stop(ztl_evloop_t* evloop);
static int iocp_destroy(ztl_evloop_t* evloop);

/// the impl interface
struct ztl_event_ops iocpops = {
    iocp_init,
    iocp_start,
    iocp_add,
    iocp_del,
    iocp_poll,
    iocp_stop,
    iocp_destroy,
    "iocp",
};

static inline void _set_iodata(ztl_connection_t* conn, per_io_data_t* iodata) {
    conn->internal = iodata;
}

static inline per_io_data_t* _get_iodata(ztl_connection_t* conn) {
    return (per_io_data_t*)conn->internal;
}

static int _iocp_recv(ztl_connection_t* conn)
{
    if (conn->disconncted)
        return 0;

    per_io_data_t* lpIoData = (per_io_data_t*)_get_iodata(conn);

    // do onthing since data is already stored in conn->inbuf when use iocp

    int rv = lpIoData->nRecvedBytes;
    lpIoData->nRecvedBytes = 0;
    return rv == 0 ? -1 : rv;
}

static int _iocp_send(ztl_connection_t* conn)
{
    int rv;
    rv = send(conn->fd, conn->wbuf + conn->bytes_sent, conn->wsize - conn->bytes_sent, 0);
    return rv;
}


static void* load_extend_function(sockhandle_t aListenFD, GUID* aGUID)
{
    DWORD ldwBytes = 0;
    void* lpfnFundAddr = NULL;
    if (SOCKET_ERROR == WSAIoctl(
        aListenFD,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        aGUID,
        sizeof(GUID),
        &lpfnFundAddr,
        sizeof(lpfnFundAddr),
        &ldwBytes,
        NULL,
        NULL))
    {
        return NULL;
    }
    return lpfnFundAddr;
}

static int iocp_extend_function(iocp_ctx_t* pctx, sockhandle_t listenfd)
{
    // AcceptEx and GetAcceptExSockaddrs GUID to aquire function pointer
    GUID lGuidAcceptEx = WSAID_ACCEPTEX;
    GUID lGuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

    // Aquire AcceptEx function pointer, which is an extend function from WinSock2 rfc
    pctx->pfnAcceptEx = (LPFN_ACCEPTEX)load_extend_function(listenfd, &lGuidAcceptEx);
    if (!pctx->pfnAcceptEx) {
        //logdebug("WSAIoctl cannot aquire AcceptEx function pointer, error code: %d\n", get_errno());
        return -1;
    }

    // Aquire GetAcceptExSockAddrs function pointer
    pctx->pfnGetAcceptExSockAddrs = (LPFN_GETACCEPTEXSOCKADDRS)load_extend_function(listenfd, &lGuidGetAcceptExSockAddrs);
    if (!pctx->pfnGetAcceptExSockAddrs) {
        //logdebug("WSAIoctl cannot aquire GetAcceptExSockAddrs function pinter, error code: %d\n", get_errno());

        pctx->pfnAcceptEx = NULL;
        return -1;
    }

    return 0;
}

static ztl_connection_t* _iocp_newconn(ztl_evloop_t* evloop, per_io_data_t* apIOData)
{
    ztl_connection_t* newconn;
    // newconn = (ztl_connection_t*)ztl_mp_alloc(evloop->conn_mp);
	newconn = (ztl_connection_t*)malloc(sizeof(ztl_connection_t));
    memset(newconn, 0, sizeof(ztl_connection_t));

    newconn->fd         = apIOData->sockfd;
    newconn->userdata1 = NULL;
    newconn->userdata2 = NULL;

    newconn->recv       = _iocp_recv;
    newconn->send       = _iocp_send;

    _set_iodata(newconn, apIOData);

#ifdef _DEBUG
    //logdebug("connection: new %d -> %p\n", newconn->sockfd, newconn);
#endif
    return newconn;
}

static int iocp_post_acceptex(ztl_evloop_t* evloop, iocp_ctx_t* pctx)
{
    DWORD lTransBytes = 0, lFlag = 0;
    per_io_data_t* lpIOData = NULL;

    lpIOData = (per_io_data_t*)ztl_mp_alloc(pctx->iodata_mp);

    // if wsaBuf.len is more length, iocp will auto accept&recv data when accept new conection
    lpIOData->wsaBuf.buf = lpIOData->buf;
    lpIOData->wsaBuf.len = (sizeof(SOCKADDR_IN) + 16) * 2;
    lpIOData->optType    = IO_OPT_ACCEPT;
    lpIOData->nRecvedBytes = 0;

    // create a socket descriptor firstly for new connection when accept(which is different from traditional accept) 
    lpIOData->sockfd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == lpIOData->sockfd)
    {
        //logdebug("create socket for Accept failed: %d", get_errno());
        return -1;
    }
#ifdef _DEBUG
    //logdebug("post AcceptEx: new %d -> %p\n", lpIOData->sockfd, lpIOData);
#endif

    set_nonblock(lpIOData->sockfd, true);
    set_tcp_keepalive(lpIOData->sockfd, true);

    // post AcceptEx request
    if (FALSE == pctx->pfnAcceptEx(evloop->listen_fd, lpIOData->sockfd, 
        lpIOData->wsaBuf.buf, lpIOData->wsaBuf.len - ((sizeof(SOCKADDR_IN) + 16) * 2),
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &lTransBytes, &lpIOData->ovlp))
    {
        if (WSA_IO_PENDING != get_errno())
        {
            //logdebug("post AcceptEx failed: %d\n", get_errno());
            return -1;
        }
    }
    return 0;
}

static int iocp_do_accept(ztl_evloop_t* evloop, ztl_connection_t* apConn, per_io_data_t* apIOData)
{
    SOCKADDR_IN* lpClientAddr = NULL;
    SOCKADDR_IN* lpLocalAddr = NULL;
    int lRemoteLen = sizeof(SOCKADDR_IN), lLocalLen = sizeof(SOCKADDR_IN);

    iocp_ctx_t* lpctx = ZTL_THE_CTX(evloop);

    // get client address info by pfnGetAcceptExSockAddrs, which could also get io data
    lpctx->pfnGetAcceptExSockAddrs(apIOData->wsaBuf.buf, apIOData->wsaBuf.len - (sizeof(SOCKADDR_IN) + 16) * 2,
        sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&lpLocalAddr, &lLocalLen, (LPSOCKADDR*)&lpClientAddr, &lRemoteLen);

    if (lpClientAddr != NULL)
    {
        // notify new connection
        ztl_connection_t* conn;
        conn = _iocp_newconn(evloop, apIOData);

        conn->port = ntohs(lpClientAddr->sin_port);
        conn->addr = lpClientAddr->sin_addr.s_addr;

        // evloop->handler(evloop, conn, ZEV_NEWCONN);
    }
    else
    {
        //logdebug("pfnGetAcceptExSockAddrs client address is null\n");
    }

    // post next AcceptEx
    if (-1 == iocp_add(evloop, evloop->listen_fd, ZEV_POLLIN, 0)) {
        return -1;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
/* impl iocp interfaces */
static int iocp_init(ztl_evloop_t* evloop)
{
    iocp_ctx_t* lpctx;
    lpctx = (iocp_ctx_t*)malloc(sizeof(iocp_ctx_t));
    if (!lpctx)
    {
        return -1;
    }

    memset(lpctx, 0, sizeof(iocp_ctx_t));
    evloop->evops_ctx = lpctx;

    // create iocp object
    lpctx->hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    // create connection objects pool
    lpctx->iodata_mp = ztl_mp_create(sizeof(per_io_data_t), _ZTL_DEF_IOCP_DATA_COUNT, 1);

    return 0;
}

static int iocp_start(ztl_evloop_t* evloop)
{
	iocp_ctx_t* lpctx;
	lpctx = ZTL_THE_CTX(evloop);

#if 0
    if (evloop->thrnum <= 0) {
        evloop->thrnum = get_cpu_number();
    }
#endif

    if (iocp_extend_function(lpctx, evloop->listen_fd) == 0)
    {
        // post n AcceptEx I/O requests
        for (int i = 0; i < _ZTL_MAX_POST_ACCEPT; i++) {

            if (-1 == iocp_add(evloop, evloop->listen_fd, ZEV_POLLIN, 0)) {
                break;
            }
        }
    }
    else {
        // start accept thread when not use AcceptEx
        _beginthreadex(NULL, 0, iocp_accept_entry, evloop, 0, NULL);
    }

    return 0;
}

static int iocp_add(ztl_evloop_t* evloop, sockhandle_t fd, int reqevents, int flags)
{
    iocp_ctx_t* lpctx;
    lpctx = ZTL_THE_CTX(evloop);

#if 0
    if (conn->added == 0)
    {
        conn->added = 1;
        CreateIoCompletionPort((HANDLE)conn->sockfd, lpctx->hIocp, (ULONG_PTR)conn, 0);
    }
#endif//0

    DWORD lTransBytes = 0, lFlag = 0;
    per_io_data_t* lpIOData = NULL;

    if (fd == evloop->listen_fd) {
        return iocp_post_acceptex(evloop, lpctx);
    }

    // post async recv or send, it will returned by iocp if io complete
    // lpIOData = _get_iodata(conn);
    lpIOData = NULL;
    if (reqevents & ZEV_POLLIN)
    {
        // lpIOData->wsaBuf.buf = conn->rbuf + conn->bytes_recved;
        // lpIOData->wsaBuf.len = conn->rsize - conn->bytes_recved;
        lpIOData->optType = IO_OPT_READ;

        WSARecv(fd, &lpIOData->wsaBuf, 1, &lTransBytes, &lFlag, &lpIOData->ovlp, NULL);
    }

    if (reqevents & ZEV_POLLOUT)
    {
#if 0
        if (conn->wsize > 0)
        {
            lpIOData->optType = IO_OPT_WRITE;
            lpIOData->wsaBuf.buf = conn->wbuf;
            lpIOData->wsaBuf.len = conn->wsize;
            WSASend(fd, &lpIOData->wsaBuf, 1, &lTransBytes, lFlag, &lpIOData->ovlp, NULL);
        }
#endif//0
    }
    return 0;
}

static int iocp_del(ztl_evloop_t* evloop, sockhandle_t fd, int delevents, int flags)
{
    (void)evloop;
    return 0;
}

static int iocp_poll(ztl_evloop_t* evloop, ztl_fired_event_t* fires, int size, int ms)
{
    DWORD       dwTrans;
    DWORD       dwFlag;
    iocp_ctx_t* lpctx;

    ztl_connection_t*   lpConn;
    per_io_data_t*      lpPerIO;

    dwTrans = 0;
    dwFlag  = 0;
    lpctx = ZTL_THE_CTX(evloop);
    lpConn = NULL;
    lpPerIO = NULL;

    // wait io completion
    BOOL bOK = GetQueuedCompletionStatus(lpctx->hIocp, &dwTrans, (PULONG_PTR)&lpConn, 
        (LPOVERLAPPED*)&lpPerIO, ms);

    if (!bOK && WAIT_TIMEOUT == get_errno()) {
        return 0;
    }

    if (lpConn == NULL || lpPerIO == NULL)
    {
#ifdef _DEBUG
        //logdebug("iocp thread got terminate signal %d.\n", get_errno());
#endif
        return 0;
    }

    // error happens or the socket fd closed by peer
    if (!bOK || (0 == dwTrans && (IO_OPT_READ == lpPerIO->optType || IO_OPT_WRITE == lpPerIO->optType)))
    {
        lpConn->disconncted = 1;
        // lpConn->handler(evloop, lpConn, ZEV_POLLIN);
        return 0;
    }

    // check the operation type here
    if (lpPerIO->optType == IO_OPT_READ) {
        lpPerIO->nRecvedBytes = dwTrans;
        lpConn->rsize += dwTrans;

        lpConn->read_handler(evloop, lpConn, ZEV_POLLIN);
    }
    else if (lpPerIO->optType == IO_OPT_WRITE) {
        lpConn->write_handler(evloop, lpConn, ZEV_POLLOUT);
    }
    else if (lpPerIO->optType == IO_OPT_ACCEPT) {
        iocp_do_accept(evloop, 0, lpPerIO);
    }
    else {
        //logdebug("#ERROR unkonwn operation type %d\n", lpPerIO->optType);
    }

    return 1;
}

static int iocp_stop(ztl_evloop_t* evloop)
{
    iocp_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    atomic_set(&evloop->running, 0);

    if (lpctx->hIocp)
    {
#if 0
        for (int i = 0; i < evloop->thrnum; ++i) {
            PostQueuedCompletionStatus(lpctx->hIocp, 0, 0, NULL);
        }
#endif//0
    }

    return 0;
}

static int iocp_destroy(ztl_evloop_t* evloop)
{
    if (!evloop) {
        return -1;
    }

    iocp_ctx_t* lpctx = ZTL_THE_CTX(evloop);
    if (lpctx->hIocp == NULL) {
        goto EV_DESTROY_FINISH;
    }

    CloseHandle(lpctx->hIocp);

EV_DESTROY_FINISH:
    ztl_mp_release(lpctx->iodata_mp);

    return 0;
}

static unsigned __stdcall iocp_accept_entry(void* arg)
{
    //logdebug("accept thread running...\n");
    ztl_evloop_t*   evloop  = (ztl_evloop_t*)arg;
    iocp_ctx_t*     lpctx   = ZTL_THE_CTX(evloop);

    int rv;
    ztl_connection_t*   lpNewConn;
    per_io_data_t*      lpIoData;

    while (evloop->running)
    {
        // accept new socket (todo: try waiting)
        rv = poll_read(evloop->listen_fd, evloop->timeout_ms);
        if (rv == 0) {
            continue;
        }
        else if (rv < 0)
        {
            if (!is_wouldblock(get_errno())) {
                perror("accept failed");
                break;
            }
            continue;
        }

        struct sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET ns = accept(evloop->listen_fd, (struct sockaddr*)&clientAddr, &addrLen);

        // TCP_NODELAY, RCV_BUFSIZE could be set in cbconn function
        set_tcp_nodelay(ns, true);
        set_nonblock(ns, true);

        // get a new ztl_connection_t and associate to iocp
        lpIoData = (per_io_data_t*)ztl_mp_alloc(lpctx->iodata_mp);
        lpNewConn = _iocp_newconn(evloop, lpIoData);

        lpNewConn->port = ntohs(clientAddr.sin_port);
        lpNewConn->addr = clientAddr.sin_addr.s_addr;

#ifdef _DEBUG
        char addrtext[32] = "";
        inetaddr_to_string(addrtext, sizeof(addrtext), clientAddr.sin_addr.s_addr);
        //logdebug("Accepted: %d->%s\n", ns, addrtext);
#endif

        // notify new connection
        // evloop->handler(evloop, lpNewConn, ZEV_NEWCONN);
    }
    return 0;
}


#endif//_WIN32
