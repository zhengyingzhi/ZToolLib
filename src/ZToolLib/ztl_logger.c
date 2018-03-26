#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "lockfreequeue.h"
#include "ztl_mempool.h"

#include "ztl_network.h"
#include "ztl_logger.h"
#include "ztl_threads.h"
#include "ztl_utils.h"
#include "ztl_atomic.h"

#ifdef _MSC_VER
#include <windows.h>
#else
#include <sys/klog.h>
#include <linux/kernel.h>
#endif//_MSC_VER

static const char* levels[10]	= {
    "NONE ",
    "TRACE"
    "DEBUG",
    "INFO ",
    "NOTICE",
    "WARN ",
    "ERROR",
    "CRITI", "", ""
};

#define ZTL_LOG_QUEUE_SIZE  8192
#define ZTL_LOGBUF_SIZE     4096
#define ZTL_LOG_FLUSH_COUNT 1024
#define ZTL_LOG_TYPE_NONE   0
#define ZTL_LOG_TYPE_UDP    1

typedef void(*OutputFunc)(FILE* logfp, char* buf, int length);

typedef struct 
{
    uint16_t        type : 2;       // ZTL_LOG_TYPE_UDP
    uint16_t        size : 14;      // the udp message size 16384
    uint32_t        sequence;       // the udp message sequence
}ztl_log_header_t;
#define ZTL_LOG_HEAD_OFFSET sizeof(ztl_log_header_t)

struct ztl_log_st {
    FILE*               logfp;          // log file pointer
    int                 log_level;      // log level
    int                 outputType;     // log output type
    int                 bAsyncLog;      // is use async log
    int                 running;        // is running for async log thread
    int                 bExited;        // the log thread whether exit or not
    OutputFunc          pfLogFunc;

    ztl_thread_t        thr;            // the thread handle

    char                filename[256];  // the filename
    uint32_t            itemCount;      // the pending count of log message

    lfqueue_t*          queue;
    ztl_mempool_t*      pool;

    uint32_t            sequence;       // udp message sequence
    sockhandle_t        udpsock;        // udp socket descriptor
    uint16_t            issender;       // is udp sender
    uint16_t            udpport;        // peer udp port
    char                udpip[32];      // peer udp ip
    struct sockaddr_in  udpaddr;        // peer udp addr
    ztl_thread_mutex_t  lock;
    ztl_thread_cond_t   cond;
};

static uint32_t gTimesToFlush = 0;

static void _Output2File(FILE* logfp, char* buf, int len)
{
    fwrite(buf, len, 1, logfp);

    if (++gTimesToFlush & (ZTL_LOG_FLUSH_COUNT - 1)) {
        fflush(logfp);
    }
}
static void _Output2DbgView(FILE* logfp, char* buf, int len)
{
    (void)logfp;
    (void)len;
#ifdef _MSC_VER
    OutputDebugStringA(buf);
#else
    (void)buf;
#endif//WIN32
}
static void _Output2Scrn(FILE* logfp, char* buf, int len)
{
    (void)len;
    fprintf(logfp, buf);
}
static void _Output2Syslog(FILE* logfp, char* buf, int len)
{
    (void)logfp;
#ifdef __linux__
    (void)buf;
    (void)len;
    //syslog(KERN_INFO, buf, len);
    //klogctl(KERN_INFO, buf, len);
#endif
}

#define CountToIndex(idx) (idx) & (ARR_SIZE - 1)

static ztl_log_t* _InitNewLogger()
{
    ztl_log_t* log;
    log = (ztl_log_t*)malloc(sizeof(ztl_log_t));
    memset(log, 0, sizeof(ztl_log_t));

    log->logfp      = NULL;
    log->log_level  = ZTL_LOG_INFO;
    log->outputType = ZTL_WritFile;
    log->bAsyncLog  = true;
    log->running    = 1;
    log->bExited    = 0;

    memset(log->filename, 0, sizeof(log->filename));

    log->sequence= 0;
    log->udpsock = 0;
    log->udpport = 0;
    memset(log->udpip, 0, sizeof(log->udpip));
    memset(&log->udpaddr, 0, sizeof(log->udpaddr));
    return log;
}

static char* _WaitLogMsg(ztl_log_t* log)
{
    char* lpBuff;
    
    do 
    {
        lpBuff = NULL;
        if (lfqueue_pop(log->queue, &lpBuff) != 0) {
            ztl_thread_cond_wait(&log->cond, &log->lock);
            continue;
        }

        ztl_atomic_dec(&log->itemCount, 1);
        break;
    } while (log->running);

    return lpBuff;
}

