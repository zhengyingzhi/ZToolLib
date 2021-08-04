#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_heap.h>


void Test_ztl_heap(ZuTest* zt)
{
    heap_t* hp;
    void* p;

    hp = heap_new(5, -1, heap_cmp_least_int);
    ZuAssertIntEquals(zt, 0, heap_length(hp));

    heap_push(hp, (void*)0x01);
    ZuAssertIntEquals(zt, 1, heap_length(hp));
    p = heap_peek(hp, 1);
    ZuAssertPtrEquals(zt, (void*)0x01, p);

    heap_push(hp, (void*)0x02);
    heap_push(hp, (void*)0x03);
    ZuAssertIntEquals(zt, 3, heap_length(hp));

    p = heap_pop(hp);
    ZuAssertPtrEquals(zt, (void*)0x01, p);
    ZuAssertIntEquals(zt, 2, heap_length(hp));

    p = heap_pop(hp);
    ZuAssertPtrEquals(zt, (void*)0x02, p);
    ZuAssertIntEquals(zt, 1, heap_length(hp));

    heap_push(hp, (void*)0x04);
    heap_push(hp, (void*)0x05);
    heap_push(hp, (void*)0x06);
    p = heap_pop(hp);
    ZuAssertPtrEquals(zt, (void*)0x03, p);
    p = heap_pop(hp);
    ZuAssertPtrEquals(zt, (void*)0x04, p);
    ZuAssertIntEquals(zt, 2, heap_length(hp));

    // push same elem
    heap_push(hp, (void*)0x05);
    heap_push(hp, (void*)0x05);
    p = heap_pop(hp);
    ZuAssertPtrEquals(zt, (void*)0x05, p);
    p = heap_pop(hp);
    ZuAssertPtrEquals(zt, (void*)0x05, p);
    p = heap_pop(hp);
    ZuAssertPtrEquals(zt, (void*)0x05, p);
    ZuAssertIntEquals(zt, 1, heap_length(hp));

    p = heap_peek(hp, 1);
    ZuAssertPtrEquals(zt, (void*)0x06, p);

    heap_free(hp);
}
