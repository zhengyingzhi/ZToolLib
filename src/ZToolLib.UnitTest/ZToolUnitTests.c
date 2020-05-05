/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <ZToolLib/ztl_unit_test.h>
#include "ZToolUnitTest.h"


extern void Test_ztl_array(ZuTest*);
extern void Test_ztl_array2(ZuTest* zt);
extern void Test_ztl_array3(ZuTest* zt);
extern void Test_ztl_vector1(ZuTest* zt);
extern void Test_ztl_vector2(ZuTest* zt);
extern void Test_ztl_vector3(ZuTest* zt);
extern void Test_cJSON(ZuTest* zt);
extern void Test_cJSON2(ZuTest* zt);
extern void Test_ztl_util(ZuTest*);
extern void Test_ztl_ll2string(ZuTest*);
extern void Test_ztl_atoin(ZuTest*);
extern void Test_ztl_trim(ZuTest*);
extern void Test_ztl_parse_size(ZuTest*);
extern void Test_ztl_strdelimiter(ZuTest*);
extern void Test_ztl_ztlncpy(ZuTest* zt);

extern void Test_ztl_buffer(ZuTest* zt);
extern void Test_ztl_buffer2(ZuTest* zt);
extern void Test_ztl_linklist(ZuTest* zt);
extern void Test_ztl_dlist(ZuTest* zt);

extern void Test_ztl_mempool(ZuTest* zt);
extern void Test_ztl_shm_readonly(ZuTest* zt);
extern void Test_ztl_shm_readwrite(ZuTest* zt);

extern void Test_ztl_map(ZuTest* zt);
extern void Test_ztl_map_ex(ZuTest* zt);

extern void Test_lfqueue0(ZuTest* zt);
extern void Test_lfqueue(ZuTest* zt);
extern void Test_lfqueue2(ZuTest* zt);
extern void Test_lfqueue3(ZuTest* zt);
extern void Test_ztl_producer_consumer(ZuTest* zt);

extern void Test_ztl_times(ZuTest* zt);
extern void Test_ztl_base64(ZuTest* zt);
extern void Test_ztl_encrypt(ZuTest* zt);
extern void Test_ztl_dict(ZuTest* zt);


void RunAllTests(void)
{
    fprintf(stderr, "ztl version:v%s\n", ZTL_Version);

    ZuString *output = ZuStringNew();
    ZuSuite* suite = ZuSuiteNew();

    SUITE_ADD_TEST(suite, Test_ztl_array);
    SUITE_ADD_TEST(suite, Test_ztl_array2);
    SUITE_ADD_TEST(suite, Test_ztl_array3);
    SUITE_ADD_TEST(suite, Test_ztl_vector1);
    SUITE_ADD_TEST(suite, Test_ztl_vector2);
    SUITE_ADD_TEST(suite, Test_ztl_vector3);
    SUITE_ADD_TEST(suite, Test_cJSON);
    SUITE_ADD_TEST(suite, Test_cJSON2);
    SUITE_ADD_TEST(suite, Test_ztl_util);
    SUITE_ADD_TEST(suite, Test_ztl_ll2string);
    SUITE_ADD_TEST(suite, Test_ztl_atoin);
    SUITE_ADD_TEST(suite, Test_ztl_trim);
    SUITE_ADD_TEST(suite, Test_ztl_parse_size);
    SUITE_ADD_TEST(suite, Test_ztl_strdelimiter);
    SUITE_ADD_TEST(suite, Test_ztl_ztlncpy);

    SUITE_ADD_TEST(suite, Test_ztl_buffer);
    SUITE_ADD_TEST(suite, Test_ztl_buffer2);
    SUITE_ADD_TEST(suite, Test_ztl_linklist);
    SUITE_ADD_TEST(suite, Test_ztl_dlist);
    SUITE_ADD_TEST(suite, Test_ztl_mempool);
    SUITE_ADD_TEST(suite, Test_ztl_shm_readonly);
    SUITE_ADD_TEST(suite, Test_ztl_shm_readwrite);

    SUITE_ADD_TEST(suite, Test_ztl_map);
    SUITE_ADD_TEST(suite, Test_ztl_map_ex);

    SUITE_ADD_TEST(suite, Test_lfqueue0);
    SUITE_ADD_TEST(suite, Test_lfqueue);
    SUITE_ADD_TEST(suite, Test_lfqueue2);
    SUITE_ADD_TEST(suite, Test_lfqueue3);
    SUITE_ADD_TEST(suite, Test_ztl_producer_consumer);

    SUITE_ADD_TEST(suite, Test_ztl_times);
    SUITE_ADD_TEST(suite, Test_ztl_base64);
    SUITE_ADD_TEST(suite, Test_ztl_encrypt);
    SUITE_ADD_TEST(suite, Test_ztl_dict);

    ZuSuiteRun(suite);
    ZuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main(int argc, char* argv[])
{
    ZTL_NOTUSED(argc);
    ZTL_NOTUSED(argv);

    net_init();

    RunAllTests();
    return 0;
}
