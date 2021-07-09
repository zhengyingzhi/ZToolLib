#ifndef _ZTL_DYNAMIC_SO_H_
#define _ZTL_DYNAMIC_SO_H_


#ifdef _MSC_VER
#define     RTLD_LAZY   1
#define     RTLD_LOCAL  0
#else
#include <dlfcn.h>
#endif//_MSC_VER


/// exported types
typedef struct  ztl_dso_handle_st ztl_dso_handle_t;
typedef void*   ztl_hlib_t;

struct ztl_dso_handle_st
{
    ztl_hlib_t hlib;
    char path[1000];
};


#ifdef __cplusplus
extern "C" {
#endif

/// load a DSO library.
int ztl_dso_load(ztl_dso_handle_t* dso, const char* libpath, int flags);

/// close the DSO library.
void ztl_dso_unload(ztl_dso_handle_t* dso);

/// load a symbol from a DSO handle.
void* ztl_dso_symbol(ztl_dso_handle_t* dso, const char* symname);

/// report more information when a DSO function fails.
int ztl_dso_error(ztl_dso_handle_t* dso, char* buf, int bufsize);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_DYNAMIC_SO_H_
