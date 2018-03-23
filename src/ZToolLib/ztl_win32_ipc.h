#ifndef WIN32_API_HPP_SIM
#define WIN32_API_HPP_SIM

#ifdef _WIN32

#include <stdint.h>
#include <stdbool.h>

#include <windows.h>


//Own defines
static const unsigned long MaxPath  = 260;

static const unsigned long error_sharing_violation_tries = 3L;
static const unsigned long error_sharing_violation = 32L;
static const unsigned long error_sharing_violation_sleep_ms = 250L;
static const unsigned long delete_access = 0x00010000L;

//Native API constants
static const unsigned long file_open_for_backup_intent = 0x00004000;
static const int file_share_valid_flags = 0x00000007;
static const long file_delete_on_close = 0x00001000L;
static const long obj_case_insensitive = 0x00000040L;
static const long delete_flag = 0x00010000L;

static const unsigned long create_new = 1;
static const unsigned long create_always = 2;
static const unsigned long open_existing = 3;
static const unsigned long open_always = 4;
static const unsigned long truncate_existing = 5;



#ifdef _WIN64
typedef __int64(__stdcall *farproc_t)();
#else
typedef int(__stdcall *farproc_t)();
#endif  // _WIN64

typedef HMODULE hmodule;

typedef struct {
	unsigned short Length;
	unsigned short MaximumLength;
	wchar_t *Buffer;
}unicode_string_t;

typedef struct {
	unsigned long Length;
	void * RootDirectory;
	unicode_string_t *ObjectName;
	unsigned long Attributes;
	void *SecurityDescriptor;
	void *SecurityQualityOfService;
}object_attributes_t;

typedef struct {
	union {
		long Status;
		void *Pointer;
	};

	unsigned long *Information;
}io_status_block_t;

typedef struct system_timeofday_information_data
{
    __int64 liKeBootTime;
    __int64 liKeSystemTime;
    __int64 liExpTimeZoneBias;
    unsigned long uCurrentTimeZoneId;
    unsigned long dwReserved;
    unsigned long long ullBootTimeBias;
    unsigned long long ullSleepTimeBias;
} system_timeofday_information_data_t;

typedef enum 
{
	object_basic_information,
	object_name_information,
	object_type_information,
	object_all_information,
	object_data_information
}object_information_class;


typedef struct
{
	unicode_string_t Name;
	wchar_t NameBuffer[1];
}object_name_information_t;

enum system_information_class {
	system_basic_information = 0,
	system_performance_information = 2,
	system_time_of_day_information = 3,
	system_process_information = 5,
	system_processor_performance_information = 8,
	system_interrupt_information = 23,
	system_exception_information = 33,
	system_registry_quota_information = 37,
	system_lookaside_information = 45
};


enum file_information_class_t {
	file_directory_information = 1,
	file_full_directory_information,
	file_both_directory_information,
	file_basic_information,
	file_standard_information,
	file_internal_information,
	file_ea_information,
	file_access_information,
	file_name_information,
	file_rename_information,
	file_link_information,
	file_names_information,
	file_disposition_information,
	file_position_information,
	file_full_ea_information,
	file_mode_information,
	file_alignment_information,
	file_all_information,
	file_allocation_information,
	file_end_of_file_information,
	file_alternate_name_information,
	file_stream_information,
	file_pipe_information,
	file_pipe_local_information,
	file_pipe_remote_information,
	file_mailslot_query_information,
	file_mailslot_set_information,
	file_compression_information,
	file_copy_on_write_information,
	file_completion_information,
	file_move_cluster_information,
	file_quota_information,
	file_reparse_point_information,
	file_network_open_information,
	file_object_id_information,
	file_tracking_information,
	file_ole_directory_information,
	file_content_index_information,
	file_inherit_content_index_information,
	file_ole_information,
	file_maximum_information
};

typedef struct 
{
	unsigned long nLength;
	void *lpSecurityDescriptor;
	int bInheritHandle;
}interprocess_security_attributes;

