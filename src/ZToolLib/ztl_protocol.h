#ifndef _ZTL_PROTOCOL_H_
#define _ZTL_PROTOCOL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#pragma pack(push, 1)

/* ztl protocol header
 * 8 bytes
 */
struct ZHeader
{
    uint16_t type;
    uint16_t version;
    uint32_t length;
};

#pragma pack(pop)

/* the protocol body */
#define ZPROTO_BODY(head)       ((struct ZHeader*)(head) + 1)


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_PROTOCOL_H_
