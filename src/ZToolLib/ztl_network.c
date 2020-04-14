#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ztl_network.h"


/// millisec to timeval
static void to_timeval(struct timeval* ptv, int timeMS)
{
    ptv->tv_sec = timeMS / 1000;
    ptv->tv_usec = (timeMS % 1000) * 1000;
}


#ifdef _MSC_VER
void net_init()
{
    WORD wVer = MAKEWORD(2, 2);
    struct WSAData wsa;
    WSAStartup(wVer, &wsa);
}
void net_cleanup()
{
    WSACleanup();
}

int  get_errno() {
    return WSAGetLastError();
}
bool is_wouldblock(int nErrno) {
    return (nErrno == WSAEWOULDBLOCK || nErrno == WSAEINPROGRESS);
}
bool is_einterrupt(int nErrno) {
    return (nErrno == WSAEINTR);
}

#else /* linux platform */

void ignore_sigpipe()
{
    signal(SIGPIPE, SIG_IGN);
}

void net_init(){}
void net_cleanup(){}

int  get_errno() {
    return errno;
}
bool is_wouldblock(int nErrno) {
    return (nErrno == EINPROGRESS || nErrno == EAGAIN || nErrno == EWOULDBLOCK);
}
bool is_einterrupt(int nErrno) {
    return (nErrno == EINTR);
}
#endif//_MSC_VER


/// create a socket
sockhandle_t create_socket(int socktype)
{
    sockhandle_t sockfd = INVALID_SOCKET;

    if (socktype == SOCK_STREAM || socktype == SOCK_DGRAM ||
        socktype == SOCK_RAW || socktype == SOCK_RDM || socktype == SOCK_SEQPACKET) {
#ifdef _MSC_VER
        sockfd = WSASocket(AF_INET, socktype, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
        sockfd = socket(AF_INET, socktype, 0);
#endif//_MSC_VER
    }

    return sockfd;
}

/// close the socket
void close_socket(sockhandle_t sockfd)
{
    if (sockfd != INVALID_SOCKET) {
#ifdef _MSC_VER
        closesocket(sockfd);
#else
        close(sockfd);
#endif//_MSC_VER
    }
}

/// shutdown the socket
int shutdown_socket(sockhandle_t sockfd, int how)
{
    return shutdown(sockfd, how);
}

/// set non-block
int set_nonblock(sockhandle_t sockfd, bool on)
{
#ifdef _MSC_VER
    unsigned long flags = on ? 1 : 0;
    return ioctlsocket(sockfd, FIONBIO, &flags);
#else
    int flags = fcntl(sockfd, F_GETFL, 0);

    flags = on ? flags | O_NONBLOCK : flags & ~O_NONBLOCK;
    return fcntl(sockfd, F_SETFL, flags);
#endif//_MSC_VER
}

/// set reuse addr
int set_reuseaddr(sockhandle_t sockfd, bool on)
{
    int flag = on ? 1 : 0;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag));
}

/// set tcp nodelay
int set_tcp_nodelay(sockhandle_t sockfd, bool on)
{
    int flag = on ? 1 : 0;
    return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));
}

/// set tcp keep alive
int set_tcp_keepalive(sockhandle_t sockfd, bool on)
{
    int flag = on ? 1 : 0;
    return setsockopt(sockfd, IPPROTO_TCP, SO_KEEPALIVE, (const char*)&flag, sizeof(flag));
}

int set_closeonexec(sockhandle_t sockfd)
{
#ifdef _MSC_VER
    (void) SetHandleInformation((HANDLE)sockfd, HANDLE_FLAG_INHERIT, 0);
#else
    fcntl(sockfd, F_SETFD, FD_CLOEXEC);
#endif//_MSC_VER
    return 0;
}

/// set broadcast property
int set_broadcast(sockhandle_t sockfd, bool on)
{
    int flag = on ? 1 : 0;
    return setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&flag, sizeof(flag));
}

