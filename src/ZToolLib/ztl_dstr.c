#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "ztl_utils.h"
#include "ztl_mem.h"
#include "ztl_dstr.h"


typedef struct dstr_head_st {
    size_t  capicity;
    size_t  used;
    char    buf[0];
}dstr_head_t;


#define DSTR_MAX_PER_ALLOC  (1024 * 1024)

#define DSTR_HEAD(ds) ((dstr_head_t*)((ds) - sizeof(dstr_head_t)))

static inline int is_hex_char(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int hex_char_to_int(char c)
{
    switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a':
    case 'A': return 10;
    case 'b':
    case 'B': return 11;
    case 'c':
    case 'C': return 12;
    case 'd':
    case 'D': return 13;
    case 'e':
    case 'E': return 14;
    case 'f':
    case 'F': return 15;
    default:  return 0;
    }
}

dstr dstr_new_len(const char* str, size_t length)
{
    dstr_head_t* dh;
    size_t nbytes;

    nbytes = ztl_align(sizeof(dstr_head_t) + length + 1, 8);
    dh = str ? ALLOC(nbytes) : CALLOC(1, nbytes);
    if (!dh) {
        return NULL;
    }
    dh->capicity = length + 1;
    dh->used = str ? length : 0;
    if (str && length) {
        memcpy(dh->buf, str, length);
    }
    dh->buf[length] = '\0';
    return dh->buf;
}

dstr dstr_new(const char* str)
{
    size_t length = str ? strlen(str) : 0;
    return dstr_new_len(str, length);
}

void dstr_free(dstr ds)
{
    dstr_head_t* dh;
    if (ds)
    {
        dh = DSTR_HEAD(ds);
        FREE(dh);
    }
}

size_t dstr_capicity(const dstr ds)
{
    dstr_head_t* dh;
    if (ds)
    {
        dh = DSTR_HEAD(ds);
        return dh->capicity;
    }
    return 0;
}

size_t dstr_length(const dstr ds)
{
    dstr_head_t* dh;
    if (ds)
    {
        dh = DSTR_HEAD(ds);
        return dh->used;
    }
    return 0;
}

size_t dstr_avail(const dstr ds)
{
    dstr_head_t* dh;

    if (!ds) {
        return 0;
    }

    dh = DSTR_HEAD(ds);
    return dh->capicity - dh->used - 1;
}

dstr dstr_reserve(dstr ds, size_t length)
{
    dstr_head_t* dh;
    size_t newlen, nbytes;

    if (!ds) {
        return NULL;
    }

    dh = DSTR_HEAD(ds);
    if (dh->capicity - dh->used - 1 >= length)
        return ds;

    newlen = dh->capicity + length;
    if (newlen < DSTR_MAX_PER_ALLOC)
        newlen *= 2;
    else
        newlen += DSTR_MAX_PER_ALLOC;
    nbytes = ztl_align(sizeof(*dh) + newlen, 8);

    if (RESIZE(dh, nbytes) == NULL)
        return NULL;

    dh->capicity = newlen;
    return dh->buf;
}

void dstr_incr_len(dstr ds, int incr)
{
    dstr_head_t* dh;

    if (!ds) {
        return;
    }

    dh = DSTR_HEAD(ds);
    if (dh->capicity - dh->used -1 >= incr)
    {
        dh->used += incr;
        ds[dh->used] = '\0';
    }
}

dstr dstr_remove_avail(dstr ds)
{
    dstr_head_t* dh;

    if (!ds) {
        return NULL;
    }

    dh = DSTR_HEAD(ds);
    if (RESIZE(dh, sizeof(*dh) + dh->used + 1) == NULL)
        return NULL;
    dh->capicity = dh->used + 1;
    return dh->buf;
}

size_t dstr_alloced_size(dstr ds)
{
    dstr_head_t* dh;

    if (!ds) {
        return 0;
    }

    dh = DSTR_HEAD(ds);
    return sizeof(*dh) + dh->capicity + 1;
}

