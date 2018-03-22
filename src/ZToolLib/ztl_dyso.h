#ifndef _ZTL_DYNAMIC_SO_H_
#define _ZTL_DYNAMIC_SO_H_

/// exported types
typedef struct dso_handle_st dso_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/// load a DSO library.
dso_handle_t* dso_load(const char* libpath);

/// close the DSO library.
void dso_unload(dso_handle_t* dso);

/// load a symbol from a DSO handle.
void* dos_symbol(dso_handle_t* dso, const char* symname);

/// report more information when a DSO function fails.
void apr_dso_error(dso_handle_t* dso, char* buf, int bufsize);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_DYNAMIC_SO_H_
