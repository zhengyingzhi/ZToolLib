/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_SHM_H_INCLUDE_
#define _ZTL_SHM_H_INCLUDE_

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/* Tries to open a named shared memory which mapped to a file 
 * or an anonymous shared memory, 
 * both could work with hugepage if os permissioned
 */
typedef struct ztl_shm_st ztl_shm_t;

typedef enum {
    ztl_open_only       = 0x01,
    ztl_create_only     = 0x02,
    ztl_open_or_create  = 0x03
}ztl_enum_create_t;
 
typedef enum {
    ztl_read_only       = 0x01,
    ztl_read_write      = 0x02,
    ztl_copy_on_write	= 0x03,
    ztl_read_private	= 0x04
}ztl_enum_mode_t;


/* Tries to open a shared memory object with name "name", with the access mode "mode",
 * which will map to a file is apName is not empty, otherwise open an anonymous shared memory
 * We could specify whether work with hugepage
 */
ztl_shm_t* ztl_shm_create(const char* apName, int aOpenOrCreate, int aAccessMode, bool aIsUseHugepage);

/* Tries to open a segment of shared memory, 
 * if apNameKey is empty, an anonymous shared memory opened,
 * We could specify whether work with hugepage
 */
ztl_shm_t* ztl_shm_segment_create(const char* apNameKey, int aOpenOrCreate, const bool aIsUseHugepage);

/* release the shm object */
void ztl_shm_release(ztl_shm_t* zshm);

/* Erases a shared memory object from the system. */
bool ztl_shm_remove(const char* apName);

/* Set the size of the shared memory mapping */
int ztl_shm_truncate(ztl_shm_t* zshm, uint64_t aSize);

/* Map the whole shared memory to this process */
int ztl_shm_map_region(ztl_shm_t* zshm, int aAccessMode);

/* Detach address from process */
int ztl_shm_detach(ztl_shm_t* zshm);

/* Get base process address of shared memory if mapped successful before */
void* ztl_shm_get_address(ztl_shm_t* zshm);

/* Flush the data in memory to file, aIsAsyncFlag is async flush or not */
int ztl_shm_flush_to_file(ztl_shm_t* zshm, bool aIsAsyncFlag, void* apAddr, uint64_t aSize);

/* Return the name of the shared memory object. */
const char* ztl_shm_get_name(ztl_shm_t* zshm);

/* Return true size of the shared memory object */
uint64_t ztl_shm_get_size(ztl_shm_t* zshm);

/* Return access mode */
int ztl_shm_get_mode(ztl_shm_t* zshm);

#endif//_ZTL_SHM_H_INCLUDE_
