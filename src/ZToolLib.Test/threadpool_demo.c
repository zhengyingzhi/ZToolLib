#include <assert.h>
#include <stdlib.h>

#include "ZToolLib/ztl_threads.h"
#include "ZToolLib/ztl_threadpool.h"


static void _free_func(ztl_thrpool_t* tp, void* arg)
{
    if (arg) {
        free(arg);
    }
}

static void _work_func(ztl_thrpool_t* tp, void* arg1, void* arg2)
{
    int* p1;
    p1 = (int*)arg1;
    assert(*p1 == 1);
}

void threadpool_demo(int argc, char* argv[])
{
    ztl_thrpool_t* tp;
    tp = ztl_thrpool_create(4, 1024);

    int count = 100;

    for (int i = 0; i < count; ++i)
    {
        int* p1;
        p1 = malloc(sizeof(int));
        *p1 = 1;
        ztl_thrpool_dispatch(tp, _work_func, p1, NULL, _free_func, NULL);
        if (i == 50)
            sleepms(10);
    }

    sleepms(1000);
    ztl_thrpool_stop(tp);
    ztl_thrpool_join(tp);
    ztl_thrpool_release(tp);
}
