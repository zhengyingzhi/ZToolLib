#ifndef _ZTL_DYNAMIC_SO_H_
#define _ZTL_DYNAMIC_SO_H_

/// exported types
typedef struct ztl_dso_handle_st ztl_dso_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/// load a DSO library.
ztl_dso_handle_t* ztl_dso_load(const char* libpath);

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
