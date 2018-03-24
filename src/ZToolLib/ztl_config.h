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

/* close configure file */
void ztl_config_close(ztl_config_t* zconf);

/* read configure items */
bool ztl_config_read_str(ztl_config_t* zconf, const char* key, char** outval, uint32_t* outlen);
bool ztl_config_read_short(ztl_config_t* zconf, const char* key, void* outi16);
bool ztl_config_read_int(ztl_config_t* zconf, const char* key, void* outi32);
bool ztl_config_read_int64(ztl_config_t* zconf, const char* key, void* outi64);
bool ztl_config_read_double(ztl_config_t* zconf, const char* key, void* outdbl);

bool ztl_config_read_bool(ztl_config_t* zconf, const char* key, bool* outbool);

/* have the configure item, return the value ptr if find */
char* ztl_config_have(ztl_config_t* zconf, const char* key);

#ifdef __cplusplus
}
#endif


#endif//_ZTL_CONFIG_H_INCLUDED_
