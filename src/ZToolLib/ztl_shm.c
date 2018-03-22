#include <string.h>
#include <stdio.h>

#include "ztl_shm.h"

#ifdef _MSC_VER
#include <Windows.h>
#include "win32_api_sim.hpp"
#else

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>

#define INVALID_HANDLE_VALUE (-1)

#endif//_MSC_VER


struct ztl_shm_st
{
	char        m_Name[256];
	uint64_t    m_Size;
	bool        m_Hugepage;
	int         m_OpenOrCreate;
	int         m_Mode;
	int         m_ShmType;		// anonymous, file-map, share-segment
#ifdef _MSC_VER
	void*       m_Handle;
#else
	int         m_Handle;		// the file handle or anonymous shm handle
#endif//_MSC_VER
	void*       m_FileMapping;
	void*       m_pAddress;
};

#ifdef __cplusplus
namespace ipcdetails
{
#endif//__cplusplus

#ifdef _MSC_VER
bool get_file_size(void* aHandle, uint64_t& aSize)
{
	return 0 != GetFileSizeEx(aHandle, (LARGE_INTEGER*)&aSize);
}

bool set_file_pointer_ex(void* aHandle, int64_t aDistance, int64_t* apNewFilePtr, unsigned long aMoveMethod)
{
	LARGE_INTEGER ld;
	ld.QuadPart = aDistance;
	return 0 != SetFilePointerEx(aHandle, ld, (LARGE_INTEGER*)apNewFilePtr, aMoveMethod);
}

bool set_end_of_file(void* aHandle)
{
	return 0 != SetEndOfFile(aHandle);
}

void write_file(void* aHandle, const char* apData, long aSize, unsigned long* apWritten)
{
	WriteFile(aHandle, apData, aSize, apWritten, 0);
}

bool create_directory(const char* apPath)
{
	if (strcmp(apPath, "./") == 0 ||
		strcmp(apPath, ".\\") == 0) {
		return true;
	}
    return 0 != CreateDirectoryA(apPath, NULL);
}

bool get_os_opencreated(int& aOpenOrCreate)
{
	switch (aOpenOrCreate)
	{
	case ztl_open_only: aOpenOrCreate = OPEN_EXISTING; break;
	case ztl_create_only:
	case ztl_open_or_create: aOpenOrCreate = OPEN_ALWAYS; break;
	default: aOpenOrCreate = OPEN_ALWAYS;
	}
	return true;
}

bool get_os_accessmode(int& aAccessMode)
{
	switch (aAccessMode)
	{
	case HShareMemory::read_only:  aAccessMode = GENERIC_READ; break;
	case HShareMemory::read_write:
	case HShareMemory::copy_on_write: aAccessMode = GENERIC_READ | GENERIC_WRITE; break;
	default: aAccessMode = GENERIC_READ | GENERIC_WRITE;
	}
	return true;
}

#else /* linux platform */

bool get_file_size(const char* apPath, uint64_t& aSize)
{
	struct stat lStat;
	if (stat(apPath, &lStat) < 0) {
		return false;
	}
	else {
		aSize = lStat.st_size;
	}
	return true;
}
int64_t set_file_pointer_ex(int aHandle, int64_t aDistance, int64_t* apNewFilePtr, unsigned long aMoveMethod)
{
	(void)apNewFilePtr;
	(void)aMoveMethod;
	return lseek(aHandle, 0, aDistance);
}

void write_file(int aHandle, const char* apData, long aSize, unsigned long* apWritten)
{
	long rv;
	rv = write(aHandle, apData, aSize);
	if (rv >= 0) {
		*apWritten = rv;
	}
	else {
		*apWritten = 0;
	}
}

bool create_directory(const char* apPath)
{
    int lrv = mkdir(apPath, 0755);
    if (lrv < 0)
    {
        char lBuf[512];
        sprintf(lBuf, "mkdir -p %s", apPath);
        system(lBuf);
        lrv = mkdir(apPath, 0755);
    }
    if (lrv < 0 && errno == EEXIST) {
        return true;
    }
    return false;
}

bool get_os_opencreated(int& aOpenOrCreate)
{
	switch (aOpenOrCreate)
	{
	case HShareMemory::open_only: aOpenOrCreate = O_RDONLY; break;
	case HShareMemory::create_only:
	case HShareMemory::open_or_create: aOpenOrCreate = O_CREAT | O_RDWR; break;
	default: aOpenOrCreate = O_CREAT | O_RDWR;
	}
	return true;
}

bool get_os_accessmode(int& aAccessMode)
{
	switch (aAccessMode)
	{
	case HShareMemory::read_only:  aAccessMode = PROT_READ; break;
	case HShareMemory::read_write:
	case HShareMemory::copy_on_write: aAccessMode = PROT_WRITE | PROT_READ; break;
	default: aAccessMode = PROT_READ;
	}
	return true;
}

int get_key_from_file(const char* filename)
{
	bool bOnlyDigit = true;
	int lch, lval = 0;
	while ((lch = *filename++) != '\0') {
		lval ^= lch;
		if (!isdigit(lch))
			bOnlyDigit = false;
	}

	if (bOnlyDigit) {
		lval = atoi(filename);
	}
	else {
		lval &= 0xFF;
		if (lval == 0) lval = 1;
	}
	return ftok(filename, lval);
}

#endif

void get_directory(std::string& aDirection, const char* apFilePath)
{
	const char* lpFileName = apFilePath + strlen(apFilePath);
	while (lpFileName != apFilePath)
	{
		if (*lpFileName == '/' || *lpFileName == '\\')
		{
			aDirection = std::string(apFilePath, lpFileName - apFilePath);
			break;
		}
		--lpFileName;
	}
}
void get_filename(std::string& aFileName, const char* apFilePath)
{
    const char* lpFileName = apFilePath + strlen(apFilePath);
    while (lpFileName != apFilePath)
    {
        if (*lpFileName == '/' || *lpFileName == '\\')
        {
            lpFileName++;
            break;
        }
        --lpFileName;
    }
    aFileName = std::string(lpFileName);
}

#ifdef _MSC_VER
bool truncate_file(void* aHandle, size_t aRemaningSize)
#else
bool truncate_file(int aHandle, size_t aRemaningSize)
#endif//_MSC_VER
{
	const std::size_t lDataSize = 1024;
	static char lData[lDataSize];

	std::size_t lWriteSize = 0;
	unsigned long lWritten = 0;
	while (aRemaningSize > 0)
	{
		lWriteSize = lDataSize < aRemaningSize ? lDataSize : aRemaningSize;
		lWritten = 0;
		write_file(aHandle, lData, (long)lWriteSize, &lWritten);
		if (lWritten != lWriteSize)
		{
			return false;
		}

		aRemaningSize -= lWriteSize;
	}//while
	return true;
}

#ifdef __cplusplus
}//namespace ipcdetails
#endif

