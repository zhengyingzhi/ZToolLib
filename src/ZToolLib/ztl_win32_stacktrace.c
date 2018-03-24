/*
* Copyright (c), SZKingdom, Inc.
* All rights reserved.
* trace stack on windows platform, could generate a mini dump if specified
*/

#include <Windows.h>
#include <DbgHelp.h>
//#include <imagehlp.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "ztl_win32_stacktrace.h"

#pragma comment(lib, "dbghelp.lib")


static IMAGEHLP_SYMBOL64* pSymbol = NULL;
static IMAGEHLP_LINE64 line;
static BOOLEAN processingException = FALSE;
static CHAR modulePath[MAX_PATH];
static LPTOP_LEVEL_EXCEPTION_FILTER defaultTopLevelExceptionHandler = NULL;

static const char* gDumpName = NULL;


static const char* exceptionDescription(DWORD code)
{
    switch (code)
    {
        case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
        case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
        case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
        case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
        case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
        case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
        case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
        case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
        case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
        case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
        default: return "UNKNOWN EXCEPTION";
    }
}

/* Returns the index of the last backslash in the file path */
int GetFilenameStart(CHAR* path)
{
    int pos = 0;
    int found = 0;
    if (path != NULL)
    {
        while (path[pos] != '\0' && pos < MAX_PATH)
        {
            if (path[pos] == '\\') {
                found = pos + 1;
            }
            ++pos;
        }
    }

    return found;
}


static BOOL _IsDataSectionNeeded(const WCHAR* pModuleName)
{
    if (pModuleName == 0)
    {
        return FALSE;
    }

    WCHAR szFileName[_MAX_FNAME] = L"";
    _wsplitpath(pModuleName, NULL, NULL, szFileName, NULL);

    if (_wcsicmp(szFileName, L"ntdll") == 0)
    {
        return TRUE;
    }

    return FALSE;
}

static BOOL CALLBACK _MiniDumpCallback(PVOID pParam,
    const PMINIDUMP_CALLBACK_INPUT   pInput,
    PMINIDUMP_CALLBACK_OUTPUT        pOutput)
{
    if (pInput == 0 || pOutput == 0)
    {
        return FALSE;
    }

    switch (pInput->CallbackType)
    {
    case ModuleCallback:
        if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
        {
            if (!_IsDataSectionNeeded(pInput->Module.FullPath))
                pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
        }
    case IncludeModuleCallback:
    case IncludeThreadCallback:
    case ThreadCallback:
    case ThreadExCallback:
        return TRUE;
    default:
        break;
    }

    return FALSE;
}

static void _CreateMiniDump(EXCEPTION_POINTERS* pep, LPCTSTR strFileName)
{
    HANDLE hFile = CreateFile(strFileName, GENERIC_READ | GENERIC_WRITE,
        0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
    {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = pep;
        mdei.ClientPointers = FALSE;

        MINIDUMP_CALLBACK_INFORMATION mci;
        mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)_MiniDumpCallback;
        mci.CallbackParam = 0;

        MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(
            MiniDumpWithPrivateReadWriteMemory |
            MiniDumpWithDataSegs |
            MiniDumpWithHandleData |
            0x00000800 /*MiniDumpWithFullMemoryInfo*/ |
            0x00001000 /*MiniDumpWithThreadInfo*/ |
            MiniDumpWithUnloadedModules);

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
            hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci);

        CloseHandle(hFile);
    }
}

