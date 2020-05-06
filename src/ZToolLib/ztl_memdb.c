#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

#include "ztl_memdb.h"


#ifdef _MSC_VER
#include <Windows.h>
int os_error() { return GetLastError(); }
#define FMT64   "%lld"
#define SEQ_FMT "%u"
#else
#include <errno.h>
int os_error() { return errno; }
#define FMT64   "%ld"
#define SEQ_FMT "%u"
#endif//_MSC_VER


#if defined(_DEBUG) || defined(DEBUG)
#define MEMDB_DEBUG 1
#else
#define MEMDB_DEBUG 0
#endif

#if (MEMDB_DEBUG)
#define DBDebugInfo(fd,fmt,...)     fprintf(fd,fmt,##__VA_ARGS__)
#else
static  inline void DBPrintNull(FILE* fd, const char* fmt, ...) { (void)fd; (void)fmt; }
#define DBDebugInfo(fd,fmt,...)     DBPrintNull(fd,fmt,##__VA_ARGS__)
#endif//MEMDB_DEBUG
#define DBDbgFd     stdout

/// memory status
typedef enum
{
    MF_UNUSED   = 0,
    MF_ALLOCED  = 1,
    MF_DEALLOC  = 2,
    MF_FILLED   = 3
}MemFlag;

/// one memory head info
#pragma pack(push, 1)
typedef struct {
    uint32_t Length;
    uint32_t PreLength;
    uint32_t Sequence;
    uint8_t  Flag;
    uint8_t  CheckSum;
    uint16_t Padding;
}MemHeadInfo;
#pragma pack(pop)


#define DEFAULT_N_INDEX 8
typedef struct {
    uint64_t    ShmSize;
    ztl_seq_t   MaxIndex;
    ztl_seq_t   StartIndex[DEFAULT_N_INDEX];
    ztl_seq_t   ReceiveIndex[DEFAULT_N_INDEX];
}HAShmInfo;

/// memory header size: 16 bytes
#define MHEAD_SIZE      sizeof(MemHeadInfo)
#define MBASE_OFFSET    1024

#define INVALID_SEQ     0xffffffffffffffffU
#define CHECK_SUM_1     0x07
#define CHECK_SUM_2     0x0F


/// get one memory's header pointer
#define TheHeadPtr(buffer)  (MemHeadInfo*)((uint8_t*)(buffer) - MHEAD_SIZE)

/// get next memory header pointer
#define NextHeadPtr(HI)     (MemHeadInfo*)((uint8_t*)(HI) + HI->Length + MHEAD_SIZE);
#define PrevHeadPtr(HI)     (MemHeadInfo*)((uint8_t*)(HI) + HI->PreLength - MHEAD_SIZE);

/// get actual memory size
#define MemSize(length)     ((length) + MHEAD_SIZE)

/// clear the buffer content
#define ClearBuffer(HI)     memset((uint8_t*)(HI) + MHEAD_SIZE, 0, (HI)->Length)

/// calc checksum for each buffer
#define CalcCheckSum(HI)    ((CHECK_SUM_1 & (HI)->Length) + (CHECK_SUM_2 & (HI)->PreLength))

/// merge memory from apCurAddr
static uint8_t* MergeMemory(uint8_t* apBaseAddr, uint8_t* apCurAddr)
{
    MemHeadInfo* lpHICurr = (MemHeadInfo*)apCurAddr;
    MemHeadInfo* lpHIPrev = PrevHeadPtr(lpHICurr);
    uint8_t* lpAddr = apCurAddr;

    if (MF_DEALLOC != lpHICurr->Flag)
    {
        return lpAddr;
    }

    while (true)
    {
        lpHICurr->Sequence = 0;
        lpHICurr->Flag = MF_UNUSED;
        lpHICurr->CheckSum = 0;
        lpHICurr->PreLength = 0;
        lpHICurr->Length = 0;
        lpAddr = (uint8_t*)lpHIPrev;

        if (MF_DEALLOC != lpHIPrev->Flag || lpAddr == apBaseAddr)
        {
            if (MF_UNUSED == lpHIPrev->Flag)
            {
                DBDebugInfo(DBDbgFd, "MergeMemory: Flag is MF_UNUSED at %p\n", (void*)lpAddr);
            }
            break;
        }

        lpHICurr = lpHIPrev;
        lpHIPrev = PrevHeadPtr(lpHIPrev);
    }

    return lpAddr;
}

