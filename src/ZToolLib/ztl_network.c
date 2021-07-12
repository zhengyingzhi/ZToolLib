#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ztl_errors.h"
#include "ztl_network.h"


/// millisec to timeval
static void to_timeval(struct timeval* ptv, int time_ms)
{
    ptv->tv_sec = time_ms / 1000;
    ptv->tv_usec = (time_ms % 1000) * 1000;
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

static char win_err_buf[64] = "";
const char* get_strerror(int no)
{
    // also see winerror.h
    switch (no)
    {
    case WSAEINTR:          return "WSAEINTR";
    case WSAEBADF:          return "WSAEBADF";
    case WSAEACCES:         return "WSAEACCES";
    case WSAEINVAL:         return "WSAEINVAL";
    case WSAEMFILE:         return "WSAEMFILE";
    case WSAEWOULDBLOCK:    return "WSAEWOULDBLOCK";
    case WSAEINPROGRESS:    return "WSAEINPROGRESS";
    case WSAENOTSOCK:       return "WSAENOTSOCK";
    case WSAEDESTADDRREQ:   return "WSAEDESTADDRREQ";
    case WSAEMSGSIZE:       return "WSAEMSGSIZE";
    case WSAEADDRINUSE:     return "WSAEADDRINUSE";
    case WSAECONNRESET:     return "WSAECONNRESET";
    case WSAENOBUFS:        return "WSAENOBUFS";
    case WSAETIMEDOUT:      return "WSAETIMEDOUT";
    case WSAECONNREFUSED:   return "WSAECONNREFUSED";
    case WSASYSNOTREADY:    return "WSASYSNOTREADY";
    case WSAVERNOTSUPPORTED:return "WSAVERNOTSUPPORTED";
    case WSANOTINITIALISED: return "WSANOTINITIALISED";
    default:
        return strerror(no);
        // sprintf(win_err_buf, "Unknown:%d", no);
        // return win_err_buf;
    }
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

#if 0 // -std=c99 or -std=gnu99
struct ip_mreq
{
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
};
#endif

void ignore_sigpipe()
{
    signal(SIGPIPE, SIG_IGN);
}

void net_init(){}
void net_cleanup(){}

const char* get_strerror(int no) {
    return strerror(no);
}
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


static void net_set_error(char *err, size_t errlen, const char *fmt, ...)
{
    va_list ap;

    if (err == NULL)
        return;
    va_start(ap, fmt);
    vsnprintf(err, errlen, fmt, ap);
    va_end(ap);
}


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

void close_socket(sockhandle_t sockfd)
{
    if (IS_VALID_SOCKET(sockfd))
    {
#ifdef _MSC_VER
        closesocket(sockfd);
#else
        close(sockfd);
#endif//_MSC_VER
    }
}

int shutdown_socket(sockhandle_t sockfd, int how)
{
    if (IS_VALID_SOCKET(sockfd)) {
        return shutdown(sockfd, how);
    }
    return ZTL_ERR_BadFD;
}

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

int set_reuseaddr(sockhandle_t sockfd, bool on)
{
    int flag = on ? 1 : 0;
    return setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(flag));
}

int set_tcp_nodelay(sockhandle_t sockfd, bool on)
{
    int flag = on ? 1 : 0;
    return setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(flag));
}

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

int set_broadcast(sockhandle_t sockfd, bool on)
{
    int flag = on ? 1 : 0;
    return setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (const char*)&flag, sizeof(flag));
}

int join_multicast(sockhandle_t sockfd, const char* multiip, const char* bindip)
{
    struct ip_mreq mreq_info;
    memset(&mreq_info, 0, sizeof(mreq_info));
    inet_pton(AF_INET, multiip, &mreq_info.imr_multiaddr);

    if (bindip == NULL || *bindip == '\0')
        mreq_info.imr_interface.s_addr = htonl(INADDR_ANY);
    else
        inet_pton(AF_INET, bindip, &mreq_info.imr_interface);

    // ---- below is for linux ? ----
    // setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_IF, &mreq_info, sizeof(mreq_info));
    return setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char*)&mreq_info, sizeof(mreq_info));
}

int leave_multicast(sockhandle_t sockfd, const char* multiip, const char* bindip)
{
    struct ip_mreq mreq_info;
    memset(&mreq_info, 0, sizeof(mreq_info));
    inet_pton(AF_INET, multiip, &mreq_info.imr_multiaddr);

    if (bindip == NULL || *bindip == '\0')
        mreq_info.imr_interface.s_addr = htonl(INADDR_ANY);
    else
        inet_pton(AF_INET, bindip, &mreq_info.imr_interface);
    return setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (const char*)&mreq_info, sizeof(mreq_info));
}

int set_multicast_interface(sockhandle_t sockfd, const char* bindip)
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


