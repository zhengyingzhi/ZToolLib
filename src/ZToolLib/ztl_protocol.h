/*
* Copyright(C) Yingzhi Zheng.
* Copyright(C) <zhengyingzhi112@163.com>
*/

#ifndef _ZTL_PROTOCOL_H_
#define _ZTL_PROTOCOL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#pragma pack(push , 1)

/* ztl protocol header
 * 16 bytes
 */
struct ZHeader
{
    uint16_t m_StartType;
    uint16_t m_Version;
    uint32_t m_PayloadLength;
    uint16_t m_Type;
    uint16_t m_Flags;
    uint32_t m_SessionKey;
};

#pragma pack(pop)



#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_PROTOCOL_H_
