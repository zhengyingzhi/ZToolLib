#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_bufcat.h>



void Test_ztl_bufcat(ZuTest* zt)
{
    bufcat_t bc1;
    char buf[128] = "";
    bufcat_init(&bc1, buf, sizeof(buf), NULL);
    bufcat_str(&bc1, "hello");
    bufcat_str(&bc1, "world");
    bc1.buf[bc1.len] = 0;
    ZuAssertStrEquals(zt, "helloworld", bc1.buf);
    bufcat_free(&bc1);

    bufcat_t bc2;
    bufcat_init(&bc2, NULL, 8, "|");
    bufcat_str(&bc2, "req");
    bufcat_int(&bc2, 10);
    bufcat_double(&bc2, 12.2, 1);
    bufcat_str(&bc2, "\r\n");
    bc2.buf[bc2.len] = 0;
    ZuAssertStrEquals(zt, "req|10|12.2|\r\n", bc2.buf);
    bufcat_free(&bc2);

    bufcat_t bc3;
    bufcat_init(&bc3, NULL, 8, "\r\n");
    bufcat_str(&bc3, "req");
    bufcat_int64(&bc3, 100);
    bufcat_double(&bc3, 12.2348, 3);
    bc3.buf[bc3.len] = 0;
    ZuAssertStrEquals(zt, "req\r\n100\r\n12.235", bc3.buf);
    bufcat_free(&bc3);
}
