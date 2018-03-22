#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ztl_unit_test.h"

/*-------------------------------------------------------------------------*
 * CuStr
 *-------------------------------------------------------------------------*/

char* ZuStrAlloc(int size)
{
	char* newStr = (char*) malloc( sizeof(char) * (size) );
	return newStr;
}

char* ZuStrCopy(const char* old)
{
	int len = strlen(old);
	char* newStr = CuStrAlloc(len + 1);
	strcpy(newStr, old);
	return newStr;
}

/*-------------------------------------------------------------------------*
 * ZuString
 *-------------------------------------------------------------------------*/

void ZuStringInit(ZuString* str)
{
	str->length = 0;
	str->size = STRING_MAX;
	str->buffer = (char*) malloc(sizeof(char) * str->size);
	str->buffer[0] = '\0';
}

ZuString* ZuStringNew(void)
{
	ZuString* str = (ZuString*) malloc(sizeof(ZuString));
	str->length = 0;
	str->size = STRING_MAX;
	str->buffer = (char*) malloc(sizeof(char) * str->size);
	str->buffer[0] = '\0';
	return str;
}

void ZuStringResize(ZuString* str, int newSize)
{
	str->buffer = (char*) realloc(str->buffer, sizeof(char) * newSize);
	str->size = newSize;
}

void ZuStringAppend(ZuString* str, const char* text)
{
	int length;

	if (text == NULL) {
		text = "NULL";
	}

	length = strlen(text);
	if (str->length + length + 1 >= str->size)
		ZuStringResize(str, str->length + length + 1 + STRING_INC);
	str->length += length;
	strcat(str->buffer, text);
}

void ZuStringAppendChar(ZuString* str, char ch)
{
	char text[2];
	text[0] = ch;
	text[1] = '\0';
	ZuStringAppend(str, text);
}

void ZuStringAppendFormat(ZuString* str, const char* format, ...)
{
	va_list argp;
	char buf[HUGE_STRING_LEN];
	va_start(argp, format);
	vsprintf(buf, format, argp);
	va_end(argp);
	ZuStringAppend(str, buf);
}

void ZuStringInsert(ZuString* str, const char* text, int pos)
{
	int length = strlen(text);
	if (pos > str->length)
		pos = str->length;
	if (str->length + length + 1 >= str->size)
		ZuStringResize(str, str->length + length + 1 + STRING_INC);
	memmove(str->buffer + pos + length, str->buffer + pos, (str->length - pos) + 1);
	str->length += length;
	memcpy(str->buffer + pos, text, length);
}

/*-------------------------------------------------------------------------*
 * ZuTest
 *-------------------------------------------------------------------------*/

void ZuTestInit(ZuTest* t, const char* name, ZuTestFunction function)
{
	t->name = CuStrCopy(name);
	t->failed = 0;
	t->ran = 0;
	t->message = NULL;
	t->function = function;
	t->jumpBuf = NULL;
}

ZuTest* ZuTestNew(const char* name, ZuTestFunction function)
{
	ZuTest* tc = ZU_ALLOC(ZuTest);
	ZuTestInit(tc, name, function);
	return tc;
}

void ZuTestRun(ZuTest* tc)
{
#if 0 /* debugging */
    printf(" running %s\n", tc->name);
#endif
	jmp_buf buf;
	tc->jumpBuf = &buf;
	if (setjmp(buf) == 0)
	{
		tc->ran = 1;
		(tc->function)(tc);
	}
	tc->jumpBuf = 0;
}

static void ZuFailInternal(ZuTest* tc, const char* file, int line, ZuString* string)
{
	char buf[HUGE_STRING_LEN];

	sprintf(buf, "%s:%d: ", file, line);
	ZuStringInsert(string, buf, 0);

	tc->failed = 1;
	tc->message = string->buffer;
	if (tc->jumpBuf != 0) longjmp(*(tc->jumpBuf), 0);
}

void ZuFail_Line(ZuTest* tc, const char* file, int line, const char* message2, const char* message)
{
	ZuString string;

	ZuStringInit(&string);
	if (message2 != NULL) 
	{
		ZuStringAppend(&string, message2);
		ZuStringAppend(&string, ": ");
	}
	ZuStringAppend(&string, message);
	ZuFailInternal(tc, file, line, &string);
}

