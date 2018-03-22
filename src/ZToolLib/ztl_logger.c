#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

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
#define LOGBUF_SZ   1
#else
#define ARR_SIZE    4096
#define LOGBUF_SZ   4096
#endif//USE_SYNC_LOG
#define USED        '1'
#define UNUSE       '0'

typedef void(*OutputFunc)(char* buf, int length);

struct ztl_log_st {
    FILE*       logfp;              // log file pointer
    int         level;              // log level
    int         outputType;         // log output type
    int         bAsyncLog;          // is use async log
    int         running;            // is running for async log thread
    int         bExited;            // the log thread whether exit or not
    OutputFunc  pfLogFunc;

    char        filename[256];      // the filename
    char        logArray[ARR_SIZE][LOGBUF_SZ];  // a ring buf, 16M
    char        dataFlag[ARR_SIZE]; // has data flag
    uint32_t    wtIndex;            // write index
    uint32_t    rdIndex;            // read index for log thread
    uint32_t    itemCount;          // the count of log message

    int         udpsock;            // udp socket descriptor
    int         udpport;            // peer udp port
    char        udpip[32];          // peer udp ip
    struct sockaddr_in udpaddr;     // peer udp addr
    ztl_thread_mutex_t  lock;
    ztl_thread_cond_t   cond;
};

static void _Output2File(FILE* logfp, char* buf, int len)
{
    if (logfp)
    {
        fwrite(buf, len, 1, logfp);
        //fputs(buf, logfp);
        //fflush(logfp);
    }
}
static void _Output2DbgView(char* buf, int len)
{
    (void)len;
#ifdef WIN32
    OutputDebugStringA(buf);
#endif//WIN32
}
static void _Output2Scrn(char* buf, int len)
{
    (void)len;
    printf(buf);
}
static void _Output2Syslog(char* buf, int len)
{
#ifdef __linux__
    //syslog(KERN_INFO, buf, strlen(buf));
    //klogctl(KERN_INFO, buf, strlen(buf));
#endif
}

static unsigned int CountToIndex(uint32_t idx)
{
    return (idx % ARR_SIZE);
}

static ztl_log_t* _InitNewLogger()
{
    ztl_log_t* logger = (ztl_log_t*)malloc(sizeof(ztl_log_t));
    logger->logfp = NULL;
    logger->level = ZTL_LOG_NONE;
    logger->outputType = ZTL_WritFile;
    logger->bAsyncLog = true;
    logger->running = 1;
    logger->bExited = 0;

    logger->wtIndex = 0;
    logger->rdIndex = 0;
    logger->itemCount = 0;
    memset(logger->filename, 0, sizeof(logger->filename));
    memset(logger->dataFlag, UNUSE, sizeof(logger->dataFlag));
    for (int i = 0; i < ARR_SIZE; ++i)
    {
        memset(logger->logArray[i], 0, LOGBUF_SZ);
    }

    logger->udpsock = 0;
    logger->udpport = 0;
    memset(logger->udpip, 0, sizeof(logger->udpip));
    memset(&logger->udpaddr, 0, sizeof(logger->udpaddr));
    return logger;
}

static bool _WaitLogMsg(ztl_log_t* logger)
{
    if (logger->itemCount == 0)
    {
        ztl_thread_cond_wait(&logger->cond, &logger->lock);
    }
    return true;
}

/// async logger msg thread
static ztl_thread_result_t ZTL_THREAD_CALL _LogWorkThread(void* arg)
{
    ztl_log_t* logger = (ztl_log_t*)arg;
    if (logger == NULL)
        return 0;

    char* pBuf = NULL;
    unsigned int rdIdx = 0;
    while (logger->running)
    {
        if (!_WaitLogMsg(logger))
            continue;

        rdIdx = logger->rdIndex;
        if (logger->wtIndex != rdIdx)
        {
            // check current data of this rdIdx could be log
            if (logger->dataFlag[rdIdx] == UNUSE)
            {
                sleepms(1);
                continue;
            }

            // write log msg
            pBuf = logger->logArray[rdIdx];
            if (ZTL_WritFile == logger->outputType)
                _Output2File(logger->logfp, pBuf, 0);
            else if (logger->pfLogFunc)
                logger->pfLogFunc(pBuf, 0);

            // update rdIndex after write msg finined
            pBuf[0] = '\0';
            logger->rdIndex = CountToIndex(rdIdx + 1);
            logger->dataFlag[rdIdx] = UNUSE;
            --logger->itemCount;
        }
    }//while

    // set the logger thread as exited
    logger->bExited = 1;
    return 0;
}