/// multicast operations
int join_multicast(sockhandle_t sockfd, const char* multiip, const char* bindip)
{
    struct ip_mreq mreqInfo;
    memset(&mreqInfo, 0, sizeof(mreqInfo));
    inet_pton(AF_INET, multiip, &mreqInfo.imr_multiaddr);

    if (bindip == NULL || *bindip == '\0')
        mreqInfo.imr_interface.s_addr = htonl(INADDR_ANY);
    else
        inet_pton(AF_INET, bindip, &mreqInfo.imr_interface);
    return setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreqInfo, sizeof(mreqInfo));
}
int leave_multicast(sockhandle_t sockfd, const char* multiip, const char* bindip)
{
    struct ip_mreq mreqInfo;
    memset(&mreqInfo, 0, sizeof(mreqInfo));
    inet_pton(AF_INET, multiip, &mreqInfo.imr_multiaddr);

    if (bindip == NULL || *bindip == '\0')
        mreqInfo.imr_interface.s_addr = htonl(INADDR_ANY);
    else
        inet_pton(AF_INET, bindip, &mreqInfo.imr_interface);
    return setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char*)&mreqInfo, sizeof(mreqInfo));
}

int set_multicase_interface(sockhandle_t sockfd, const char* bindip)
{
#ifdef _MSC_VER
    DWORD lAddr;
    inet_pton(AF_INET, bindip, &lAddr);
    return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, (char*)&lAddr, sizeof(lAddr));
#else
    struct in_addr lAddr;
    inet_pton(AF_INET, bindip, &lAddr);
    return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, (char*)&lAddr, sizeof(lAddr));
#endif//_MSC_VER
}

int enable_multicast_loopback(sockhandle_t sockfd, bool enable)
{
    int on = enable ? 1 : 0;
    return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&on, sizeof(on));
}

int set_multicast_ttl(sockhandle_t sockfd, int ttl)
{
    return setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (char*)&ttl, sizeof(ttl));
}


/// set sock buffer size
int set_rcv_buffsize(sockhandle_t sockfd, int bufsize)
{
    return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(bufsize));
}
int set_snd_buffsize(sockhandle_t sockfd, int bufsize)
{
    return setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(bufsize));
}

/// set sock timeout with millisecond
int set_rcv_timeout(sockhandle_t sockfd, int timeout)
{
    struct timeval tv;
    to_timeval(&tv, timeout);
    return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
}
int set_snd_timeout(sockhandle_t sockfd, int timeout)
{
    struct timeval tv;
    to_timeval(&tv, timeout);
    return setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
}

/// get ip and port from sa
int get_ipport(char* ip, int size, uint16_t* port, const struct sockaddr_in* psa)
{
    if (psa)
    {
        inetaddr_to_string(ip, size, psa->sin_addr.s_addr);
        *port = ntohs(psa->sin_port);
        return 0;
    }
    return -1;

}

/// get local client socket's address
int get_localaddr(sockhandle_t sockfd, struct sockaddr_in* localaddr)
{
    socklen_t addrlen;
    return getsockname(sockfd, (struct sockaddr*)localaddr, &addrlen);
}

int get_peeraddr(sockhandle_t sockfd, struct sockaddr_in* peeraddr)
{
    socklen_t addrlen;
    return getpeername(sockfd, (struct sockaddr*)peeraddr, &addrlen);
}

/// make sockaddr_in by ip and port
int make_sockaddr(struct sockaddr_in* psa, const char* ip, uint16_t port)
{
    psa->sin_family = AF_INET;
    psa->sin_port = htons(port);
    psa->sin_addr.s_addr = string_to_inetaddr(ip);
    return 0;
}

uint32_t string_to_inetaddr(const char* ip)
{
    struct in_addr lAddr;
    if (ip == NULL || strlen(ip) == 0)
    {
        lAddr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        //lAddr.s_addr = inet_addr(ip);
        inet_pton(AF_INET, ip, &lAddr);
    }
    return lAddr.s_addr;
}

