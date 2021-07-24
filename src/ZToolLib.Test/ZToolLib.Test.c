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

extern void tcp_server_demo(int argc, char* argv[]);
extern void tcp_client_demo(int argc, char* argv[]);
extern void threadpool_demo(int argc, char* argv[]);
extern void event_dispatcher_demo(int argc, char* argv[]);
extern void producer_consumer_demo(int argc, char* argv[]);
extern void trans_md_demo(int argc, char* argv[]);


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

    if (argc > 2 && strcmp(argv[1], "server") == 0) {
        tcp_server_demo(argc, argv);
        return 0;
    }
    else if (argc > 2 && strcmp(argv[1], "client") == 0) {
        tcp_client_demo(argc, argv);
        return 0;
    }

    // tcp_server_demo(argc, argv);
    // trans_md_demo(argc, argv);
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