typedef enum {
	ZTL_SHT_ANON    = 0,
	ZTL_SHT_FILEMAP = 1,
	ZTL_SHT_SEGMENT = 2,
	ZTL_SHT_SHMOPEN = 3
}ztl_shm_type_t;


ztl_shm_t* ztl_shm_create(const char* apName, int aOpenOrCreate, int aAccessMode, bool aIsUseHugepage)
{
	if (NULL != apName) 
	{
		m_Name = apName;
	}

	if (!m_Name.empty())
	{
		std::string sDirectory;
		ipcdetails::get_directory(sDirectory, apName);

#ifdef __linux__
		if (sDirectory.empty())
		{
			// shm_open not support with hugetlb
			m_ShmType = m_Hugepage ? ST_SHM_SEGMENT : ST_SHM_SHMOPEN;
		}
		else
		{
			m_ShmType = ST_SHM_FILEMAP;
			ipcdetails::create_directory(sDirectory.c_str());
		}
#else
		m_ShmType = ST_SHM_FILEMAP;
		ipcdetails::create_directory(sDirectory.c_str());
#endif

		PrivOpenOrCreate(m_Name.c_str(), aOpenOrCreate, aAccessMode);
	}
	else
	{
		// TODO: check windows system could work with empty name
		m_ShmType  = ST_SHM_ANON;
	}
}

