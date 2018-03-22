#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "ztl_mempool.h"
#include "ztl_locks.h"
#include "ztl_atomic.h"
#include "ztl_utils.h"

#ifdef _MSC_VER
#include <windows.h>
#endif//_MSC_VER


/// one memory block
typedef struct ztl_memblock_st
{
    struct ztl_memblock_st* nextblk;
    short  curindex;
    short  count;
    char   block[1];
}ztl_memblock_t;

/// one entity buffer node
//typedef struct bufnode_st  bufnode_t;
typedef union ztl_bufnode_u
{
    union ztl_bufnode_u* nextbuf;
    char buf[1];
}ztl_bufnode_t;

#define ZTL_BUFNODE_ADDR(p)  (ztl_bufnode_t*)((char*)(p) - offsetof(ztl_bufnode_t, buf))
#define ZTL_DEF_MAXCOUNT     65535
#define ZTL_DEF_FREECOUNT    32 
#define ZTL_MP_USE_LOCKFREE  1

struct ztl_mempool_st
{
    uint32_t nEntitySize;
    uint32_t nInitCount;
    volatile uint32_t   nFreeCount;
    volatile uint32_t   freelock;
    volatile uint32_t   alloclock;

    ztl_memblock_t*     blocks;
    ztl_bufnode_t*      freenodes;

};

static ztl_memblock_t* _alloc_block(ztl_mempool_t* amp)
{
    ztl_memblock_t* newblock;

    newblock = (ztl_memblock_t*)malloc(amp->nEntitySize * amp->nInitCount
        + sizeof(ztl_memblock_t));
    if (newblock == NULL) {
        return NULL;
    }

    newblock->curindex = 0;
    newblock->count = amp->nInitCount;
    memset(newblock->block, 0, amp->nEntitySize * amp->nFreeCount);

    newblock->nextblk = amp->blocks;
    amp->blocks = newblock;

    return newblock;
}

/// create a memory pool
ztl_mempool_t* ztl_mp_create(int nEntitySize, int nInitCount /* = 32*/)
{
    ztl_mempool_t* mp;

    if (nEntitySize < 1 || nInitCount < 1 || nInitCount > ZTL_DEF_MAXCOUNT)
    {
        return NULL;
    }
#ifndef _DEBUG
    if (nInitCount < 2)
        nInitCount = 2;
#endif

    mp = (ztl_mempool_t*)malloc(sizeof(ztl_mempool_t));
    if (mp != NULL)
    {
        // each entity size added n bytes for bufnode_t's nextn pointer
        mp->nEntitySize = ztl_align((nEntitySize), 8);
        mp->nInitCount = nInitCount;
        mp->nFreeCount = 0;

        mp->freenodes = NULL;
        mp->blocks = NULL;
        if (_alloc_block(mp) == NULL) {
            free(mp);
            return NULL;
        }

        mp->freelock = 0;
        mp->freelock = 0;

        // prepare cached entity objects
        int32_t lFreeCount = 0;
        void* lpaddr = NULL;
        for (int i = 0; i < nInitCount && i < ZTL_DEF_FREECOUNT; ++i)
        {
            mp->nFreeCount = 0;
            if ((lpaddr = ztl_mp_alloc(mp)) == NULL)
                break;

            ztl_mp_free(mp, lpaddr);
            lFreeCount += mp->nFreeCount;
        }
        mp->nFreeCount = lFreeCount;
    }

    return mp;
}

/// destroy the memory pool
void ztl_mp_destroy(ztl_mempool_t* mp)
{
    if (NULL == mp) {
        return;
    }

    ztl_memblock_t* block = mp->blocks;
    ztl_memblock_t* tb = NULL;
    while (block) {
        tb = block->nextblk;
        free(block);
        block = tb;
    }

    free(mp);
}

/// alloc memory from pool
void* ztl_mp_alloc(ztl_mempool_t* mp)
{
    ztl_bufnode_t* pnode;

    // get memory from freenodes list firstly
    if (mp->nFreeCount > 0)
    {
        if (ztl_atomic_dec(&mp->nFreeCount, 1) > 0)
        {
            ztl_spinlock(&mp->freelock, 1, ZTL_LOCK_SPIN);
#if ZTL_MP_USE_LOCKFREE
            ztl_bufnode_t* pexpect;
            do {
                pexpect = mp->freenodes;
                pnode = mp->freenodes;
            } while (!ztl_atomic_casptr(&mp->freenodes, pexpect, mp->freenodes->nextbuf));
#else
            pnode = mp->freenodes;
            mp->freenodes = pnode->nextbuf;
#endif//ZTL_MP_USE_LOCKFREE

            ztl_spinunlock(&mp->freelock);

            pnode->nextbuf = NULL;
            return pnode->buf;
        }

        ztl_atomic_add(&mp->nFreeCount, 1);
    }

    // get memory from os
    ztl_spinlock(&mp->freelock, 1, ZTL_LOCK_SPIN);

    ztl_memblock_t* theblock = mp->blocks;
    if (theblock->curindex == theblock->count)
    {
        theblock = _alloc_block(mp);
    }

    pnode = (ztl_bufnode_t*)(theblock->block + (theblock->curindex * mp->nEntitySize));
    pnode->nextbuf = NULL;
    ++theblock->curindex;
    ztl_spinunlock(&mp->alloclock);

    return pnode->buf;
}

/// free memory to pool
void ztl_mp_free(ztl_mempool_t* mp, void* paddr)
{
    if (NULL == paddr) {
        return;
    }

    //bufnode_t* pnode = BUFNODE_ADDR(paddr);
    ztl_bufnode_t* pnode = (ztl_bufnode_t*)paddr;

#if ZTL_MP_USE_LOCKFREE
    ztl_bufnode_t* pexpect;
    do {
        pexpect = mp->freenodes;
        pnode->nextbuf = mp->freenodes;
    } while (!ztl_atomic_casptr(&mp->freenodes, pexpect, pnode));

    ztl_atomic_add(&mp->nFreeCount, 1);
#else
    ztl_spinlock(&mp->freelock, 1, ZTL_LOCK_SPIN);
    pnode->nextbuf = mp->freenodes;
    mp->freenodes = pnode;
    ztl_atomic_add(&mp->nFreeCount, 1);
    ztl_spinunlock(&mp->freelock);
#endif//ZTL_MP_USE_LOCKFREE
}

uint32_t ztl_mp_entity_size(ztl_mempool_t* mp)
{
    //return mp->nEntitySize - sizeof(void*);
    return mp->nEntitySize;
}
