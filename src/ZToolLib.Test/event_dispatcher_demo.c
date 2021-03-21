#include <stdio.h>
#include <string.h>

#include "ZToolLib/ztl_threads.h"
#include "ZToolLib/ztl_event_dispatcher.h"


bool _evd_handler_a(ztl_event_dispatcher_t* zevd, void* ctx, uint32_t evtype, void* evdata)
{
    int* pa = (int*)ctx;
    char* d = (char*)evdata;
    fprintf(stderr, "_evd_handler_a pa=%d, evtype=%d, d=%s\n", *pa, evtype, d);
    return false;
}

bool _evd_handler_b(ztl_event_dispatcher_t* zevd, void* ctx, uint32_t evtype, void* evdata)
{
    int* pb = (int*)ctx;
    char* d = (char*)evdata;
    fprintf(stderr, "_evd_handler_b pb=%d, evtype=%d, d=%s\n", *pb, evtype, d);
    return false;
}

void event_dispatcher_demo(int argc, char* argv[])
{
    int rv;
    uint32_t quesize = 16;
    ztl_event_dispatcher_t* ed;
    ed = ztl_evd_create(quesize);
    ztl_evd_start(ed);

    int* pa = malloc(sizeof(int));
    *pa = 1;
    ztl_evd_register(ed, pa, 11, _evd_handler_a);

    int* pa2 = malloc(sizeof(int));
    *pa2 = 2;
    ztl_evd_register(ed, pa2, 12, _evd_handler_a);

    int* pb = malloc(sizeof(int));
    *pb = 10;
    ztl_evd_register(ed, pb, 21, _evd_handler_b);

    rv = ztl_evd_post(ed, 11, "aa");
    if (rv != 0) {
        fprintf(stderr, "ztl_evd_post1 failed\n");
    }

    rv = ztl_evd_post(ed, 12, "aa2");
    if (rv != 0) {
        fprintf(stderr, "ztl_evd_post2 failed\n");
    }

    rv = ztl_evd_post(ed, 21, "bb");
    if (rv != 0) {
        fprintf(stderr, "ztl_evd_post3 failed\n");
    }

    rv = ztl_evd_post(ed, 22, "bb2");  // none event handler for this
    if (rv != 0) {
        fprintf(stderr, "ztl_evd_post4 failed\n");
    }

    sleepms(1000);
    ztl_evd_stop(ed);
    ztl_evd_release(ed);
    fprintf(stderr, "event_dispatcher_demo done!\n\n");
}