/// open a shared memory, return the base address
static void* _open_shm(ztl_memdb_t* memdb, const char* apName, uint64_t aSize)
{
    void* lpBase;
    ztl_shm_t* shm;

    // ClearError();

    // create a shared memory object, mode can only be read_only | read_write
    shm = ztl_shm_create(apName, ztl_open_or_create, ztl_read_write, memdb->UseHugepage);
    ztl_shm_truncate(shm, aSize);

    // map the whole shared memory to this process
    if (0 != ztl_shm_map_region(shm, ztl_read_write))
    {
        DBDebugInfo(DBDbgFd, "open_shm: mapregion failed for %s with usehugepage:%d errno:%d\n",
            apName, memdb->UseHugepage, os_error());
        ztl_shm_release(shm);

        shm = ztl_shm_create(apName, ztl_open_or_create, ztl_read_write, false);
        ztl_shm_truncate(shm, aSize);
    }

    memdb->MapRegion = shm;
    lpBase = ztl_shm_get_address(shm);

    return lpBase;
}

void ztl_memdb_initialize(ztl_memdb_t* memdb, uint8_t* apAddr, uint64_t aSize);


ztl_memdb_t* ztl_memdb_create(const char* dbname, uint64_t dbsize,
    uint32_t ctrl_buf_size, bool use_hugepage)
{
    ztl_memdb_t* memdb;

    memdb = calloc(1, sizeof(ztl_memdb_t));
    strncpy(memdb->DBName, dbname, sizeof(memdb->DBName) - 1);
    memdb->Capacity    = dbsize;
    memdb->CtrlBufSize = ctrl_buf_size;
    memdb->UseHugepage = use_hugepage;

    return memdb;
}

void ztl_memdb_release(ztl_memdb_t* memdb)
{
    ztl_memdb_close(memdb);
}

bool ztl_memdb_trylock_exclusive(ztl_memdb_t* memdb)
{
    return ztl_shm_trylock_exclusive(memdb->MapRegion);
}

bool ztl_memdb_unlock(ztl_memdb_t* memdb)
{
    return ztl_shm_unlock_file(memdb->MapRegion);
}


ztl_seq_t ztl_memdb_last_seq(ztl_memdb_t* memdb)
{
    return *memdb->pStartIndex + ztl_array_size(&memdb->SeqVec) - 1;
}

uint8_t* ztl_memdb_seq2entry(ztl_memdb_t* memdb, ztl_seq_t seq)
{
    return ztl_array_at(&memdb->SeqVec, (uint32_t)(seq- *memdb->pStartIndex));
}

bool ztl_memdb_remove(const char* dbname)
{
    if (!ztl_shm_remove(dbname))
    {
        DBDebugInfo(DBDbgFd, "HASMDB: Remove db %s failed!\n", dbname);
        return false;
    }
    else
    {
        DBDebugInfo(DBDbgFd, "HASMDB: Removed db %s\n", dbname);
        return true;
    }
}

void ztl_memdb_clear_error(ztl_memdb_t* memdb)
{
    memdb->ErrorID = 0;
    memdb->ErrorMsg[0] = '\0';
}

