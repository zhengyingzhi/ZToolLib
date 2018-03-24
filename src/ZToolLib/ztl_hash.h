#ifndef _ZTL_HASH_H_INCLUDE_
#define _ZTL_HASH_H_INCLUDE_

#ifdef __cplusplus
extern "C" {
#endif

/* the murmur_hash2 hash algorithm */
unsigned int ztl_murmur_hash2(unsigned char* data, unsigned int len);


#ifdef __cplusplus
}
#endif

#endif//_ZTL_HASH_H_INCLUDE_