int inetaddr_to_string(char* ip, int size, uint32_t addr)
{
    struct in_addr lAddr;
    lAddr.s_addr = addr;
    //strcpy(ip, inet_ntoa(lAddr));
    inet_ntop(AF_INET, &lAddr, ip, size);
    return 0;
}

/// get local machine's ip
int get_local_ip(char* ip, int size)
{
    struct hostent* phostinfo = gethostbyname("");
    if (phostinfo == NULL) {
        return -1;
    }
    char* p = inet_ntoa(*((struct in_addr*)(*phostinfo->h_addr_list)));
    if (p) {
        strncpy(ip, p, size);
        ip[size - 1] = '\0';
        return 0;
    }
    return -1;

}

int get_socket_error(sockhandle_t sockfd, int* perror)
{
    int val = 0;
    socklen_t len = sizeof(val);
    int rv = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&val, &len);

    *perror = val;
    return rv;
}

/// peek n bytes from socket's buffer by MSG_PEEK flag
int tcp_msg_peek(sockhandle_t sockfd, char* buf, int len)
{
    if (sockfd == INVALID_SOCKET)
        return -1;

    int rv = 0;
    rv = recv(sockfd, buf, len, MSG_PEEK);
    return rv;
}

/// accept a new socket descriptor
sockhandle_t tcp_accept(sockhandle_t listenfd, struct sockaddr_in* fromaddr)
{
    sockhandle_t ns;
    socklen_t len = sizeof(struct sockaddr_in);

    for (; ; ) {
        if ((ns = accept(listenfd, (struct sockaddr*)fromaddr, &len)) == -1)
        {
            if (is_einterrupt(get_errno()))
                continue;
        }
        break;
    }
    return ns;
}

#define _POLL_ONCE_MAX_N    8
int poll_read(sockhandle_t sockfds[], int nfds, int timeoutMS)
{
    if (nfds > _POLL_ONCE_MAX_N)
    {
        return -2;
    }

    int lRet = 0;
    struct pollfd lFdArray[_POLL_ONCE_MAX_N];
    for (int i = 0; i < nfds; ++i)
    {
        lFdArray[i].fd = sockfds[i];
        lFdArray[i].events = POLLIN;
        lFdArray[i].revents = 0;
    }

#ifdef _MSC_VER
    lRet = WSAPoll(lFdArray, nfds, timeoutMS);
#else
    lRet = poll(lFdArray, nfds, timeoutMS);
#endif//_MSC_VER

    if (lRet > 0)
    {
        int k = 0;
        for (int i = 0; i < nfds; ++i)
        {
            if (lFdArray[i].revents > 0)
            {
                sockfds[k++] = lFdArray[i].fd;
            }
        }
    }

    return lRet;
}

int send_iov(sockhandle_t sockfd, EIOVEC* iovec, int iovec_cnt)
{
#ifdef _MSC_VER
    DWORD lSendCountDWORD;
    int lRet;

    lRet = WSASend(sockfd, iovec, (DWORD)iovec_cnt, &lSendCountDWORD, 0, NULL, NULL);
    if (lRet >= 0) {
        lRet = (int)lSendCountDWORD;
    }

    return lRet;
#else
    ssize_t lRet = writev(sockfd, iovec, iovec_cnt);
    return (int)lRet;
#endif//_MSC_VER
}

//////////////////////////////////////////////////////////////////////////
/// connect to server with a blocking socket
int net_connect(sockhandle_t connfd, const char* ip, uint16_t port)
{
    // connect to the ip:port
    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof laddr);
    make_sockaddr(&laddr, ip, port);
    return connect(connfd, (struct sockaddr*)&laddr, sizeof laddr);
}