int ztl_memdb_open(ztl_memdb_t* memdb)
{
    if (!memdb->DBName[0]) {
        return ZTL_MEMDB_Error;
    }

    if (memdb->BaseAddr) {
        return ZTL_MEMDB_AlreadyOpened;
    }

    uint8_t*    lpBase;
    HAShmInfo*  lpInfo;
    uint64_t    lSize;

    do {
        lpBase = (uint8_t*)_open_shm(memdb, memdb->DBName, memdb->Capacity);

        DBDebugInfo(DBDbgFd, "HASMDB: shared memory %s, " FMT64 "bytes at [%p]\n",
            memdb->DBName, memdb->Capacity, (void*)lpBase);

        if (NULL == lpBase)
        {
            return ZTL_MEMDB_MapFailed;
        }

        // the db state info
        lpInfo = (HAShmInfo*)(lpBase + memdb->CtrlBufSize);
        memdb->pStartIndex = lpInfo->StartIndex;
        memdb->pReceiveIndex = lpInfo->ReceiveIndex;
        memdb->pMaxIndex = &lpInfo->MaxIndex;

        for (int i = 0; i < DEFAULT_N_INDEX; ++i)
            if (0 == memdb->pStartIndex[i])
                memdb->pStartIndex[i] = 1;

        for (int i = 0; i < DEFAULT_N_INDEX; ++i)
            if (0 == memdb->pReceiveIndex[i])
                memdb->pReceiveIndex[i] = 1;

        if (0 == lpInfo->ShmSize)
        {
            lpInfo->ShmSize = memdb->Capacity;
        }
        else if (lpInfo->ShmSize != memdb->Capacity)
        {
            DBDebugInfo(DBDbgFd, "HASMDB: share memory %p, old_size:" FMT64 ", now_size:" FMT64 "\n",
                memdb->DBName, lpInfo->ShmSize, memdb->Capacity);
            if (memdb->Capacity < lpInfo->ShmSize)
            {
                // auto re-mapping whole shared memory
                memdb->Capacity = lpInfo->ShmSize;

                ztl_memdb_close(memdb);
                continue;
            }
        }
        break;
    } while (true);

    DBDebugInfo(DBDbgFd, "MEMDB: initializing...\n");

    // initial shared memory to this process address
    lpBase += memdb->CtrlBufSize + MBASE_OFFSET;
    lSize = memdb->Capacity - (memdb->CtrlBufSize + MBASE_OFFSET);
    ztl_memdb_initialize(memdb, lpBase, lSize);

    DBDebugInfo(DBDbgFd, "MEMDB: initializ finished\n");

    // we reserve a head at start address
    MemHeadInfo* lpHI = (MemHeadInfo*)memdb->BaseAddr;
    if (MF_ALLOCED != lpHI->Flag)
    {
        lpHI->Length = 0;
        lpHI->PreLength = 0;
        lpHI->Sequence = INVALID_SEQ;
        lpHI->Flag = MF_ALLOCED;
        lpHI->CheckSum = 0;
    }

    return ZTL_MEMDB_OK;
}

int ztl_memdb_close(ztl_memdb_t* memdb)
{
    if (memdb->MapRegion)
    {
        ztl_shm_t* shm;
        shm = (ztl_shm_t*)memdb->MapRegion;
        ztl_shm_release(shm);

        memdb->MapRegion = NULL;
        memdb->BaseAddr = NULL;
    }
    return ZTL_MEMDB_OK;
}


ztl_entry_t ztl_memdb_alloc_entry(ztl_memdb_t* memdb, uint32_t length)
{
    ztl_memdb_clear_error(memdb);

    ztl_spinlock(&memdb->MemMutex, 1, 2048);

    ztl_entry_t  lpRet = NULL;
    MemHeadInfo* lpHILast = (MemHeadInfo*)memdb->CurAddr;
    MemHeadInfo* lpHI = NextHeadPtr(lpHILast);

    if ((uint8_t*)lpHI + MemSize(length) >= memdb->EndAddr)
    {
        memdb->ErrorID = ZTL_MEMDB_NoEnoughMem;
        int n = snprintf(memdb->ErrorMsg, sizeof(memdb->ErrorMsg) - 1,
            "AllocEntryBuffer: no enough memory when alloc %dbytes", length);
        memdb->ErrorMsg[n] = '\0';

        ztl_unlock(&memdb->MemMutex);
        return NULL;
    }

    // move to next available allocated position
    memdb->CurAddr = (uint8_t*)lpHI;
    lpRet = (uint8_t*)lpHI + MHEAD_SIZE;

#if MEMDB_DEBUG
    MemHeadInfo lHead;
    memset(&lHead, 0, sizeof(lHead));
    int iret = memcmp(lpHI, &lHead, sizeof(lHead));
    assert(0 == iret);
#endif

    if (0 != lpHI->Length)
    {
        DBDebugInfo(DBDbgFd, "AllocEntryBuffer: invalid memory data at %p, Length:%d\n",
            (void*)lpHI, lpHI->Length);
        ClearBuffer(lpHI);
    }

    lpHI->Length    = length;
    lpHI->PreLength = lpHILast->Length;
    lpHI->Sequence  = 0;
    lpHI->Flag      = MF_ALLOCED;
    lpHI->CheckSum  = CalcCheckSum(lpHI);

    ztl_unlock(&memdb->MemMutex);
    return lpRet;
}

