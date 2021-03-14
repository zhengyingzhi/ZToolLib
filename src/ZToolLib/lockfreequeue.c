#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lockfreequeue.h"
#include "ztl_atomic.h"
#include "ztl_utils.h"

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
    volatile uint32_t*  rdindex;            // where the next element where be extracted from
    volatile uint32_t*  wtindex;            // where a new element will be inserted to

    // @brief maximum read index for multiple producer queues
    /// If it's not the same as m_writeIndex it means
    /// there are writes pending to be "committed" to the queue, that means,
    /// the place for the data was reserved (the index in the array) but
    /// data is still not in the queue, so the thread trying to read will have
    /// to wait for those other threads to save the data into the queue
    ///
    /// note this index is only used for MultipleProducerThread queues
    volatile uint32_t*  maxreadindex;

    char*    arrdata;           // array to keep the elements

    int32_t  outside_mem;       // is out side memory
    uint32_t eltsize;           // element size
    uint32_t size;              // the queue size
};


/// @brief calculate the index in the circular array that corresponds
/// to a particular "count" value
#define TO_INDEX(idx,size) ((idx) < (size) ? (idx) : ((idx)-(size)))


static void lfqueue_initmember(lfqueue_t* que, uint32_t quesize, void* addr)
{
    queue_op_data_t* opdata;

    opdata              = (queue_op_data_t*)addr;
    que->rdindex        = &opdata->read_index;
    que->wtindex        = &opdata->write_index;
    que->maxreadindex   = &opdata->max_read_index;

    if (opdata->array_size == 0) {
        opdata->array_size = quesize;
    }

    if (opdata->array_size != quesize) {
        printf("error size: memory %d but passed %d\n", opdata->array_size, quesize);
    }

    que->size = quesize;
    que->arrdata = (char*)addr + sizeof(queue_op_data_t);
}


int64_t lfqueue_memory_size(uint32_t quesize, uint32_t elemsize)
{
    int64_t size;
    elemsize = ztl_align(elemsize, sizeof(void*));

    size = sizeof(queue_op_data_t) + (quesize + 1) * elemsize;
    size = ztl_align(size, 64);

    return size;
}

lfqueue_t* lfqueue_create(uint32_t quesize, uint32_t elemsize)
{
    lfqueue_t*  que;
    char*       addr;
    int64_t     memsize;

    que = (lfqueue_t*)malloc(sizeof(lfqueue_t));
    if (que == NULL) {
        return NULL;
    }
    memset(que, 0, sizeof(lfqueue_t));

    memsize = lfqueue_memory_size(quesize, elemsize);
    addr = (char*)calloc(1, (size_t)memsize);
    if (addr == NULL) {
        free(que);
        return NULL;
    }

    que->outside_mem = 0;
    que->eltsize = elemsize;

    lfqueue_initmember(que, quesize, addr);

    return que;
}

lfqueue_t* lfqueue_create2(uint32_t quesize, uint32_t elemsize, void* memory, int64_t memsize)
{
    lfqueue_t*  que;

    if (memsize < lfqueue_memory_size(quesize, elemsize)) {
        return NULL;
    }

    que = (lfqueue_t*)malloc(sizeof(lfqueue_t));
    if (que == NULL) {
        return NULL;
    }
    memset(que, 0, sizeof(lfqueue_t));

    que->outside_mem = 1;
    que->eltsize = elemsize;

    lfqueue_initmember(que, quesize, memory);

    return que;
}


int lfqueue_push(lfqueue_t* que, const void* pdata)
{
    char*    dstaddr;
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

    // save the data pointer since the current write index is reserved for us
    dstaddr = que->arrdata + que->eltsize * curWriteIndex;
    ztlncpy(dstaddr, pdata, que->eltsize);

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


int lfqueue_pop(lfqueue_t* que, void** ppdata)
{
    uint32_t curMaxReadIndex;
    uint32_t curReadIndex;
    char*    srcaddr;

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

        // retrieve the data pointer from the queue
        srcaddr = que->arrdata + que->eltsize * curReadIndex;
        // memcpy(ppdata, srcaddr, que->eltsize);
        ztlncpy(ppdata, srcaddr, que->eltsize);

        // we automic increase the readIndex_ using CAS operation
        if (ztl_atomic_cas(que->rdindex, curReadIndex, TO_INDEX(curReadIndex + 1, que->size)))
            return 0;

        // here, if failed retrieving the element off the queue
        // someone else is reading the element at curReadIndex before we perform CAS operation
        ztl_sched_yield();
    } while (true); // keep looping to try again

    // to avoid compile warning
    return -1;
}


int lfqueue_head(lfqueue_t* que, void** ppdata)
{
    uint32_t curMaxReadIndex;
    uint32_t curReadIndex;
    char*    srcaddr;

    curReadIndex = *que->rdindex;
    curMaxReadIndex = *que->maxreadindex;

    // empty or not
    if (curReadIndex == curMaxReadIndex) {
        return -1;
    }

    // retrieve the data pointer from the queue
    srcaddr = que->arrdata + que->eltsize * curReadIndex;
    // memcpy(ppdata, srcaddr, que->eltsize);
    ztlncpy(ppdata, srcaddr, que->eltsize);
    return 0;
}


uint32_t lfqueue_size(lfqueue_t* que)
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

uint32_t lfqueue_max_size(lfqueue_t* que)
{
    return que->size;
}

uint32_t lfqueue_elem_size(lfqueue_t* que)
{
    return que->eltsize;
}

bool lfqueue_empty(lfqueue_t* que)
{
    if (que == NULL)
        return true;
    return (*que->wtindex == *que->rdindex) ? true : false;
}

int lfqueue_release(lfqueue_t* que)
{
    char* lpDataBegin;
    if (que)
    {
        if (que->arrdata && !que->outside_mem) {
            lpDataBegin = (char*)que->arrdata - sizeof(queue_op_data_t);
            free(lpDataBegin);
        }

        free(que);
    }
    return 0;
}
