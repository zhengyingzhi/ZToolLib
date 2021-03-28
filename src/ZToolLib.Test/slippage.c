#include <stdio.h>
#include <string.h>

#include "ZToolLib/ztl_utils.h"

#include "slippage.h"


static int process_order_snapshot(slippage_t* slip, order_t* order, void* md, int32_t* pfilled_qty, double* pfilled_price)
{
    snapshot_t* snap;
    snap = (snapshot_t*)md;

    if ((order->direction == 'B' && order->order_price >= snap->last_price) ||
        (order->direction == 'S' && order->order_price <= snap->last_price) ||
        (order->order_type == 'U'))
    {
        *pfilled_qty = (int)(ztl_min(order->order_qty - order->filled_qty, snap->volume));
        *pfilled_price = snap->last_price;
    }

    return 0;
}

slippage_t slippage_snapshot = {
    NULL,
    process_order_snapshot,
    "snapshot"
};

//////////////////////////////////////////////////////////////////////////
static int process_order_each_trade(slippage_t* slip, order_t* order, void* md,
    int32_t* pfilled_qty, double* pfilled_price)
{
    each_trade_t* et;
    et = (each_trade_t*)md;

    if (et->bs_flag == 'C') {
        return -1;
    }

    if ((order->direction == 'B' && order->order_price >= et->price) ||
        (order->direction == 'S' && order->order_price <= et->price) ||
        (order->order_type == 'U'))
    {
        *pfilled_qty = ztl_min(order->order_qty - order->filled_qty, et->volume);
        *pfilled_price = et->price;
    }

    return 0;
}

slippage_t slippage_each_trade = {
    NULL,
    process_order_each_trade,
    "each_trade"
};

//////////////////////////////////////////////////////////////////////////

static int process_order_kline(slippage_t* slip, order_t* order, void* md,
    int32_t* pfilled_qty, double* pfilled_price)
{
    kline_t* bar;
    bar = (kline_t*)md;

    if ((order->direction == 'B' && order->order_price >= bar->close) ||
        (order->direction == 'S' && order->order_price <= bar->close) ||
        (order->order_type == 'U'))
    {
        *pfilled_qty = (int)(ztl_min(order->order_qty - order->filled_qty, bar->volume));
        *pfilled_price = bar->close;
    }

    return 0;
}

slippage_t slippage_kline = {
    NULL,
    process_order_kline,
    "kline"
};

//////////////////////////////////////////////////////////////////////////
slippage_t* get_slippage(const char* name)
{
    if (strcmp(name, SLIPPAGE_NAME_EachTrade) == 0)
        return &slippage_each_trade;
    if (strcmp(name, SLIPPAGE_NAME_Snapshot) == 0)
        return &slippage_snapshot;
    if (strcmp(name, SLIPPAGE_NAME_KLine) == 0)
        return &slippage_kline;
    return NULL;
}
