#ifndef _ZTL_HASH_H_INCLUDE_
#define _ZTL_HASH_H_INCLUDE_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the murmur_hash2 hash algorithm */
uint32_t ztl_murmur_hash2(unsigned char* data, uint32_t len);
uint64_t ztl_murmur_hash2_64(const void* data, uint32_t len, uint64_t seed);

/* Peter J. Weinberger hash */
uint32_t ztl_hashpjw(const void* key);

uint32_t ztl_hashdjb2(const void* key);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_HASH_H_INCLUDE_
