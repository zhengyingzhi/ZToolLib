#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "ZToolLib/ztl_threads.h"
#include "ZToolLib/ztl_threadpool.h"
#include "ZToolLib/ztl_times.h"


static void _tp_free_func(ztl_thrpool_t* tp, void* arg)
{
    fprintf(stderr, "_tp_free_func arg=%p\n", arg);
    if (arg) {
        free(arg);
    }
}

static void _tp_work_func(ztl_thrpool_t* tp, void* arg1, void* arg2)
{
    int* p1;
    p1 = (int*)arg1;
    fprintf(stderr, "_tp_work_func val=%d,%p\n", *p1, p1);
}

void threadpool_demo(int argc, char* argv[])
{
    ztl_thrpool_t* tp;
    tp = ztl_thrpool_create(4, 1024);
    ztl_thrpool_start(tp);

    int rv;
    int count = 10;

    for (int i = 0; i < count; ++i)
    {
        int* p1;
        p1 = malloc(sizeof(int));
        *p1 = i + 1;

        if (i == 6) {
            rv = ztl_thrpool_dispatch_priority(tp, _tp_work_func,
                p1, NULL, _tp_free_func, NULL);
        }
        else {
            rv = ztl_thrpool_dispatch(tp, _tp_work_func,
                p1, NULL, _tp_free_func, NULL);
        }

        if (rv != 0) {
            fprintf(stderr, "thrpool_dispath failed:%d\n", rv);
        }
        if (i == 5)
            sleepms(10);
    }

    sleepms(1000);
    ztl_thrpool_stop(tp);
    ztl_thrpool_join(tp, -1);
    ztl_thrpool_release(tp);
    fprintf(stderr, "threadpool_demo done!\n\n");
}
