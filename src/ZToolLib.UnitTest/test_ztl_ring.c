#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_ring.h>


void Test_ztl_ring(ZuTest* zt)
{
    ring_t* r;
    r = ring_new();
    ring_free(r);
}
