#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "ztl_atomic.h"
#include "ztl_errors.h"
#include "ztl_mem.h"
#include "ztl_msg_buffer.h"
#include "ztl_utils.h"


static void _zmb_free(ztl_msg_buffer_t* zmb)
{
    if (zmb)
        FREE(zmb);
}

ztl_msg_buffer_t* zlt_mb_alloc(uint32_t body_size)
{
    ztl_msg_buffer_t* zmb;
    uint32_t nbytes;
    nbytes = ztl_align(sizeof(ztl_msg_buffer_t) + body_size, 8);
    zmb = (ztl_msg_buffer_t*)ALLOC(nbytes);
    ztl_mb_init(zmb, body_size);
    zmb->cleanup = _zmb_free;
    return zmb;
}

ztl_msg_buffer_t* zlt_mb_clone(ztl_msg_buffer_t* zmb)
{
    ztl_msg_buffer_t* new_zmb;
    new_zmb = (ztl_msg_buffer_t*)zlt_mb_alloc(zmb->body_size);
    memcpy(new_zmb, zmb, sizeof(ztl_msg_buffer_t) + zmb->body_size);
    new_zmb->refcount = 1;
    new_zmb->cleanup = _zmb_free;
    return new_zmb;
}

#if 0
ztl_msg_buffer_t* zlt_mb_realloc(ztl_msg_buffer_t* zmb, uint32_t body_size)
{
    uint32_t nbytes;

    if (body_size <= zmb->body_size) {
        return zmb;
    }

    nbytes = ztl_align(sizeof(ztl_msg_buffer_t) + body_size, 8);
    zmb = (ztl_msg_buffer_t*)realloc(zmb, nbytes);
    // FIXME: how to proc refcount
    return zmb;
}
#endif

int ztl_mb_init(ztl_msg_buffer_t* zmb, uint32_t body_size)
{
    memset(zmb, 0, sizeof(ztl_msg_buffer_t));
    zmb->body_size  = body_size;
    zmb->used       = 0;
    zmb->refcount   = 1;
    return 0;
}

uint32_t ztl_mb_addref(ztl_msg_buffer_t* zmb)
{
    return ztl_atomic_add(&zmb->refcount, 1) + 1;
}

uint32_t ztl_mb_decref_release(ztl_msg_buffer_t* zmb)
{
    uint32_t old = ztl_atomic_dec(&zmb->refcount, 1);
    if (1 == old) {
        zmb->cleanup(zmb);
    }

    return old - 1;
}

int ztl_mb_append(ztl_msg_buffer_t* zmb, void* data, uint32_t size)
{
    if (ztl_mb_avail(zmb) < size)
        return ZTL_ERR_OutOfMem;
    memcpy(ztl_mb_data(zmb) + zmb->used, data, size);
    zmb->used += size;
    return 0;
}

int ztl_mb_insert(ztl_msg_buffer_t* zmb, uint32_t index, void* data, uint32_t size)
{
    char* body;

    if (ztl_mb_avail(zmb) < size)
        return ZTL_ERR_OutOfMem;

    body = ztl_mb_data(zmb);
    memmove(body + index + size, body + index, zmb->used - index);
    memcpy(body + index, data, size);
    return 0;
}

int ztl_mb_update_used(ztl_msg_buffer_t* zmb, int size)
{
    zmb->used += size;
    return 0;
}
