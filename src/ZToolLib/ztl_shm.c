#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include "ztl_shm.h"

#ifdef _MSC_VER
#include <Windows.h>
#include "ztl_win32_ipc.h"

#else

#include <errno.h>
#include <unistd.h>
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
bool get_file_size(void* aHandle, uint64_t* apSize)
{
    return 0 != GetFileSizeEx(aHandle, (LARGE_INTEGER*)apSize);
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

bool get_os_opencreated(int* aOpenOrCreate)
{
    switch (*aOpenOrCreate)
    {
    case ztl_open_only:      *aOpenOrCreate = OPEN_EXISTING; break;
    case ztl_create_only:
    case ztl_open_or_create: *aOpenOrCreate = OPEN_ALWAYS; break;
    default: *aOpenOrCreate = OPEN_ALWAYS;
    }
    return true;
}

bool get_os_accessmode(int* aAccessMode)
{
    switch (*aAccessMode)
    {
    case ztl_read_only:  *aAccessMode = GENERIC_READ; break;
    case ztl_read_write:
    case ztl_copy_on_write: *aAccessMode = GENERIC_READ | GENERIC_WRITE; break;
    default: *aAccessMode = GENERIC_READ | GENERIC_WRITE;
    }
    return true;
}

#else /* linux platform */

bool get_file_size(const char* apPath, uint64_t* apSize)
{
    struct stat lStat;
    if (stat(apPath, &lStat) < 0) {
        return false;
    }
    else {
        *apSize = lStat.st_size;
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

bool get_os_opencreated(int* aOpenOrCreate)
{
    switch (*aOpenOrCreate)
    {
    case ztl_open_only: *aOpenOrCreate = O_RDONLY; break;
    case ztl_create_only:
    case ztl_open_or_create: *aOpenOrCreate = O_CREAT | O_RDWR; break;
    default: *aOpenOrCreate = O_CREAT | O_RDWR;
    }
    return true;
}

bool get_os_accessmode(int* aAccessMode)
{
    switch (*aAccessMode)
    {
    case ztl_read_only:  *aAccessMode = PROT_READ; break;
    case ztl_read_write:
    case ztl_copy_on_write: *aAccessMode = PROT_WRITE | PROT_READ; break;
    default: *aAccessMode = PROT_READ;
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

void get_directory(char* aDirection, const char* apFilePath)
{
    const char* lpFileName = apFilePath + strlen(apFilePath);
    while (lpFileName != apFilePath)
    {
        if (*lpFileName == '/' || *lpFileName == '\\')
        {
            strncpy(aDirection, apFilePath, lpFileName - apFilePath);
            break;
        }
        --lpFileName;
    }
}

void get_filename(char* aFileName, const char* apFilePath)
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
    strcpy(aFileName, lpFileName);
}

#ifdef _MSC_VER
bool truncate_file(void* aHandle, size_t aRemaningSize)
#else
bool truncate_file(int aHandle, size_t aRemaningSize)
#endif//_MSC_VER
{
    const size_t lDataSize = 1024;
    static char lData[1024];

    size_t lWriteSize = 0;
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

static int _PrivOpenOrCreate(ztl_shm_t* zshm, const char* apName, int aOpenOrCreate, int aAccessMode);

ztl_shm_t* ztl_shm_create(const char* apName, int aOpenOrCreate, int aAccessMode, bool aIsUseHugepage)
{
    ztl_shm_t* lpshm;

    lpshm = (ztl_shm_t*)malloc(sizeof(ztl_shm_t));

    memset(lpshm, 0, sizeof(ztl_shm_t));
    lpshm->m_OpenOrCreate = aOpenOrCreate;
    lpshm->m_Mode = aAccessMode;
    lpshm->m_Hugepage = aIsUseHugepage;

    if (NULL != apName)
    {
        strncpy(lpshm->m_Name, apName, sizeof(lpshm->m_Name) - 1);
    }

    if (lpshm->m_Name[0])
    {
        char sDirectory[256] = "";
        get_directory(sDirectory, apName);

#ifdef __linux__
        if (!sDirectory[0])
        {
            // shm_open not support with hugetlb
            lpshm->m_ShmType = lpshm->m_Hugepage ? ZTL_SHT_SEGMENT : ZTL_SHT_SHMOPEN;
        }
        else
        {
            lpshm->m_ShmType = ZTL_SHT_FILEMAP;
            create_directory(sDirectory);
        }
#else
        lpshm->m_ShmType = ZTL_SHT_FILEMAP;
        create_directory(sDirectory);
#endif//__linux__

        _PrivOpenOrCreate(lpshm, lpshm->m_Name, aOpenOrCreate, aAccessMode);
    }
    else
    {
        // TODO: check windows system could work with empty name
        lpshm->m_ShmType = ZTL_SHT_ANON;
    }

    return lpshm;
}


ztl_shm_t* ztl_shm_segment_create(const char* apNameKey, int aOpenOrCreate, const bool aIsUseHugepage)
{
    ztl_shm_t* lpshm;

    lpshm = (ztl_shm_t*)malloc(sizeof(ztl_shm_t));

    memset(lpshm, 0, sizeof(ztl_shm_t));
    lpshm->m_OpenOrCreate = aOpenOrCreate;
    lpshm->m_Mode = ztl_read_write;
    lpshm->m_Hugepage = aIsUseHugepage;

    if (NULL != apNameKey)
    {
        strncpy(lpshm->m_Name, apNameKey, sizeof(lpshm->m_Name) - 1);
    }

    if (lpshm->m_Name[0])
    {
        lpshm->m_ShmType = ZTL_SHT_SEGMENT;
    }
    else
    {
        lpshm->m_ShmType = ZTL_SHT_ANON;
    }
    lpshm->m_Mode = ztl_read_write;

    return lpshm;
}

void ztl_shm_release(ztl_shm_t* zshm)
{
#ifdef _WIN32
    if (NULL != zshm->m_pAddress) {
        UnmapViewOfFile(zshm->m_pAddress);
    }
    if (NULL != zshm->m_FileMapping) {
        CloseHandle(zshm->m_FileMapping);
    }
    if (zshm->m_Handle != INVALID_HANDLE_VALUE) {
        CloseHandle(zshm->m_Handle);
    }
#else
    if (ZTL_SHT_SEGMENT != zshm->m_ShmType)
    {
        if (NULL != zshm->m_pAddress) {
            munmap(zshm->m_pAddress, zshm->m_Size);
        }

        if (INVALID_HANDLE_VALUE != zshm->m_Handle) {
            close(zshm->m_Handle);
        }
    }
    else
    {
        if (NULL != zshm->m_pAddress) {
            shmdt(zshm->m_pAddress);
        }

        if (INVALID_HANDLE_VALUE != zshm->m_Handle) {
            shmctl(zshm->m_Handle, IPC_RMID, NULL);
        }
    }
#endif//_WIN32

    free(zshm);
}

/* static function to remmove file */
bool ztl_shm_remove(const char* apName)
{
#ifdef _WIN32
    return unlink_file(apName);
#else
    char sDirectory[256];
    get_directory(sDirectory, apName);
    if (!sDirectory[0])
    {
        return 0 == shm_unlink(apName);
    }
    else
    {
        // remove the file
        return 0 == unlink(apName);
    }
#endif//_WIN32
}

int ztl_shm_truncate(ztl_shm_t* zshm, uint64_t aSize)
{

#ifdef _WIN32
    uint64_t lFileSize;
    if (ZTL_SHT_ANON == zshm->m_ShmType || ZTL_SHT_SEGMENT == zshm->m_ShmType) {
        zshm->m_Size = aSize;
        return 0;
    }

    if (!get_file_size(zshm->m_Handle, &lFileSize))
    {
        //perror("get_file_size");
        return -2;
    }

    //avoid unused variable warnings in 32 bit systems
    if (aSize > (uint64_t)INT_MAX)
    {
        return -3;
    }

    if (aSize > lFileSize)
    {
        if (!set_file_pointer_ex(zshm->m_Handle, lFileSize, NULL, 0)) {
            return -4;
        }

        //We will write zeros in the end of the file
        //since set_end_of_file does not guarantee this

        size_t lRemaining = (size_t)(aSize - lFileSize);
        truncate_file(zshm->m_Handle, lRemaining);
    }
    else
    {
        if (!set_file_pointer_ex(zshm->m_Handle, aSize, NULL, 0)) {
            return -6;
        }

        if (!set_end_of_file(zshm->m_Handle)) {
            return -7;
        }
    }
#else
    if (ZTL_SHT_SHMOPEN == zshm->m_ShmType || ZTL_SHT_FILEMAP == zshm->m_ShmType)
    {
        if (zshm->m_OpenOrCreate != ztl_open_only && 0 != ftruncate(zshm->m_Handle, aSize))
        {
            perror("ftruncate");
            return -1;
        }
    }
#endif//_WIN32

    zshm->m_Size = aSize;
    return 0;
}


int ztl_shm_map_region(ztl_shm_t* zshm, int aAccessMode)
{
    if (zshm->m_pAddress != NULL)
    {
        return 0;
    }

    int lProtMode;

#ifdef _WIN32

    // if open handle failed before
    if (INVALID_HANDLE_VALUE == zshm->m_Handle && ZTL_SHT_FILEMAP == zshm->m_ShmType)
    {
        return -1;
    }

    int lMapAccess = FILE_MAP_ALL_ACCESS;
    switch (aAccessMode)
    {
    case ztl_read_only:
        lProtMode = PAGE_READONLY;
        lMapAccess = FILE_MAP_READ;
        break;
    case ztl_read_write:
    case ztl_copy_on_write:
        lProtMode = PAGE_READWRITE;
        lMapAccess = FILE_MAP_ALL_ACCESS;
        break;
    default:
        return -2;
    }

    LARGE_INTEGER lInteger;
    lInteger.QuadPart = zshm->m_Size;

    HANDLE lpMapping = OpenFileMappingA(lMapAccess, FALSE, zshm->m_Name);
    if (NULL == lpMapping && zshm->m_OpenOrCreate != ztl_open_only) {
        lpMapping = CreateFileMappingA(zshm->m_Handle, NULL, lProtMode, lInteger.HighPart, lInteger.LowPart, zshm->m_Name);
    }

    if (NULL == lpMapping)
    {
        fprintf(stderr, "CreateFileMappingA [%s] failed %d!\n", zshm->m_Name, GetLastError());
        return -3;
    }

    zshm->m_pAddress = MapViewOfFileEx(lpMapping,
        lMapAccess,
        0, 0,
        (SIZE_T)(zshm->m_Size), NULL);

    zshm->m_FileMapping = lpMapping;
    //CloseHandle(lpMapping);

#else

    if (INVALID_HANDLE_VALUE == zshm->m_Handle)
    {
        // if open handle failed before
        if ((ZTL_SHT_FILEMAP == zshm->m_ShmType) || ZTL_SHT_SHMOPEN == zshm->m_ShmType)
        {
            return -1;
        }
    }

    int lAccessMode = aAccessMode;
    get_os_accessmode(&lAccessMode);
    lProtMode = lAccessMode;

    if (ZTL_SHT_SEGMENT == zshm->m_ShmType)
    {
        int lShmKey = get_key_from_file(zshm->m_Name);
        int lShmFlag = SHM_R;
        if (zshm->m_OpenOrCreate != ztl_open_only)
            lShmFlag |= IPC_CREAT | IPC_EXCL;
        if (lAccessMode != ztl_read_only)
            lShmFlag |= SHM_W;

        // with hugetlb flag
        if (zshm->m_Hugepage)
            lShmFlag |= SHM_HUGETLB;

        zshm->m_Handle = shmget(lShmKey, zshm->m_Size, lShmFlag);
        if (zshm->m_Handle == INVALID_HANDLE_VALUE && errno == EEXIST && 
            zshm->m_OpenOrCreate != ztl_open_only)
        {
            lShmFlag = SHM_R;
            if (lAccessMode != ztl_read_only)
                lShmFlag |= SHM_W;
            if (zshm->m_Hugepage)
                lShmFlag |= SHM_HUGETLB;
            zshm->m_Handle = shmget(lShmKey, zshm->m_Size, lShmFlag);
        }

        if (INVALID_HANDLE_VALUE == zshm->m_Handle)
        {
            perror("shmget");
            return -1;
        }

        zshm->m_pAddress = shmat(zshm->m_Handle, 0, 0);
        if (NULL == zshm->m_pAddress)
        {
            perror("shmat");
            return -1;
        }
    }
    else
    {
        int lHandle = zshm->m_Handle;
        int lMapFlag = MAP_SHARED;
        if (ZTL_SHT_ANON == zshm->m_ShmType && zshm->m_Hugepage)
        {
            lMapFlag |= MAP_HUGETLB | MAP_ANONYMOUS;
            lHandle = -1;
        }

        // map it to the address space
        zshm->m_pAddress = mmap(NULL, zshm->m_Size, lProtMode, lMapFlag, lHandle, 0);
        if (MAP_FAILED == zshm->m_pAddress)
        {
            perror("mmap");
            zshm->m_pAddress = NULL;
            return -1;
        }
    }
#endif
    return 0;
}

int ztl_shm_detach(ztl_shm_t* zshm)
{
    if (NULL != zshm->m_pAddress)
    {
#ifdef _WIN32
        UnmapViewOfFile(zshm->m_pAddress);
#else
        if (ZTL_SHT_SEGMENT == zshm->m_ShmType) {
            shmdt(zshm->m_pAddress);
        }
        else {
            munmap(zshm->m_pAddress, zshm->m_Size);
        }
#endif//_WIN32
        zshm->m_pAddress = NULL;
        return 0;
    }
    return -1;
}

void* ztl_shm_get_address(ztl_shm_t* zshm)
{
    return zshm->m_pAddress;
}

const char* ztl_shm_get_name(ztl_shm_t* zshm)
{
    return zshm->m_Name;
}

uint64_t ztl_shm_get_size(ztl_shm_t* zshm)
{
    return zshm->m_Size;
}

int ztl_shm_get_mode(ztl_shm_t* zshm)
{
    return zshm->m_Mode;
}

int ztl_shm_flush_to_file(ztl_shm_t* zshm, bool aIsAsyncFlag, void* apAddr, uint64_t aSize)
{
    if (aSize > zshm->m_Size) {
        return -1;
    }

    if (0 == aSize)	aSize = zshm->m_Size;
    if (NULL == apAddr) apAddr = zshm->m_pAddress;

    if (apAddr < zshm->m_pAddress ||
        ((char*)apAddr + aSize) >((char*)zshm->m_pAddress + zshm->m_Size)) {
        return -1;
    }

    if (ZTL_SHT_FILEMAP == zshm->m_ShmType)
    {
#ifdef _WIN32
        FlushViewOfFile(apAddr, (SIZE_T)(aSize));
#else
        int lFlag = aIsAsyncFlag ? MS_ASYNC : MS_SYNC;
        return msync(apAddr, aSize, lFlag);
#endif//_WIN32
    }
    return 0;
}

static int _PrivOpenOrCreate(ztl_shm_t* zshm, const char* apName, int aOpenOrCreate, int aAccessMode)
{
    int lRet = 0;

#ifdef _WIN32

    get_os_opencreated(&aOpenOrCreate);
    get_os_accessmode(&aAccessMode);

    int lShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
    if (aOpenOrCreate != ztl_open_only) {
        lShareMode |= FILE_SHARE_DELETE;
    }

    if (ZTL_SHT_FILEMAP == zshm->m_ShmType)
    {
        zshm->m_Handle = CreateFileA(apName,
            aAccessMode,
            lShareMode,
            NULL, aOpenOrCreate, FILE_ATTRIBUTE_NORMAL, 0);

        if (INVALID_HANDLE_VALUE == zshm->m_Handle)
        {
            fprintf(stderr, "CreateFileA %s failed %d\n", apName, GetLastError());
            lRet = -1;
        }
    }

#else

    get_os_opencreated(&aOpenOrCreate);

    int lDoSysCall = 0;
    int lUnixPerm = 0664;

    if (ZTL_SHT_FILEMAP == zshm->m_ShmType)
    {
        lDoSysCall = 1;
        zshm->m_Handle = open(apName, aOpenOrCreate, lUnixPerm);

        // if successful, change real permissions
        if (zshm->m_Handle >= 0)
        {
            fchmod(zshm->m_Handle, lUnixPerm);
        }
    }
    else
    {
        // try to create a share memory
        if (ZTL_SHT_SHMOPEN == zshm->m_ShmType)
        {
            lDoSysCall = 1;
            zshm->m_Handle = shm_open(apName, aOpenOrCreate, lUnixPerm);
        }
    }

    if (zshm->m_Handle < 0 && lDoSysCall)
    {
        char lErrBuf[512] = "";
        sprintf(lErrBuf, "open [%s] failed", apName);
        perror(lErrBuf);
        lRet = -1;
    }
#endif//_WIN32

    zshm->m_Mode = aAccessMode;
    return lRet;
}