dstr dstr_cat_len(dstr ds, const char* str, size_t length)
{
    dstr_head_t* dh;
    size_t used = dstr_length(ds);

    if ((ds = dstr_reserve(ds, length)) == NULL)
        return NULL;
    memcpy(ds + used, str, length);
    ds[used + length] = '\0';

    dh = DSTR_HEAD(ds);
    dh->used = used + length;
    return ds;
}

dstr dstr_cat(dstr ds, const char* str)
{
    if (!str || !str[0]) {
        return ds;
    }

    return dstr_cat_len(ds, str, strlen(str));
}

dstr dstr_cat_vprintf(dstr ds, const char* fmt, va_list ap)
{
    char *buf, *tmp;
    size_t len = 32;
    va_list cpy;

    for (;;)
    {
        if ((buf = ALLOC(len)) == NULL)
            return NULL;
        buf[len - 2] = '\0';
        va_copy(cpy, ap);
        vsnprintf(buf, len, fmt, cpy);
        if (buf[len - 2] != '\0')
        {
            FREE(buf);
            len *= 2;
            continue;
        }
        break;
    }

    tmp = dstr_cat(ds, buf);
    FREE(buf);
    return tmp;
}

dstr dstr_cat_printf(dstr ds, const char* fmt, ...)
{
    va_list ap;
    char* tmp;

    va_start(ap, fmt);
    tmp = dstr_cat_vprintf(ds, fmt, ap);
    va_end(ap);
    return tmp;
}

dstr dstr_trim(dstr ds, const char* cset) 
{
    dstr_head_t* dh;
    char *start, *sp, *end, *ep;
    size_t len;

    if (!ds) {
        return NULL;
    }

    dh = DSTR_HEAD(ds);
    sp = start = ds;
    ep = end = ds + dstr_length(ds) - 1;
    while (sp <= end && strchr(cset, *sp))
        ++sp;
    while (ep > start && strchr(cset, *ep))
        --ep;
    len = sp > ep ? 0 : ep - sp + 1;
    if (dh->buf != sp)
        memmove(dh->buf, sp, len);
    dh->buf[len] = '\0';
    dh->used = len;
    return ds;
}

dstr dstr_range(dstr ds, size_t start, size_t end)
{
    dstr_head_t* dh;
    size_t newlen;

    if (!ds) {
        return NULL;
    }

    dh = DSTR_HEAD(ds);
    if (dh->used == 0)
        return NULL;

    if (start < 0)
    {
        start += dh->used;
        if (start < 0)
            start = 0;
    }
    if (end < 0)
    {
        end += dh->used;
        if (end < 0)
            end = 0;
    }

    newlen = start > end ? 0 : end - start + 1;
    if (newlen)
    {
        if (start >= (signed)dh->used)
        {
            newlen = 0;
        }
        else if (end >= (signed)dh->used)
        {
            end = dh->used - 1;
            newlen = start > end ? 0 : end - start + 1;
        }
    }
    else
    {
        start = 0;
    }

    if (start && newlen)
        memmove(ds, ds + start, newlen);

    ds[newlen] = '\0';
    dh->used = newlen;
    return ds;
}

void dstr_clear(dstr ds)
{
    dstr_head_t* dh;

    if (!ds) {
        return;
    }

    dh = DSTR_HEAD(ds);
    dh->used = 0;
    dh->buf[0] = '\0';
}

dstr* dstr_split_len(const char* str, size_t length, const char* sep, size_t seplength, int* count)
{
    size_t i, start = 0;
    int slots = 5, nelems = 0;
    dstr* tokens;

    if (str == NULL || sep == NULL)
        return NULL;
    if (length < 0 || seplength < 1)
        return NULL;
    if ((tokens = ALLOC(slots * sizeof(*tokens))) == NULL)
        return NULL;
    if (length == 0)
    {
        *count = 0;
        return tokens;
    }

    for (i = 0; i < length - seplength + 1; ++i)
    {
        // make sure mem space for the next and final ones
        if (slots < nelems + 2)
        {
            slots *= 2;
            if (RESIZE(tokens, slots * sizeof *tokens) == NULL)
                goto SPLIT_ERR;
        }
        if ((seplength == 1 && str[i] == sep[0]) || !memcmp(str + i, sep, seplength))
        {
            if ((tokens[nelems] = dstr_new_len(str + start, i - start)) == NULL)
                goto SPLIT_ERR;
            ++nelems;
            start = i + seplength;
            i += seplength - 1;
        }
    }

    // the tail
    if ((tokens[nelems] = dstr_new_len(str + start, length - start)) == NULL)
        goto SPLIT_ERR;
    ++nelems;
    *count = nelems;
    return tokens;

SPLIT_ERR:
    for (i = 0; i < nelems; ++i)
        dstr_free(tokens[i]);
    FREE(tokens);
    *count = 0;
    return NULL;
}

