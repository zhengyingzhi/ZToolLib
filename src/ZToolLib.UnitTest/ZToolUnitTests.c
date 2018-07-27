/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <ZToolLib/ztl_unit_test.h>
#include "ZToolUnitTest.h"


extern void Test_ztl_array(ZuTest*);
extern void Test_ztl_util(ZuTest*);
extern void Test_ztl_ll2string(ZuTest*);
extern void Test_ztl_atoin(ZuTest*);
extern void Test_ztl_trim(ZuTest*);
extern void Test_ztl_parse_size(ZuTest*);
extern void Test_ztl_strdelimiter(ZuTest*);
extern void Test_ztl_buffer(ZuTest* zt);
extern void Test_ztl_buffer2(ZuTest* zt);
extern void Test_ztl_linklist(ZuTest* zt);

extern void Test_lfqueue(ZuTest* zt);
extern void Test_lfqueue2(ZuTest* zt);
extern void Test_lfqueue3(ZuTest* zt);
extern void Test_ztl_producer_consumer(ZuTest* zt);

extern void Test_ztl_times(ZuTest* zt);


void RunAllTests(void)
{
    ZuString *output = ZuStringNew();
    ZuSuite* suite = ZuSuiteNew();


    SUITE_ADD_TEST(suite, Test_ztl_array);
    SUITE_ADD_TEST(suite, Test_ztl_util);
    SUITE_ADD_TEST(suite, Test_ztl_ll2string);
    SUITE_ADD_TEST(suite, Test_ztl_atoin);
    SUITE_ADD_TEST(suite, Test_ztl_trim);
    SUITE_ADD_TEST(suite, Test_ztl_parse_size);
    SUITE_ADD_TEST(suite, Test_ztl_strdelimiter);

    SUITE_ADD_TEST(suite, Test_ztl_buffer);
    SUITE_ADD_TEST(suite, Test_ztl_buffer2);
    SUITE_ADD_TEST(suite, Test_ztl_linklist);

    SUITE_ADD_TEST(suite, Test_lfqueue);
    SUITE_ADD_TEST(suite, Test_lfqueue2);
    SUITE_ADD_TEST(suite, Test_lfqueue3);
    SUITE_ADD_TEST(suite, Test_ztl_producer_consumer);

    SUITE_ADD_TEST(suite, Test_ztl_times);

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