HShareMemory::HShareMemory(const char* apNameKey, int aOpenOrCreate, const bool aIsUseHugepage/* = false*/)
	: m_Size(0), m_Hugepage(aIsUseHugepage), 
	m_OpenOrCreate(aOpenOrCreate), m_ShmType(ST_SHM_SEGMENT),
	m_Handle(INVALID_HANDLE_VALUE), m_pAddress(NULL)
{
	if (NULL != apNameKey)
	{
		m_Name = apNameKey;
	}

	if (!m_Name.empty())
	{
		m_ShmType = ST_SHM_SEGMENT;
	}
	else
	{
		m_ShmType  = ST_SHM_ANON;
	}
	m_Mode = read_write;
}

HShareMemory::~HShareMemory()
{
#ifdef _WIN32
	if (NULL != m_pAddress) {
		UnmapViewOfFile(m_pAddress);
	}
	if (NULL != m_FileMapping) {
		CloseHandle(m_FileMapping);
	}
	if (m_Handle != INVALID_HANDLE_VALUE) {
		CloseHandle(m_Handle);
	}
#else
	if (ST_SHM_SEGMENT != m_ShmType)
	{
		if (NULL != m_pAddress) {
			munmap(m_pAddress, m_Size);
		}

		if (INVALID_HANDLE_VALUE != m_Handle) {
			close(m_Handle);
		}
	}
	else
	{
		if (NULL != m_pAddress) {
			shmdt(m_pAddress);
		}

		if (INVALID_HANDLE_VALUE != m_Handle) {
			shmctl(m_Handle, IPC_RMID, NULL);
		}
	}
#endif//_WIN32
}

/* static function to remmove file */
bool HShareMemory::Remove(const char* apName)
{
#ifdef _WIN32
	return ipcdetails::unlink_file(apName);
#else
	string sDirectory;
	ipcdetails::get_directory(sDirectory, apName);
	if (sDirectory.empty())
	{
		return 0 == shm_unlink(apName);
	}
	else
	{
		// remove the file
		return 0 == unlink(apName);
	}
#endif
}

int HShareMemory::Truncate(uint64_t aSize)
{

#ifdef _WIN32
	uint64_t lFileSize;
	if (ST_SHM_ANON == m_ShmType || ST_SHM_SEGMENT == m_ShmType) {
		m_Size = aSize;
		return 0;
	}

	if (!ipcdetails::get_file_size(m_Handle, lFileSize))
	{
		//perror("get_file_size");
		return -2;
	}

	//avoid unused variable warnings in 32 bit systems
	if (aSize > uint64_t(INT_MAX))
	{
		return -3;
	}

	if (aSize > lFileSize)
	{
		if (!ipcdetails::set_file_pointer_ex(m_Handle, lFileSize, NULL, 0)) {
			return -4;
		}

		//We will write zeros in the end of the file
		//since set_end_of_file does not guarantee this

		std::size_t lRemaining = size_t(aSize - lFileSize);
		ipcdetails::truncate_file(m_Handle, lRemaining);
	}
	else
	{
		if (!ipcdetails::set_file_pointer_ex(m_Handle, aSize, NULL, 0)) {
			return -6;
		}

		if (!ipcdetails::set_end_of_file(m_Handle)) {
			return -7;
		}
	}
#else
	if (ST_SHM_SHMOPEN == m_ShmType || ST_SHM_FILEMAP == m_ShmType)
	{
		if (m_OpenOrCreate != open_only && 0 != ftruncate(m_Handle, aSize))
		{
			perror("ftruncate");
			return -1;
		}
	}
#endif

	m_Size = aSize;
	return 0;
}