static int _MakeLogLineTime(char* buf, int level)
{
    int lLength = 0;
    lLength += current_time(buf + lLength, 16, true);
    lLength += sprintf(buf + lLength, " [%u] [%s] ", (uint32_t)ztl_thread_self(), levels[level]);
    return lLength;
}

/// async logger msg thread
static ztl_thread_result_t ZTL_THREAD_CALL _LogWorkThread(void* arg)
{
    ztl_log_t* log = (ztl_log_t*)arg;

    ztl_log_header_t* lpHead;
    char* lpBuf;

    while (log->running)
    {
        lpBuf = _WaitLogMsg(log);
        if (!lpBuf) {
            continue;
        }

        lpHead = (ztl_log_header_t*)lpBuf;

        if (log->pfLogFunc)
            log->pfLogFunc(log->logfp, lpBuf + ZTL_LOG_HEAD_OFFSET, 
                lpHead->size - ZTL_LOG_HEAD_OFFSET);

        ztl_mp_free(log->pool, lpBuf);
    }

    // set the log thread as exited
    log->bExited = 1;
    return 0;
}

/// udp msg receiver thread
static ztl_thread_result_t ZTL_THREAD_CALL _UdpLogWorkThread(void* arg)
{
    ztl_log_t* log = (ztl_log_t*)arg;
    ztl_log_header_t* lpHead;
    char lBuffer[4096] = "";
    int  lSize, rv;
    uint32_t lSequence = 0;

    while (log->running)
    {
        lSize = sizeof(lBuffer) - 1;
        rv = udp_recv(log->udpsock, lBuffer, lSize, NULL, 100);
        if (rv == 0) {
            continue;
        }
        else if (rv < 0) {
            fprintf(stderr, "udp log udp_recv() error %d\n", get_errno());
            break;
        }

        lpHead = (ztl_log_header_t*)lBuffer;
        if (lpHead->type != ZTL_LOG_TYPE_UDP) {
            continue;
        }
        if (lpHead->sequence > 0 && lpHead->sequence <= lSequence) {
            continue;
        }
        if (lpHead->size > ZTL_LOGBUF_SIZE) {
            continue;
        }

        if (lSequence + 1 != lpHead->sequence) {
            char lTempBuf[512] = "";
            int  lTempLen = 0;
            lTempLen += _MakeLogLineTime(lTempBuf, ZTL_LOG_WARN);
            lTempLen += sprintf(lTempBuf + lTempLen, "udp log message lost expect:%d, actual:%d\r\n", lSequence + 1, lpHead->sequence);

            log->pfLogFunc(log->logfp, lTempBuf, lTempLen);
        }

        lBuffer[rv] = '\0';
        log->pfLogFunc(log->logfp, lBuffer + ZTL_LOG_HEAD_OFFSET, rv - ZTL_LOG_HEAD_OFFSET);
    }

    log->bExited = 1;
    return 0;
}

static int _ztl_log_createfile(ztl_log_t* log)
{
    if (log->outputType == ZTL_WritFile)
    {
        // create a file
        char lRealFileName[512] = "";
        char lDate[32] = "";
        current_date(lDate, sizeof(lDate), 0);

        const char* psep = strchr(log->filename, '.');
        if (psep) {
            strncpy(lRealFileName, log->filename, psep - log->filename);
            sprintf(lRealFileName + strlen(lRealFileName), "_%s%s", lDate, psep);
        }
        else {
            sprintf(lRealFileName, "%s_%s.log", log->filename, lDate);
        }

        log->logfp = fopen(lRealFileName, "a+");
        if (!log->logfp) {
            return -1;
        }
        log->pfLogFunc = _Output2File;
    }
    else if (log->outputType == ZTL_PrintScrn)
    {
        log->logfp = stderr;
        log->pfLogFunc = _Output2Scrn;
    }
    else if (log->outputType == ZTL_Debugview)
    {
        log->logfp = NULL;
        log->pfLogFunc = _Output2DbgView;
    }
    else if (log->outputType == ZTL_SygLog)
    {
        log->logfp = NULL;
        log->pfLogFunc = _Output2Syslog;
    }
    else {
        log->logfp = stderr;
        log->pfLogFunc = _Output2Scrn;
    }

    return 0;
}

