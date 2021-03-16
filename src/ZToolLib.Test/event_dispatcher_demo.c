#include "ZToolLib/ztl_event_dispatcher.h"

void event_dispatcher_demo(int argc, char* argv[])
{
    ztl_event_dispatcher_t* ed;
    ed = ztl_evd_create(16);
    ztl_evd_release(ed);
}