int HShareMemory::MapRegion(int aAccessMode)
{
	if (m_pAddress != NULL)
	{
		return 0;
	}

	int lProtMode;

#ifdef _WIN32

	// if open handle failed before
	if (INVALID_HANDLE_VALUE == m_Handle && ST_SHM_FILEMAP == m_ShmType)
	{
		return -1;
	}

	int lMapAccess = FILE_MAP_ALL_ACCESS;
	switch (aAccessMode) 
	{
	case read_only:  
		lProtMode = PAGE_READONLY;
		lMapAccess = FILE_MAP_READ;
		break;
	case read_write:
	case copy_on_write:
		lProtMode = PAGE_READWRITE;
		lMapAccess = FILE_MAP_ALL_ACCESS;
		break;
	default: 
		return -2;
	}

	LARGE_INTEGER lInteger;
	lInteger.QuadPart = m_Size;
	
	HANDLE lpMapping = OpenFileMappingA(lMapAccess, FALSE, m_Name.c_str());
	if (NULL == lpMapping && m_OpenOrCreate != open_only) {
		lpMapping = CreateFileMappingA(m_Handle, NULL, lProtMode, lInteger.HighPart, lInteger.LowPart, m_Name.c_str());
	}

	if (NULL == lpMapping)
	{
		fprintf(stderr, "CreateFileMappingA [%s] failed %d!\n", m_Name.c_str(), GetLastError());
		return -3;
	}

	m_pAddress = MapViewOfFileEx(lpMapping,
		lMapAccess,
		0, 0,
		SIZE_T(m_Size), NULL);
	
	m_FileMapping = lpMapping;
	//CloseHandle(lpMapping);

#else

	if (INVALID_HANDLE_VALUE == m_Handle)
	{
		// if open handle failed before
		if ((ST_SHM_FILEMAP == m_ShmType) || ST_SHM_SHMOPEN == m_ShmType)
		{
			return -1;
		}
	}

	int lAccessMode = aAccessMode;
	ipcdetails::get_os_accessmode(lAccessMode);
	lProtMode = lAccessMode;

	if (ST_SHM_SEGMENT == m_ShmType)
	{
		int lShmKey = ipcdetails::get_key_from_file(m_Name.c_str());
		int lShmFlag = SHM_R;
		if (m_OpenOrCreate != open_only)
			lShmFlag |= IPC_CREAT | IPC_EXCL;
		if (lAccessMode != read_only)
			lShmFlag |= SHM_W;

		// with hugetlb flag
		if (m_Hugepage)
			lShmFlag |= SHM_HUGETLB;

		m_Handle = shmget(lShmKey, m_Size, lShmFlag);
		if (m_Handle == INVALID_HANDLE_VALUE && errno == EEXIST && m_OpenOrCreate != open_only)
		{
			lShmFlag = SHM_R;
			if (lAccessMode != read_only)
				lShmFlag |= SHM_W;
			if (m_Hugepage)
				lShmFlag |= SHM_HUGETLB;
			m_Handle = shmget(lShmKey, m_Size, lShmFlag);
		}

		if (INVALID_HANDLE_VALUE == m_Handle)
		{
			perror("shmget");
			return -1;
		}
		
		m_pAddress = shmat(m_Handle, 0, 0);
		if (NULL == m_pAddress)
		{
			perror("shmat");
			return -1;
		}
	}
	else
	{
		int lHandle = m_Handle;
		int lMapFlag = MAP_SHARED | MAP_NORESERVE;
		if (ST_SHM_ANON == m_ShmType && m_Hugepage)
		{
			lMapFlag |= MAP_HUGETLB | MAP_ANONYMOUS;
			lHandle = -1;
		}

		// map it to the address space
		m_pAddress = mmap(NULL, m_Size, lProtMode, lMapFlag, lHandle, 0);
		if (MAP_FAILED == m_pAddress)
		{
			perror("mmap");
			m_pAddress = NULL;
			return -1;
		}
	}
#endif
	return 0;
}

