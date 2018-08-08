
#ifndef _ZTL_MD5_H_INCLUDED_
#define _ZTL_MD5_H_INCLUDED_

#include <stdint.h>


typedef struct {
    uint64_t  bytes;
    uint32_t  a, b, c, d;
    uint8_t   buffer[64];
}ztl_md5_t;


void ztl_md5_init(ztl_md5_t *ctx);
void ztl_md5_update(ztl_md5_t *ctx, const void *data, size_t size);
void ztl_md5_final(unsigned char result[16], ztl_md5_t *ctx);


#endif //_ZTL_MD5_H_INCLUDED_
