#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_errors.h>
#include <ZToolLib/ztl_msg_buffer.h>

void Test_ztl_msg_buffer(ZuTest* zt)
{
    int rv;
    uint32_t body_size;
    ztl_msg_buffer_t* zmb;

    body_size = 16;
    zmb = ztl_mb_alloc(body_size);

    ztl_mb_addref(zmb);
    rv = ztl_mb_append(zmb, "hello", 5);
    ZuAssertIntEquals(zt, 0, rv);
    ZuAssertTrue(zt, strncmp("hello", ztl_mb_data(zmb), 5) == 0);
    ztl_mb_decref_release(zmb);

    rv = ztl_mb_append(zmb, "world", 5);
    ZuAssertIntEquals(zt, 0, rv);
    ZuAssertTrue(zt, strncmp("helloworld", ztl_mb_data(zmb), 10) == 0);
    rv = ztl_mb_append(zmb, "123456789", 9);
    ZuAssertIntEquals(zt, ZTL_ERR_OutOfMem, rv);

    rv = ztl_mb_insert(zmb, 5, "333", 3);
    ZuAssertIntEquals(zt, 0, rv);
    ZuAssertTrue(zt, strncmp("hello333world", ztl_mb_data(zmb), 13) == 0);

    ztl_mb_decref_release(zmb);
}