/// non-block connect to server, timeout is milli-second
int net_connect_nonb(sockhandle_t connfd, const char* ip, uint16_t port, int timeout)
{
    // set as non-blocking firstly
    set_nonblock(connfd, true);
    set_rcv_timeout(connfd, 0);
    set_snd_timeout(connfd, 0);

    int rv;
    fd_set wtfds;
    struct timeval* ptv = NULL;
    struct timeval tv;
    if (timeout >= 0)
    {
        to_timeval(&tv, timeout);
        ptv = &tv;
    }

    // connect to ip:port
    if ((rv = net_connect(connfd, ip, port)) == 0)
        goto conn_done;
    else if (rv < 0 && !is_wouldblock(get_errno()))
        goto conn_done;

    // use select to check writable event
    FD_ZERO(&wtfds);
    FD_SET(connfd, &wtfds);
    if ((rv = select((int)(connfd + 1), NULL, &wtfds, NULL, ptv)) == 0)
    {
        rv = -1;
        goto conn_done;
    }

    if (FD_ISSET(connfd, &wtfds))
    {
        int error = 0;
        int iret = get_socket_error(connfd, &error);

        // if has error, getsockopt returns -1 on Solaris, returns 0 on Berkely,
        /// and both of them set error type to 'error' variable
        if (iret < 0 || error)
        {
            rv = -1;
            goto conn_done;
        }
        rv = 0;
    }
    else
    {
#if defined(_DEBUG) || defined(DEBUG)
        fprintf(stderr, "select error: sockfd not set [%d]", (int)connfd);
#endif//DEBUG
    }

conn_done:
    return rv;
}

/// pass a tcp socket desc and make listening, return socket descriptor if success, otherwise return -1
int tcp_listen(sockhandle_t listenfd, const char* ip, uint16_t port, bool reuse, int backlog/* = SOMAXCONN*/)
{
    int rv = 0;
    if (listenfd < 0) {
        return -1;
    }

    set_reuseaddr(listenfd, reuse);

    // bind to the addr
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    make_sockaddr(&addr, ip, port);
    if ((rv = bind(listenfd, (struct sockaddr*)&addr, sizeof addr)) < 0)
    {
#ifdef _DEBUG
        fprintf(stderr, "tcp listen: bind to %s:%d failed [%d]", ip, port, get_errno());
#endif//_DEBUG
        return rv;
    }

    if ((rv = listen(listenfd, backlog)) < 0)
    {
#ifdef _DEBUG
        fprintf(stderr, "tcp listen: listen at %s:%d failed [%d]", ip, port, get_errno());
#endif
    }
    return rv;
}

/// read count bytes
int tcp_readn(sockhandle_t sockfd, char* buf, int count)
{
    int len = 0, nread;
    while (len != count)
    {
        nread = recv(sockfd, buf + len, count - len, 0);
        if (nread == 0) {
            return 0;
        }
        else if (nread == -1)
        {
            if (is_wouldblock(get_errno()) || is_einterrupt(get_errno()))
                nread = 0;
            else
                return -1;
        }
        len += nread;
    }
    return len;
}

/// make a simple tcp server, if new event, the callback functions will be invoked
int tcp_simple_server(sockhandle_t listenfd, pfonevent eventcb, void* udata)
{
    int rv;
    sockhandle_t maxfd = listenfd + 1;
    sockhandle_t ns;
    fd_set rdfds0, wtfds0;
    fd_set rdfds, wtfds;
    FD_ZERO(&rdfds0);
    FD_ZERO(&wtfds0);
    FD_ZERO(&rdfds);
    FD_ZERO(&wtfds);
    FD_SET(listenfd, &rdfds0);

    for (; ; )
    {
        memcpy(&rdfds, &rdfds0, sizeof(fd_set));
        memcpy(&wtfds, &wtfds0, sizeof(fd_set));

        if ((rv = select((int)maxfd, &rdfds, &wtfds, NULL, NULL)) < 0)
            break;

        for (sockhandle_t i = 1; i < maxfd; ++i)
        {
            if (FD_ISSET(i, &rdfds) && i != listenfd)
            {
                if (eventcb(udata, i, false) <= 0) {
                    FD_CLR(i, &rdfds0);
                    FD_CLR(i, &wtfds0);
                    continue;
                }

                FD_SET(i, &rdfds0);
            }
            if (FD_ISSET(i, &wtfds) && i != listenfd)
            {
                eventcb(udata, i, true);
            }
        }

        if (FD_ISSET(listenfd, &rdfds))
        {
            struct sockaddr_in clientaddr;
            ns = tcp_accept(listenfd, &clientaddr);
            if (ns < 0) {
                perror("accept failed");
                break;
            }

            if (ns > maxfd)
            {
                maxfd = ns + 1;
            }

            FD_SET(ns, &rdfds0);
        }
    }

    return rv;
}

