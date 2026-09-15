#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "proxy_names.h"
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
static FARPROC functions[sizeof(names)/sizeof(names[0])];
static HMODULE self;
static BOOL CALLBACK initialize(PINIT_ONCE,PVOID,PVOID*) {
    wchar_t path[32768];
    UINT n=GetSystemDirectoryW(path,32768);
    if(!n || n>32700) return FALSE;
    std::wstring system=std::wstring(path)+L"\\opengl32.dll";
    HMODULE real=LoadLibraryExW(system.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!real) return FALSE;
    for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i) {
        functions[i]=GetProcAddress(real,names[i]); if(!functions[i]) return FALSE;
    }
    n=GetModuleFileNameW(self,path,32768);
    if(n && n<32768) {
        std::wstring runtime(path); runtime.resize(runtime.find_last_of(L"\\/"));
        runtime+=L"\\SSCMods\\runtime.dll";
        HMODULE module=LoadLibraryExW(runtime.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(module) {
            auto start=reinterpret_cast<void(WINAPI*)()>(GetProcAddress(module,"SscModInitialize"));
            if(start) start();
        }
    }
    return TRUE;
}
extern "C" FARPROC proxy_resolve(unsigned index) {
    if(!InitOnceExecuteOnce(&once,initialize,nullptr,nullptr) || index>=sizeof(names)/sizeof(names[0]))
        TerminateProcess(GetCurrentProcess(),ERROR_PROC_NOT_FOUND);
    return functions[index];
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {self=module; DisableThreadLibraryCalls(module);}
    return TRUE;
}
