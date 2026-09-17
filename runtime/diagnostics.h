#pragma once
#include <windows.h>
#include <dbghelp.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
namespace ssc_diagnostics {
inline wchar_t crash_text[32768]{},crash_dump[32768]{};
inline LPTOP_LEVEL_EXCEPTION_FILTER previous=nullptr;
inline LONG handling=0;
using Dump=BOOL(WINAPI*)(HANDLE,DWORD,HANDLE,MINIDUMP_TYPE,PMINIDUMP_EXCEPTION_INFORMATION,PMINIDUMP_USER_STREAM_INFORMATION,PMINIDUMP_CALLBACK_INFORMATION);
inline Dump write_dump=nullptr;
inline LONG WINAPI crashed(EXCEPTION_POINTERS* exception){
    if(InterlockedExchange(&handling,1)==0&&exception&&exception->ExceptionRecord){
        HANDLE file=CreateFileW(crash_text,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(file!=INVALID_HANDLE_VALUE){
            char text[512];HMODULE module=nullptr;auto address=exception->ExceptionRecord->ExceptionAddress;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(address),&module);
            int n=std::snprintf(text,sizeof(text),"SSC Mod Menu 0.1.2\r\nUnhandled exception: %08lX\r\nAddress: %p\r\nModule base: %p\r\nModule offset: %llX\r\nSee runtime.log for game fingerprint and module checks.\r\nThis report does not establish that the mod caused the crash.\r\n",exception->ExceptionRecord->ExceptionCode,address,static_cast<void*>(module),static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(address)-reinterpret_cast<uintptr_t>(module)));
            DWORD written=0;if(n>0)WriteFile(file,text,DWORD(n),&written,nullptr);FlushFileBuffers(file);CloseHandle(file);
        }
        if(write_dump){HANDLE dump=CreateFileW(crash_dump,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(dump!=INVALID_HANDLE_VALUE){MINIDUMP_EXCEPTION_INFORMATION info{GetCurrentThreadId(),exception,FALSE};write_dump(GetCurrentProcess(),GetCurrentProcessId(),dump,MiniDumpNormal,&info,nullptr,nullptr);FlushFileBuffers(dump);CloseHandle(dump);}}
    }
    return previous?previous(exception):EXCEPTION_CONTINUE_SEARCH;
}
inline void initialize(const std::filesystem::path& state){
    auto directory=state/L"diagnostics";std::error_code ec;std::filesystem::create_directories(directory,ec);if(ec)return;
    SYSTEMTIME t;GetSystemTime(&t);wchar_t name[100];swprintf(name,100,L"crash-%04u%02u%02u-%02u%02u%02u-%lu",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetCurrentProcessId());
    auto base=directory/name;auto text=base.wstring()+L".txt",dump=base.wstring()+L".dmp";
    if(text.size()>=32768||dump.size()>=32768)return;
    wcscpy(crash_text,text.c_str());wcscpy(crash_dump,dump.c_str());
    auto lib=LoadLibraryExW(L"dbghelp.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(lib)write_dump=reinterpret_cast<Dump>(GetProcAddress(lib,"MiniDumpWriteDump"));
    previous=SetUnhandledExceptionFilter(crashed);
}
inline void open_logs(const std::filesystem::path& state){ShellExecuteW(nullptr,L"open",state.c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
}
