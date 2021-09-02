#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ztl_bufcat.h"

#ifdef _MSC_VER
#define I64_FMT     "%lld"
#else
#define I64_FMT     "%ld"
#endif

#define ERR_OOM     -2


static int bufcat_expand_if_needed(bufcat_t* bc, size_t len)
{
    if (bc->len + bc->sep_len + len >= bc->capicity)
    {
        if (!bc->alloced) {
            return -1;
        }

        bc->capicity = bc->capicity * 2 + len;
        bc->buf = realloc(bc->buf, bc->capicity);
        if (!bc->buf) {
            return ERR_OOM;
        }
    }
    return 0;
}


void bufcat_init(bufcat_t* bc, char buf[], int capicity, const char* sep)
{
    bc->buf         = buf;
    bc->len         = 0;
    bc->capicity    = capicity;
    bc->alloced     = buf ? 0 : 1;
    bc->sep         = (sep && *sep) ? sep : NULL;
    bc->sep_len     = bc->sep ? (int)strlen(bc->sep) : 0;

    if (!buf)
    {
        bc->buf = (char*)malloc(capicity);
    }
}

void bufcat_free(bufcat_t* bc)
{
    if (bc->alloced)
        free(bc->buf);
}

int bufcat_append_sep(bufcat_t* bc)
{
    if (bc->len == 0)
        return -1;

    switch (bc->sep_len)
    {
    case 1:
        bc->buf[bc->len++] = *bc->sep;
        break;
    case 2:
    {
        bc->buf[bc->len++] = *bc->sep;
        bc->buf[bc->len++] = *(bc->sep + 1);
        break;
    }
    default:
    {
        memcpy(bc->buf, bc->sep, bc->sep_len);
        bc->len += bc->sep_len;
        break;
    }
    }
    return 0;
}

int bufcat_str_len(bufcat_t* bc, const char* str, int len)
{
    int rv;
    if ((rv = bufcat_expand_if_needed(bc, len)) == ERR_OOM) {
        return rv;
    }

    if (bc->sep_len)
        bufcat_append_sep(bc);
    memcpy(bc->buf + bc->len, str, len);
    bc->len += len;
    return 0;
}

int bufcat_str(bufcat_t* bc, const char* str)
{
    return bufcat_str_len(bc, str, (int)strlen(str));
}

int bufcat_int(bufcat_t* bc, int val)
{
    int rv;
    if ((rv = bufcat_expand_if_needed(bc, 12)) == ERR_OOM) {
        return rv;
    }

    if (bc->sep_len)
        bufcat_append_sep(bc);
    bc->len += sprintf(bc->buf + bc->len, "%d", val);
    return 0;
}

int bufcat_int64(bufcat_t* bc, int64_t val)
{
    int rv;
    if ((rv = bufcat_expand_if_needed(bc, 64)) == ERR_OOM) {
        return rv;
    }

    if (bc->sep_len)
        bufcat_append_sep(bc);
    bc->len += sprintf(bc->buf + bc->len, I64_FMT, val);
    return 0;
}

int bufcat_double(bufcat_t* bc, double val, int precision)
{
    int rv;
    if ((rv = bufcat_expand_if_needed(bc, 64)) == ERR_OOM) {
        return rv;
    }

    if (bc->sep_len)
        bufcat_append_sep(bc);

    const char* fmt = "%lf";
    switch (precision)
    {
    case 1:
        fmt = "%.1lf";
        break;
    case 2:
        fmt = "%.2lf";
        break;
    case 3:
        fmt = "%.3lf";
        break;
    case 4:
        fmt = "%.4lf";
        break;
    case 6:
        fmt = "%.6lf";
        break;
    case 5:
        fmt = "%.5lf";
        break;
    default:
        break;
    }
    bc->len += sprintf(bc->buf + bc->len, fmt, val);
    return 0;
}