/// create a logger by the filename, and specify how to output log msgs
ztl_log_t* ztl_log_create(const char* filename, ztl_log_output_t outType, bool bAsyncLog)
{
    ztl_log_t* logger = _InitNewLogger();
    if (logger == NULL)
        return NULL;

    strncpy(logger->filename, filename, sizeof(logger->filename) - 1);
    logger->outputType = outType;
    logger->bAsyncLog = bAsyncLog;

    if (bAsyncLog)
    {
#ifdef WIN32
        ztl_thread_mutex_init(&logger->lock, NULL);
#else
        ztl_thread_mutexattr_t ma;
        ztl_thread_mutexattr_init(&ma);
        ztl_thread_mutexattr_settype(&ma, THREAD_MUTEX_ADAPTIVE_NP);
        ztl_thread_mutex_init(&logger->lock, &ma);
        ztl_thread_mutexattr_destroy(&ma);
#endif//WIN32
        ztl_thread_cond_init(&logger->cond, NULL);

        // create log thread
        ztl_thread_t thr;
        ztl_thread_create(&thr, NULL, _LogWorkThread, logger);
    }

    if (outType == ZTL_WritFile)
    {
        // create a file
        char realFileName[256] = "";
        char sDate[32] = "";
        current_date(sDate, sizeof sDate, "%04d%02d%02d");
        const char* psep = strchr(filename, '.');
        if (psep != NULL)
        {
            strncpy(realFileName, filename, psep - filename);
            sprintf(realFileName + strlen(realFileName), "_%s%s", sDate, psep);
        }
        else
        {
            sprintf(realFileName, "%s_%s.log", sDate, psep);
        }
        logger->logfp = fopen(realFileName, "a+");
    }

    if (outType == ZTL_PrintScrn)
    {
        logger->pfLogFunc = _Output2Scrn;
    }
    else if (outType == ZTL_Debugview)
    {
        logger->pfLogFunc = _Output2DbgView;
    }
    else if (outType == ZTL_SygLog)
    {
        logger->pfLogFunc = _Output2Syslog;
    }
    else {
        logger->pfLogFunc = _Output2File;
    }

    return logger;
}

/// create a udp log parameter
ztl_log_t* ztl_log_create_udp(const char* filename, ztl_log_output_t outType, const char* udpip, uint16_t udpport)
{
    net_init();

#ifndef _MSC_VER
    ignore_sigpipe();
#endif//_MSC_VER

    int udpfd = create_socket(SOCK_DGRAM);
    if (udpfd < 0)
        return NULL;

    ztl_log_t* logger = _InitNewLogger();
    if (logger == NULL)
        return NULL;

    strncpy(logger->filename, filename, sizeof(logger->filename) - 1);
    logger->outputType = outType;
    logger->bAsyncLog = false;

    // init udp component
    memset(&logger->udpaddr, 0, sizeof(logger->udpaddr));
    make_sockaddr(&logger->udpaddr, udpip, udpport);
    set_snd_buffsize(udpfd, 4 * 1024 * 1024);
    set_nonblock(udpfd, true);
    connect(udpfd, (struct sockaddr*)&logger->udpaddr, sizeof(logger->udpaddr));

    logger->udpsock = udpfd;
    logger->udpport = udpport;
    memset(logger->udpip, 0, sizeof(logger->udpip));
    strcpy(logger->udpip, udpip);

    // send udp init info, which means let peer know how to make a log file
    char initBuf[256] = "";
    int initLen = sprintf(initBuf, "0|%s|%d", filename, outType);

    // even if udp socket, but we call connect, and then call send will work as well
    send(udpfd, initBuf, initLen, 0);

    return logger;
}//CreateUdpLOG

/// close the logger
void ztl_log_close(ztl_log_t* log)
{
    if (log == NULL)
        return;

    // set running flag
    log->running = 0;

    if (log->udpsock > 0)
    {
        close_socket(log->udpsock);
        log->udpsock = -1;
    }
    if (log->bAsyncLog)
    {
        log->running = 0;
        ztl_thread_cond_signal(&log->cond);
        while (!log->bExited)
            sleepms(10);
    }
    if (log->logfp)
    {
        fclose(log->logfp);
        log->logfp = NULL;
    }
}