/// create a logger by the filename, and specify how to output log msgs
ztl_log_t* ztl_log_create(const char* filename, ztl_log_output_t outType, bool bAsyncLog)
{
    ztl_log_t* log = _InitNewLogger();
    if (log == NULL)
        return NULL;

    strncpy(log->filename, filename, sizeof(log->filename) - 1);
    log->outputType = outType;
    log->bAsyncLog = bAsyncLog;

    if (_ztl_log_createfile(log) != 0) {
        free(log);
        return NULL;
    }

    if (log->bAsyncLog)
    {
#ifdef _WIN32
        ztl_thread_mutex_init(&log->lock, NULL);
#else
        ztl_thread_mutexattr_t ma;
        ztl_thread_mutexattr_init(&ma);
        ztl_thread_mutexattr_settype(&ma, ZTL_THREAD_MUTEX_ADAPTIVE_NP);
        ztl_thread_mutex_init(&log->lock, &ma);
        ztl_thread_mutexattr_destroy(&ma);
#endif//_WIN32
        ztl_thread_cond_init(&log->cond, NULL);

        log->queue  = lfqueue_create(ZTL_LOG_QUEUE_SIZE, sizeof(char*));
        log->pool   = ztl_mp_create(ZTL_LOGBUF_SIZE, 256, 1);

        // create log thread
        ztl_thread_create(&log->thr, NULL, _LogWorkThread, log);
    }

    return log;
}

/// create a udp log parameter
ztl_log_t* ztl_log_create_udp(const char* filename, ztl_log_output_t outType, 
    const char* udpip, uint16_t udpport, int issender)
{
    net_init();

#ifndef _MSC_VER
    ignore_sigpipe();
#endif//_MSC_VER

    ztl_log_t* log = _InitNewLogger();
    if (log == NULL)
        return NULL;

    sockhandle_t udpfd = create_socket(SOCK_DGRAM);
    if (udpfd < 0) {
        ztl_log_close(log);
        return NULL;
    }

    strncpy(log->filename, filename, sizeof(log->filename) - 1);
    log->outputType = outType;
    log->bAsyncLog = false;

    if (_ztl_log_createfile(log) != 0) {
        free(log);
        return NULL;
    }

    // init udp component
    memset(&log->udpaddr, 0, sizeof(log->udpaddr));
    make_sockaddr(&log->udpaddr, udpip, udpport);
    if (issender)
        set_snd_buffsize(udpfd, 4 * 1024 * 1024);
    else
        set_rcv_buffsize(udpfd, 4 * 1024 * 1024);
    connect(udpfd, (struct sockaddr*)&log->udpaddr, sizeof(log->udpaddr));

    log->issender= issender;
    log->udpsock = udpfd;
    log->udpport = udpport;
    memset(log->udpip, 0, sizeof(log->udpip));
    strcpy(log->udpip, udpip);

    if (!issender)
    {
        if (bind(log->udpsock, (struct sockaddr*)&log->udpaddr, sizeof(log->udpaddr)) < 0)
        {
            ztl_log_close(log);
            fprintf(stderr, "ztl_log udp bind to %s:%d failed\n", udpip, udpport);
            return NULL;
        }

#ifdef _WIN32
        ztl_thread_mutex_init(&log->lock, NULL);
#else
        ztl_thread_mutexattr_t ma;
        ztl_thread_mutexattr_init(&ma);
        ztl_thread_mutexattr_settype(&ma, ZTL_THREAD_MUTEX_ADAPTIVE_NP);
        ztl_thread_mutex_init(&log->lock, &ma);
        ztl_thread_mutexattr_destroy(&ma);
#endif//_WIN32
        ztl_thread_cond_init(&log->cond, NULL);

        // create udp log recv thread
        ztl_thread_create(&log->thr, NULL, _UdpLogWorkThread, log);
    }

    return log;
}

void ztl_log_close(ztl_log_t* log)
{
    if (log == NULL)
        return;

    // set running flag
    log->running = 0;

    if (log->udpsock > 0)
    {
        close_socket(log->udpsock);

        if (log->thr) {
            void* retval;
            ztl_thread_join(log->thr, &retval);
        }

        log->udpsock = -1;
    }

    if (log->bAsyncLog)
    {
        ztl_thread_cond_signal(&log->cond);

        void* retval;
        ztl_thread_join(log->thr, &retval);
    }

    if (log->logfp && log->logfp != stderr && log->logfp != stdout)
    {
        fflush(log->logfp);
        fclose(log->logfp);
        log->logfp = NULL;
    }

    if (log->queue) {
        lfqueue_release(log->queue);
    }
    if (log->pool) {
        ztl_mp_release(log->pool);
    }

    free(log);
}

void ztl_log_set_level(ztl_log_t* log, ztl_log_level_t minLevel)
{
    if (log)
        log->log_level = minLevel;
}

extern ztl_log_level_t ztl_log_get_level(ztl_log_t* log)
{
    if (log)
        return log->log_level;
    return ZTL_LOG_NONE;
}

