#ifndef _ZTL_MEM_POOL_H_
#define _ZTL_MEM_POOL_H_

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
}ztl_enum__create_t;
 
typedef enum {
	ztl_read_only       = 0x01,
	ztl_read_write      = 0x02,
	ztl_copy_on_write	= 0x03,
	ztl_read_private	= 0x04
}ztl_enum__mode_t;


/// Tries to open a shared memory object with name "name", with the access mode "mode",
/// which will map to a file is apName is not empty, otherwise open an anonymous shared memory
/// We could specify whether work with hugepage
ztl_shm_t* ztl_shm_create(const char* apName, int aOpenOrCreate, int aAccessMode, bool aIsUseHugepage);

/// Erases a shared memory object from the system.
bool ztl_shm_remove(ztl_shm_t* zshm, const char* apName);

/// Sets the size of the shared memory mapping
int ztl_shm_truncate(ztl_shm_t* zshm, uint64_t aSize);

/// Map the whole shared memory to this process
int ztl_shm_map_region(ztl_shm_t* zshm, int aAccessMode);

/// Detach address from process
int ztl_shm_detach(ztl_shm_t* zshm);

/// Get base process address of shared memory if mapped successful before
void* ztl_shm_get_address(ztl_shm_t* zshm) const;

/// Flush the data in memory to file, aIsAsyncFlag is async flush or not
int ztl_shm_flush_to_file(ztl_shm_t* zshm, bool aIsAsyncFlag, void* apAddr, uint64_t aSize);


/// Returns the name of the shared memory object.
const char* ztl_shm_get_name(ztl_shm_t* zshm);

/// Returns true size of the shared memory object
uint64_t ztl_shm_get_size(ztl_shm_t* zshm);

/// Returns access mode
int ztl_shm_get_mode(ztl_shm_t* zshm);

class HShareMemory
{
public:
	typedef enum {
		open_only      = 1,
		create_only    = 2,
		open_or_create = 3,
	} create_enum_t;

	typedef enum {
		read_only     = 0x01,
		read_write    = 0x02,
		copy_on_write = 0x03,
		read_private  = 0x04
	} mode_t;

public:
	/// Tries to open a shared memory object with name "name", with the access mode "mode",
	/// which will map to a file is apName is not empty, otherwise open an anonymous shared memory
	/// We could specify whether work with hugepage
	HShareMemory(const char* apName, int aOpenOrCreate, int aAccessMode, bool aIsUseHugepage = false);

	/// Tries to open a segment of shared memory, 
	/// if apNameKey is empty, an anonymous shared memory opened,
	/// We could specify whether work with hugepage
	HShareMemory(const char* apNameKey, int aOpenOrCreate, const bool aIsUseHugepage = false);

	virtual ~HShareMemory();

	/// Erases a shared memory object from the system.
	static bool Remove(const char* apName);

public:
	/// Sets the size of the shared memory mapping
	int Truncate(uint64_t aSize);

	/// Map the whole shared memory to this process
	int MapRegion(int aAccessMode);

	/// Detach address from process
	int Detach();

	/// Get base process address of shared memory if mapped successful before
	void* GetAddress() const;

	/// Flush the data in memory to file, aIsAsyncFlag is async flush or not
	int FlushToFile(bool aIsAsyncFlag, void* apAddr = NULL, uint64_t aSize = 0);

public:
	/// Returns the name of the shared memory object.
	const char* GetName() const;

	/// Returns true size of the shared memory object
	uint64_t GetSize() const;

	/// Returns access mode
	int GetMode() const;

protected:
	int PrivOpenOrCreate(const char* apName, int aOpenOrCreate, int aAccessMode);

private:
	string   m_Name;
	uint64_t m_Size;
	bool     m_Hugepage;
	int      m_OpenOrCreate;
	int      m_Mode;
	int      m_ShmType;		// anonymous, file-map, share-segment
#ifdef _WIN32
	void*    m_Handle;
#else
	int      m_Handle;		// the file handle or anonymous shm handle
#endif
	void*    m_FileMapping;
	void*    m_pAddress;
};

