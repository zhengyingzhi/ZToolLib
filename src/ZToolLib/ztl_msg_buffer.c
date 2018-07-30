#include <stdlib.h>

#include "ztl_atomic.h"

#include "ztl_msg_buffer.h"

ztl_msg_buffer_t* zlt_msg_buffer_alloc(uint32_t size)
{
	ztl_msg_buffer_t* zmb;
	zmb = (ztl_msg_buffer_t*)malloc(sizeof(ztl_msg_buffer_t));
	memset(zmb, 0, sizeof(ztl_msg_buffer_t));
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
	if (1 == ztl_atomic_dec(&zmb->refcount, 1))
	{
		zlt_msg_buffer_free(zmb);
	}
}
