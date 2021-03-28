#include <stdio.h>
#include <string.h>

#include "ZToolLib/ztl_threads.h"
#include "ZToolLib/ztl_producer_consumer.h"
#include "ZToolLib/ztl_times.h"


static bool _pc_handler(ztl_producer_consumer_t* zpc, int64_t type, void* data)
{
    fprintf(stderr, "_pc_handler type=%d, data=%s\n", (int)type, (char*)data);
    return true;
}

void producer_consumer_demo(int argc, char* argv[])
{
    ztl_producer_consumer_t* pc;
    pc = ztl_pc_create(16);
    ztl_pc_start(pc);

    ztl_pc_post(pc, _pc_handler, 101, "abc");
    ztl_pc_post(pc, _pc_handler, 102, "def");

    ztl_sleepms(1000);
    ztl_pc_stop(pc);
    ztl_pc_release(pc);
    fprintf(stderr, "producer_consumer_demo done!\n\n");
}
