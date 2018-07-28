#ifndef _ZTL_FIXAPI_H_INCLUDE_
#define _ZTL_FIXAPI_H_INCLUDE_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* exported types
 */
#define ZTL_FIX_BUF_SIZE    4000

typedef uint32_t fixkey_t;

typedef struct ztl_fixapi_s ztl_fixapi_t;


/* exported interfaces
 */
ztl_fixapi_t* ztl_fixapi_create();

void ztl_fixapi_release(ztl_fixapi_t* fixapi);

void ztl_fixapi_clear(ztl_fixapi_t* fixapi);

char* ztl_fixapi_data(ztl_fixapi_t* fixapi);
int ztl_fixapi_length(ztl_fixapi_t* fixapi);

void ztl_fixapi_setbuffer(ztl_fixapi_t* fixapi, char* buffer, int size);

bool ztl_fixapi_have(ztl_fixapi_t* fixapi, fixkey_t id);

int ztl_fixapi_set_char(ztl_fixapi_t* fixapi, fixkey_t id, const char val);
int ztl_fixapi_set_int16(ztl_fixapi_t* fixapi, fixkey_t id, const uint16_t val);
int ztl_fixapi_set_int32(ztl_fixapi_t* fixapi, fixkey_t id, const uint32_t val);
int ztl_fixapi_set_int64(ztl_fixapi_t* fixapi, fixkey_t id, const int64_t val);
int ztl_fixapi_set_float(ztl_fixapi_t* fixapi, fixkey_t id, const float val);
int ztl_fixapi_set_double(ztl_fixapi_t* fixapi, fixkey_t id, const double val);
int ztl_fixapi_set_str(ztl_fixapi_t* fixapi, fixkey_t id, const char* val, int len);


int ztl_fixapi_get_char(ztl_fixapi_t* fixapi, fixkey_t id, char* pval);
int ztl_fixapi_get_int16(ztl_fixapi_t* fixapi, fixkey_t id, uint16_t* pval);
int ztl_fixapi_get_int32(ztl_fixapi_t* fixapi, fixkey_t id, uint32_t* pval);
int ztl_fixapi_get_int64(ztl_fixapi_t* fixapi, fixkey_t id, int64_t* pval);
int ztl_fixapi_get_float(ztl_fixapi_t* fixapi, fixkey_t id, float* pval);
int ztl_fixapi_get_double(ztl_fixapi_t* fixapi, fixkey_t id, double* pval);
int ztl_fixapi_get_str(ztl_fixapi_t* fixapi, fixkey_t id, char* pval, int* plen);
int ztl_fixapi_get_value(ztl_fixapi_t* fixapi, fixkey_t id, void** ppval, int* plen);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_FIXAPI_H_INCLUDE_