int ztl_memdb_free_entry(ztl_memdb_t* memdb, ztl_entry_t entry)
{
    MemHeadInfo* lpHI = TheHeadPtr(entry);
    ztl_seq_t    lSequence = lpHI->Sequence;

    lpHI->Sequence = 0;
    ClearBuffer(lpHI);
    ztl_memdb_clear_error(memdb);

    // if the buffer was appended before
    if (0 != lSequence)
    {
        ztl_spinlock(&memdb->MemMutex, 1, 2048);

        if (lSequence < ztl_memdb_start_seq(memdb) ||
            lSequence > ztl_memdb_last_seq(memdb))
        {
            int n = snprintf(memdb->ErrorMsg, sizeof(memdb->ErrorMsg) - 1,
                "FreeEntryBuffer: invalid Entry:%p, Sequence:" SEQ_FMT ", since StartSeq:" SEQ_FMT ", LastSeq:" SEQ_FMT,
                entry, lSequence, ztl_memdb_start_seq(memdb), ztl_memdb_last_seq(memdb));
            memdb->ErrorMsg[n] = '\0';
            memdb->ErrorID = ZTL_MEMDB_InvalidSeq;

            return ZTL_MEMDB_InvalidSeq;
        }
#if MEMDB_DEBUG
        assert(lSequence == ztl_memdb_last_seq(memdb));
        assert(*memdb->pMaxIndex > 0);
#endif

        *memdb->pMaxIndex -= 1;
        memdb->TotalBytes -= lpHI->Length;
        ztl_array_pop_back(&memdb->SeqVec);
        ztl_unlock(&memdb->MemMutex);
    }

    ztl_spinlock(&memdb->MemMutex, 1, 2048);
    lpHI->Flag = MF_DEALLOC;

    memdb->CurAddr = MergeMemory(memdb->BaseAddr, memdb->CurAddr);

    ztl_unlock(&memdb->MemMutex);
    return ZTL_MEMDB_OK;
}

int ztl_memdb_free_seq_prefix(ztl_memdb_t* memdb, ztl_seq_t seq)
{
    DBDebugInfo(DBDbgFd, "FreeSequencePrefix: aSequence:" SEQ_FMT ", StartIndex:" SEQ_FMT "\n",
        seq, ztl_memdb_start_seq(memdb));
    ztl_memdb_clear_error(memdb);

    ztl_spinlock(&memdb->SeqMutex, 1, 2048);

    if (seq < ztl_memdb_start_seq(memdb) ||
        seq > ztl_memdb_last_seq(memdb))
    {
        return ZTL_MEMDB_InvalidSeq;
    }

    ztl_spinlock(&memdb->MemMutex, 1, 2048);

    // update start index firstly
    uint64_t lCount = seq - *memdb->pStartIndex;
    *memdb->pStartIndex = seq;

    ztl_entry_t lpEntry;
    MemHeadInfo* lpHI;
    for (uint64_t i = 0; i < lCount; ++i)
    {
        lpEntry = ztl_array_at2(&memdb->SeqVec, 0);
        lpHI = TheHeadPtr(lpEntry);

        lpHI->Sequence = 0;
        ClearBuffer(lpHI);
        lpHI->Flag = MF_DEALLOC;
        memdb->TotalBytes -= lpHI->Length;

        // TODO: 
        // ztl_array_pop_front(&memdb->SeqVec);
    }

    return ZTL_MEMDB_OK;
}

