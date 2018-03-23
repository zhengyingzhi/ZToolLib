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

// if we want to reduce memory space, 
// we can define MICRO: "NO_ASYNC_LOG" when compile this file
#ifdef NO_ASYNC_LOG
#define ARR_SIZE    1
#define LOGBUF_SZ   4096
#else
#define ARR_SIZE    1024
#define LOGBUF_SZ   4096
#endif//USE_SYNC_LOG

#define USED        '1'
#define UNUSE       '0'

#define ZTL_LOG_FLUSH_COUNT 1024
#define ZTL_UDP_LOG_TYPE    1

typedef void(*OutputFunc)(FILE* logfp, char* buf, int length);

typedef struct 
{
    uint16_t        type : 2;       // UDP_LOG_TYPE
    uint16_t        size : 14;      // the udp message size 16384
    uint32_t        sequence;       // the udp message sequence
}ztl_log_udp_header_t;
#define ZTL_UDP_LOG_OFFSET  sizeof(ztl_log_udp_header_t)

struct ztl_log_st {
    FILE*       logfp;                  // log file pointer
    int         level;                  // log level
    int         outputType;             // log output type
    int         bAsyncLog;              // is use async log
    int         running;                // is running for async log thread
    int         bExited;                // the log thread whether exit or not
    OutputFunc  pfLogFunc;

    ztl_thread_t    thr;                // the thread handle

    char        filename[256];          // the filename
    char        logArray[ARR_SIZE][LOGBUF_SZ];  // a ring buf, 16M
    char        dataFlag[ARR_SIZE];     // has data flag
    uint32_t    wtIndex;                // write index
    uint32_t    rdIndex;                // read index for log thread
    uint32_t    itemCount;              // the count of log message

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
#ifdef WIN32
    OutputDebugStringA(buf);
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
    //syslog(KERN_INFO, buf, len);
    //klogctl(KERN_INFO, buf, len);
#endif
}

#define CountToIndex(idx) (idx) & (ARR_SIZE - 1)

static ztl_log_t* _InitNewLogger()
{
    ztl_log_t* log;
    log = (ztl_log_t*)malloc(sizeof(ztl_log_t));

    log->logfp      = NULL;
    log->level      = ZTL_LOG_NONE;
    log->outputType = ZTL_WritFile;
    log->bAsyncLog  = true;
    log->running    = 1;
    log->bExited    = 0;

    log->wtIndex    = 0;
    log->rdIndex    = 0;
    log->itemCount  = 0;
    memset(log->filename, 0, sizeof(log->filename));
    memset(log->dataFlag, UNUSE, sizeof(log->dataFlag));

    for (int i = 0; i < ARR_SIZE; ++i) {
        memset(log->logArray[i], 0, LOGBUF_SZ);
    }

    log->sequence= 0;
    log->udpsock = 0;
    log->udpport = 0;
    memset(log->udpip, 0, sizeof(log->udpip));
    memset(&log->udpaddr, 0, sizeof(log->udpaddr));
    return log;
}

static bool _WaitLogMsg(ztl_log_t* log)
{
    if (log->itemCount == 0)
    {
        ztl_thread_cond_wait(&log->cond, &log->lock);
    }
    return true;
}

static int _MakeLogLineTime(char* buf, int level)
{
    int lLength = 0;
    lLength += current_time(buf + lLength, 16, true);
    lLength += sprintf(buf + lLength, " [%s] ", levels[level]);
    return lLength;
}

/// async logger msg thread
static ztl_thread_result_t ZTL_THREAD_CALL _LogWorkThread(void* arg)
{
    ztl_log_t* log = (ztl_log_t*)arg;

    char* pBuf = NULL;
    unsigned int rdIdx = 0;
    while (log->running)
    {
        if (!_WaitLogMsg(log))
            continue;

        rdIdx = log->rdIndex;
        if (log->wtIndex != rdIdx)
        {
            // check current data of this rdIdx could be log
            if (log->dataFlag[rdIdx] == UNUSE)
            {
                sleepms(1);
                continue;
            }

            // write log msg
            pBuf = log->logArray[rdIdx];
             log->pfLogFunc(log->logfp, pBuf, 0);

            // update rdIndex after write msg finined
            pBuf[0] = '\0';
            log->rdIndex = CountToIndex(rdIdx + 1);
            log->dataFlag[rdIdx] = UNUSE;
            --log->itemCount;
        }
    }//while

    // set the log thread as exited
    log->bExited = 1;
    return 0;
}