typedef long(__stdcall *NtSetInformationFile_t)(void *FileHandle, io_status_block_t *IoStatusBlock, void *FileInformation, unsigned long Length, int FileInformationClass);
typedef long(__stdcall *NtQueryObject_t)(void*, object_information_class, void *, unsigned long, unsigned long *);
typedef long(__stdcall *NtQuerySystemInformation_t)(int, void*, unsigned long, unsigned long *);
typedef long(__stdcall *NtOpenFile_t)(void*, unsigned long, object_attributes_t*, io_status_block_t*, unsigned long, unsigned long);
typedef long(__stdcall *NtClose_t) (void*);

//#ifdef __cplusplus
//extern "C" 
//#endif
//__declspec(dllimport) void * __stdcall CreateFileA(const char *, unsigned long, unsigned long, struct interprocess_security_attributes*, unsigned long, unsigned long, void *);


static const long BootstampLength = sizeof(int64_t);
static const long BootAndSystemstampLength = sizeof(int64_t) * 2;
static const long SystemTimeOfDayInfoLength = sizeof(system_timeofday_information_data_t);


void sched_yield()
{
	if (!SwitchToThread()) {
		Sleep(0);
	}
}

void sleep_tick()
{
	Sleep(1);
}

void sleep(unsigned long ms)
{
	Sleep(ms);
}

unsigned long get_current_thread_id()
{
	return GetCurrentThreadId();
}

unsigned long get_current_process_id()
{
	return GetCurrentProcessId();
}

unsigned int close_handle(void* handle)
{
	return CloseHandle(handle);
}

void *create_file(const char *name, unsigned long access, unsigned long creation_flags, unsigned long attributes, interprocess_security_attributes *psec)
{
	for (unsigned int attempt = 0; attempt < error_sharing_violation_tries; ++attempt) {
		void * const handle = CreateFileA(name, access,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			psec, creation_flags, attributes, 0);

		bool const invalid = (INVALID_HANDLE_VALUE == handle);
		if (!invalid) {
			return handle;
		}
		if (error_sharing_violation != GetLastError()) {
			return handle;
		}
		sleep(error_sharing_violation_sleep_ms);
	}
	return INVALID_HANDLE_VALUE;
}

void initialize_object_attributes(object_attributes_t *pobject_attr, unicode_string_t *name,
	unsigned long attr, void *rootdir, void *security_descr)
{
	pobject_attr->Length = sizeof(object_attributes_t);
	pobject_attr->RootDirectory = rootdir;
	pobject_attr->Attributes = attr;
	pobject_attr->ObjectName = name;
	pobject_attr->SecurityDescriptor = security_descr;
	pobject_attr->SecurityQualityOfService = 0;
}

farproc_t get_proc_address(hmodule module, const char *name)
{
	return GetProcAddress(module, name);
}

hmodule get_module_handle(const char *name)
{
	return GetModuleHandleA(name);
}

//A class that locates and caches loaded DLL function addresses.
//template<int Dummy>
//struct function_address_holder
//{
enum function_address_index {
	NtSetInformationFile
	, NtQuerySystemInformation
	, NtQueryObject
	, NtQuerySemaphore
	, NtQuerySection
	, NtOpenFile
	, NtClose
	, NtQueryTimerResolution
	, NtSetTimerResolution
	, QueryPerformanceCounter2
	, QueryPerformanceFrequency2
	, NumFunction
};
enum function_module_index { 
	NtDll_dll, 
	Kernel32_dll, 
	NumModule
};

static const char *FunctionNames[NumFunction] = {
	"NtSetInformationFile",
	"NtQuerySystemInformation",
	"NtQueryObject",
	"NtQuerySemaphore",
	"NtQuerySection",
	"NtOpenFile",
	"NtClose",
	"NtQueryTimerResolution",
	"NtSetTimerResolution",
	"QueryPerformanceCounter2",
	"QueryPerformanceFrequency2"
};
	
