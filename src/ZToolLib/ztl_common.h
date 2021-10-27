/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#ifndef _ZTL_COMMON_H_INCLUDED_
#define _ZTL_COMMON_H_INCLUDED_

/* ztl version */
#define ZTL_Version     "1.1.0"

/* avoid unused param build warning */
#define ZTL_NOTUSED(V)  ((void)V)

#if defined(_DEBUG) || defined(DEBUG)
#define ZTL_DEBUG 1
#endif//DEBUG


#ifdef _MSC_VER
#define ZTL_STDCALL         __stdcall
#if defined(ZTL_HIDE_SYMBOLS)
#define ZTL_API
#elif defined(ZTL_EXPORTS)
#define ZTL_API             __declspec(dllexport)
#else
#define ZTL_API             __declspec(dllimport)
#endif
#define ZTL_DECLARE(type)   ZTL_API type ZTL_STDCALL
#else /* !WIN32 */
#define ZTL_API
#define ZTL_STDCALL         /* leave blank for other systems */
#define ZTL_DECLARE(type)   type
#endif//_MSC_VER



#endif//_ZTL_COMMON_H_INCLUDED_
