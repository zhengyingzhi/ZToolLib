#ifndef ZTL_CRC_H
#define ZTL_CRC_H

#include <stdint.h>
#include <string.h>


extern uint32_t  *ztl_crc32_table_short;
extern uint32_t   ztl_crc32_table256[];


static uint32_t ztl_crc32_short(uint8_t *p, size_t len)
{
    uint8_t    c;
    uint32_t  crc;

    crc = 0xffffffff;

    while (len--) {
        c = *p++;
        crc = ztl_crc32_table_short[(crc ^ (c & 0xf)) & 0xf] ^ (crc >> 4);
        crc = ztl_crc32_table_short[(crc ^ (c >> 4)) & 0xf] ^ (crc >> 4);
    }

    return crc ^ 0xffffffff;
}


static uint32_t ztl_crc32_long(uint8_t *p, size_t len)
{
    uint32_t  crc;

    crc = 0xffffffff;

    while (len--) {
        crc = ztl_crc32_table256[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    }

    return crc ^ 0xffffffff;
}


#define ztl_crc32_init(crc)     crc = 0xffffffff


static void ztl_crc32_update(uint32_t *crc, uint8_t *p, size_t len)
{
    uint32_t  c;

    c = *crc;

    while (len--) {
        c = ztl_crc32_table256[(c ^ *p++) & 0xff] ^ (c >> 8);
    }

    *crc = c;
}


#define ztl_crc32_final(crc)        crc ^= 0xffffffff


int32_t ztl_crc32_table_init(void);



uint64_t ztl_crc64(uint64_t crc, const unsigned char *s, uint64_t l);


#endif//ZTL_CRC_H