#ifdef _WIN64
void LogStackTrace()
{
    BOOL            result;
    HANDLE          thread;
    HANDLE          process;
    CONTEXT         context;
    STACKFRAME64    stack;
    ULONG           frame;
    DWORD64         dw64Displacement;
    DWORD           dwDisplacement;

    memset(&stack, 0, sizeof(STACKFRAME64));
    memset(pSymbol, '\0', sizeof(*pSymbol) + MAX_PATH);
    memset(&modulePath[0], '\0', sizeof(modulePath));
    line.LineNumber = 0;

    RtlCaptureContext(&context);
    process = GetCurrentProcess();
    thread = GetCurrentThread();
    dw64Displacement = 0;
    stack.AddrPC.Offset = context.Rip;
    stack.AddrPC.Mode = AddrModeFlat;
    stack.AddrStack.Offset = context.Rsp;
    stack.AddrStack.Mode = AddrModeFlat;
    stack.AddrFrame.Offset = context.Rbp;
    stack.AddrFrame.Mode = AddrModeFlat;

    for (frame = 0;; frame++)
    {
        result = StackWalk64(
            IMAGE_FILE_MACHINE_AMD64,
            process,
            thread,
            &stack,
            &context,
            NULL,
            SymFunctionTableAccess64,
            SymGetModuleBase64,
            NULL
            );

        pSymbol->MaxNameLength = MAX_PATH;
        pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);

        SymGetSymFromAddr64(process, stack.AddrPC.Offset, &dw64Displacement, pSymbol);
        SymGetLineFromAddr64(process, stack.AddrPC.Offset, &dwDisplacement, &line);

        DWORD64 moduleBase = SymGetModuleBase64(process, stack.AddrPC.Offset);
        if (moduleBase)
        {
            GetModuleFileNameA((HINSTANCE) moduleBase, modulePath, MAX_PATH);
        }

        char lBuffer[4096] = "";
        snprintf(lBuffer, sizeof(lBuffer) - 1, "%s!%s(%s:%d)(0x%08LX, 0x%08LX, 0x%08LX, 0x%08LX)\n",
            &modulePath[GetFilenameStart(modulePath)],
            pSymbol->Name,
            line.FileName,
            line.LineNumber,
            stack.Params[0],
            stack.Params[1],
            stack.Params[2],
            stack.Params[3]
            );
        fprintf(stderr, lBuffer);

        if (!result) {
            break;
        }
    }
}
#else
void LogStackTrace() {}
#endif

void StackTraceInfo()
{
    fprintf(stderr, "---- STACK TRACE ----");
    LogStackTrace();
}

void BugReportEnd()
{
    fprintf(stderr,
        "==== BUG REPORT END ==== \n\n"
        "     Please report this bug by following the instructions at:\n\n"
        "     zhengyingzhi112@163.com\n\n"
        );
}

LONG WINAPI UnhandledExceptiontHandler(PEXCEPTION_POINTERS info)
{
    if (gDumpName && *gDumpName) {
        _CreateMiniDump(info, gDumpName);
    }

    if (!processingException)
    {
        bool headerLogged = false;

#ifdef __cplusplus
        try
        {
#endif
            const char* exDescription = "Exception code not available";
            processingException = true;
            if (info != NULL && info->ExceptionRecord != NULL && info->ExceptionRecord->ExceptionCode != 0) {
                exDescription = exceptionDescription(info->ExceptionRecord->ExceptionCode);
            }

            // Call antirez routine to log the start of the bug report
            headerLogged = true;
            //fprintf(stderr, "---- %s", exDescription);
            StackTraceInfo();
#ifdef __cplusplus
        }
        catch (...) {}
#endif

        if (headerLogged) {
            BugReportEnd();
        }

        if (defaultTopLevelExceptionHandler != NULL && info != NULL) {
            defaultTopLevelExceptionHandler(info);
        }

        processingException = false;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

/* Handler to trap abort() calls */
#ifdef __cplusplus
extern "C" 
#endif
void AbortHandler(int signal_number)
{
    (void)signal_number;
    fprintf(stderr, "---- ABORT");
    StackTraceInfo();
    BugReportEnd();
}

void InitSymbols()
{
    // Preload symbols so they will be available in case of out-of-memory exception
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
}


/* init win32 stack trace */
void ztl_stack_trace_init(const char* apMiniDumpName)
{
    if (apMiniDumpName && *apMiniDumpName) {
        gDumpName = _strdup(apMiniDumpName);
    }

    if (pSymbol == NULL) {
        pSymbol = (IMAGEHLP_SYMBOL64*)malloc(sizeof(IMAGEHLP_SYMBOL64) + MAX_PATH*sizeof(TCHAR));
    }

    InitSymbols();

    // Global handler for unhandled exceptions
    defaultTopLevelExceptionHandler = SetUnhandledExceptionFilter(UnhandledExceptiontHandler);

    // Handler for abort()
    signal(SIGABRT, &AbortHandler);
}