/// set the minimum log level
void ztl_log_set_level(ztl_log_t* log, ztl_log_level_t minLevel)
{
    if (log)
        log->level = minLevel;
}

/// log a piece of msg
void ztl_log(ztl_log_t* log, ztl_log_level_t level, const char* fmt, ...)
{
    if (!log || log->level > level || 0 == log->running)
        return;

    if (log->bAsyncLog)
    {
        // get the write index may be in the multi thread race
        unsigned int wtIdx = 0;
        do
        {
            wtIdx = CountToIndex(ztl_atomic_add(&log->wtIndex, 1));

            // if full queue
            if (wtIdx == log->rdIndex && log->itemCount > 0)
            {
                ztl_atomic_dec(&log->wtIndex, 1);
                sleepms(1);
                continue;
            }
            break;
        } while (1);

        // format log message's header and content
        char* pBuf = log->logArray[wtIdx];
        int len = current_time(pBuf, 16, true);
        len += sprintf(pBuf + len, " [%s] ", levels[level]);

        va_list args;
        va_start(args, fmt);
        vsprintf(pBuf + len, fmt, args);
        va_end(args);
        //strcat(pBuf, "\r\n");

        // update as used after finished fill log message
        log->dataFlag[wtIdx] = USED;
        if (ztl_atomic_add(&log->itemCount, 1) < 2)
            ztl_thread_cond_signal(&log->cond);
    }
    else
    {
        // write or send log msg
        char buf[4096] = "";
        int tmLen = current_time(buf, 16, true);
        tmLen += sprintf(buf + tmLen, " [%s] ", levels[level]);

        va_list args;
        va_start(args, fmt);
        int len = vsprintf(buf + tmLen, fmt, args);
        va_end(args);
        //strcat(pBuf, "\r\n");

        // send to udp client or output directly
        if (log->udpsock > 0)
        {
            if (send(log->udpsock, buf, len + tmLen, 0) <= 0)
            {
                // notify error msg
            }
        }
        else
        {
            if (log->outputType == ZTL_WritFile)
                _Output2File(log->logfp, buf, 0);
            else if (log->pfLogFunc, 0)
                log->pfLogFunc(buf, 0);
        }
    }
}//zlog

void ztl_log2(ztl_log_t* log, ztl_log_level_t level, const char* line, int len)
{
    if (!log || log->level > level || 0 == log->running)
        return;

    if (log->bAsyncLog)
    {
        // get the write index may be in the multi thread race
        unsigned int wtIdx = 0;
        do
        {
            wtIdx = CountToIndex(ztl_atomic_add(&log->wtIndex, 1));

            // if full queue
            if (wtIdx == log->rdIndex && log->itemCount > 0)
            {
                ztl_atomic_dec(&log->wtIndex, 1);
                sleepms(1);
                continue;
            }
            break;
        } while (1);

        // format log message's header and content
        char* pBuf = log->logArray[wtIdx];
        int tmLen = current_time(pBuf, 16, true);
        tmLen += sprintf(pBuf + tmLen, " [%s] ", levels[level]);
        strncpy(pBuf + tmLen, line, LOGBUF_SZ - tmLen);

        // update as used after finished fill log message for reading flag
        log->dataFlag[wtIdx] = USED;
        if (ztl_atomic_add(&log->itemCount, 1) < 4)
            ztl_thread_cond_signal(&log->cond);
    }
    else
    {
        // write or send log msg
        char buf[LOGBUF_SZ] = "";
        int tmLen = current_time(buf, 16, true);
        tmLen += sprintf(buf + tmLen, " [%s] ", levels[level]);
        strncpy(buf + tmLen, line, LOGBUF_SZ - tmLen);

        // send to udp client or output directly
        if (log->udpsock > 0)
        {
            if (send(log->udpsock, buf, len + tmLen, 0) <= 0)
            {
                // notify error msg
            }
        }
        else
        {
            if (log->outputType == ZTL_WritFile)
                _Output2File(log->logfp, buf, 0);
            else if (log->pfLogFunc)
                log->pfLogFunc(buf, 0);
        }
    }
}