static const char *ModuleNames[NumModule] = {
	"ntdll.dll",
	"kernel32.dll"
};
	
static farproc_t FunctionAddresses[NumFunction];
static unsigned int FunctionModules[NumFunction] = {
    NtDll_dll,
    NtDll_dll,
    NtDll_dll,
    NtDll_dll,
    NtDll_dll,
    NtDll_dll,
    NtDll_dll,
    NtDll_dll,
    NtDll_dll,
    Kernel32_dll,
    Kernel32_dll
};

static volatile long FunctionStates[NumFunction];
static hmodule ModuleAddresses[NumModule];
static volatile long ModuleStates[NumModule];

static hmodule get_module_from_id(unsigned int id)
{
    hmodule addr = get_module_handle(ModuleNames[id]);
    return addr;
}

static hmodule get_module(const unsigned int id)
{
    for (unsigned i = 0; ModuleStates[id] < 2; ++i) {
        if (InterlockedCompareExchange(&ModuleStates[id], 1, 0) == 0) {
            ModuleAddresses[id] = get_module_from_id(id);
            InterlockedIncrement(&ModuleStates[id]);
            break;
        }
        else if (i & 1) {
            sched_yield();
        }
        else {
            sleep_tick();
        }
    }
    return ModuleAddresses[id];
}

static farproc_t get_address_from_dll(const unsigned int id)
{
    farproc_t addr = get_proc_address(get_module(FunctionModules[id]), FunctionNames[id]);
    return addr;
}

static farproc_t dll_func_address_get(const unsigned int id)
{
    for (unsigned i = 0; FunctionStates[id] < 2; ++i) {
        if (InterlockedCompareExchange(&FunctionStates[id], 1, 0) == 0) {
            FunctionAddresses[id] = get_address_from_dll(id);
            InterlockedIncrement(&FunctionStates[id]);
            break;
        }
        else if (i & 1) {
            sched_yield();
        }
        else {
            Sleep(1);
        }
    }
    return FunctionAddresses[id];
}


#if 0
template<int Dummy>
farproc_t function_address_holder<Dummy>::FunctionAddresses[function_address_holder<Dummy>::NumFunction];

template<int Dummy>
volatile long function_address_holder<Dummy>::FunctionStates[function_address_holder<Dummy>::NumFunction];

template<int Dummy>
hmodule function_address_holder<Dummy>::ModuleAddresses[function_address_holder<Dummy>::NumModule];

template<int Dummy>
volatile long function_address_holder<Dummy>::ModuleStates[function_address_holder<Dummy>::NumModule];
#endif//0


//Writes the hexadecimal value of the buffer, in the wide character string.
//str must be twice length
void buffer_to_wide_str(const void *buf, size_t length, wchar_t *str)
{
    const wchar_t Characters[] =
    { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7'
        , L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };
    size_t char_counter = 0;
    const char *chbuf = (const char *)(buf);
    for (size_t i = 0; i != length; ++i) {
        str[char_counter++] = Characters[(chbuf[i] & 0xF0) >> 4];
        str[char_counter++] = Characters[(chbuf[i] & 0x0F)];
    }
}

bool get_system_time_of_day_information(system_timeofday_information_data_t* info)
{
    NtQuerySystemInformation_t pNtQuerySystemInformation = (NtQuerySystemInformation_t)
        dll_func_address_get(NtQuerySystemInformation);
    unsigned long res;
    long status = pNtQuerySystemInformation(system_time_of_day_information, info, sizeof(*info), &res);
    if (status) {
        return false;
    }
    return true;
}

bool get_boot_and_system_time_wstr(wchar_t *bootsystemstamp, size_t* s)
//will write BootAndSystemstampLength chars
{
    if ((long)*s < (BootAndSystemstampLength * 2))
        return false;
    system_timeofday_information_data_t info;
    bool ret = get_system_time_of_day_information(&info);
    if (!ret) {
        return false;
    }

    buffer_to_wide_str(&info, BootAndSystemstampLength, bootsystemstamp);
    *s = BootAndSystemstampLength * 2;
    return true;
}

