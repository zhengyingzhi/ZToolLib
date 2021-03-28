#ifndef _ZTL_SLIPPAGE_H_
#define _ZTL_SLIPPAGE_H_

#include <stdint.h>


#define SLIPPAGE_NAME_EachTrade     "each_trade"
#define SLIPPAGE_NAME_Snapshot      "snapshot"
#define SLIPPAGE_NAME_KLine         "kline"

/* 实现一个撮合模型 Demo
 */

typedef struct order_st
{
    char    instrument[16];
    double  order_price;
    int32_t order_qty;
    int32_t filled_qty;
    char    direction;
    char    offset;
    char    order_type;
    int8_t  padding;
}order_t;

typedef struct snapshot_st
{
    char    instrument[16];
    char    exchange[8];
    double  last_price;
    double  turnover;
    int64_t volume;
}snapshot_t;

typedef struct each_trade_st
{
    char    instrument[16];
    char    exchange[8];
    int64_t seq_num;
    int64_t bid_seq_num;
    int64_t ask_seq_num;
    double  price;
    int32_t volume;
    char    bs_flag;
    char    filled_type;
}each_trade_t;

typedef struct kline_st
{
    char    instrument[16];
    char    exchange[8];
    double  open;
    double  high;
    double  low;
    double  close;
    double  turnover;
    int64_t volume;
}kline_t;



typedef struct slippage_st slippage_t;
struct slippage_st
{
    void*       udata;
    int(*process_order)(slippage_t* slip, order_t* order, void* md, int32_t* pfilled_qty, double* pfilled_price);
    const char* name;
};


// get slippage model by SLIPPAGE_NAME_XxxYyy
slippage_t* get_slippage(const char* name);


#endif//_ZTL_SLIPPAGE_H_
