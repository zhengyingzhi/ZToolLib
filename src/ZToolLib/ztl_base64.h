#ifndef _ZBASE64_H_INCLUDE_
#define _ZBASE64_H_INCLUDE_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _ZTL_BASE64_ENCODE_LENGTH_MIN(bin_size)		(((bin_size) + 2) / 3 * 4 + 1)

int32_t ztl_base64_encode(const char* apInBinData, uint32_t aInLength, char* apOutBase64, uint32_t* apInOutLength);
int32_t ztl_base64_decode(const char* apInBase64, uint32_t aInLength, char* apOutBinData, uint32_t* apInOutLength);

#ifdef __cplusplus
}
#endif

#endif//_ZBASE64_H_INCLUDE_
