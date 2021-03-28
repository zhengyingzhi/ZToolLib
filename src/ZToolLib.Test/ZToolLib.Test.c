#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ZToolLib/ztl_atomic.h>
#include <ZToolLib/ztl_buffer.h>
#include <ZToolLib/ztl_common.h>
#include <ZToolLib/ztl_config.h>
#include <ZToolLib/ztl_logger.h>
#include <ZToolLib/ztl_dstr.h>
#include <ZToolLib/lockfreequeue.h>

#include <ZToolLib/ztl_base64.h>
#include <ZToolLib/ztl_memdb.h>
#include <ZToolLib/ztl_times.h>
#include <ZToolLib/ztl_utils.h>

#include "high_time.h"
#include "slippage.h"

#ifdef _MSC_VER
#include <Winsock2.h>

#pragma comment(lib, "winmm.lib")
#endif


extern void net_init();
void test_ztl_config();
void test_ztl_log();
void test_lfqueue();
void test_base64();
void test_read_file();
void test_char_conv();
void test_memdb();
void test_dstr();

void test_high_time();
void test_slippage();

extern void tcp_server_demo(int argc, char* argv[]);
extern void tcp_client_demo(int argc, char* argv[]);
extern void threadpool_demo(int argc, char* argv[]);
extern void event_dispatcher_demo(int argc, char* argv[]);
extern void producer_consumer_demo(int argc, char* argv[]);
extern void trans_md_demo(int argc, char* argv[]);

extern slippage_t* get_slippage(const char* name);


int main(int argc, char* argv[])
{
    ZTL_NOTUSED(argc);
    ZTL_NOTUSED(argv);

    net_init();

    //test_ztl_config();
    //test_ztl_log();

    // test_lfqueue();
    // test_base64();
    // test_read_file();
    // test_char_conv();

    // test_memdb();
    // test_dstr();
    // test_high_time();
    // test_slippage();

    // TODO: 再实现一个行情转发系统
    /* 2个 connection to server，2个 listen on 8500 & 8501
     * 1. 向mdgw发起登录请求，并保持心跳
     * 2. 从mdgw接收行情，并转发给client端，没有客户端则直接抛弃数据包
     * 3. 接收client端的登录请求，并返回应答（大端模式数据），且要操持心跳
     * 4. 
     */

    if (argc > 2 && strcmp(argv[1], "server") == 0) {
        tcp_server_demo(argc, argv);
        return 0;
    }
    else if (argc > 2 && strcmp(argv[1], "client") == 0) {
        tcp_client_demo(argc, argv);
        return 0;
    }

    // tcp_server_demo(argc, argv);
    trans_md_demo(argc, argv);
    return 0;

    threadpool_demo(argc, argv);
    event_dispatcher_demo(argc, argv);
    producer_consumer_demo(argc, argv);

    return 0;
}

void test_ztl_config()
{
    ztl_config_t* zconf;
    zconf = ztl_config_open("ztl_test_config.txt", '#', ' ');
    assert(zconf);

    char* lpval = NULL;
    ztl_config_read_str(zconf, "ip", &lpval, NULL);

    uint16_t port = 0;
    ztl_config_read_int16(zconf, "port", &port);

    uint32_t lnum = 0;
    ztl_config_read_int32(zconf, "num", &lnum);

    ztl_config_close(zconf);
}

void test_ztl_log()
{
    ztl_log_t* log1, *log2;
    log1 = ztl_log_create("test_ztl_log1.log", ZTL_PrintScrn, false);

    log2 = ztl_log_create("test_ztl_log2.log", ZTL_WritFile, true);

    ztl_log_set_level(log1, ZTL_LOG_DEBUG);
    ztl_log_set_level(log2, ZTL_LOG_INFO);

    ztl_log_info(log1, "hello log %d", 1);
    ztl_log_warn(log2, "hello log 2");
    getchar();

    ztl_log_close(log1);
    ztl_log_close(log2);
}


void test_lfqueue()
{
    typedef struct  
    {
        void* handle;
        void* arg;
    }ztest_data_t;
    lfqueue_t* que;
    que = lfqueue_create(4, sizeof(ztest_data_t));

    ztest_data_t ldata;
    ldata.handle = (void*)1;
    ldata.arg = (void*)2;
    lfqueue_push(que, &ldata);

    ldata.handle = (void*)3;
    ldata.arg = (void*)4;
    lfqueue_push(que, &ldata);

    lfqueue_pop(que, (void**)&ldata);
    lfqueue_pop(que, (void**)&ldata);

}

