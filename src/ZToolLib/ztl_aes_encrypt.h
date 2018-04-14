#ifndef _ZTL_AES_ENCRYPT_H_
#define _ZTL_AES_ENCRYPT_H_


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


int ztl_encrypt(const char* key, const char* rawdata, int rawlen, char* encryptdata, int* encryptlen);
int ztl_decrypt(const char* key, const char* encryptdata, int encryptlen, char* rawdata, int* rawlen);


#ifdef __cplusplus
}
#endif


#endif//_ZTL_AES_ENCRYPT_H_