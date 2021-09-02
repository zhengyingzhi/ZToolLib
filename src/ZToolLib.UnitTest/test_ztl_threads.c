#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <ZToolLib/ztl_evloop.h>
#include <ZToolLib/ztl_evtimer.h>
#include <ZToolLib/ztl_times.h>
#include <ZToolLib/ztl_simple_event.h>
#include <ZToolLib/ztl_threads.h>
#include <ZToolLib/ztl_threadpool.h>
#include <ZToolLib/ztl_utils.h>
#include <ZToolLib/ztl_unit_test.h>


void _test_thread_func(void* args)
{
    int* p1 = (int*)args;
    (void)p1;
    assert(*p1 == 1);
}

void Test_ztl_thread(ZuTest* zt)
{
    void* rval;
    ztl_thread_t thr;
    ztl_thread_mutex_t mutex;
    ztl_simevent_t* sev;

    ztl_thread_mutex_init(&mutex, NULL);
    ztl_thread_mutex_lock(&mutex);
    ztl_thread_mutex_unlock(&mutex);
    ztl_thread_mutex_destroy(&mutex);

    sev = ztl_simevent_create();
    ztl_simevent_signal(sev);
    ztl_simevent_release(sev);

    int* p1;
    ztl_new_val(p1, int, 1);
    ztl_thread_create2(&thr, _test_thread_func, p1);
    ztl_sleepms(10);

    ztl_thread_join(thr, &rval);
    (void)rval;
}


static void _test_thrpool_work(ztl_thrpool_t* tp, void* arg1, void* arg2)
{
    int* p1 = (int*)arg1;
    (void)tp;
    (void)p1;
    (void)arg2;
    assert(*p1 == 1);
}

static void _test_thrpool_free(ztl_thrpool_t* tp, void* arg1)
{
    (void)tp;
    if (arg1)
    {
        int* p1 = (int*)arg1;
        assert(*p1 == 1);
        free(arg1);
    }
}

void Test_ztl_threadpool(ZuTest* zt)
{
    ztl_thrpool_t* tp;
    tp = ztl_thrpool_create(2, 64);
    ZuAssertTrue(zt, tp != NULL);

    ztl_thrpool_start(tp);

    ztl_sleepms(10);

    int* p1;
    ztl_new_val(p1, int, 1);
    ztl_thrpool_dispatch(tp, _test_thrpool_work, p1, NULL, _test_thrpool_free, NULL);

    sleepms(10);
    ztl_thrpool_stop(tp);
    ztl_thrpool_join(tp, -1);
    ztl_thrpool_release(tp);
}
