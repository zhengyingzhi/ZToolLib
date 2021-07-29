/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_LOGGER_H_
#define _ZTL_LOGGER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

typedef enum
{
    ZTL_PrintScrn = 1,      // to print screen
    ZTL_WritFile  = 2,      // to log file
    ZTL_Debugview = 4,      // to debugview, only support on windows
    ZTL_SygLog    = 8       // to sys log, only support on linux
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
    ZTL_LOG_ERROR,
    ZTL_LOG_CRITICAL
}ztl_log_level_t;

typedef struct ztl_log_st ztl_log_t;

#ifdef __cplusplus
extern "C" {
#endif

/* create a logger by the filename, and specify how to output log msgs
 * if filename contains '_YYYYMMDD', the log fine will has the current created date,
 * otherwise, all the log message will append to the tail of the specified file
 */
extern ztl_log_t* ztl_log_create(const char* filename, ztl_log_output_t out_type, bool is_async);

/* create a udp logger
 * if 'issender' is true, log will send to peer machine
 * if 'issender' is false, this logger would recv udp log message from peer
 * and output log message to the file
 */
extern ztl_log_t* ztl_log_create_udp(const char* filename, ztl_log_output_t out_type, 
    const char* udpip, uint16_t udpport, int issender);

/* close the logger
 */
extern void ztl_log_close(ztl_log_t* logger);

/* set the minimum log level
 */
extern void ztl_log_set_level(ztl_log_t* logger, ztl_log_level_t level);
extern int  ztl_log_set_levelstr(ztl_log_t* logger, const char* level);

/* get current log level
 */
extern ztl_log_level_t ztl_log_get_level(ztl_log_t* logger);

/* redirect stderr to logger
 */
extern int zlt_log_redirect_stderr(ztl_log_t* logger);

/* log a piece of msg
 */
extern void ztl_log(ztl_log_t* logger, ztl_log_level_t level, const char* fmt, ...);

extern void ztl_log2(ztl_log_t* logger, ztl_log_level_t level, const char* line, int len);

#define ztl_log_debug(log, ...)     ztl_log(log, ZTL_LOG_DEBUG, __VA_ARGS__)
#define ztl_log_info(log, ...)      ztl_log(log, ZTL_LOG_INFO, __VA_ARGS__)
#define ztl_log_warn(log, ...)      ztl_log(log, ZTL_LOG_WARN, __VA_ARGS__)
#define ztl_log_error(log, ...)     ztl_log(log, ZTL_LOG_ERROR, __VA_ARGS__)
#define ztl_log_critical(log, ...)  ztl_log(log, ZTL_LOG_CRITICAL, __VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif//_ZTL_LOGGER_H_
