#ifndef _LOCK_FREE_QUEUE_H_
#define _LOCK_FREE_QUEUE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* exported types */

/* a lock free queue based on a circular array,
 * supports multiple producers and multiple consumers
 */
typedef struct lfqueue_st lfqueue_t;


/* get minimum memory size for creating a lock free queue by queue size & elem size
 */
int64_t lfqueue_memory_size(uint32_t quesize, uint32_t elemsize);

/* @brief  create a queue by max queue size
 * @return the queue object
 * @note   recommend elemsize aligned to 4 or 8 bytes
 */
lfqueue_t* lfqueue_create(uint32_t quesize, uint32_t elemsize);

/* @brief  create a queue on the specified memory address
 * @param  memory size could be got by calling 'lfqueue_memory_size'
 * @return the queue object
 */
lfqueue_t* lfqueue_create2(uint32_t quesize, uint32_t elemsize, void* memory, int64_t memsize);


/* @brief  push an element's pointer at the tail of the queue
 * @param  the element to insert in the queue
 * @return 0 if the element was inserted in the queue. other if the queue was full
 * @note   element size was specified when 'lfqueue_create',
 *         and the queue internally will do a copy of pdata (no keep the pointer)
 *         use 'lfqueue_pop' to retrieve elem
 */
int lfqueue_push(lfqueue_t* que, const void* pdata);

/* @brief  pop an element's address pointer from the queue at the head
 * @param  the element to pop out, output the elem's address
 * @return 0 if the element was successfully extracted from the queue. 
 *          other if the queue was empty
 * @note   must reserve at least elemsize space for pdata
 */
int lfqueue_pop(lfqueue_t* que, void** ppdata);

/* @brief  get the data of the queue head
 * @return 0 is success
 */
int lfqueue_head(lfqueue_t* que, void** ppdata);


/* @return the queue size which might be bogus value
 */
uint32_t lfqueue_size(lfqueue_t* que);

/* @return the queue max size
 */
uint32_t lfqueue_max_size(lfqueue_t* que);

/* @return the queue's element size
 */
uint32_t lfqueue_elem_size(lfqueue_t* que);

/* @return true if the queue is empty or not
 */
bool lfqueue_empty(lfqueue_t* que);

/* @brief free the queue object
 */
int lfqueue_release(lfqueue_t* que);

#ifdef __cplusplus
}//__cplusplus
#endif

#endif//_LOCK_FREE_QUEUE_H_
