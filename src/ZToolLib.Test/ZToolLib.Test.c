#include <stdio.h>
#include <assert.h>

#include <ZToolLib/ztl_atomic.h>
#include <ZToolLib/ztl_buffer.h>
#include <ZToolLib/ztl_common.h>
#include <ZToolLib/ztl_config.h>
#include <ZToolLib/ztl_logger.h>

void test_ztl_config();
void test_ztl_log();

int main(int argc, char* argv[])
{
    ZTL_NOTUSED(argc);
    ZTL_NOTUSED(argv);

    test_ztl_config();
    //test_ztl_log();

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

    ztl_log_error(log1, ZTL_LOG_INFO, "hello log %d", 1);
    ztl_log_error(log2, ZTL_LOG_WARN, "hello log 2");
    getchar();

    ztl_log_close(log1);
    ztl_log_close(log2);
}
