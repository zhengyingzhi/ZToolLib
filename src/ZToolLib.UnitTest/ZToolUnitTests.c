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


    ZuSuiteRun(suite);
    ZuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main(int argc, char* argv[])
{
    net_init();

    RunAllTests();
    return 0;
}