typedef struct {
    int Replace;
    void *RootDir;
    unsigned long FileNameLength;
    wchar_t FileName[1];
}file_rename_information_t;

typedef union
{
    object_name_information_t name;
    struct ren_t
    {
        file_rename_information_t info;
        wchar_t buf[1];
    } ren;
}ntquery_mem_t;


static const size_t rename_offset = offsetof(ntquery_mem_t, ren.info.FileName) -
offsetof(ntquery_mem_t, name.Name.Buffer);
// Timestamp                      process id              atomic count
static const size_t rename_suffix =
(sizeof(system_timeofday_information_data_t) + sizeof(unsigned long) + sizeof(uint32_t)) * 2;

typedef struct
{
    size_t m_size;
    char*  m_buf;
}nt_query_mem_deleter;

void nt_query_mem_deleter_construct(nt_query_mem_deleter* obj, size_t object_name_information_size)
{
    obj->m_size = object_name_information_size + rename_offset + rename_suffix;
    obj->m_buf = malloc(obj->m_size);
}
void nt_query_mem_deleter_destruct(nt_query_mem_deleter* obj)
{
    if (obj->m_buf) {
        free(obj->m_buf);
        obj->m_buf = NULL;
    }
}

void nt_query_mem_deleter_realloc_mem(nt_query_mem_deleter* obj, size_t num_bytes)
{
    num_bytes += rename_suffix + rename_offset;
    char* buf = obj->m_buf;
    obj->m_buf = malloc(num_bytes);
    free(buf);
    obj->m_size = num_bytes;
}

ntquery_mem_t* nt_query_mem_deleter_query_mem(nt_query_mem_deleter* obj)
{
    return (ntquery_mem_t*)(obj->m_buf);
}

unsigned long nt_query_mem_deleter_name_information_size(nt_query_mem_deleter* obj)
{
    return obj->m_size - rename_offset - SystemTimeOfDayInfoLength * 2;
}

unsigned long nt_query_mem_deleter_rename_information_size(nt_query_mem_deleter* obj)
{
    return obj->m_size;
}


