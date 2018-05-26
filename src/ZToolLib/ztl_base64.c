#include "ztl_base64.h"  

int32_t ztl_base64_encode(const char* apInBinData, uint32_t aInLength, char* apOutBase64, uint32_t* apInOutLength)
{
    static char const table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    char* lpData = apOutBase64;
    uint32_t bits = 0;
    uint32_t left = aInLength;
    int32_t  shift = 0;

    if (!apInBinData || !apOutBase64 || *apInOutLength < _ZTL_BASE64_ENCODE_LENGTH_MIN(aInLength))
        return -1;

    while (left)
    {
        bits = (bits << 8) + *apInBinData++;
        left--;
        shift += 8;

        do
        {
            *lpData++ = table[(bits << 6 >> shift) & 0x3f];
            shift -= 6;
        } while (shift > 6 || (left == 0 && shift > 0));
    }

    while ((lpData - apOutBase64) & 3) *lpData++ = '=';
    *lpData = '\0';

    *apInOutLength = (uint32_t)(lpData - apOutBase64);
    return 0;
}


int32_t ztl_base64_decode(const char* apInBase64, uint32_t aInLength, char* apOutBinData, uint32_t* apInOutLength)
{
    static uint8_t table[] =
    {
        0x3e, 0xff, 0xff, 0xff, 0x3f, 0x34, 0x35, 0x36
        ,   0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff
        ,   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x01
        ,   0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
        ,   0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11
        ,   0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19
        ,   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a, 0x1b
        ,   0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23
        ,   0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b
        ,   0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
    };

    uint32_t    i = 0;
    uint32_t    v = 0;
    uint8_t*  lpData = (uint8_t*)apOutBinData;
    uint32_t   tn = sizeof(table) / sizeof(uint8_t);


    if (!apInBase64 && !apOutBinData)
        return -1;

    for (i = 0; i < aInLength && apInBase64[i] && apInBase64[i] != '='; i++)
    {
        uint32_t idx = apInBase64[i] - 43;
        if (idx >= tn || table[idx] == 0xff) return 0;

        v = (v << 6) + table[idx];
        if (i & 3)
        {
            if ((uint32_t)(lpData - (uint8_t*)apOutBinData) < *apInOutLength)
                *lpData++ = v >> (6 - 2 * (i & 3));
        }
    }

    *apInOutLength = (uint32_t)(lpData - (uint8_t*)apOutBinData);
    return 0;
}

