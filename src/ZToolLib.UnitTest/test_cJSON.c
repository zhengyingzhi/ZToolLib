#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/cJSON.h>

char* my_strdup(const char *s)
{
    size_t len = strlen(s) + 1;
    char* dst = (char*)malloc(len);
    if (!dst)
        return NULL;
    memcpy(dst, s, len);
    return dst;
}

void Test_cJSON(ZuTest* zt)
{
    const char* jvalue = "{\"love\":[\"LOL\",\"Go shopping\"]}";

    cJSON* json = cJSON_Parse(jvalue);

    char* json_data;
    json_data = cJSON_Print(json);
    printf("json_data:\n%s\n", json_data);
    free(json_data);

    cJSON_Delete(json);

    //////////////////////////////////////////////////////////////////////////
    const char* str = "{\"family\":[\"father\",\"mother\",\"brother\",\"sister\",\"somebody\"]}";

    cJSON* node;
    json = cJSON_Parse(str);
    ZuAssertTrue(zt, json != NULL);

    // verify whether key is string option, return 1 if has, otherwise return 0
    ZuAssertTrue(zt, cJSON_HasObjectItem(json, "family"));
    node = cJSON_GetObjectItem(json, "family");
    ZuAssertPtrNotNull(zt, node);

    node = cJSON_GetObjectItem(json, "family");
    ZuAssertTrue(zt, node->type == cJSON_Array);
    ZuAssertIntEquals(zt, 5, cJSON_GetArraySize(node));

    //非array类型的node 被当做array获取size的大小是未定义的行为 不要使用

    cJSON* tnode = NULL;
    int sz = cJSON_GetArraySize(node);
    int i;
    for (i = 0; i < sz; ++i)
    {
        tnode = cJSON_GetArrayItem(node, i);
        if (tnode->type == cJSON_String)
        {
            printf("value[%d]:%s\n", i, tnode->valuestring);
        }
        else
        {
            printf("node's type is not a string");
        }
    }

    // traverse array
    cJSON_ArrayForEach(tnode, node)
    {
        if (tnode->type == cJSON_String)
        {
            printf("int foreach: value:%s\n", tnode->valuestring);
        }
        else
        {
            printf("int foreach: node's type is not string\n");
        }
    }

    cJSON_Delete(json);
}

typedef struct  
{
    char* Name;
    char* Symbols;
}test_stg_st;

typedef struct  
{
    char AccountID[16];
    char Password[16];
    char BrokerID[8];
    char TradeAPI[8];
    char TradeAddr[64];
    char MdAddr[64];
    char MdAPI[8];
    test_stg_st Strategies[8];
}test_json_st;

static int _get_string_object_value(cJSON* tnode, const char* key, char* dst, int dstSize)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(tnode, key);
    if (obj) {
        strncpy(dst, obj->valuestring, dstSize);
        return 0;
    }
    return -1;
}

void parse_account_config(test_json_st* account, cJSON* tnode)
{
    int rv;
    rv = _get_string_object_value(tnode, "AccountID", account->AccountID, sizeof(account->AccountID) - 1);
    if (0 != rv)
    {
        return;
    }

    _get_string_object_value(tnode, "Password", account->Password, sizeof(account->Password) - 1);
    _get_string_object_value(tnode, "BrokerID", account->BrokerID, sizeof(account->BrokerID) - 1);
    _get_string_object_value(tnode, "TradeAPI", account->TradeAPI, sizeof(account->TradeAPI) - 1);
    _get_string_object_value(tnode, "TradeAddr", account->TradeAddr, sizeof(account->TradeAddr) - 1);
    _get_string_object_value(tnode, "MdAddr", account->MdAddr, sizeof(account->MdAddr) - 1);
    _get_string_object_value(tnode, "MdAPI", account->MdAPI, sizeof(account->MdAPI) - 1);

    cJSON* obj;
    obj = cJSON_GetObjectItem(tnode, "Strategy");
    if (obj)
    {
        if (obj->type != cJSON_Array) {
            printf("error config for 'Strategy' item\n");
            return;
        }

        test_stg_st* stg = NULL;
        cJSON* subitem = NULL;
        int index;
        int sz2 = cJSON_GetArraySize(obj);
        for (index = 0; index < sz2; ++index)
        {
            subitem = cJSON_GetArrayItem(obj, index);
            if (subitem->type != cJSON_Object) {
                continue;
            }
            stg = &account->Strategies[index];

            cJSON* nameObj = cJSON_GetObjectItem(subitem, "name");
            cJSON* symbolObj = cJSON_GetObjectItem(subitem, "symbol");

            if (nameObj) {
                stg->Name = my_strdup(nameObj->valuestring);
            }
            if (symbolObj) {
                stg->Symbols = my_strdup(symbolObj->valuestring);
            }
        }
    }
}

void Test_cJSON2(ZuTest* zt)
{
    FILE* fp;
    fp = fopen("test_cjson.json", "r");
    if (!fp) {
        return;
    }

    // int length = 0;
    char buffer[1024] = "";
    fread(buffer, sizeof(buffer) - 1, 1, fp);
    fclose(fp);

    cJSON* json;
    json = cJSON_Parse(buffer);
    ZuAssertTrue(zt, json != NULL);

    int sz = cJSON_GetArraySize(json);
    printf("json arr sz=%d\n", sz);

    // result
    test_json_st accounts[8] = { 0 };
    int index = 0;

    cJSON* tnode = NULL;
    cJSON_ArrayForEach(tnode, json)
    {
        if (tnode->type == cJSON_Object)
        {
            printf("parsing tnode type:%d\n", tnode->type);

            // cJSON* object = tnode->child;
            parse_account_config(&accounts[index++], tnode);
        }
    }

    cJSON_Delete(json);
}
