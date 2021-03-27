#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <ZToolLib/ztl_evloop.h>
#include <ZToolLib/ztl_evtimer.h>
#include <ZToolLib/ztl_times.h>
#include <ZToolLib/ztl_threads.h>
#include <ZToolLib/ztl_unit_test.h>


static int trigger_count = 0;
static int trigger_count2 = 0;

static void _test_evt_handler(void* ctx, ztl_rbtree_node_t* node)
{
    (void)ctx;
    (void)node;
    assert(ctx != NULL);
    trigger_count += 1;
}

static int _test_timer_handler(ztl_evloop_t* evloop, uint64_t timer_id, void* udata)
{
    (void)evloop;
    (void)timer_id;
    (void)udata;
    assert(udata != NULL);
    trigger_count2 += 1;
    return 0;
}

void Test_ztl_evtimer(ZuTest* zt)
{
    uint64_t currtime;
    ztl_evtimer_t evt;
    ztl_evtimer_init(&evt);

    currtime = get_timestamp();
    ztl_evtimer_update_time(&evt, currtime);

    ztl_rbtree_node_t timer1;
    ztl_evtimer_add(&evt, &timer1, 200, 0);

    ztl_sleepms(201);
    ztl_evtimer_expire(&evt, get_timestamp(), _test_evt_handler, &evt);

    ztl_sleepms(1);
    ZuAssertIntEquals(zt, 1, trigger_count);
}

void Test_ztl_evtimer2(ZuTest* zt)
{
    ztl_evloop_t* el;
    ztl_evloop_create(&el, 1024);

    ztl_evloop_init(el);
    ztl_evloop_start(el);

    ztl_evloop_addtimer(el, 100, _test_timer_handler, NULL, el);

    sleepms(101);
    ztl_evloop_expire(el, get_timestamp());

    sleepms(1);
    ZuAssertIntEquals(zt, 1, trigger_count2);
}
