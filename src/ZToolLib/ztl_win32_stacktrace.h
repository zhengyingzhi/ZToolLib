/*
 * Copyright (c), Yingzhi Zheng.
 * All rights reserved.
 * trace stack on windows platform, could generate a mini dump if specified
 */

#ifndef _ZTL_WIN32_STACK_TRACE_H_
#define _ZTL_WIN32_STACK_TRACE_H_

#ifdef __cplusplus
extern "C" {
#endif

void ztl_stack_trace_init(const char* apMiniDumpName);

#ifdef __cplusplus
}
#endif

#endif//_ZTL_WIN32_STACK_TRACE_H_
