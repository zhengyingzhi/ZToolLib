#ifndef _ZTL_UNIT_TEST_H_
#define _ZTL_UNIT_TEST_H_

#include <setjmp.h>
#include <stdarg.h>

/* ZuString */

char* ZuStrAlloc(int size);
char* ZuStrCopy(const char* old);

#define ZU_ALLOC(TYPE)      ((TYPE*) malloc(sizeof(TYPE)))

#define HUGE_STRING_LEN     8192
#define STRING_MAX          256
#define STRING_INC          256

typedef struct
{
    int length;
    int size;
    char* buffer;
} ZuString;

void ZuStringInit(ZuString* str);
ZuString* ZuStringNew(void);
void ZuStringRead(ZuString* str, const char* path);
void ZuStringAppend(ZuString* str, const char* text);
void ZuStringAppendChar(ZuString* str, char ch);
void ZuStringAppendFormat(ZuString* str, const char* format, ...);
void ZuStringInsert(ZuString* str, const char* text, int pos);
void ZuStringResize(ZuString* str, int newSize);

/* ZuTest */

typedef struct tagZuTest ZuTest;

typedef void (*ZuTestFunction)(ZuTest *);

struct tagZuTest
{
    const char*     name;
    ZuTestFunction  function;
    int             failed;
    int             ran;
    const char*     message;
    jmp_buf*        jumpBuf;
};

void ZuTestInit(ZuTest* t, const char* name, ZuTestFunction function);
ZuTest* ZuTestNew(const char* name, ZuTestFunction function);
void ZuTestRun(ZuTest* tc);

/* Internal versions of assert functions -- use the public versions */
void ZuFail_Line(ZuTest* tc, const char* file, int line, const char* message2, const char* message);
void ZuAssert_Line(ZuTest* tc, const char* file, int line, const char* message, int condition);
void ZuAssertStrEquals_LineMsg(ZuTest* tc, 
	const char* file, int line, const char* message, 
	const char* expected, const char* actual);
void ZuAssertIntEquals_LineMsg(ZuTest* tc, 
	const char* file, int line, const char* message, 
	int expected, int actual);
void ZuAssertDblEquals_LineMsg(ZuTest* tc, 
	const char* file, int line, const char* message, 
	double expected, double actual, double delta);
void ZuAssertPtrEquals_LineMsg(ZuTest* tc, 
	const char* file, int line, const char* message, 
	void* expected, void* actual);

/* public assert functions */

#define ZuFail(tc, ms)                        ZuFail_Line(  (tc), __FILE__, __LINE__, NULL, (ms))
#define ZuAssert(tc, ms, cond)                ZuAssert_Line((tc), __FILE__, __LINE__, (ms), (cond))
#define ZuAssertTrue(tc, cond)                ZuAssert_Line((tc), __FILE__, __LINE__, "assert failed", (cond))

#define ZuAssertStrEquals(tc,ex,ac)           ZuAssertStrEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac))
#define ZuAssertStrEquals_Msg(tc,ms,ex,ac)    ZuAssertStrEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac))
#define ZuAssertIntEquals(tc,ex,ac)           ZuAssertIntEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac))
#define ZuAssertIntEquals_Msg(tc,ms,ex,ac)    ZuAssertIntEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac))
#define ZuAssertDblEquals(tc,ex,ac,dl)        ZuAssertDblEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac),(dl))
#define ZuAssertDblEquals_Msg(tc,ms,ex,ac,dl) ZuAssertDblEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac),(dl))
#define ZuAssertPtrEquals(tc,ex,ac)           ZuAssertPtrEquals_LineMsg((tc),__FILE__,__LINE__,NULL,(ex),(ac))
#define ZuAssertPtrEquals_Msg(tc,ms,ex,ac)    ZuAssertPtrEquals_LineMsg((tc),__FILE__,__LINE__,(ms),(ex),(ac))

#define ZuAssertPtrNotNull(tc,p)              ZuAssert_Line((tc),__FILE__,__LINE__,"null pointer unexpected",(p != NULL))
#define ZuAssertPtrNotNullMsg(tc,msg,p)       ZuAssert_Line((tc),__FILE__,__LINE__,(msg),(p != NULL))

/* ZuSuite */

#define ZTL_MAX_TEST_CASES	1024

#define SUITE_ADD_TEST(SUITE,TEST)	ZuSuiteAdd(SUITE, ZuTestNew(#TEST, TEST))

typedef struct
{
    int count;
    int failCount;
    ZuTest* list[ZTL_MAX_TEST_CASES];

} ZuSuite;


void ZuSuiteInit(ZuSuite* testSuite);
ZuSuite* ZuSuiteNew(void);
void ZuSuiteAdd(ZuSuite* testSuite, ZuTest *testCase);
void ZuSuiteAddSuite(ZuSuite* testSuite, ZuSuite* testSuite2);
void ZuSuiteRun(ZuSuite* testSuite);
void ZuSuiteSummary(ZuSuite* testSuite, ZuString* summary);
void ZuSuiteDetails(ZuSuite* testSuite, ZuString* details);

#endif /* _ZTL_UNIT_TEST_H_ */