void ZuAssert_Line(ZuTest* tc, const char* file, int line, const char* message, int condition)
{
	if (condition) return;
	ZuFail_Line(tc, file, line, NULL, message);
}

void ZuAssertStrEquals_LineMsg(ZuTest* tc, const char* file, int line, const char* message, 
	const char* expected, const char* actual)
{
	ZuString string;
	if ((expected == NULL && actual == NULL) ||
	    (expected != NULL && actual != NULL &&
	     strcmp(expected, actual) == 0))
	{
		return;
	}

	ZuStringInit(&string);
	if (message != NULL) 
	{
		ZuStringAppend(&string, message);
		ZuStringAppend(&string, ": ");
	}
	ZuStringAppend(&string, "expected <");
	ZuStringAppend(&string, expected);
	ZuStringAppend(&string, "> but was <");
	ZuStringAppend(&string, actual);
	ZuStringAppend(&string, ">");
	ZuFailInternal(tc, file, line, &string);
}

void ZuAssertIntEquals_LineMsg(ZuTest* tc, const char* file, int line, const char* message, 
	int expected, int actual)
{
	char buf[STRING_MAX];
	if (expected == actual) return;
	sprintf(buf, "expected <%d> but was <%d>", expected, actual);
	ZuFail_Line(tc, file, line, message, buf);
}

void ZuAssertDblEquals_LineMsg(ZuTest* tc, const char* file, int line, const char* message, 
	double expected, double actual, double delta)
{
	char buf[STRING_MAX];
	if (fabs(expected - actual) <= delta) return;
	sprintf(buf, "expected <%lf> but was <%lf>", expected, actual);
	ZuFail_Line(tc, file, line, message, buf);
}

void ZuAssertPtrEquals_LineMsg(ZuTest* tc, const char* file, int line, const char* message, 
	void* expected, void* actual)
{
	char buf[STRING_MAX];
	if (expected == actual) return;
	sprintf(buf, "expected pointer <0x%p> but was <0x%p>", expected, actual);
	ZuFail_Line(tc, file, line, message, buf);
}


/*-------------------------------------------------------------------------*
 * ZuSuite
 *-------------------------------------------------------------------------*/

void ZuSuiteInit(ZuSuite* testSuite)
{
	testSuite->count = 0;
	testSuite->failCount = 0;
}

ZuSuite* ZuSuiteNew(void)
{
	ZuSuite* testSuite = CU_ALLOC(ZuSuite);
	ZuSuiteInit(testSuite);
	return testSuite;
}

void ZuSuiteAdd(ZuSuite* testSuite, ZuTest *testCase)
{
	assert(testSuite->count < MAX_TEST_CASES);
	testSuite->list[testSuite->count] = testCase;
	testSuite->count++;
}

void ZuSuiteAddSuite(ZuSuite* testSuite, ZuSuite* testSuite2)
{
	int i;
	for (i = 0 ; i < testSuite2->count ; ++i)
	{
		ZuTest* testCase = testSuite2->list[i];
		ZuSuiteAdd(testSuite, testCase);
	}
}

void ZuSuiteRun(ZuSuite* testSuite)
{
	int i;
	for (i = 0 ; i < testSuite->count ; ++i)
	{
		ZuTest* testCase = testSuite->list[i];
		ZuTestRun(testCase);
		if (testCase->failed) { testSuite->failCount += 1; }
	}
}

void ZuSuiteDetails(ZuSuite* testSuite, ZuString* details)
{
	int i;
	int failCount = 0;

        ZuStringAppendFormat(details, "%d..%d\n", 1, testSuite->count);

        for (i = 0 ; i < testSuite->count ; ++i)
        {
                ZuTest* testCase = testSuite->list[i];

                if (testCase->failed)
                {
                    failCount++;
                    ZuStringAppendFormat(details, "not ok %d - %s #%s\n",
                            i+1, testCase->name, testCase->message);
                }
                else
                {
                    ZuStringAppendFormat(details, "ok %d - %s\n",
                            i+1, testCase->name);
                }
        }
}
