/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_MEMDB_H_INCLUDED_
#define _ZTL_MEMDB_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>

#include "ztl_array.h"
#include "ztl_locks.h"
#include "ztl_shm.h"


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/* the reserved memory size for user */
#define ZTL_CTRL_BUF_SIZE   4096

/* the MEMDB error code */
#define ZTL_MEMDB_OK             0
#define ZTL_MEMDB_Error         -1
#define ZTL_MEMDB_MapFailed     (ZTL_MEMDB_Error - 1)
#define ZTL_MEMDB_NoEnoughMem   (ZTL_MEMDB_Error - 2)
#define ZTL_MEMDB_InvalidBuffer (ZTL_MEMDB_Error - 3)
#define ZTL_MEMDB_InvalidData   (ZTL_MEMDB_Error - 4)
#define ZTL_MEMDB_InvalidSeq    (ZTL_MEMDB_Error - 5)
#define ZTL_MEMDB_DupAppend     (ZTL_MEMDB_Error - 6)
#define ZTL_MEMDB_AlreadyOpened (ZTL_MEMDB_Error - 7)


/* the entry buffer type */
typedef void* ztl_entry_t;

/* the callback type when call foreach_buffer */
typedef void (*ztl_memdb_foreach_ptr)(ztl_entry_t* entry, void* userdata);

/* the sequence type */
typedef uint32_t ztl_seq_t;

/* the memdb struct */
typedef struct ztl_memdb_st ztl_memdb_t;

struct ztl_memdb_st
{
    void*       MapRegion;
    uint8_t*    BaseAddr;
    uint8_t*    CurAddr;
    uint8_t*    EndAddr;
    uint64_t    Capacity;
    uint64_t    CtrlBufSize;
    uint64_t    TotalBytes;

    ztl_seq_t*  pStartIndex;
    ztl_seq_t*  pReceiveIndex;
    ztl_seq_t*  pMaxIndex;

    ztl_array_t SeqVec;
    uint32_t    SeqMutex;
    uint32_t    MemMutex;

    int32_t     UseHugepage;
    int32_t     ErrorID;
    char        ErrorMsg[256];
    char        DBName[256];
};


/* Tries to open a high available memory db based on shm
 * we could alloc/free memory, and search memory by sequence number
 * dbname: we could pass a filename as the db file
 * create_mode: ztl_enum_create_t
 * access_mode: ztl_enum_mode_t
 */
ztl_memdb_t* ztl_memdb_create(const char* dbname, uint64_t dbsize,
    uint32_t ctrl_buf_size, bool use_hugepage);

/* release the memdb object */
void ztl_memdb_release(ztl_memdb_t* memdb);

/* erase the db file from the system. */
bool ztl_memdb_remove(const char* dbname);

void ztl_memdb_clear_error(ztl_memdb_t* memdb);

int ztl_memdb_open(ztl_memdb_t* memdb);
int ztl_memdb_close(ztl_memdb_t* memdb);

bool ztl_memdb_trylock_exclusive(ztl_memdb_t* memdb);
bool ztl_memdb_unlock(ztl_memdb_t* memdb);


ztl_entry_t ztl_memdb_alloc_entry(ztl_memdb_t* memdb, uint32_t length);
int ztl_memdb_free_entry(ztl_memdb_t* memdb, ztl_entry_t entry);

ztl_seq_t ztl_memdb_direct_append(ztl_memdb_t* memdb, ztl_entry_t entry);

ztl_entry_t ztl_memdb_get_entry(ztl_memdb_t* memdb, ztl_seq_t seq);

ztl_seq_t ztl_memdb_last_entry_seq(ztl_memdb_t* memdb);
ztl_seq_t ztl_memdb_start_seq(ztl_memdb_t* memdb);
bool ztl_memdb_set_start_seq(ztl_memdb_t* memdb, ztl_seq_t start_seq);


uint64_t ztl_memdb_total_bytes(ztl_memdb_t* memdb);
uint64_t ztl_memdb_total_used(ztl_memdb_t* memdb);

uint32_t ztl_memdb_count(ztl_memdb_t* memdb);
uint32_t ztl_memdb_entry_length(ztl_entry_t entry);

void* ztl_memdb_ctrl_buffer(ztl_memdb_t* memdb);

void ztl_memdb_reserve(ztl_memdb_t* memdb, uint32_t count);

int ztl_memdb_foreach(ztl_memdb_t* memdb, 
    ztl_memdb_foreach_ptr func, void* userdata,
    ztl_seq_t start_seq);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_MEMDB_H_INCLUDED_
