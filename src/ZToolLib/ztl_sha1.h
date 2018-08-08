#ifndef ZTL_SHA1_H
#define ZTL_SHA1_H
/* ================ sha1.h ================ */
/*
SHA-1 in C
By Steve Reid <steve@edmweb.com>
100% Public Domain
*/

#include <stdint.h>

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
}ZTL_SHA1_CTX;

void ZTL_SHA1Transform(uint32_t state[5], const unsigned char buffer[64]);
void ZTL_SHA1Init(ZTL_SHA1_CTX* context);
void ZTL_SHA1Update(ZTL_SHA1_CTX* context, const unsigned char* data, uint32_t len);
void ZTL_SHA1Final(unsigned char digest[20], ZTL_SHA1_CTX* context);

#endif//ZTL_SHA1_H
