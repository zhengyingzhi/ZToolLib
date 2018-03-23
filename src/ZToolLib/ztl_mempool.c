#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "ztl_locks.h"
#include "ztl_atomic.h"
#include "ztl_utils.h"
#include "ztl_malloc.h"
#include "ztl_mempool.h"

#ifdef _MSC_VER
#include <windows.h>
#endif//_MSC_VER


/* one memory block */
typedef struct ztl_memblock_st
{
    struct ztl_memblock_st* nextblk;
    uint32_t  curindex;
    uint32_t  count;
    char      block[1];
}ztl_memblock_t;

/* one entity buffer node */
typedef union ztl_bufnode_u
{
    union ztl_bufnode_u* nextbuf;
    char buf[1];
}ztl_bufnode_t;

#define ZTL_BUFNODE_ADDR(p)  (ztl_bufnode_t*)((char*)(p) - offsetof(ztl_bufnode_t, buf))

struct ztl_mempool_st
{
    uint32_t nEntitySize;
    uint32_t nInitCount;
    int32_t  AutoExpand;
    volatile uint32_t   nFreeCount;
    volatile uint32_t   freelock;
    volatile uint32_t   alloclock;

    ztl_memblock_t*     blocks;
    ztl_bufnode_t*      freenodes;

};

// alloc one memory block
static ztl_memblock_t* _alloc_block(ztl_mempool_t* amp)
{
    ztl_memblock_t* newblock;

    uint32_t lSize = amp->nEntitySize * amp->nInitCount
        + sizeof(ztl_memblock_t);
    newblock = (ztl_memblock_t*)ztl_malloc(ztl_align(lSize, 64));
    if (newblock == NULL) {
        return NULL;
    }

    newblock->curindex = 0;
    newblock->count = amp->nInitCount;
    //memset(newblock->block, 0, amp->nEntitySize * amp->nFreeCount);

    newblock->nextblk = amp->blocks;
    amp->blocks = newblock;

    return newblock;
}

//////////////////////////////////////////////////////////////////////////

ztl_mempool_t* ztl_mp_create(int nEntitySize, int nInitCount, int aAutoExpand)
{
    ztl_mempool_t* mp;

    if (nEntitySize < sizeof(int) || nInitCount < 1) {
        return NULL;
    }

    mp = (ztl_mempool_t*)ztl_malloc(sizeof(ztl_mempool_t));
    if (mp)
    {
        mp->nEntitySize = ztl_align(nEntitySize, 8);
        mp->nInitCount  = nInitCount;
        mp->nFreeCount  = 0;
        mp->AutoExpand  = aAutoExpand;

        mp->nFreeCount  = 0;
        mp->freelock    = 0;
        mp->alloclock   = 0;

        mp->blocks      = NULL;
        mp->freenodes   = NULL;

        if (_alloc_block(mp) == NULL) {
            ztl_free(mp);
            return NULL;
        }

        // try prepare one
        void* lpaddr = ztl_mp_alloc(mp);
        ztl_mp_free(mp, lpaddr);
    }

    return mp;
}

void ztl_mp_release(ztl_mempool_t* mp)
{
    if (NULL == mp) {
        return;
    }

    ztl_memblock_t* block;
    ztl_memblock_t* tb;

    block = mp->blocks;
    while (block) {
        tb = block->nextblk;
        ztl_free(block);
        block = tb;
    }

    ztl_free(mp);
}

char* ztl_mp_alloc(ztl_mempool_t* mp)
{
    ztl_bufnode_t*  pnode;
    ztl_memblock_t* theblock;

    ztl_spinlock(&mp->freelock, 1, ZTL_LOCK_SPIN);
    if (mp->nFreeCount > 0)
    {
        // get memory from freenodes list firstly
        pnode           = mp->freenodes;
        mp->freenodes   = pnode->nextbuf;

        ztl_atomic_dec(&mp->nFreeCount, 1);
    }
    else
    {
        // get memory from os
        theblock = mp->blocks;
        if (theblock->curindex == theblock->count) {
            if (mp->AutoExpand)
                theblock = _alloc_block(mp);
            else
                return NULL;
        }

        pnode = (ztl_bufnode_t*)(theblock->block + (theblock->curindex * mp->nEntitySize));
        ++theblock->curindex;
    }

    ztl_unlock(&mp->alloclock);

    pnode->nextbuf = NULL;
    return pnode->buf;
}

void ztl_mp_free(ztl_mempool_t* mp, void* paddr)
{
    if (NULL == paddr) {
        return;
    }

    ztl_bufnode_t* pnode = (ztl_bufnode_t*)paddr;

    // put to freenodes list head
    ztl_spinlock(&mp->freelock, 1, ZTL_LOCK_SPIN);
    pnode->nextbuf = mp->freenodes;
    mp->freenodes = pnode;
    ztl_unlock(&mp->freelock);

    ztl_atomic_add(&mp->nFreeCount, 1);
}

int ztl_mp_entity_size(ztl_mempool_t* mp)
{
    return mp->nEntitySize;
}