/// udp msg receiver thread
static ztl_thread_result_t ZTL_THREAD_CALL _UdpLogWorkThread(void* arg)
{
    ztl_log_t* log = (ztl_log_t*)arg;
    ztl_log_udp_header_t* lpHead;
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

        lpHead = (ztl_log_udp_header_t*)lBuffer;
        if (lpHead->type != ZTL_UDP_LOG_TYPE) {
            continue;
        }
        if (lpHead->sequence > 0 && lpHead->sequence <= lSequence) {
            continue;
        }
        if (lpHead->size > LOGBUF_SZ) {
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
        log->pfLogFunc(log->logfp, lBuffer + ZTL_UDP_LOG_OFFSET, rv - ZTL_UDP_LOG_OFFSET);
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
        current_date(lDate, sizeof(lDate), '/');

        const char* psep = strchr(log->filename, '.');
        if (psep) {
            strncpy(lRealFileName, log->filename, psep - log->filename);
            sprintf(lRealFileName + strlen(lRealFileName), "_%s%s", lDate, psep);
        }
        else
        {
            sprintf(lRealFileName, "%s_%s.log", lDate, psep);
        }

        log->logfp = fopen(lRealFileName, "a+");
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

    _ztl_log_createfile(log);

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

    _ztl_log_createfile(log);

    // init udp component
    memset(&log->udpaddr, 0, sizeof(log->udpaddr));
    make_sockaddr(&log->udpaddr, udpip, udpport);
    set_snd_buffsize(udpfd, 4 * 1024 * 1024);
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

        void* retval;
        ztl_thread_join(log->thr, &retval);

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
        fclose(log->logfp);
        log->logfp = NULL;
    }

    free(log);
}

void ztl_log_set_level(ztl_log_t* log, ztl_log_level_t minLevel)
{
    if (log)
        log->level = minLevel;
}

void ztl_log(ztl_log_t* log, ztl_log_level_t level, const char* fmt, ...)
{
    if (!log || log->level > level || 0 == log->running)
        return;

    char    lBuffer[LOGBUF_SZ] = "";
    char*   lpBuff;
    int     lLength;

    lLength = 0;
    if (log->udpsock > 0 && log->issender) {
        lLength += ZTL_UDP_LOG_OFFSET;
    }

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
    vsnprintf(lpBuff + lLength, LOGBUF_SZ - lLength, fmt, args);
    va_end(args);

    if (log->bAsyncLog) {
        if (ztl_atomic_add(&log->itemCount, 1) == 0) {
            ztl_thread_cond_signal(&log->cond);
        }
    }
    else if (log->udpsock > 0 && log->issender)
    {
        ztl_log_udp_header_t* lpHead;
        lpHead = (ztl_log_udp_header_t*)lpBuff;
        lpHead->type = ZTL_UDP_LOG_TYPE;
        lpHead->size = lLength;
        lpHead->sequence = ztl_atomic_add(&log->sequence, 1) + 1;

        udp_send(log->udpsock, lpBuff, lLength, &log->udpaddr);
    }
    else
    {
        if (log->pfLogFunc)
            log->pfLogFunc(log->logfp, lpBuff, lLength);
    }

}

void ztl_log2(ztl_log_t* log, ztl_log_level_t level, const char* line, int len)
{
    if (!log || log->level > level || 0 == log->running)
        return;

    char    lBuffer[LOGBUF_SZ] = "";
    char*   lpBuff;
    int     lLength;

    lLength = 0;
    if (log->udpsock > 0 && log->issender) {
        lLength += ZTL_UDP_LOG_OFFSET;
    }

    if (log->bAsyncLog) {
        lpBuff = ztl_mp_alloc(log->pool);
    }
    else {
        lpBuff = lBuffer;
    }

    // format log message's header and content
    lLength += _MakeLogLineTime(lpBuff + lLength, level);

    len = ztl_min((LOGBUF_SZ - lLength), len);
    memcpy(lpBuff + lLength, line, len);
    lLength += len;

    if (log->bAsyncLog) {
        if (ztl_atomic_add(&log->itemCount, 1) == 0) {
            ztl_thread_cond_signal(&log->cond);
        }
    }
    else if (log->udpsock > 0 && log->issender)
    {
        ztl_log_udp_header_t* lpHead;
        lpHead = (ztl_log_udp_header_t*)lpBuff;
        lpHead->type = ZTL_UDP_LOG_TYPE;
        lpHead->size = lLength;
        lpHead->sequence = ztl_atomic_add(&log->sequence, 1) + 1;

        udp_send(log->udpsock, lpBuff, lLength, &log->udpaddr);
    }
    else
    {
        if (log->pfLogFunc)
            log->pfLogFunc(log->logfp, lpBuff, lLength);
    }
}