int HShareMemory::Detach()
{
	if (NULL != m_pAddress)
	{
#ifdef _WIN32
		UnmapViewOfFile(m_pAddress);
#else
		if (ST_SHM_SEGMENT == m_ShmType) {
			shmdt(m_pAddress);
		}
		else {
			munmap(m_pAddress, m_Size);
		}
#endif//_WIN32
		m_pAddress = NULL;
		return 0;
	}
	return -1;
}

void* HShareMemory::GetAddress() const
{
	return m_pAddress;
}

const char* HShareMemory::GetName() const
{
	return m_Name.c_str();
}

uint64_t HShareMemory::GetSize() const
{
	return m_Size;
}

int HShareMemory::GetMode() const
{
	return m_Mode;
}

int HShareMemory::FlushToFile(bool aIsAsyncFlag, void* apAddr/* = NULL*/, uint64_t aSize/* = 0*/)
{
	if (aSize > m_Size) {
		return -1;
	}

	if (0 == aSize)	aSize = m_Size;
	if (NULL == apAddr) apAddr = m_pAddress;
	
	if (apAddr < m_pAddress || 
		((char*)apAddr + aSize) > ((char*)m_pAddress + m_Size)) {
		return -1;
	}

	if (ST_SHM_FILEMAP == m_ShmType)
	{
#ifdef _WIN32
		FlushViewOfFile(apAddr, SIZE_T(aSize));
#else
		int lFlag = aIsAsyncFlag ? MS_ASYNC : MS_SYNC;
		return msync(apAddr, aSize, lFlag);
#endif//_WIN32
	}
	return 0;
}

int HShareMemory::PrivOpenOrCreate(const char* apName, int aOpenOrCreate, int aAccessMode)
{
	int lRet = 0;

#ifdef _WIN32

	ipcdetails::get_os_opencreated(aOpenOrCreate);
	ipcdetails::get_os_accessmode(aAccessMode);

	int lShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
	if (aOpenOrCreate != open_only) {
		lShareMode |= FILE_SHARE_DELETE;
	}

	if (ST_SHM_FILEMAP == m_ShmType)
	{
		m_Handle = CreateFileA(apName,
			aAccessMode,
			lShareMode,
			NULL, aOpenOrCreate, FILE_ATTRIBUTE_NORMAL, 0);

		if (INVALID_HANDLE_VALUE == m_Handle)
		{
			fprintf(stderr, "CreateFileA %s failed %d\n", apName, GetLastError());
			lRet = -1;
		}
	}

#else

	ipcdetails::get_os_opencreated(aOpenOrCreate);

	int lDoSysCall = 0;
	int lUnixPerm = 0664;

	if (ST_SHM_FILEMAP == m_ShmType)
	{
		lDoSysCall = 1;
		m_Handle = open(apName, aOpenOrCreate, lUnixPerm);
		
		// if successful, change real permissions
		if (m_Handle >= 0)
		{
			::fchmod(m_Handle, lUnixPerm);
		}
	}
	else
	{
		// try to create a share memory
		if (ST_SHM_SHMOPEN == m_ShmType)
		{
			lDoSysCall = 1;
			m_Handle = shm_open(apName, aOpenOrCreate, lUnixPerm);
		}
	}

	if (m_Handle < 0 && lDoSysCall)
	{
		char lErrBuf[512] = "";
		sprintf(lErrBuf, "open [%s] failed", apName);
		perror(lErrBuf);
		lRet = -1;
	}
#endif//_WIN32

	m_Mode = aAccessMode;
	return lRet;
}

