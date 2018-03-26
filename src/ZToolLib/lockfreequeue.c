#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "lockfreequeue.h"
#include "ztl_atomic.h"

#ifdef _MSC_VER
#include <windows.h>
#else

#define __USE_SVID
#include <sys/shm.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>
#include <errno.h>
#endif//_MSC_VER

/// the index to operate queue
struct queue_op_data_st
{
    uint32_t read_index;
    uint32_t write_index;
    uint32_t max_read_index;
    uint32_t array_size;
};
typedef struct queue_op_data_st queue_op_data_t;

/// an array lock free queue struct
struct lfqueue_st
{
    volatile uint32_t*	rdindex;			// where the next element where be extracted from
    volatile uint32_t*	wtindex;			// where a new element will be inserted to

    // @brief maximum read index for multiple producer queues
    /// If it's not the same as m_writeIndex it means
    /// there are writes pending to be "committed" to the queue, that means,
    /// the place for the data was reserved (the index in the array) but
    /// data is still not in the queue, so the thread trying to read will have
    /// to wait for those other threads to save the data into the queue
    ///
    /// note this index is only used for MultipleProducerThread queues
    volatile uint32_t*	maxreadindex;

#ifdef _WIN32
    HANDLE   handle;
#else
    int      handle;
#endif//_WIN32
    void*    paddr;

    uint32_t size;              // the queue size
    void**  arrdata;            // array to keep the elements
};


/// @brief calculate the index in the circular array that corresponds
/// to a particular "count" value
#define TO_INDEX(idx,size) ((idx) < (size) ? (idx) : ((idx)-(size)))


static void lfqueue_initmember(lfqueue_t* que, uint32_t size)
{
    queue_op_data_t* lpMember = (queue_op_data_t*)que->paddr;
    que->rdindex = &lpMember->read_index;
    que->wtindex = &lpMember->write_index;
    que->maxreadindex = &lpMember->max_read_index;

    if (lpMember->array_size == 0)
    {
        lpMember->array_size = size;
    }
    if (lpMember->array_size != size)
    {
        printf("error size: memory %d but passed %d\n", lpMember->array_size, size);
    }
    que->size = size;
    que->arrdata = (void**)((char*)que->paddr + sizeof(queue_op_data_t));
}

static bool lfqueue_memory(lfqueue_t* que, const char* quename, uint32_t size)
{
    void* lpBase;
    uint32_t lMapSize = (size + 1) * sizeof(void*) + sizeof(queue_op_data_t);

#ifdef _WIN32
    bool lByCreate = false;
    HANDLE lpMap;
    lpMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, quename);
    if (NULL == lpMap)
    {
        lByCreate = true;
        lpMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, lMapSize, quename);
    }

    lpBase = MapViewOfFile(lpMap, FILE_MAP_ALL_ACCESS, 0, 0, lMapSize);
    if (NULL == lpBase)
    {
        return false;
    }
    if (lByCreate)
    {
        memset(lpBase, 0, size);
    }
    que->handle = lpMap;
#else

    int lShmKey = IPC_PRIVATE;
    if (quename != NULL && *quename)
        lShmKey = atoi(quename);

    int lHandle = shmget(lShmKey, lMapSize, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W);
    if (lHandle == -1 && errno == EEXIST)
    {
        lHandle = shmget(lShmKey, lMapSize, SHM_R | SHM_W);
    }

    if (-1 == lHandle)
    {
        perror("shmget");
        return false;
    }

    lpBase = shmat(lHandle, 0, 0);
    if (NULL == lpBase)
    {
        perror("shmat");
        return false;
    }
    que->handle = lHandle;
#endif//_WIN32

    que->paddr = lpBase;

    lfqueue_initmember(que, size);
    return true;
}

/// create lock free queue by size
lfqueue_t* lfqueue_create(unsigned int size, const char* quename/* = 0*/)
{
    lfqueue_t* que;
    que = (lfqueue_t*)malloc(sizeof(lfqueue_t));
    if (que == NULL)
        return NULL;

    if (!lfqueue_memory(que, quename, size))
    {
        free(que);
        return NULL;
    }
    return que;
}

/// push an item to the tail of the queue, return 0 if success
int lfqueue_push(lfqueue_t* que, void* pdata)
{
    if (que == NULL)
        return -1;

    uint32_t curWriteIndex;
    uint32_t curReadIndex;

    // find the index to be inserted to
    do
    {
        curWriteIndex = *que->wtindex;
        curReadIndex = *que->rdindex;

        // if queue is full
        if (TO_INDEX(curWriteIndex + 1, que->size) == curReadIndex)
            return -1;
    } while (!ztl_atomic_cas(que->wtindex, curWriteIndex, TO_INDEX(curWriteIndex + 1, que->size)));

    // save the data since the current write index is reserved for us
    que->arrdata[curWriteIndex] = pdata;

    // update the maximum read index after saving the data.
    // It wouldn't fail if there is only one producer thread intserting data into the queue.
    // It will failed once more than 1 producer threads because
    // the maxReadIndex_ should be done automic as the previous CAS
    while (!ztl_atomic_cas(que->maxreadindex, curWriteIndex, TO_INDEX(curWriteIndex + 1, que->size)))
    {
        ztl_sched_yield();
    }
    return 0;
}

/// pop an item at the head of the queue, return 0 if success
int lfqueue_pop(lfqueue_t* que, void** ppdata)
{
    if (que == NULL)
        return -1;

    uint32_t curMaxReadIndex;
    uint32_t curReadIndex;
    void* olddata = *ppdata;

    // find the valid index to be read
    do
    {
        // to ensure thread-safety when there is more than 1 producer threads
        // a second index is defined: maxReadIndex_
        curReadIndex = *que->rdindex;
        curMaxReadIndex = *que->maxreadindex;

        // if the queue is empty or
        // a producer thread has occupied space in the queue,
        // but is waiting to commit the data into it
        if (curReadIndex == curMaxReadIndex)
            return -1;

        // retrieve the data from the queue
        *ppdata = que->arrdata[curReadIndex];

        // we automic increase the readIndex_ using CAS operation
        if (ztl_atomic_cas(que->rdindex, curReadIndex, TO_INDEX(curReadIndex + 1, que->size)))
            return 0;

        // rollback the data
        *ppdata = olddata;

        // here, if failed retrieving the element off the queue
        // someone else is reading the element at curReadIndex before we perform CAS operation
        ztl_sched_yield();
    } while (true); // keep looping to try again

    // to avoid compile warning
    return -1;
}

/// get queue size which may be a vogus value
int lfqueue_size(lfqueue_t* que)
{
    if (que)
    {
        uint32_t curWriteIndex = *que->wtindex;
        uint32_t curReadIndex = *que->rdindex;

        // curWriteIndex may be at the front of curReadIndex
        if (curWriteIndex >= curReadIndex)
            return curWriteIndex - curReadIndex;
        else
            return (que->size + curWriteIndex - curReadIndex);
    }
    return 0;
}

/// @returns true if the queue is empty
bool lfqueue_empty(lfqueue_t* que)
{
    if (que == NULL)
        return true;
    return (que->wtindex == que->rdindex) ? true : false;
}

int lfqueue_release(lfqueue_t* que)
{
    if (que)
    {
#ifdef _WIN32
        CloseHandle(que->handle);
        UnmapViewOfFile(que->paddr);
#else
        shmdt(que->paddr);
        shmctl(que->handle, IPC_RMID, NULL);
#endif//_WIN32

        free(que);
    }
    return 0;
}
