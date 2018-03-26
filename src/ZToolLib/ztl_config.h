/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_CONFIG_H_INCLUDED_
#define _ZTL_CONFIG_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

/* a simple configure reading & process
 */
typedef struct ztl_config_s ztl_config_t;

#ifdef __cplusplus
extern "C" {
#endif

/* open configure file
 * line comment char like '#'
 * delimiter char like '='
 */
ztl_config_t* ztl_config_open(const char* filename, char comment, char delimiter);

/* close configure file
 */
void ztl_config_close(ztl_config_t* zconf);

/* manual set key-value to configs
 */
bool ztl_config_set_item(ztl_config_t* zconf, const char* key, const char* val, bool overwrite);

/* read configure items
 * if return false (read failed), the outval will not be changed
 */
bool ztl_config_read_str(ztl_config_t* zconf, const char* key, char** outval, int* outlen);
bool ztl_config_read_int16(ztl_config_t* zconf, const char* key, void* outi16);
bool ztl_config_read_int32(ztl_config_t* zconf, const char* key, void* outi32);
bool ztl_config_read_int64(ztl_config_t* zconf, const char* key, void* outi64);
bool ztl_config_read_double(ztl_config_t* zconf, const char* key, void* outdbl);
bool ztl_config_read_bool(ztl_config_t* zconf, const char* key, bool* outbool);

/* parse bool desc string to true or false
 * desc string like: 1,0,yes,no,true,false,on,off,
 */
bool ztl_boolvalue_loopup(const char* val_desc);

/* have the configure item or not,
 * return the value ptr if find
 */
char* ztl_config_have(ztl_config_t* zconf, const char* key);

#ifdef __cplusplus
}
#endif


#endif//_ZTL_CONFIG_H_INCLUDED_