void test_base64()
{
    char lString[256] = "111111";
    char* lpChanged, *lpRaw;

    char lBase64String[256] = "";
    char lRawString[256] = "";
    uint32_t lBase64Length, lRawLength;

#if 0
    lpChanged = zpassword_change(lString);
    lpRaw = zpassword_change(lpChanged);

    assert(strcmp(lpRaw, lString) == 0);

    char lBase64String[256] = "";
    uint32_t lBase64Length = sizeof(lBase64String) - 1;
    ztl_base64_encode(lpRaw, strlen(lpRaw), lBase64String, &lBase64Length);

    char lRawString[256] = "";
    uint32_t lRawLength = sizeof(lRawString) - 1;
    ztl_base64_decode(lBase64String, lBase64Length, lRawString, &lRawLength);

    assert(strcmp(lRawString, lString) == 0);

    printf("%s\n", lRawString);

#endif

    // change and encode
    lpChanged = zpassword_change(lString);
    lBase64Length = sizeof(lBase64String) - 1;
    ztl_base64_encode(lpChanged, (uint32_t)strlen(lpChanged), lBase64String, &lBase64Length);

    // decode and change
    char lTemp[256] = "";
    strcpy(lTemp, lBase64String);
    lRawLength = sizeof(lRawString) - 1;
    ztl_base64_decode(lTemp, lBase64Length, lRawString, &lRawLength);
    lpRaw = zpassword_change(lRawString);

    assert(strcmp(lpRaw, "111111") == 0);
}

void test_read_file()
{
    typedef struct {
        char name[16];
        int  age;
        double score;
    }test_rf_st;

    const char* filename = "test_read.txt";
    char buf1[16] = "";
    read_file_content(filename, buf1, sizeof(buf1) - 1);
    printf("read: %s\n", buf1);

    const char* filename2 = "test_read_ex.txt";
    test_rf_st rfst = { 0 };
    strcpy(rfst.name, "yizhe");
    rfst.age = 29;
    rfst.score = 7.6;
    FILE* fp = fopen(filename2, "w+");
    if (fp)
    {
        fwrite(&rfst, sizeof(rfst), 1, fp);
        fclose(fp);
    }

    test_rf_st rfst1 = { 0 };
    read_file_content(filename2, (char*)&rfst1, 18);

    test_rf_st rfst2 = { 0 };
    read_file_content(filename2, (char*)&rfst2, sizeof(rfst2));
    printf("rfst2: %s,%d,%lf\n", rfst2.name, rfst2.age, rfst2.score);
}

static const uint32_t powers_of_10_32[] = {
    UINT32_C(0),          UINT32_C(10),       UINT32_C(100),
    UINT32_C(1000),       UINT32_C(10000),    UINT32_C(100000),
    UINT32_C(1000000),    UINT32_C(10000000), UINT32_C(100000000),
    UINT32_C(1000000000)
};

static inline uint32_t to_chars_len(uint32_t value)
{
    // FIXME error
    // const unsigned t = (32 - __builtin_clz(value | 1)) * 1233 >> 12;
    const unsigned t = 0;
    return t - (value < powers_of_10_32[t]) + 1;
}

void test_char_conv()
{
    for (int i = 0; i < 10; ++i)
    {
        int x = powers_of_10_32[i];
        printf("%d,%d\n", x, to_chars_len(x));
    }
    printf("\n");
}



typedef struct sim_tick_s
{
    char instrument[8];
    double last_price;
    double turnover;
    int64_t volume;
}sim_tick_t;

void test_memdb()
{
    const char* dbname = "mdb_test.dat";
    uint32_t dbsize = 32 * 1024 * 1024;
    int rv;
    ztl_memdb_t* mdb;
    mdb = ztl_memdb_create(dbname, dbsize, 4096, 0);
    rv = ztl_memdb_open(mdb);
    fprintf(stderr, "ztl_memdb_open rv1:%d\n", rv);

    uint32_t exist_count;
    exist_count = ztl_memdb_count(mdb);
    fprintf(stderr, "ztl_memdb_count:%d\n", exist_count);

    ztl_seq_t seq;
    ztl_entry_t* entry;
    sim_tick_t* stick;
    sim_tick_t* rtick;

    ztl_seq_t seqs[1024] = { 0 };
    uint32_t count = 5;
    for (uint32_t i = exist_count; i < exist_count + count; ++i)
    {
        // write some data here
        entry = ztl_memdb_alloc_entry(mdb, ztl_align(sizeof(sim_tick_t), 8));
        stick = (sim_tick_t*)entry;
        sprintf(stick->instrument, "%06d", i + 1);
        stick->last_price = 20 + i + i * 0.1;
        stick->volume = 100 * (i + 1);
        stick->turnover = stick->last_price * stick->volume;
        seq = ztl_memdb_direct_append(mdb, entry);

        if (seq != 0) {
            seqs[i] = seq;
        }
    }

    // read
    for (uint32_t i = 1; i <= exist_count + count; ++i)
    {
        // rtick = (sim_tick_t*)ztl_memdb_get_entry(mdb, seqs[i]);
        rtick = (sim_tick_t*)ztl_memdb_get_entry(mdb, i);
        if (!rtick)
        {
            fprintf(stderr, "11 tick null when read index:%d\n", i);
            break;
        }
        fprintf(stderr, "tick%d instr:%s, price:%.2lf, vol:%lld\n", i, rtick->instrument, rtick->last_price, rtick->volume);
    }

    ztl_memdb_release(mdb);
    mdb = NULL;

    // re-open the db
    mdb = ztl_memdb_create(dbname, dbsize, 4096, 0);
    rv = ztl_memdb_open(mdb);
    fprintf(stderr, "ztl_memdb_open rv2:%d\n", rv);

    // check some data here
    for (uint32_t i = 0; i < count; ++i)
    {
        rtick = (sim_tick_t*)ztl_memdb_get_entry(mdb, seqs[i]);
        if (!rtick)
        {
            fprintf(stderr, "22 tick null when read index:%d\n", i);
            break;
        }
        fprintf(stderr, "tick%d instr:%s, price:%.2lf, vol:%lld\n", i, rtick->instrument, rtick->last_price, rtick->volume);
    }

    ztl_memdb_release(mdb);
}