int set_rcv_buffsize(sockhandle_t sockfd, int bufsize)
{
    return setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&bufsize, sizeof(bufsize));
}

int set_snd_buffsize(sockhandle_t sockfd, int bufsize)
{
    return setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&bufsize, sizeof(bufsize));
}

int set_rcv_timeout(sockhandle_t sockfd, int timeout_ms)
{
    struct timeval tv;
    to_timeval(&tv, timeout_ms);
    return setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
}

int set_snd_timeout(sockhandle_t sockfd, int timeout_ms)
{
    struct timeval tv;
    to_timeval(&tv, timeout_ms);
    return setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
}

int get_ipport(char* ip, int size, uint16_t* port, const struct sockaddr_in* psa)
{
    if (!psa) {
        return ZTL_ERR_InvalParam;
    }

    if (ip)
        inetaddr_to_string(ip, size, psa->sin_addr.s_addr);
    if (port)
        *port = ntohs(psa->sin_port);
    return 0;

}

int get_localaddr(sockhandle_t sockfd, struct sockaddr_in* localaddr)
{
    socklen_t addrlen = sizeof(struct sockaddr);
    return getsockname(sockfd, (struct sockaddr*)localaddr, &addrlen);
}

int get_peeraddr(sockhandle_t sockfd, struct sockaddr_in* peeraddr)
{
    socklen_t addrlen = sizeof(struct sockaddr);
    return getpeername(sockfd, (struct sockaddr*)peeraddr, &addrlen);
}

int make_sockaddr(struct sockaddr_in* psa, const char* ip, uint16_t port)
{
    psa->sin_family      = AF_INET;
    psa->sin_port        = htons(port);
    psa->sin_addr.s_addr = string_to_inetaddr(ip);
    return 0;
}

uint32_t string_to_inetaddr(const char* ip)
{
    struct in_addr lAddr;
    if (ip == NULL || *ip == '\0') {
        lAddr.s_addr = htonl(INADDR_ANY);
    }
    else {
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
    int val = 0, rv;
    socklen_t len = sizeof(val);
    rv = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&val, &len);
    *perror = val;
    return rv;
}

int tcp_msg_peek(sockhandle_t sockfd, char* buf, int len)
{
    if (!IS_VALID_SOCKET(sockfd))
        return ZTL_ERR_BadFD;

    int rv = 0;
    rv = recv(sockfd, buf, len, MSG_PEEK);
    return rv;
}

sockhandle_t tcp_accept(sockhandle_t listenfd, struct sockaddr_in* fromaddr)
{
    sockhandle_t ns;
    socklen_t len = sizeof(struct sockaddr_in);

    for (; ; )
    {
        if ((ns = accept(listenfd, (struct sockaddr*)fromaddr, &len)) == -1)
        {
            if (is_einterrupt(get_errno()))
                continue;
            return -1;
        }
        break;
    }
    return ns;
}

sockhandle_t tcp_accept2(sockhandle_t listenfd, char ip[], int sz, uint16_t* port)
{
    sockhandle_t ns;
    struct sockaddr_in fromaddr;
    socklen_t len = sizeof(struct sockaddr_in);

    for (; ; )
    {
        if ((ns = accept(listenfd, (struct sockaddr*)&fromaddr, &len)) == -1)
        {
            if (is_einterrupt(get_errno()))
                continue;
            return -1;
        }

        get_ipport(ip, sz, port, &fromaddr);
        break;
    }
    return ns;
}

#define _POLL_ONCE_MAX_N    8
int poll_reads(sockhandle_t sockfds[], int nfds, int timeout_ms)
{
    if (nfds > _POLL_ONCE_MAX_N) {
        return -2;
    }

    int n = 0;
    struct pollfd fds[_POLL_ONCE_MAX_N];
    for (int i = 0; i < nfds; ++i)
    {
        fds[i].fd = sockfds[i];
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }

#ifdef _MSC_VER
    n = WSAPoll(fds, nfds, timeout_ms);
#else
    n = poll(fds, nfds, timeout_ms);
#endif//_MSC_VER

    if (n > 0)
    {
        int k = 0;
        for (int i = 0; i < nfds; ++i)
        {
            if (fds[i].revents > 0)
            {
                sockfds[k++] = fds[i].fd;
            }
        }
    }

    return n;
}

int poll_read(sockhandle_t sockfd, int timeout_ms)
{
    int n = 0;
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLIN;
    fds.revents = 0;

#ifdef _MSC_VER
    n = WSAPoll(&fds, 1, timeout_ms);
#else
    n = poll(&fds, 1, timeout_ms);
#endif//_MSC_VER

    if (n > 0)
        return fds.revents > 0 ? 1 : 0;
    return n;
}