int ztl_memdb_free_seq_suffix(ztl_memdb_t* memdb, ztl_seq_t seq, ztl_memdb_foreach_ptr func, void* userdata)
{
    DBDebugInfo(DBDbgFd, "FreeSequenceSuffix: seq:" SEQ_FMT ", last_seq:" SEQ_FMT "\n",
        seq, ztl_memdb_last_entry_seq(memdb));
    ztl_memdb_clear_error(memdb);

    ztl_spinlock(&memdb->SeqMutex, 1, 2048);

    if (seq < ztl_memdb_start_seq(memdb) ||
        seq > ztl_memdb_last_seq(memdb))
    {
        ztl_unlock(&memdb->SeqMutex);
        return ZTL_MEMDB_InvalidSeq;
    }

    uint64_t     lCount;
    void*        lpEntry;
    MemHeadInfo* lpHI;

    // free the buffer from backend
    lCount = ztl_memdb_last_seq(memdb) - seq;
    if (lCount > 0)
        *memdb->pMaxIndex = seq;

    ztl_spinlock(&memdb->MemMutex, 1, 2048);

    for (uint64_t i = 0; i < lCount; ++i)
    {
        lpEntry = ztl_memdb_seq2entry(memdb, ztl_memdb_last_seq(memdb));
        lpHI = TheHeadPtr(lpEntry);

        if (func)
            func(lpEntry, userdata);

        lpHI->Sequence = 0;
        ClearBuffer(lpHI);
        lpHI->Flag = MF_DEALLOC;

        ztl_array_pop_back(&memdb->SeqVec);
        memdb->TotalBytes -= lpHI->Length;
    }

    memdb->CurAddr = MergeMemory(memdb->BaseAddr, memdb->CurAddr);
    ztl_unlock(&memdb->MemMutex);

    ztl_unlock(&memdb->SeqMutex);
    return ZTL_MEMDB_OK;
}


ztl_seq_t ztl_memdb_direct_append(ztl_memdb_t* memdb, ztl_entry_t entry)
{
    ztl_memdb_clear_error(memdb);
    if (NULL == entry)
    {
        memdb->ErrorID = ZTL_MEMDB_InvalidBuffer;
        strcpy(memdb->ErrorMsg, "DirectAppend: buffer is null");
        return 0;
    }

    MemHeadInfo* lpHI = TheHeadPtr(entry);
    if (0 < lpHI->Sequence)
    {
        memdb->ErrorID = ZTL_MEMDB_DupAppend;
        int n = snprintf(memdb->ErrorMsg, sizeof(memdb->ErrorMsg) - 1,
            "DirectAppend: append repeatedly for buffer:%p, which sequence:" SEQ_FMT,
            entry, lpHI->Sequence);
        memdb->ErrorMsg[n] = '\0';

#if MEMDB_DEBUG
        assert(0 < lpHI->Length);
        assert(MF_FILLED == lpHI->Flag);
        assert(entry == ztl_memdb_get_entry(memdb, lpHI->Sequence));
#endif
        return lpHI->Sequence;
    }

    if (MF_ALLOCED != lpHI->Flag)
    {
        memdb->ErrorID = ZTL_MEMDB_InvalidBuffer;
        int n = snprintf(memdb->ErrorMsg, sizeof(memdb->ErrorMsg) - 1,
            "DirectAppend: at %p, illegal Flag:%d, Length:%u",
            entry, lpHI->Flag, lpHI->Length);
        memdb->ErrorMsg[n] = '\0';
        return 0;
    }

    ztl_spinlock(&memdb->MemMutex, 1, 2048);

    // !!keep buffer address, and the vector size is sequence number!!
    ztl_array_push_back(&memdb->SeqVec, &entry);
    memdb->TotalBytes += lpHI->Length;

    ztl_seq_t lSeq = ztl_memdb_last_seq(memdb);

    lpHI->Sequence = lSeq;
    lpHI->Flag = MF_FILLED;
    *memdb->pMaxIndex = lSeq;
    ztl_unlock(&memdb->MemMutex);

    return lSeq;
}

ztl_entry_t ztl_memdb_get_entry(ztl_memdb_t* memdb, ztl_seq_t seq)
{
    ztl_spinlock(&memdb->SeqMutex, 1, 2048);

    if (seq < ztl_memdb_start_seq(memdb) ||
        seq > ztl_memdb_last_seq(memdb))
    {
        return NULL;
    }

    ztl_entry_t lpEntry;
    lpEntry = ztl_memdb_seq2entry(memdb, seq);

#if MEMDB_DEBUG
    // for debug...
    MemHeadInfo* lpHI = TheHeadPtr(lpEntry);
    assert(lpHI->Sequence == seq);
    assert(lpHI->CheckSum == CalcCheckSum(lpHI));
    //fprintf(stderr, "GetEntry: aSequence=%lld, HeadInfo=%3d,%2d,%ld\n", aSequence, lpHI->Length, lpHI->Flag, lpHI->Sequence;
#endif

    ztl_unlock(&memdb->SeqMutex);

    return lpEntry;
}

uint64_t ztl_memdb_total_bytes(ztl_memdb_t* memdb)
{
    return memdb->TotalBytes;
}

