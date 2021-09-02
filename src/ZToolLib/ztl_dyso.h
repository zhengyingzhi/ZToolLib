#ifndef _ZTL_DYNAMIC_SO_H_
#define _ZTL_DYNAMIC_SO_H_


#ifdef _MSC_VER
#define     RTLD_LAZY   1
#define     RTLD_LOCAL  0
#else
#include <dlfcn.h>
#endif//_MSC_VER


/// exported types
typedef void* ztl_hlib_t;

#ifdef __cplusplus
extern "C" {
#endif

/// load a DSO library.
ztl_hlib_t ztl_dyso_load(const char* libpath, int flags);

/// close the DSO library.
void ztl_dyso_unload(ztl_hlib_t* self);

/// load a symbol from a DSO handle.
void* ztl_dyso_symbol(ztl_hlib_t self, const char* symname);

/// report more information when a DSO function fails.
int ztl_dyso_error(ztl_hlib_t self, char* buf, int bufsize);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_DYNAMIC_SO_H_
