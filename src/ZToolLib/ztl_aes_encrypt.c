#include <string.h>
#include <stdlib.h>

#include "ztl_base64.h"
#include "ztl_aes.h"
#include "ztl_aes_encrypt.h"


int ztl_encrypt(const char* key, const char* rawdata, int rawlen, char* encryptdata, int* encryptlen)
{
    ztl_aes_t aes;
    ztl_aes_init(&aes, (unsigned char*)key);

    ztl_aes_encode_withlen(&aes, (void*)rawdata, rawlen);

    if (rawlen & 15)
    {
        //rawlen = (rawlen / 16 + 1) * 16;
        rawlen = ((rawlen >> 4) + 1) << 4;
    }

    return ztl_base64_encode(rawdata, rawlen, encryptdata, (uint32_t*)encryptlen);
}

int ztl_decrypt(const char* key, const char* encryptdata, int encryptlen, char* rawdata, int* rawlen)
{
    ztl_base64_decode(encryptdata, encryptlen, rawdata, (uint32_t*)rawlen);

    ztl_aes_t aes;
    ztl_aes_init(&aes, key);
    rawdata = (char*)ztl_aes_decode_withlen(&aes, rawdata, *rawlen);
    return 0;
}