uint64_t ztl_memdb_total_used(ztl_memdb_t* memdb)
{
    return memdb->TotalBytes + ztl_array_size(&memdb->SeqVec) * MHEAD_SIZE;
}

uint32_t ztl_memdb_count(ztl_memdb_t* memdb)
{
    return ztl_array_size(&memdb->SeqVec);
}

uint32_t ztl_memdb_entry_length(ztl_entry_t entry)
{
    if (!entry) {
        return 0;
    }

    return (TheHeadPtr(entry))->Length;
}

void* ztl_memdb_ctrl_buffer(ztl_memdb_t* memdb)
{
    return (memdb->BaseAddr - memdb->CtrlBufSize - MBASE_OFFSET);
}

void ztl_memdb_reserve(ztl_memdb_t* memdb, uint32_t count)
{
    (void)memdb;
    (void)count;
    // 
}

int ztl_memdb_foreach(ztl_memdb_t* memdb,
    ztl_memdb_foreach_ptr func, void* userdata,
    ztl_seq_t start_seq)
{
    if (!func) {
        return ZTL_MEMDB_Error;
    }

    ztl_spinlock(&memdb->SeqMutex, 1, 2048);
    if (start_seq < ztl_memdb_start_seq(memdb) ||
        start_seq > ztl_memdb_last_seq(memdb))
    {
        ztl_unlock(&memdb->SeqMutex);
        return ZTL_MEMDB_InvalidSeq;
    }

    ztl_entry_t lpEntry;
    ztl_seq_t i;
    ztl_seq_t last_seq = ztl_memdb_last_seq(memdb);
    for (i = start_seq; i <= last_seq; ++i)
    {
        lpEntry = ztl_memdb_seq2entry(memdb, i);
        if (!lpEntry) {
            break;
        }

        func(lpEntry, userdata);
    }

    ztl_unlock(&memdb->SeqMutex);
    return ZTL_MEMDB_OK;
}


ztl_seq_t ztl_memdb_last_entry_seq(ztl_memdb_t* memdb)
{
    ztl_spinlock(&memdb->SeqMutex, 1, 2048);
    ztl_seq_t seq = ztl_memdb_last_seq(memdb);
    ztl_unlock(&memdb->SeqMutex);
    return seq;
}

ztl_seq_t ztl_memdb_start_seq(ztl_memdb_t* memdb)
{
    return *memdb->pStartIndex;
}

bool ztl_memdb_set_start_seq(ztl_memdb_t* memdb, ztl_seq_t start_seq)
{
    DBDebugInfo(DBDbgFd, "SetStartSequence:" SEQ_FMT ", current data count:%d\n",
        start_seq, ztl_memdb_count(memdb));

    ztl_spinlock(&memdb->SeqMutex, 1, 2048);
    if (0 == ztl_array_size(&memdb->SeqVec))
    {
        *memdb->pStartIndex = start_seq;
        ztl_unlock(&memdb->SeqMutex);
        return true;
    }

    ztl_unlock(&memdb->SeqMutex);
    return false;
}

