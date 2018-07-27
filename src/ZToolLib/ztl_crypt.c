#include <string.h>
#include <stdlib.h>

#include "ztl_base64.h"
#include "ztl_aes.h"
#include "ztl_crypt.h"


int ztl_aes_encrypt(const char* key, const char* rawdata, int rawlen, char* encryptdata, int* encryptlen)
{
    ztl_aes_t aes;
    ztl_aes_init(&aes, (unsigned char*)key);

    char lrawbuf[4000] = "";
    char* lpraw;
    if (rawlen > sizeof(lrawbuf)) {
        lpraw = (char*)malloc(rawlen + 1);
    }
    else {
        lpraw = lrawbuf;
    }
    memcpy(lpraw, rawdata, rawlen);

    ztl_aes_encode_withlen(&aes, (void*)lpraw, rawlen);

    if (rawlen & 15)
    {
        //rawlen = (rawlen / 16 + 1) * 16;
        rawlen = ((rawlen >> 4) + 1) << 4;
    }

    int rv;
    rv = ztl_base64_encode(lpraw, rawlen, encryptdata, (uint32_t*)encryptlen);

    if (lpraw != lrawbuf) {
        free(lpraw);
    }
    return rv;
}

int ztl_aes_decrypt(const char* key, const char* encryptdata, int encryptlen, char* rawdata, int* rawlen)
{
    ztl_base64_decode(encryptdata, encryptlen, rawdata, (uint32_t*)rawlen);

    ztl_aes_t aes;
    ztl_aes_init(&aes, key);
    rawdata = (char*)ztl_aes_decode_withlen(&aes, rawdata, *rawlen);
    return 0;
}


