#!/bin/bash

# Auto generate single AllTests file for ZtlTest.
# Searches through all *.c files in the current directory.
# Prints to stdout.

FILES=*.c

#if test $# -eq 0 ; then FILES=*.c ; else FILES=$* ; fi

echo '

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <ZToolLib/ztl_unit_test.h>
#include "ZToolUnitTest.h"

'

cat $FILES | grep '^void Test' | 
    sed -e 's/(.*$//' \
        -e 's/$/(ZuTest*);/' \
        -e 's/^/extern /'

echo \
'

void RunAllTests(void) 
{
    ZuString *output = ZuStringNew();
    ZuSuite* suite = ZuSuiteNew();

'
cat $FILES | grep '^void Test' | 
    sed -e 's/^void //' \
        -e 's/(.*$//' \
        -e 's/^/    SUITE_ADD_TEST(suite, /' \
        -e 's/$/);/'

echo \
'
    ZuSuiteRun(suite);
    ZuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
}

int main(int argc, char* argv[])
{
    net_init();
	
    RunAllTests();
	
    getchar();
    return 0;
}
'