void ztl_memdb_initialize(ztl_memdb_t* memdb, uint8_t* apAddr, uint64_t aSize)
{
    ztl_memdb_clear_error(memdb);
    memdb->BaseAddr = apAddr;
    memdb->CurAddr = memdb->BaseAddr;
    memdb->EndAddr = memdb->BaseAddr + aSize;

    ztl_seq_t lStartIndex = *memdb->pStartIndex;
    ztl_seq_t lMaxIndex = *memdb->pMaxIndex;
    MemHeadInfo* lpHI = (MemHeadInfo*)memdb->CurAddr;
    uint8_t* lpCurAddr = memdb->CurAddr + MemSize(lpHI->Length);

    // scan the memory
    while ((lpCurAddr + MHEAD_SIZE) < memdb->EndAddr)
    {
        lpHI = (MemHeadInfo*)lpCurAddr;
        if (0 == lpHI->Length)
        {
#if MEMDB_DEBUG
            assert(0 == lpHI->Sequence);
            assert(0 == lpHI->Flag);
            assert(0 == lpHI->CheckSum);
            assert(0 == lpHI->PreLength);
#endif
            bool lVerify = (0 != lpHI->Sequence) || (0 != lpHI->Flag) || (0 != lpHI->PreLength);
            if (lVerify)
            {
                int n = snprintf(memdb->ErrorMsg, sizeof(memdb->ErrorMsg) - 1,
                    "Init: Length is 0, but Sequence:" SEQ_FMT ", Flag:%d, PreLength:%d\n",
                    lpHI->Sequence, lpHI->Flag, lpHI->PreLength);
                memdb->ErrorMsg[n] = '\0';
                memdb->ErrorID = ZTL_MEMDB_InvalidData;
            }
            break;
        }

        if (lpHI->CheckSum != CalcCheckSum(lpHI))
        {
            DBDebugInfo(DBDbgFd, "Init: CheckSum error at %p, Length:%d,%d, CheckSum:%d, Sequence:" SEQ_FMT "\n",
                (void*)lpCurAddr,
                lpHI->Length, lpHI->PreLength, lpHI->CheckSum, lpHI->Sequence);
            //assert(0);
        }

        if (lpHI->Sequence < lStartIndex || lpHI->Sequence > lMaxIndex)
        {
            // rollback memory state for merge them later
            if (lpHI->Sequence != 0)
            {
                DBDebugInfo(DBDbgFd, "Init: invalid Sequence:" SEQ_FMT ", Flag:%d, but StartIndex:" SEQ_FMT ", MaxIndex:" SEQ_FMT "\n",
                    lpHI->Sequence, lpHI->Flag, lStartIndex, lMaxIndex);
                lpHI->Sequence = 0;
                ClearBuffer(lpHI);
            }

            if (MF_DEALLOC != lpHI->Flag)
            {
                lpHI->Flag = MF_DEALLOC;
                ClearBuffer(lpHI);
            }

            if (lpHI->PreLength == 0 && memdb->CurAddr != memdb->BaseAddr)
            {
                lpHI->PreLength = ((MemHeadInfo*)memdb->CurAddr)->Length;
                DBDebugInfo(DBDbgFd, "Init: PreLength is 0 and changed as %d at %p\n",
                    lpHI->PreLength, (void*)lpHI);
            }
        }
        else
        {
            if (MF_FILLED != lpHI->Flag)
            {
                DBDebugInfo(DBDbgFd, "Init: invalid Sequence:" SEQ_FMT ", Flag:%d which is not MF_FILLED\n",
                    lpHI->Sequence, lpHI->Flag);
                lpHI->Sequence = 0;
                ClearBuffer(lpHI);
                lpHI->Flag = MF_DEALLOC;

                memdb->CurAddr = lpCurAddr;
                lpCurAddr += MemSize(lpHI->Length);
                continue;
            }

            // store sequence to current process space
            int64_t lTempN = lpHI->Sequence - ztl_memdb_last_seq(memdb);
            for (int64_t i = 0; i < lTempN; ++i)
            {
                ztl_array_pop_back(&memdb->SeqVec);
            }

            uint32_t lPos = (uint32_t)(lpHI->Sequence - lStartIndex);
            (void)lPos;
            // memdb->SeqVec[lPos] = lpCurAddr + MHEAD_SIZE; // TODO:
            memdb->TotalBytes += lpHI->Length;
        }

        // next node
        memdb->CurAddr = lpCurAddr;
        lpCurAddr += MemSize(lpHI->Length);
    }//while

    lMaxIndex = lStartIndex + ztl_array_size(&memdb->SeqVec) - 1;

    if (*memdb->pMaxIndex != lMaxIndex)
    {
        DBDebugInfo(DBDbgFd, "Init: new MaxIndex:" SEQ_FMT ", old MaxIndex:" SEQ_FMT "\n",
            lMaxIndex, *memdb->pMaxIndex);
        //assert(*m_pMaxIndex > lMaxIndex);
        *memdb->pMaxIndex = lMaxIndex;
    }

    // try fast visit left memory
    const int32_t lPageBytes = 4096;
    int n = 0;
    if ((memdb->EndAddr - lpCurAddr) > lPageBytes)
    {
        while ((lpCurAddr + lPageBytes) < memdb->EndAddr && (n++ < 10240))
        {
            if (0 != *(uint32_t*)lpCurAddr) {
                memset(lpCurAddr, 0, (uint32_t)(lPageBytes));
            }
            lpCurAddr += lPageBytes;
        }
    }

    // merge memory from backend
    memdb->CurAddr = MergeMemory(memdb->BaseAddr, memdb->CurAddr);
}
