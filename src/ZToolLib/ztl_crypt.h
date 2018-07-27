#ifndef _ZTL_CRYPT_H_
#define _ZTL_CRYPT_H_


#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* encrypt by aes algorithm, output base64 data
 */
int ztl_aes_encrypt(const char* key, const char* rawdata, int rawlen, char* encryptdata, int* encryptlen);

/* input the encpyted base64 data, output raw data 
 */
int ztl_aes_decrypt(const char* key, const char* encryptdata, int encryptlen, char* rawdata, int* rawlen);


#ifdef __cplusplus
}
#endif


#endif//_ZTL_CRYPT_H_