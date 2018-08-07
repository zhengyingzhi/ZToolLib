/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_BIT_SET_H_
#define _ZTL_BIT_SET_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* a bitset base int32
 */
struct ztl_bitset_st {
    uint32_t  capacity;
    uint32_t  intnum;
    uint32_t  outmemory;
    uint32_t  pmem[1];
};
typedef struct ztl_bitset_st ztl_bitset_t;

/* get array sub-index since 'int' type is 32 bits */
#define ZTL_ARR_IDX(idx)        ((idx)>>5)

/* get the rlv bit index in a int */
#define ZTL_BIT_IDX(idx)        ((idx)&31)
#define ZTL_CHECK_IDX(idx,bits) if((idx) >= (bits)->capacity) return false
#define ZTL_BIT_UNIT            32
#define ztl_bits_align(d, a)    (((d) + (a - 1)) & ~(a - 1))

#ifdef __cplusplus
#define ZTL_INLINE inline
extern "C" {
#else
#define ZTL_INLINE
#endif//__cplusplus

/* get the minimum number of power 2, but bigger than aNumber
 */
ZTL_INLINE uint32_t ztl_gethigh2(uint32_t aNumber)
{
    uint32_t lRet = 1;
    while (lRet < aNumber) {
        lRet <<= 1;
    }

    return lRet;
}

ZTL_INLINE uint32_t bitset_calc_space(uint32_t bits_num)
{
	bits_num = ztl_bits_align(bits_num, ZTL_BIT_UNIT);
	return sizeof(ztl_bitset_t) + (bits_num / ZTL_BIT_UNIT) * sizeof(uint32_t);
}

/* create a bitset object
 */
ZTL_INLINE ztl_bitset_t* ztl_bitset_new(uint32_t bits_num, void* apaddr)
{
    bits_num = ztl_bits_align(bits_num, ZTL_BIT_UNIT);
    uint32_t lIntNum = bits_num / ZTL_BIT_UNIT;

    ztl_bitset_t* lbits;
    if (apaddr == NULL) {
        lbits = (ztl_bitset_t*)malloc(bitset_calc_space(bits_num));
        lbits->outmemory = 0;
    }
    else {
        lbits = (ztl_bitset_t*)apaddr;
        lbits->outmemory = 1;
    }

    lbits->capacity = bits_num;
    lbits->intnum = lIntNum;
    memset(lbits->pmem, 0, (lbits->intnum + 1) * sizeof(uint32_t));
    return lbits;
}

/* return true if the bit at index is '1' */
ZTL_INLINE bool ztl_bitset_get(ztl_bitset_t* bits, uint32_t index)
{
    ZTL_CHECK_IDX(index, bits);
    return (bits->pmem[ZTL_ARR_IDX(index)] & (1 << ZTL_BIT_IDX(index))) != 0;
}

/* set the bit at index */
ZTL_INLINE bool ztl_bitset_set1(ztl_bitset_t* bits, uint32_t index)
{
    ZTL_CHECK_IDX(index, bits);
    bits->pmem[ZTL_ARR_IDX(index)] |= 1 << ZTL_BIT_IDX(index);
    return true;
}

ZTL_INLINE bool ztl_bitset_set0(ztl_bitset_t* bits, uint32_t index)
{
    ZTL_CHECK_IDX(index, bits);
    bits->pmem[ZTL_ARR_IDX(index)] &= ~(1 << ZTL_BIT_IDX(index));
    return true;
}

/* xor the bit at index */
ZTL_INLINE bool ztl_bitset_flip(ztl_bitset_t* bits, uint32_t index)
{
    ZTL_CHECK_IDX(index, bits);
    bits->pmem[ZTL_ARR_IDX(index)] ^= 1 << ZTL_BIT_IDX(index);
    return true;
}

/* reset all the data */
ZTL_INLINE void ztl_bitset_reset(ztl_bitset_t* bits) {
    memset(bits->pmem, 0, (bits->intnum + 1) * sizeof(uint32_t));
}

/* return true if any bit is '1' */
ZTL_INLINE bool ztl_bitset_any(ztl_bitset_t* bits)
{
    for (uint32_t idx = 0; idx < bits->intnum; ++idx)
    {
        if (bits->pmem[idx])
            return true;
    }
    return false;
}

ZTL_INLINE uint32_t ztl_bitset_capacity(ztl_bitset_t* bits) {
    return bits->capacity;
}

/* return the bits count of '1' */
ZTL_INLINE uint32_t ztl_bitset_count(ztl_bitset_t* bits)
{
    uint32_t cnt = 0;
    uint32_t ival;
    for (uint32_t i = 0; i < bits->intnum; ++i)
    {
        ival = bits->pmem[i];
        for (uint32_t x = 0; x < ZTL_BIT_UNIT; ++x)
        {
            if (ival & (1 << x))
                ++cnt;
        }
    }
    return cnt;
}

/* convert to a char buffer */
ZTL_INLINE int ztl_bitset_tostring(ztl_bitset_t* bits, char* buf, uint32_t size)
{
    for (uint32_t i = 0; i < bits->intnum * 8 && i < size; ++i)
    {
        if (bits->pmem[i >> 5] & (1 << i))
            buf[i] = '1';
        else
            buf[i] = '0';
    }

    int x = 0, y = 31;
    while (x < y) {
        char ch = buf[x];
        buf[x] = buf[y];
        buf[y] = ch;
        ++x;
        --y;
    }

    buf[size - 1] = 0;
    return (int)strlen(buf);
}

/* release the object */
ZTL_INLINE void ztl_bitset_free(ztl_bitset_t* bits)
{
    if (bits && !bits->outmemory)
    {
        free(bits);
    }
}

#ifdef __cplusplus
}
#endif

#endif//_ZTL_BIT_SET_H_
