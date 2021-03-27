/*
 * Copyright (C) Yingzhi Zheng
 * Copyright (C) zhengyingzhi112@163.com
 */

#ifndef _ZTL_MESSAGE_BUFFER_H_
#define _ZTL_MESSAGE_BUFFER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* exported types */
typedef struct ztl_msg_buffer_st ztl_msg_buffer_t;

struct ztl_msg_buffer_st
{
    ztl_msg_buffer_t*   prev;
    ztl_msg_buffer_t*   next;
    void              (*cleanup)(ztl_msg_buffer_t*);
    void*               owner;
    void*               udata;
    uint32_t            flags;
    uint32_t            body_size;
    uint32_t            used;
    uint32_t            refcount;
};


/* Create a msg buffer, and the returned object's refcount is already 1
 * @param body_size nbytes space to use
 * @note  you can use prev/next/owner/udata/flags variables
 */
ztl_msg_buffer_t* zlt_mb_alloc(uint32_t body_size);
ztl_msg_buffer_t* zlt_mb_clone(ztl_msg_buffer_t* zmb);

/* Init a msg buffer if you created ztl_msg_buffer_t object
 */
int ztl_mb_init(ztl_msg_buffer_t* zmb, uint32_t body_size);

/* Add refcount for the object,
 * @return the new refcount
 */
uint32_t ztl_mb_addref(ztl_msg_buffer_t* zmb);

/* Dec refcount for the object, will free it if the last reference,
 * we usually call this to free the object
 * @return the new refcount
 */
uint32_t ztl_mb_decref_release(ztl_msg_buffer_t* zmb);

/* try append data to the tail,
 * @return 0 if success, otherwise maybe ERR_OutOfMem
 */
int ztl_mb_append(ztl_msg_buffer_t* zmb, void* data, uint32_t size);

/* try insert data at the index,
 * @return 0 if success, otherwise maybe ERR_OutOfMem
 */
int ztl_mb_insert(ztl_msg_buffer_t* zmb, uint32_t index, void* data, uint32_t size);

/* update used length, and size maybe negative
 */
int ztl_mb_update_used(ztl_msg_buffer_t* zmb, int size);


#define ztl_mb_length(zmb)      ((zmb)->used)
#define ztl_mb_avail(zmb)       ((zmb)->body_size - (zmb)->used)
#define ztl_mb_data(zmb)        (char*)((zmb) + 1)
#define ztl_mb_object(zmb)      (ztl_msg_buffer_t*)((char*)(zmb) - sizeof(ztl_msg_buffer_t))


#ifdef __cplusplus
}
#endif

#endif//_ZTL_MESSAGE_BUFFER_H_
