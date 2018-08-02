#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_map.h>

void Test_ztl_map(ZuTest* zt)
{
    ztl_map_t* lmap;
    lmap = ztl_map_create(4);

    ZuAssertTrue(zt, ztl_map_empty(lmap));

    int arr[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); ++i)
    {
        ztl_map_add(lmap, arr[i], &arr[i]);
        ZuAssertTrue(zt, arr[i] == ztl_map_size(lmap));

        int* pi = ztl_map_find(lmap, arr[i]);
        ZuAssertTrue(zt, arr[i] == *pi);
    }

    ZuAssertTrue(zt, 10 == ztl_map_size(lmap));

    // find elem
    int* pi;
    pi = ztl_map_find(lmap, 0);
    ZuAssertTrue(zt, NULL == pi);

    pi = ztl_map_find(lmap, 1);
    ZuAssertTrue(zt, 1 == *pi);

    pi = ztl_map_find(lmap, 10);
    ZuAssertTrue(zt, 10 == *pi);

    // delete elem
    pi = ztl_map_del(lmap, 2);
    ZuAssertTrue(zt, 2 == *pi);

    pi = ztl_map_del(lmap, 2);
    ZuAssertTrue(zt, NULL == pi);
    ZuAssertTrue(zt, 9 == ztl_map_size(lmap));

    pi = ztl_map_del(lmap, 5);
    ZuAssertTrue(zt, 5 == *pi);

    ztl_map_release(lmap);
}

void Test_ztl_map_ex(ZuTest* zt)
{
    ztl_map_t* lmap;
    lmap = ztl_map_create(0);

    ZuAssertTrue(zt, ztl_map_empty(lmap));

    int arr[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); ++i)
    {
        ztl_rbtree_node_t* node = (ztl_rbtree_node_t*)malloc(sizeof(ztl_rbtree_node_t));
        node->udata = (void*)arr[i];

        ztl_map_add_ex(lmap, arr[i], node);
        ZuAssertTrue(zt, arr[i] == ztl_map_size(lmap));

        ztl_rbtree_node_t* retnode;
        retnode = ztl_map_find_ex(lmap, arr[i]);
        ZuAssertTrue(zt, node == retnode);
        ZuAssertTrue(zt, arr[i] == *(int*)retnode);
    }

    ZuAssertTrue(zt, 10 == ztl_map_size(lmap));

    // find elem
    ztl_rbtree_node_t* retnode;
    int iv;

    retnode = ztl_map_find_ex(lmap, 0);
    ZuAssertTrue(zt, NULL == retnode);

    retnode = ztl_map_find_ex(lmap, 1);
    iv = (int)(uint64_t)retnode->udata;
    ZuAssertTrue(zt, 1 == iv);

    retnode = ztl_map_find_ex(lmap, 10);
    iv = (int)(uint64_t)retnode->udata;
    ZuAssertTrue(zt, 10 == iv);

    // delete elem
    retnode = ztl_map_del_ex(lmap, 2);
    iv = (int)(uint64_t)retnode->udata;
    ZuAssertTrue(zt, 2 == iv);
    free(retnode);

    retnode = ztl_map_del_ex(lmap, 2);
    ZuAssertTrue(zt, NULL == retnode);
    ZuAssertTrue(zt, 9 == ztl_map_size(lmap));

    retnode = ztl_map_del_ex(lmap, 5);
    iv = (int)(uint64_t)retnode->udata;
    ZuAssertTrue(zt, 5 == iv);
    free(retnode);

    ztl_map_release(lmap);
}

