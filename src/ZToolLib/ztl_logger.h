#ifndef _ZLOGGER_H_
#define _ZLOGGER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

typedef enum
{
    ZTL_PrintScrn,      // to print screen
    ZTL_WritFile,       // to log file
    ZTL_Debugview,      // to debugview, only support on windows
    ZTL_SygLog          // to sys log, only support on linux
}ztl_log_output_t;

typedef enum
{
    ZTL_LOG_NONE,       // if log level specified as this, will not output any msg
    ZTL_LOG_STDERR,
    ZTL_LOG_TRACE,
    ZTL_LOG_DEBUG,
    ZTL_LOG_INFO,
    ZTL_LOG_NOTICE,
    ZTL_LOG_WARN,
    ZTL_LOG_CRITICAL
}ztl_log_level_t;

typedef struct ztl_log_st ztl_log_t;

#ifdef __cplusplus
extern "C" {
#endif

/// create a logger by the filename, and specify how to output log msgs
extern ztl_log_t* ztl_log_create(const char* filename, ztl_log_output_t outType, bool bAsyncLog);

/// create a udp log parameter
extern ztl_log_t* ztl_log_create_udp(const char* filename, ztl_log_output_t outType, 
    const char* udpip, uint16_t udpport, int issender);

/// close the logger
extern void ztl_log_close(ztl_log_t* logger);

/// set the minimum log level
extern void ztl_log_set_level(ztl_log_t* logger, ztl_log_level_t minLevel);

extern ztl_log_level_t ztl_log_get_level(ztl_log_t* logger);

/// redirect stderr to logger
extern int zlt_log_redirect_stderr(ztl_log_t* logger);

/// log a piece of msg
extern void ztl_log(ztl_log_t* logger, ztl_log_level_t level, const char* fmt, ...);

extern void ztl_log2(ztl_log_t* logger, ztl_log_level_t level, const char* line, int len);

#define ztl_log_error(log,level,...)    ztl_log(log, level, __VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif//_ZLOGGER_H_
