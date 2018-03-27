#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_buffer.h>

void Test_ztl_buffer(ZuTest* zt)
{
    char ldata[256] = "";
    uint32_t size;
    ztl_buffer_t zbuf;
    ztl_buffer_init(&zbuf);

    ZuAssertTrue(zt, ztl_buffer_empty(&zbuf));

    strcpy(ldata, "hello");
    size = (uint32_t)strlen(ldata);
    ztl_buffer_append(&zbuf, ldata, size);

    ZuAssertIntEquals(zt, size, ztl_buffer_size(&zbuf));
    ZuAssertTrue(zt, 0 == strncmp(ldata, ztl_buffer_data(&zbuf), size));

    ztl_buffer_insert(&zbuf, 3, "123", 3);
    ZuAssertIntEquals(zt, size + 3, ztl_buffer_size(&zbuf));

    strcpy(ldata, "hel123lo");
    size = ztl_buffer_size(&zbuf);
    ZuAssertTrue(zt, 0 == strncmp(ldata, ztl_buffer_data(&zbuf), size));

    ztl_buffer_clear(&zbuf);

    // add again
    strcpy(ldata, "world");
    size = (uint32_t)strlen(ldata);
    ztl_buffer_append(&zbuf, ldata, size);
    ZuAssertTrue(zt, 0 == strncmp(ldata, ztl_buffer_data(&zbuf), size));

    ztl_buffer_release(&zbuf);
}


static void* _ztl_buffer_alloc(void* ctx, void* oldaddr, uint32_t size)
{
    (void)ctx;
    if (oldaddr)
        return realloc(oldaddr, size);
    else
        return malloc(size);
}

static void  _ztl_buffer_dealloc(void* ctx, void* addr)
{
    (void)ctx;
    if (addr)
        free(addr);
}

void Test_ztl_buffer2(ZuTest* zt)
{
    char ldata[256] = "";
    ztl_buffer_t zbuf;
    ztl_buffer_init(&zbuf);

    ZuAssertTrue(zt, ztl_buffer_empty(&zbuf));

    ztl_buffer_set_alloc_func(&zbuf, _ztl_buffer_alloc, _ztl_buffer_dealloc, NULL);

    ztl_buffer_append(&zbuf, "12", 2);
    ztl_buffer_append(&zbuf, "6789", 4);
    ztl_buffer_insert(&zbuf, 2, "345", 3);

    ZuAssertTrue(zt, 0 == strncmp("123456789", ztl_buffer_data(&zbuf), 9));
    ZuAssertIntEquals(zt, 9, ztl_buffer_size(&zbuf));

    ztl_buffer_clear(&zbuf);

    ztl_buffer_release(&zbuf);
}
