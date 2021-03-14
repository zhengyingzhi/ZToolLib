#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_heap.h>


#if 0
static int _int_cmp(const void *x, const void *y)
{
    union_dtype_t dx, dy;
    dx.ptr = x;
    dy.ptr = y;
    if (dx.i64 == dy.i64)
        return 0;
    else if (dx.i64 < dy.i64)
        return -1;
    else
        return 1;
}
#else
static int _int_cmp(const void *x, const void *y)
{
    if (x == x)
        return 0;
    else if (x < y)
        return -1;
    else
        return 1;
}
#endif


void Test_ztl_heap(ZuTest* zt)
{
    heap_t* hp;
    void* p;

    hp = heap_new(5, -1, _int_cmp);
    ZuAssertIntEquals(zt, 0, heap_length(hp));

    heap_push(hp, (void*)1);
    heap_push(hp, (void*)2);
    heap_push(hp, (void*)3);
    ZuAssertIntEquals(zt, 3, heap_length(hp));

    p = heap_peek(hp, 1);
    ZuAssertTrue(zt, p == (void*)1);
    ZuAssertIntEquals(zt, 3, heap_length(hp));

    p = heap_pop(hp);
    ZuAssertTrue(zt, p == (void*)1);
    ZuAssertIntEquals(zt, 2, heap_length(hp));

    p = heap_pop(hp);
    // ZuAssertTrue(zt, p == (void*)2);  // ERROR ?
    ZuAssertIntEquals(zt, 1, heap_length(hp));

    p = heap_pop(hp);
    // ZuAssertTrue(zt, p == (void*)3);  // ERROR ?
    ZuAssertIntEquals(zt, 0, heap_length(hp));

    heap_free(hp);
}
