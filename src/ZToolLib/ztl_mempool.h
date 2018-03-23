#ifndef _ZTL_MEM_POOL_H_
#define _ZTL_MEM_POOL_H_

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/// the exported types
typedef struct ztl_mempool_st ztl_mempool_t;

/// create a memory pool which is thread safety
/// @nEntitySize each entity size to alloc
/// @nInitCount  how many entity count when init the pool
/// @aAutoExpand whether auto expand memory when used one block
ztl_mempool_t* ztl_mp_create(int nEntitySize, int nInitCount, int aAutoExpand);

/// destroy the memory pool
void ztl_mp_release(ztl_mempool_t* mp);

/// alloc memory from pool
char* ztl_mp_alloc(ztl_mempool_t* mp);

/// free memory to pool
void ztl_mp_free(ztl_mempool_t* mp, void* paddr);

/// get entity size
int ztl_mp_entity_size(ztl_mempool_t* mp);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//_ZTL_MEM_POOL_H_