/// make a tcp echo server
static int echo_func(void* udata, sockhandle_t ns, int isoutev)
{
    (void)isoutev;
    int rv;
    char buf[4096] = "";

    rv = recv(ns, buf, sizeof(buf) - 1, 0);
    if (rv <= 0) {
        return rv;
        //fprintf(stderr, "recv error: %d", errno);
    }

    // echo back msg
    rv = send(ns, buf, rv, 0);
    return rv;
}

int tcp_echo_server(const char* listenip, uint16_t listenport)
{
    int rv;
    sockhandle_t listenfd;
    listenfd = create_socket(SOCK_STREAM);
    rv = tcp_listen(listenfd, listenip, listenport, true, 32);
    if (rv < 0) {
        close_socket(listenfd);
        return rv;
    }
    fprintf(stderr, "tcp echo server listen at %s:%d\n", listenip, listenport);

    return tcp_simple_server(listenfd, echo_func, NULL);
}

/// a udp receiver, return udp socket descriptor
sockhandle_t udp_receiver(const char* localip, uint16_t localport, bool reuseaddr)
{
    int rv = 0;
    sockhandle_t udpFd = create_socket(SOCK_DGRAM);
    if (udpFd < 0) {
        return -1;
    }

    if (reuseaddr) {
        set_reuseaddr(udpFd, true);
    }

    // bind to the addr
    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof laddr);
    make_sockaddr(&laddr, localip, localport);
    if ((rv = bind(udpFd, (struct sockaddr*)&laddr, sizeof laddr)) < 0)
    {
        close_socket(udpFd);
        return rv;
    }
    return udpFd;
}

