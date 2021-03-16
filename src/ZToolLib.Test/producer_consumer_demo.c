#include "ZToolLib/ztl_producer_consumer.h"


static bool _pc_handler(ztl_producer_consumer_t* zpc, int64_t type, void* data)
{
    return true;
}

void producer_consumer_demo(int argc, char* argv[])
{
    ztl_producer_consumer_t* pc;
    pc = ztl_pc_create(16);

    ztl_pc_release(pc);
}
