/* AES encrypt algorithm implemented by C
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_AES_H_
#define _ZTL_AES_H_


typedef struct  
{
    unsigned char Sbox[256];
    unsigned char InvSbox[256];
    unsigned char w[11][4][4];
}ztl_aes_t;

#ifdef __cplusplus
extern "C" {
#endif


void ztl_aes_init(ztl_aes_t* aes, const unsigned char* key);

unsigned char* ztl_aes_encode(ztl_aes_t* aes, unsigned char* input);
unsigned char* ztl_aes_decode(ztl_aes_t* aes, unsigned char* input);

void* ztl_aes_encode_withlen(ztl_aes_t* aes, void* input, int length);
void* ztl_aes_decode_withlen(ztl_aes_t* aes, void* input, int length);


#ifdef __cplusplus
}
#endif


#endif//_ZTL_AES_H_
