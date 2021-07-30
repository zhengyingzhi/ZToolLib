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

    table_node_t tnode = table_find(tbl, "show", sizeof("show"));
    char* ch = (char*)table_node_value(tnode);
    ZuAssertTrue(zt, 0 == strcmp("s", ch));

   

    table_remove(tbl, "config", sizeof("config"));
    ZuAssertIntEquals(zt, (int)(ncmds-1), table_length(tbl));

    table_iter_t itr = table_iter_new(tbl);

    for (int i = 0; i < table_length(tbl); ++i)
    {
        tnode = table_next(itr);
        char* ch = (char*)table_node_value(tnode);
        printf("val = %s\n", ch);
    }

    table_iter_rewind(itr);
    tnode = table_next(itr);
    ch = (char*)table_node_value(tnode);
    printf("val = %s\n", ch);

    table_set_int(tnode, 1);
    ZuAssertTrue(zt, 1 == table_node_int(tnode));

    table_clear(tbl);
    ZuAssertIntEquals(zt, 0, table_length(tbl));

    /*int size = table_size(tbl) + 10000;
    table_expand(tbl, 10000);
    ZuAssertIntEquals(zt, size, table_size(tbl));*/

    table_iter_free(itr);
    table_free(&tbl);
}
