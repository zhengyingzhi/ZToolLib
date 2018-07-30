#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "ztl_atomic.h"

#include "ztl_msg_buffer.h"

ztl_msg_buffer_t* zlt_msg_buffer_alloc(uint32_t size)
{
    ztl_msg_buffer_t* zmb;
    zmb = (ztl_msg_buffer_t*)malloc(sizeof(ztl_msg_buffer_t));
    memset(zmb, 0, sizeof(ztl_msg_buffer_t));
    return zmb;
}

void zlt_msg_buffer_free(ztl_msg_buffer_t* zmb)
{
    if (zmb)
    {
        free(zmb);
    }
}

uint32_t ztl_mb_addref(ztl_msg_buffer_t* zmb)
{
    return ztl_atomic_add(&zmb, 1) + 1;
}

uint32_t ztl_mb_decref_release(ztl_msg_buffer_t* zmb)
{
    uint32_t lOld = ztl_atomic_dec(&zmb->refcount, 1);
    if (1 == lOld)
    {
        zlt_msg_buffer_free(zmb);
    }

    return lOld;
}