bool unlink_file(const char *filename)
{
    //Don't try to optimize doing a DeleteFile first
    //as there are interactions with permissions and
    //in-use files.
    //
    //if(!delete_file(filename)){
    //   (...)
    //

    //This functions tries to emulate UNIX unlink semantics in windows.
    //
    //- Open the file and mark the handle as delete-on-close
    //- Rename the file to an arbitrary name based on a random number
    //- Close the handle. If there are no file users, it will be deleted.
    //  Otherwise it will be used by already connected handles but the
    //  file name can't be used to open this file again
    NtSetInformationFile_t pNtSetInformationFile =
        (NtSetInformationFile_t)dll_func_address_get(NtSetInformationFile);

    NtQueryObject_t pNtQueryObject = (NtQueryObject_t)dll_func_address_get(NtQueryObject);

    //First step: Obtain a handle to the file using Win32 rules. This resolves relative paths
    void *fh = create_file(filename, GENERIC_READ | delete_access, OPEN_EXISTING, 0, 0);
    if (fh == INVALID_HANDLE_VALUE) {
        return false;
    }

    {
        //Obtain name length
        unsigned long size;
        const size_t initial_string_mem = 512u;

        nt_query_mem_deleter nt_query_mem;
        nt_query_mem_deleter_construct(&nt_query_mem, sizeof(ntquery_mem_t) + initial_string_mem);

        //Obtain file name with guessed length
        if (pNtQueryObject(fh, object_name_information, nt_query_mem_deleter_query_mem(&nt_query_mem), nt_query_mem_deleter_name_information_size(&nt_query_mem), &size)) {
            //Obtain file name with exact length buffer
            nt_query_mem_deleter_realloc_mem(&nt_query_mem, size);
            if (pNtQueryObject(fh, object_name_information, nt_query_mem_deleter_query_mem(&nt_query_mem), nt_query_mem_deleter_name_information_size(&nt_query_mem), &size)) {

                close_handle(fh);
                return false;
            }
        }
        ntquery_mem_t *pmem = nt_query_mem_deleter_query_mem(&nt_query_mem);
        file_rename_information_t *pfri = &pmem->ren.info;
        const size_t RenMaxNumChars =
            (((char*)(pmem)+nt_query_mem_deleter_rename_information_size(&nt_query_mem) - (char*)&pmem->ren.info.FileName[0]) / sizeof(wchar_t));

        //Copy filename to the rename member
        memmove(pmem->ren.info.FileName, pmem->name.Name.Buffer, pmem->name.Name.Length);
        size_t filename_string_length = pmem->name.Name.Length / sizeof(wchar_t);

        //Search '\\' character to replace from it
        for (size_t i = filename_string_length; i != 0; --filename_string_length) {
            if (pmem->ren.info.FileName[--i] == L'\\')
                break;
        }

        //Add random number
        size_t s = RenMaxNumChars - filename_string_length;
        if (!get_boot_and_system_time_wstr(&pfri->FileName[filename_string_length], &s)) {
            close_handle(fh);
            return false;
        }
        filename_string_length += s;

        //Sometimes the precission of the timestamp is not enough and we need to add another random number.
        //The process id (to exclude concurrent processes) and an atomic count (to exclude concurrent threads).
        //should be enough
        const unsigned long pid = get_current_process_id();
        buffer_to_wide_str(&pid, sizeof(pid), &pfri->FileName[filename_string_length]);
        filename_string_length += sizeof(pid) * 2;

        static volatile uint32_t u32_count = 0;
        InterlockedDecrement((volatile long*)(&u32_count));
        buffer_to_wide_str((const uint32_t *)(&u32_count), sizeof(uint32_t), &pfri->FileName[filename_string_length]);
        filename_string_length += sizeof(uint32_t) * 2;

        //Fill rename information (FileNameLength is in bytes)
        pfri->FileNameLength = (unsigned long)(sizeof(wchar_t)*(filename_string_length));
        pfri->Replace = 1;
        pfri->RootDir = 0;

        //Cange the name of the in-use file...
        io_status_block_t io;
        if (0 != pNtSetInformationFile(fh, &io, pfri, nt_query_mem_deleter_rename_information_size(&nt_query_mem), file_rename_information)) {
            close_handle(fh);
            return false;
        }

        close_handle(fh);
        fh = 0;
    }

    //...and mark it as delete-on-close
    {
        //Don't use pNtSetInformationFile with file_disposition_information as it can return STATUS_CANNOT_DELETE
        //if the file is still mapped. Reopen it with NtOpenFile and file_delete_on_close
        NtOpenFile_t pNtOpenFile = (NtOpenFile_t)dll_func_address_get(NtOpenFile);
        NtClose_t pNtClose = (NtClose_t)dll_func_address_get(NtClose);
        const wchar_t empty_str[] = L"";
        unicode_string_t ustring = { sizeof(empty_str) - sizeof(wchar_t)   //length in bytes without null
            , sizeof(empty_str)   //total size in bytes of memory allocated for Buffer.
            , (wchar_t*)(empty_str)
        };
        object_attributes_t object_attr;
        initialize_object_attributes(&object_attr, &ustring, 0, fh, 0);
        void* fh2 = 0;
        io_status_block_t io;
        pNtOpenFile(&fh2, delete_flag, &object_attr, &io
            , FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, file_delete_on_close);
        pNtClose(fh2);
        //Even if NtOpenFile fails, the file was renamed and the original no longer exists, so return a success status
        return true;
    }

    return true;
}

#endif//_WIN32

#endif//WIN32_API_HPP_SIM
