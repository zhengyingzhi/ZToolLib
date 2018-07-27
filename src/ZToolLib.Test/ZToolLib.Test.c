#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ZToolLib/ztl_atomic.h>
#include <ZToolLib/ztl_buffer.h>
#include <ZToolLib/ztl_common.h>
#include <ZToolLib/ztl_config.h>
#include <ZToolLib/ztl_logger.h>
#include <ZToolLib/lockfreequeue.h>

#include <ZToolLib/ztl_base64.h>

#include <ZToolLib/ztl_utils.h>


void test_ztl_config();
void test_ztl_log();

void test_lfqueue();

void test_base64();


int main(int argc, char* argv[])
{
    ZTL_NOTUSED(argc);
    ZTL_NOTUSED(argv);

    //test_ztl_config();
    //test_ztl_log();

    //test_lfqueue();

	test_base64();

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

    lfqueue_pop(que, &ldata);
    lfqueue_pop(que, &ldata);

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
	ztl_base64_encode(lpChanged, strlen(lpChanged), lBase64String, &lBase64Length);

	// decode and change
	char lTemp[256] = "";
	strcpy(lTemp, lBase64String);
	lRawLength = sizeof(lRawString) - 1;
	ztl_base64_decode(lTemp, lBase64Length, lRawString, &lRawLength);
	lpRaw = zpassword_change(lRawString);

	assert(strcmp(lpRaw, "111111") == 0);
}