/// udp recv data with timeout, return 0 timeout, -1 error, else received length, timeout is micro-seconds
int udp_recv(sockhandle_t sockfd, char* buf, int size, struct sockaddr_in* fromaddr, int timeoutms)
{
    if (sockfd == INVALID_SOCKET)
        return -1;

    struct timeval* ptv = NULL;
    struct timeval tv;
    if (timeoutms >= 0)
    {
        tv.tv_sec = timeoutms / 1000000;
        tv.tv_usec = timeoutms % 1000000;
        ptv = &tv;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    int rv = 0;
    rv = select((int)(sockfd + 1), &readfds, NULL, NULL, ptv);

    // timeout or error
    if (rv > 0)
    {
        if (fromaddr == NULL)
        {
            rv = recvfrom(sockfd, buf, size, 0, NULL, NULL);
        }
        else
        {
            socklen_t addrLen = sizeof(*fromaddr);
            rv = recvfrom(sockfd, buf, size, 0, (struct sockaddr*)fromaddr, &addrLen);
        }
    }
    return rv;
}

/// udp send data
int udp_send(sockhandle_t sockfd, const char* buf, int len, struct sockaddr_in* toaddr)
{
    return sendto(sockfd, buf, len, 0, (struct sockaddr*)toaddr, sizeof(struct sockaddr));
}
int udp_sendex(sockhandle_t sockfd, const char* buf, int len, const char* ip, uint16_t port)
{
    struct sockaddr_in toaddr;
    make_sockaddr(&toaddr, ip, port);
    return sendto(sockfd, buf, len, 0, (struct sockaddr*)&toaddr, sizeof(struct sockaddr));
}

/// make a udp echo server
int udp_echo_server(const char* localip, uint16_t localport, pfonrecv msgcallback)
{
    sockhandle_t udpfd;
    udpfd = udp_receiver(localip, localport, true);
    if (udpfd < 0) {
        return -1;
    }

    bool lResult = true;
    int  rv = 0;
    char buf[4096] = "";
    struct sockaddr_in fromaddr;

    while (lResult)
    {
        rv = udp_recv(udpfd, buf, sizeof(buf) - 1, &fromaddr, 100);
        if (rv < 0) {
            perror("udp recv failed");
            break;
        }
        else if (rv == 0) {
            continue;
        }

        if (msgcallback) {
            lResult = msgcallback(udpfd, buf, rv, &fromaddr);
        }
        else {
            udp_send(udpfd, buf, rv, &fromaddr);
        }
    }
    return rv;
}

/// make a pair of socket descriptors, type is SOCK_STREAM or SOCK_DGRAM
int make_sockpair(sockhandle_t sockfds[], int type)
{
    int rv;
    sockhandle_t lsock;
    sockhandle_t connfd, newfd;
    socklen_t addrlen;
    struct sockaddr_in addr;
    struct sockaddr_in fromaddr;

    connfd = -1;
    newfd = -1;

    lsock = socket(AF_INET, type, 0);
    if (lsock < 0) {
        return -1;
    }

    // bind to port 0
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);	//0x7f000001
    rv = bind(lsock, (const struct sockaddr*)&addr, sizeof(addr));
    if (rv < 0 && type != SOCK_DGRAM) {
        perror("make_sockpair bind failed");
        goto PAIR_END;
    }

    if (type == SOCK_STREAM)
    {
        if ((rv = listen(lsock, 2)) < 0) {
            perror("make_sockpair listen failed");
            goto PAIR_END;
        }
    }

    // get actually listen port
    addrlen = sizeof(addr);
    rv = getsockname(lsock, (struct sockaddr*)&addr, &addrlen);
    //fprintf(stderr, "make_sockpair get listen port: %d\n", ntohs(addr.sin_port));

    // connect to 
    connfd = socket(AF_INET, type, 0);
    rv = connect(connfd, (struct sockaddr*)&addr, sizeof(addr));
    if (rv < 0 && type != SOCK_DGRAM) {
        perror("make_sockpair connect failed");
        goto PAIR_END;
    }

    // accept another fd
    if (type == SOCK_STREAM)
    {
        newfd = accept(lsock, (struct sockaddr*)&fromaddr, &addrlen);
        if (newfd < 0)
        {
            perror("make_sockpair accept failed");
            goto PAIR_END;
        }

        // close the listen fd since we have got newfd
        close_socket(lsock);
    }
    else
    {
        newfd = lsock;

        // if udp, connfd also need bind to a port, and newfd do connect
        addr.sin_family = AF_INET;
        addr.sin_port = 0;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        bind(connfd, (const struct sockaddr*)&addr, sizeof(addr));

        rv = getsockname(connfd, (struct sockaddr*)&addr, &addrlen);
        //fprintf(stderr, "make_sockpair get listen port2: %d\n", ntohs(addr.sin_port));

        connect(newfd, (struct sockaddr*)&addr, sizeof(addr));
    }

    // make success
    sockfds[0] = newfd;
    sockfds[1] = connfd;
    rv = 0;

PAIR_END:
    if (rv < 0)
    {
        close_socket(lsock);
        if (connfd > 0)
            close_socket(connfd);
        if (newfd > 0 && newfd != lsock)
            close_socket(newfd);
    }
    return rv;
}
