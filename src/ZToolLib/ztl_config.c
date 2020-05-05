#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ztl_palloc.h"
#include "ztl_config.h"

#define ZTL_CONFIG_DEFAULT_ITEMS    512

typedef struct  
{
    char*   desc;       // like "1", "true", "false", "on", "off"
    uint8_t val;        // 0 or 1
}ztl_bool_value_t;

typedef struct 
{
    char*   key;
    char*   value;
}ztl_pair_value_t;

static ztl_bool_value_t zbooltable[] = {
    { "1",      1 },
    { "0",      0 },
    { "YES",    1 },
    { "NO",     0 },
    { "Yes",    1 },
    { "No",     0 },
    { "yes",    1 },
    { "no",     0 },
    { "ON",     1 },
    { "OFF",    0 },
    { "on",     1 },
    { "off",    0 },
    { "TRUE",   1 },
    { "FALSE",  0 },
    { "True",   1 },
    { "False",  0 },
    { "true",   1 },
    { "false",  0 },
    { NULL,     0}
};

struct ztl_config_s
{
    char                filename[256];
    FILE*               fp;
    char                comment;
    char                delimiter;
    uint32_t            count;
    uint32_t            nalloc;
    ztl_pair_value_t*   items;
    ztl_pool_t*         pool;
};

/* do read & parse file content */
static int ztl_read_file_content(ztl_config_t* zconf);


ztl_config_t* ztl_config_open(const char* filename, char comment, char delimiter)
{
    ztl_config_t* zconf;

    if (comment == 0)
        comment = '#';
    if (delimiter == 0)
        delimiter = ' ';

    zconf = (ztl_config_t*)malloc(sizeof(ztl_config_t));
    memset(zconf, 0, sizeof(ztl_config_t));

    strncpy(zconf->filename, filename, sizeof(zconf->filename) - 1);
    zconf->comment   = comment;
    zconf->delimiter = delimiter;
    zconf->count     = 0;
    zconf->nalloc    = ZTL_CONFIG_DEFAULT_ITEMS;
    zconf->items     = calloc(1, zconf->nalloc * sizeof(ztl_pair_value_t));
    zconf->pool      = ztl_create_pool(4096);

    // open the file and read content
    if (ztl_read_file_content(zconf) != 0) {
        ztl_config_close(zconf);
        return NULL;
    }

    return zconf;
}

void ztl_config_close(ztl_config_t* zconf)
{
    if (!zconf) {
        return;
    }

    if (zconf->fp) {
        fclose(zconf->fp);
        zconf->fp = NULL;
    }

    if (zconf->pool) {
        ztl_destroy_pool(zconf->pool);
    }

    if (zconf->items) {
        free(zconf->items);
    }

    free(zconf);
}

bool ztl_config_set_item(ztl_config_t* zconf, const char* key, const char* val, bool overwrite)
{
    ztl_pair_value_t* lpitem;
    uint32_t keylen = (uint32_t)strlen(key);
    uint32_t vallen = (uint32_t)strlen(val);

    if (keylen == 0 || vallen == 0) {
        return false;
    }

    for (uint32_t i = 0; i < zconf->count; ++i)
    {
        if (strcmp(zconf->items[i].key, key) == 0) {
            if (overwrite) {
                char* lpcopy = (char*)ztl_pcalloc(zconf->pool, strlen(val) + 1);
                memcpy(lpcopy, val, strlen(val));
                zconf->items[i].value = lpcopy;
                return true;
            }
            return false;
        }
    }

    if (zconf->count >= zconf->nalloc) {
        zconf->nalloc *= 2;
        zconf->items = realloc(zconf->items, zconf->nalloc * sizeof(ztl_pair_value_t));
    }
    lpitem = &zconf->items[zconf->count++];

    lpitem->key = (char*)ztl_pcalloc(zconf->pool, keylen + 1);
    lpitem->value = (char*)ztl_pcalloc(zconf->pool, vallen + 1);
    memcpy(lpitem->key, key, keylen);
    memcpy(lpitem->value, val, vallen);

    return true;
}

bool ztl_config_read_str(ztl_config_t* zconf, const char* key, char** outval, int* outlen)
{
    char* lpv;
    lpv = ztl_config_have(zconf, key);
    if (lpv) {
        if (outval)
            *outval = lpv;
        if (outlen)
            *outlen = (int)strlen(lpv);
        return true;
    }
    return false;
}

bool ztl_config_read_int16(ztl_config_t* zconf, const char* key, void* outi16)
{
    char* lpv;
    lpv = ztl_config_have(zconf, key);
    if (lpv) {
        *(int16_t*)outi16 = atoi(lpv);
        return true;
    }
    return false;
}

bool ztl_config_read_int32(ztl_config_t* zconf, const char* key, void* outi32)
{
    char* lpv;
    lpv = ztl_config_have(zconf, key);
    if (lpv) {
        *(int32_t*)outi32 = atoi(lpv);
        return true;
    }
    return false;
}

bool ztl_config_read_int64(ztl_config_t* zconf, const char* key, void* outi64)
{
    char* lpv;
    lpv = ztl_config_have(zconf, key);
    if (lpv) {
        *(int64_t*)outi64 = atoll(lpv);
        return true;
    }
    return false;
}

bool ztl_config_read_double(ztl_config_t* zconf, const char* key, void* outdbl)
{
    char* lpv;
    lpv = ztl_config_have(zconf, key);
    if (lpv) {
        *(double*)outdbl = atof(lpv);
        return true;
    }
    return false;
}

bool ztl_config_read_bool(ztl_config_t* zconf, const char* key, bool* outbool)
{
    char* lpv;
    lpv = ztl_config_have(zconf, key);
    if (lpv) {
        *outbool = ztl_boolvalue_lookup(lpv);
        return true;
    }
    return false;
}


bool ztl_boolvalue_lookup(const char* desc)
{
    if (!desc || !desc[0]) {
        return false;
    }

    for (int i = 0; zbooltable[i].desc; ++i)
    {
        if (strcmp(zbooltable[i].desc, desc) == 0) {
            return zbooltable[i].val;
        }
    }

    return false;
}

char* ztl_config_have(ztl_config_t* zconf, const char* key)
{
    ztl_pair_value_t* lpPair;
    for (uint32_t i = 0; i < zconf->count; ++i)
    {
        lpPair = &zconf->items[i];
        if (strcmp(lpPair->key, key) == 0) {
            return lpPair->value;
        }
    }

    return NULL;
}


static int ztl_read_file_content(ztl_config_t* zconf)
{
    char        buffer[1000];
    FILE*       fp;
    char        *lpkey, *lpval;

    fp = fopen(zconf->filename, "r");
    if (fp == NULL) {
        return -1;
    }

    zconf->fp = fp;

    // parse the file content
    while (!feof(fp))
    {
        memset(buffer, 0, sizeof(buffer));
        fgets(buffer, sizeof(buffer) - 1, fp);

        lefttrim(buffer);
        if (buffer[0] == zconf->comment) {
            continue;
        }

        lpval = strchr(buffer, zconf->delimiter);
        if (lpval == NULL) {
            continue;
        }
        *lpval++ = '\0';

        // key
        lpkey = buffer;
        righttrim(lpkey);

        // value
        lefttrim(lpval);
        righttrim(lpval);

        ztl_config_set_item(zconf, lpkey, lpval, true);
    }

    return 0;
}
