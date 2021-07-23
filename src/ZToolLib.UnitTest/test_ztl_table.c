#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_table.h>
#include <ZToolLib/ztl_hash.h>


static int str_cmp(const void *x, const void *y)
{
    return strcmp((char *)x, (char *)y);
}

static uint64_t _hashpjw(const void* key, int keysz)
{
    (void)keysz;
    return (uint64_t)ztl_hashpjw(key);
}

void Test_ztl_table(ZuTest* zt)
{
    size_t i, ncmds;
    void* p;
    table_t tbl;
    tbl = table_new(str_cmp, _hashpjw, NULL, NULL);
    ZuAssertIntEquals(zt, 0, table_length(tbl));

    const char* cmds[] = { "help", "config", "show", "module", "quit" };
    const char* cmds_val[] = { "h", "c", "s", "m", "q" };
    ncmds = sizeof(cmds) / sizeof(cmds[0]);
    for (i = 0; i < ncmds; ++i)
    {
        table_insert(tbl, cmds[i], (int)strlen(cmds[i]), (void*)cmds_val[i]);
    }
    ZuAssertIntEquals(zt, (int)ncmds, table_length(tbl));

    p = table_get_value(tbl, "config", -1);
    ZuAssertTrue(zt, 0 == strcmp("c", (char*)p));

    table_free(&tbl);
}
