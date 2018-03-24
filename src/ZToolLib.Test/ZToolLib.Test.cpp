#include <stdio.h>

#include <ZToolLib/ztl_atomic.h>
#include <ZToolLib/ztl_buffer.h>
#include <ZToolLib/ztl_logger.h>

void test_ztl_log();

int main(int argc, char* argv[])
{
    test_ztl_log();

    return 0;
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
