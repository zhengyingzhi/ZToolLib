#include <stdlib.h>
#include <string.h>

#include "ztl_dyso.h"

#ifdef _MSC_VER

#include <Windows.h>
// typedef HMODULE ztl_hlib_t;

#else

#include <errno.h>
#include <dlfcn.h>
// typedef void* ztl_hlib_t;

#endif//_MSC_VER


#ifdef _MSC_VER
static ztl_hlib_t load_lib(const char* path, int flags)
{
    return LoadLibraryA(path);
}
static void unload_lib(ztl_hlib_t hlib)
{
    FreeLibrary(hlib);
}
static void* symbol_lib(ztl_hlib_t hlib, const char* symname)
{
    return GetProcAddress(hlib, symname);
}
#else// linux platform

static ztl_hlib_t load_lib(const char* path, int flags)
{
    if (flags == 0)
        flags = RTLD_LAZY | RTLD_LOCAL;
    return dlopen(path, flags);
}
static void unload_lib(ztl_hlib_t hlib)
{
    dlclose(hlib);
}
static void* symbol_lib(ztl_hlib_t hlib, const char* symname)
{
    return dlsym(hlib, symname);
}
#endif//_MSC_VER


ztl_hlib_t ztl_dyso_load(const char* libpath, int flags)
{
    ztl_hlib_t self = load_lib(libpath, flags);
    return self;
}

void ztl_dyso_unload(ztl_hlib_t* self)
{
    if (self && *self)
    {
        unload_lib(*self);
        *self = NULL;
    }
}

void* ztl_dyso_symbol(ztl_hlib_t self, const char* symname)
{
    return self ? symbol_lib(self, symname) : NULL;
}

int ztl_dyso_error(ztl_hlib_t self, char* buf, int bufsize)
{
    if (self)
    {
#if defined(WINDOWS) || defined(WIN32)
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError();
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&lpMsgBuf,
            0, NULL);
        strncpy(buf, lpMsgBuf, bufsize - 1);
        return dw;
#else
        extern int errno;
        int ret = errno;
        strncpy(buf, dlerror(), bufsize);
        return ret;
#endif//_WIN32
    }
    return -1;
}