void ztl_log(ztl_log_t* log, ztl_log_level_t level, const char* fmt, ...)
{
    if (!log || log->log_level > (int)level || 0 == log->running)
        return;

    char    lBuffer[ZTL_LOGBUF_SIZE] = "";
    char*   lpBuff;
    int     lLength;

    lLength = ZTL_LOG_HEAD_OFFSET;

    if (log->bAsyncLog) {
        lpBuff = ztl_mp_alloc(log->pool);
    }
    else {
        lpBuff = lBuffer;
    }

    // format log message's header and content
    lLength += _MakeLogLineTime(lpBuff + lLength, level);

    va_list args;
    va_start(args, fmt);
    lLength += vsnprintf(lpBuff + lLength, ZTL_LOGBUF_SIZE - lLength - 2, fmt, args);
    va_end(args);

    // append line feed
    lpBuff[lLength] = '\r';
    lpBuff[lLength+1] = '\n';
    lLength += 2;

    ztl_log_header_t* lpHead;
    lpHead          = (ztl_log_header_t*)lpBuff;
    lpHead->type    = ZTL_LOG_TYPE_NONE;
    lpHead->size    = (uint16_t)(lLength - ZTL_LOG_HEAD_OFFSET);
    lpHead->sequence= ztl_atomic_add(&log->sequence, 1) + 1;

    if (log->bAsyncLog) {
        lfqueue_push(log->queue, &lpBuff);
        if (ztl_atomic_add(&log->itemCount, 1) == 0) {
            ztl_thread_cond_signal(&log->cond);
        }
    }
    else if (log->udpsock > 0 && log->issender) {
        lpHead->type = ZTL_LOG_TYPE_UDP;
        udp_send(log->udpsock, lpBuff, lLength, &log->udpaddr);
    }
    else {
        if (log->pfLogFunc)
            log->pfLogFunc(log->logfp, lpBuff + ZTL_LOG_HEAD_OFFSET, lLength - ZTL_LOG_HEAD_OFFSET);
    }

}

void ztl_log2(ztl_log_t* log, ztl_log_level_t level, const char* line, int len)
{
    if (!log || log->log_level > (int)level || 0 == log->running)
        return;

    char    lBuffer[ZTL_LOGBUF_SIZE] = "";
    char*   lpBuff;
    int     lLength;

    lLength = ZTL_LOG_HEAD_OFFSET;

    if (log->bAsyncLog) {
        lpBuff = ztl_mp_alloc(log->pool);
    }
    else {
        lpBuff = lBuffer;
    }

    // format log message's header and content
    lLength += _MakeLogLineTime(lpBuff + lLength, level);

    len = ztl_min((ZTL_LOGBUF_SIZE - lLength - 2), len);
    memcpy(lpBuff + lLength, line, len);
    lLength += len;

    // append line feed
    lpBuff[lLength] = '\r';
    lpBuff[lLength + 1] = '\n';
    lLength += 2;

    
    ztl_log_header_t* lpHead;
    lpHead          = (ztl_log_header_t*)lpBuff;
    lpHead->type    = ZTL_LOG_TYPE_NONE;
    lpHead->size    = (uint16_t)(lLength - ZTL_LOG_HEAD_OFFSET);
    lpHead->sequence= ztl_atomic_add(&log->sequence, 1) + 1;

    if (log->bAsyncLog) {
        lfqueue_push(log->queue, &lpBuff);
        if (ztl_atomic_add(&log->itemCount, 1) == 0) {
            ztl_thread_cond_signal(&log->cond);
        }
    }
    else if (log->udpsock > 0 && log->issender) {
        lpHead->type = ZTL_LOG_TYPE_UDP;
        udp_send(log->udpsock, lpBuff, lLength, &log->udpaddr);
    }
    else {
        if (log->pfLogFunc)
            log->pfLogFunc(log->logfp, lpBuff + ZTL_LOG_HEAD_OFFSET, lLength - ZTL_LOG_HEAD_OFFSET);
    }
}


static int ztl_set_stderr(FILE* fp)
{
#ifdef _WIN32
    return SetStdHandle(STD_ERROR_HANDLE, fp);
#else
    return dup2(fp, STDERR_FILENO);
#endif//_WIN32
}

int zlt_log_redirect_stderr(ztl_log_t* log)
{
    if (log->logfp == stderr) {
        return 0;
    }

    /* file log always exists when we are called */
    if (ztl_set_stderr(log->logfp) == 0) {
        ztl_log(log, ZTL_LOG_CRITICAL, "ztl_set_stderr() failed\r\n");

        return -1;
    }

    return 0;
}
