#include <stdio.h>
#include <stdlib.h>
#include <ZToolLib/ztl_unit_test.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_utils.h>


static void _cmap_access(cmap_t* cmap, void* context1, int32_t context2, uint64_t key, int64_t value)
{
    (void)cmap;
    (void)context2;
    (void)key;

    int* arr = (int*)context1;
    int  pi = (int)value;
    // int size = context2;

    fprintf(stderr, "%d ", pi);
    bool found = false;
    for (int i = 0; i < 10; ++i)
    {
        if (arr[i] == pi)
        {
            found = true;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "_cmap_access failed");
    }
}

void Test_ztl_map(ZuTest* zt)
{
    cmap_t* lmap;
    lmap = cmap_create(4);

    ZuAssertTrue(zt, cmap_empty(lmap));

    int arr[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    for (size_t i = 0; i < sizeof(arr) / sizeof(arr[0]); ++i)
    {
        cmap_add(lmap, arr[i], arr[i]);
        ZuAssertIntEquals(zt, arr[i], cmap_size(lmap));

        int64_t pi = cmap_find(lmap, arr[i]);
        ZuAssertInt64Equals(zt, arr[i], pi);
    }

    ZuAssertIntEquals(zt, 10, cmap_size(lmap));

    cmap_traverse(lmap, _cmap_access, arr, 10);
    printf("\n");

    cmap_pair_t arr2[10] = { 0 };
    cmap_to_array(lmap, arr2, 8);
    for (int i = 0; i < 8; ++i)
    {
        printf("%d ", (int)arr2[i].Value);
    }
    printf("\n");

    // find elem
    int pi;
    pi = (int)cmap_find(lmap, 0);
    ZuAssertIntEquals(zt, -1, pi);

    pi = (int)cmap_find(lmap, 1);
    ZuAssertIntEquals(zt, 1, pi);

    pi = (int)cmap_find(lmap, 10);
    ZuAssertIntEquals(zt, 10, pi);

    // delete elem
    pi = (int)cmap_del(lmap, 2);
    ZuAssertIntEquals(zt, 2, pi);

    pi = (int)cmap_del(lmap, 2);
    ZuAssertIntEquals(zt, -1, pi);
    ZuAssertIntEquals(zt, 9, cmap_size(lmap));

    pi = (int)cmap_del(lmap, 5);
    ZuAssertIntEquals(zt, 5, pi);

    cmap_release(lmap);
}

void Test_ztl_map_ex(ZuTest* zt)
{
    rbtree_node_t* node;
    cmap_t* lmap;
    lmap = cmap_create(0);

    ZuAssertTrue(zt, cmap_empty(lmap));

    int arr[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    for (size_t i = 0; i < sizeof(arr) / sizeof(arr[0]); ++i)
    {
        node = (rbtree_node_t*)malloc(sizeof(rbtree_node_t));
        union_dtype_t d;
        d.i32 = arr[i];
        node->udata = d.ptr;

        cmap_add_ex(lmap, arr[i], node);
        ZuAssertIntEquals(zt, arr[i], cmap_size(lmap));

        rbtree_node_t* retnode;
        retnode = cmap_find_ex(lmap, arr[i]);
        ZuAssertPtrEquals(zt, node, retnode);
        ZuAssertIntEquals(zt, arr[i], *((int*)retnode));
    }

    ZuAssertIntEquals(zt, 10, cmap_size(lmap));

    // find elem
    rbtree_node_t* retnode;
    int iv;

    retnode = cmap_find_ex(lmap, 0);
    ZuAssertPtrEquals(zt, NULL, retnode);

    retnode = cmap_find_ex(lmap, 1);
    iv = (int)(uint64_t)retnode->udata;
    ZuAssertIntEquals(zt, 1, iv);

    retnode = cmap_find_ex(lmap, 10);
    iv = (int)(uint64_t)retnode->udata;
    ZuAssertIntEquals(zt, 10, iv);

    // delete elem
    retnode = cmap_del_ex(lmap, 2);
    iv = (int)(uint64_t)retnode->udata;
    ZuAssertIntEquals(zt, 2, iv);
    free(retnode);

    retnode = cmap_del_ex(lmap, 2);
    ZuAssertPtrEquals(zt, NULL, retnode);
    ZuAssertIntEquals(zt, 9, cmap_size(lmap));

    retnode = cmap_del_ex(lmap, 5);
    iv = (int)(uint64_t)retnode->udata;
    ZuAssertIntEquals(zt, 5, iv);
    free(retnode);

    cmap_release(lmap);
}