void test_dstr()
{
    dstr s1;
    dstr s2;
    dstr res;

    s1 = dstr_new("hello");
    printf("s1 len:%zu, data:%s\n", dstr_length(s1), s1);

    s2 = dstr_new_len(NULL, 32);
    printf("s2 len:%zu, data:%s\n", dstr_length(s2), s2);

    s2 = dstr_cat(s2, "this ");
    s2 = dstr_cat_len(s2, "game", 2);
    printf("s2 len:%zu, data:%s\n", dstr_length(s2), s2);

    res = dstr_new("HELLO");
    res = dstr_cat(res, "|192.168.1.178");
    res = dstr_cat(res, "|PC");
    res = dstr_cat(res, "\r\n");
    printf("res len:%zu, data:%s\n", dstr_length(res), res);
}

void test_high_time()
{
    high_time_t ht;
    struct timeval tv;
    int times[] = { 93000000, 93003000, 93003000, 93006000, 93009000, 93009000,
        93012000, 93012000, 93015000, 93018000 };
    int curr_time;
    int speed;

    speed = 1;      // 1-原速，2-2倍速，4-4倍速

#ifdef _MSC_VER
    timeBeginPeriod(1);
#endif
    high_time_init(&ht, speed);
    high_time_update_first(&ht, times[0]);

    for (int i = 0; i < sizeof(times) / sizeof(times[0]); ++i)
    {
        curr_time = times[i];
        high_time_update_and_sleep(&ht, curr_time);

        // simulate proc the business of curr_time
        gettimeofday(&tv, NULL);
        fprintf(stderr, "[%d.%06d] procing curr_time=%d\n\n",
            ztl_tointtime(tv.tv_sec), tv.tv_usec, curr_time);
        // if (i % 2 == 0)
        //     ztl_sleepus((i + 1) * 500);
    }
}

void test_slippage()
{
    int rv;
    int32_t filled_qty;
    double filled_price;
    slippage_t* slip;

    // get properate slip object
    // slip = &slippage_snapshot;

    order_t ord = { 0 };
    strcpy(ord.instrument, "000001");
    ord.order_qty = 1000;
    ord.order_price = 22.21;
    ord.direction = 'B';

    snapshot_t snap = { 0 };
    strcpy(snap.instrument, "000001");
    snap.last_price = 22.02;
    snap.volume = 123400;

    each_trade_t et = { 0 };
    strcpy(et.instrument, "000001");
    et.volume = 600;
    et.price = 22.04;
    et.bs_flag = 'B';
    et.filled_type = 'N';

    kline_t kline = { 0 };
    strcpy(kline.instrument, "000001");
    kline.open = 22.02;
    kline.high = 22.88;
    kline.low = 22;
    kline.close = 22.05;
    kline.volume = 223300;

    filled_qty = 0;
    filled_price = 0.0;
    slip = get_slippage(SLIPPAGE_NAME_Snapshot);
    rv = slip->process_order(slip, &ord, &snap, &filled_qty, &filled_price);
    fprintf(stderr, "snapshot  rv=%d, filled_qty=%d, filled_price=%.3lf\n", rv, filled_qty, filled_price);

    filled_qty = 0;
    filled_price = 0.0;
    slip = get_slippage(SLIPPAGE_NAME_EachTrade);
    rv = slip->process_order(slip, &ord, &et, &filled_qty, &filled_price);
    fprintf(stderr, "eachtrade rv=%d, filled_qty=%d, filled_price=%.3lf\n", rv, filled_qty, filled_price);

    filled_qty = 0;
    filled_price = 0.0;
    slip = get_slippage(SLIPPAGE_NAME_KLine);
    rv = slip->process_order(slip, &ord, &kline, &filled_qty, &filled_price);
    fprintf(stderr, "kline     rv=%d, filled_qty=%d, filled_price=%.3lf\n", rv, filled_qty, filled_price);
}

