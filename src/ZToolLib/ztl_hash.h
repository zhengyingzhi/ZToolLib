#ifndef _ZTL_HASH_H_INCLUDE_
#define _ZTL_HASH_H_INCLUDE_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the murmur_hash2 hash algorithm */
unsigned int ztl_murmur_hash2(unsigned char* data, unsigned int len);

uint64_t ztl_murmur_hash2_64(const void* data, uint32_t len, uint64_t seed);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_HASH_H_INCLUDE_
