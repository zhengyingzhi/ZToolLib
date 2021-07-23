#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ZToolLib/ztl_unit_test.h>

#include <ZToolLib/ztl_dstr.h>

void Test_ztl_dstr(ZuTest* zt)
{
    dstr s1, s2, s3;
    size_t cap1, cap2, cap3;

    s1 = dstr_new("hello");
    ZuAssertIntEquals(zt, 5, (int)dstr_length(s1));

    s1 = dstr_cat(s1, "world");
    ZuAssertIntEquals(zt, 10, (int)dstr_length(s1));
    ZuAssertTrue(zt, 0 == strcmp(s1, "helloworld"));

    cap1 = dstr_capicity(s1);
    dstr_clear(s1);
    cap2 = dstr_capicity(s1);
    ZuAssertIntEquals(zt, 0, (int)dstr_length(s1));
    ZuAssertIntEquals(zt, (int)cap1, (int)cap2);
    dstr_free(s1);

    s2 = dstr_new_len("hello", 5);
    cap1 = dstr_capicity(s2);
    s2 = dstr_reserve(s2, 16);
    cap2 = dstr_capicity(s2);
    ZuAssertTrue(zt, cap2 > cap1);

    s2 = dstr_cat(s2, "world");
    cap3 = dstr_capicity(s2);
    ZuAssertIntEquals(zt, 10, (int)dstr_length(s2));
    ZuAssertTrue(zt, 0 == strcmp(s2, "helloworld"));
    ZuAssertIntEquals(zt, (int)cap2, (int)cap3);
    dstr_free(s2);

    s3 = dstr_new_len(NULL, 20);
    ZuAssertIntEquals(zt, 0, (int)dstr_length(s3));
    s3 = dstr_cat_printf(s3, "%s%s", "hello", "world");
    ZuAssertIntEquals(zt, 10, (int)dstr_length(s3));
    ZuAssertTrue(zt, 0 == strcmp(s3, "helloworld"));
    ZuAssertIntEquals(zt, 10, (int)dstr_avail(s3));

    dstr s = dstr_range(s3, 5, 10);
    ZuAssertIntEquals(zt, 5, (int)dstr_length(s3));
    ZuAssertTrue(zt, 0 == strcmp(s, "world"));

    s3 = dstr_cat(s3, "!!!");
    ZuAssertTrue(zt, 0 == strcmp(s, "world!!!"));
    ZuAssertIntEquals(zt, 8, (int)dstr_length(s3));
}