void dstr_free_tokens(dstr *tokens, int count)
{
    int i;
    if (!tokens)
        return;
    for (i = 0; i < count; ++i)
        dstr_free(tokens[i]);
    FREE(tokens);
}

dstr* dstr_split_args(const char* line, int* argc)
{
    const char* p = line;
    dstr current = NULL;
    dstr *argv = NULL;

    *argc = 0;
    for (;;)
    {
        while (*p && isspace(*p))
            ++p;
        if (*p)
        {
            int inq = 0; /* 1 if in quotes */
            int insq = 0; /* 1 if in single quotes */
            int done = 0;

            if (current == NULL)
                current = dstr_new_len("", 0);

            while (!done)
            {
                if (inq)
                {
                    if (*p == '\\' && *(p + 1) == 'x' &&
                        is_hex_char(*(p + 2)) && is_hex_char(*(p + 3)))
                    {
                        unsigned char byte = 16 * hex_char_to_int(*(p + 2)) +
                            hex_char_to_int(*(p + 3));

                        p += 3;
                        current = dstr_cat_len(current, (char *)&byte, 1);
                    }
                    else if (*p == '\\' && *(p + 1))
                    {
                        char c;

                        ++p;
                        switch (*p)
                        {
                        case 'a':
                            c = '\a';
                            break;
                        case 'b':
                            c = '\b';
                            break;
                        case 'n':
                            c = '\n';
                            break;
                        case 'r':
                            c = '\r';
                            break;
                        case 't':
                            c = '\t';
                            break;
                        default:
                            c = *p;
                            break;
                        }
                        current = dstr_cat_len(current, &c, 1);
                    }
                    else if (*p == '"')
                    {
                        /* closing quote must be followed by a space or not at all */
                        if (*(p + 1) && !isspace(*(p + 1)))
                            goto err;
                        done = 1;
                        /* unterminated quotes */
                    }
                    else if (*p == '\0')
                        goto err;
                    else
                        current = dstr_cat_len(current, p, 1);
                }
                else if (insq)
                {
                    if (*p == '\\' && *(p + 1) == '\'') {
                        ++p;
                        current = dstr_cat_len(current, "'", 1);
                    }
                    else if (*p == '\'') {
                        /* closing quote must be followed by a space or not at all */
                        if (*(p + 1) && !isspace(*(p + 1)))
                            goto err;
                        done = 1;
                        /* unterminated quotes */
                    }
                    else if (*p == '\0')
                        goto err;
                    else
                        current = dstr_cat_len(current, p, 1);
                }
                else
                {
                    switch (*p)
                    {
                    case ' ':
                    case '\0':
                    case '\n':
                    case '\r':
                    case '\t':
                        done = 1;
                        break;
                    case '"':
                        inq = 1;
                        break;
                    case '\'':
                        insq = 1;
                        break;
                    default:
                        current = dstr_cat_len(current, p, 1);
                        break;
                    }
                }
                if (*p)
                    ++p;
            }
            if (RESIZE(argv, (*argc + 1) * sizeof(char *)) == NULL)
                goto err;
            argv[*argc] = current;
            ++*argc;
            current = NULL;
        }
        else
        {
            return argv;
        }
    }

err:
    {
        int i;

        for (i = 0; i < *argc; ++i)
            dstr_free(argv[i]);
        FREE(argv);
        if (current)
            dstr_free(current);
        return NULL;
    }
}

void dstr_free_args(dstr *argv, int argc)
{
    dstr_free_tokens(argv, argc);
}