int send_iov(sockhandle_t sockfd, EIOVEC* iovec, int iovec_cnt)
{
#ifdef _MSC_VER
    DWORD send_count;
    int rv;

    rv = WSASend(sockfd, iovec, (DWORD)iovec_cnt, &send_count, 0, NULL, NULL);
    if (rv >= 0) {
        rv = (int)send_count;
    }

    return rv;
#else
    ssize_t rv = writev(sockfd, iovec, iovec_cnt);
    return (int)rv;
#endif//_MSC_VER
}

//////////////////////////////////////////////////////////////////////////
int net_connect(sockhandle_t sockfd, const char* ip, uint16_t port)
{
    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof laddr);
    make_sockaddr(&laddr, ip, port);
    return connect(sockfd, (struct sockaddr*)&laddr, sizeof laddr);
}

int net_connect_nonb(sockhandle_t sockfd, const char* ip, uint16_t port, int timeout_ms)
{
    int rv;
    fd_set wtfds;
    struct timeval* ptv = NULL;
    struct timeval tv;

    set_nonblock(sockfd, true);
    set_rcv_timeout(sockfd, 0);
    set_snd_timeout(sockfd, 0);

    if (timeout_ms >= 0) {
        to_timeval(&tv, timeout_ms);
        ptv = &tv;
    }

    if ((rv = net_connect(sockfd, ip, port)) == 0)
        goto conn_done;
    else if (rv < 0 && !is_wouldblock(get_errno()))
        goto conn_done;

    FD_ZERO(&wtfds);
    FD_SET(sockfd, &wtfds);
    if ((rv = select((int)(sockfd + 1), NULL, &wtfds, NULL, ptv)) == 0) {
        rv = -1;
        goto conn_done;
    }

    if (FD_ISSET(sockfd, &wtfds))
    {
        int error = 0;
        int iret = get_socket_error(sockfd, &error);

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
        fprintf(stderr, "select error: sockfd not set [%d]", (int)sockfd);
#endif//DEBUG
    }

conn_done:
    return rv;
}

int tcp_listen(sockhandle_t listenfd, const char* ip, uint16_t port,
               bool reuse, int backlog/* = SOMAXCONN*/)
{
    int rv = 0;
    if (!IS_VALID_SOCKET(listenfd)) {
        return ZTL_ERR_BadFD;
    }

    set_reuseaddr(listenfd, reuse);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof addr);
    make_sockaddr(&addr, ip, port);
    if ((rv = bind(listenfd, (struct sockaddr*)&addr, sizeof(addr))) < 0)
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

sockhandle_t tcp_listen_ex(const char* bindip, uint16_t port, bool nonblock, bool nodelay)
{
    int rv;
    sockhandle_t fd;
    fd = create_socket(SOCK_STREAM);
    if (!IS_VALID_SOCKET(fd)) {
        return ZTL_ERR_BadFD;
    }

    if (nonblock)
        set_nonblock(fd, 1);
    if (nodelay)
        set_tcp_nodelay(fd, 1);

    rv = tcp_listen(fd, bindip, port, true, 512);
    if (rv < 0) {
        close_socket(fd);
        return rv;
    }

    return fd;
}

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

static int echo_func(void* udata, sockhandle_t ns, int isoutev)
{
    (void)udata;
    (void)isoutev;
    int rv;
    char buf[4096] = "";

    rv = recv(ns, buf, sizeof(buf) - 1, 0);
    if (rv <= 0) {
        return rv;
        //fprintf(stderr, "recv error: %d", errno);
    }

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

int net_send(sockhandle_t sockfd, const char* buf, int size, int flag)
{
    return send(sockfd, buf, size, flag);
}

sockhandle_t udp_receiver(const char* localip, uint16_t localport, bool reuseaddr)
{
    int rv = 0;
    sockhandle_t fd;
    fd = create_socket(SOCK_DGRAM);
    if (!IS_VALID_SOCKET(fd)) {
        return -1;
    }

    if (reuseaddr) {
        set_reuseaddr(fd, true);
    }

    // bind to the addr
    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof laddr);
    make_sockaddr(&laddr, localip, localport);
    if ((rv = bind(fd, (struct sockaddr*)&laddr, sizeof laddr)) < 0) {
        close_socket(fd);
        return rv;
    }
    return fd;
}

int udp_recv(sockhandle_t sockfd, char* buf, int size, struct sockaddr_in* fromaddr, int timeoutms)
{
    if (!IS_VALID_SOCKET(sockfd))
        return ZTL_ERR_BadFD;

    socklen_t addrlen;
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

    if (rv > 0)
    {
        if (fromaddr == NULL) {
            rv = recvfrom(sockfd, buf, size, 0, NULL, NULL);
        }
        else {
            addrlen = sizeof(*fromaddr);
            rv = recvfrom(sockfd, buf, size, 0, (struct sockaddr*)fromaddr, &addrlen);
        }
    }
    return rv;
}

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
    if (!IS_VALID_SOCKET(lsock)) {
        return ZTL_ERR_BadFD;
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
