#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstring>
#include "proxy_names.h"
#include "title_logo.h"
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
static FARPROC functions[sizeof(names)/sizeof(names[0])];
static HMODULE self;
static PROC(WINAPI* original_gl_lookup)(LPCSTR)=nullptr;
static FARPROC title_upload=nullptr;
static PROC(WINAPI* runtime_lookup)(const char*,PROC)=nullptr;
static ssc_title::Upload texture_original=nullptr;
static ssc_title::Query texture_query=nullptr;
static std::once_flag texture_setup;
static ssc_title::Upload texture_hook=nullptr;
static void APIENTRY upload_texture(GLenum target,GLint level,GLint internal,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels){
    std::call_once(texture_setup,[]{
        try {texture_hook=ssc_title::configure(texture_original,texture_query);}
        catch(...) {ssc_title::diagnostic("Texture-time logo initialization failed");}
    });
    (texture_hook?texture_hook:texture_original)(target,level,internal,width,height,border,format,type,pixels);
}
static PROC WINAPI lookup_gl(LPCSTR name){
    auto result=original_gl_lookup(name);
    auto value=reinterpret_cast<INT_PTR>(result);
    if(title_upload&&name&&!std::strcmp(name,"glTexImage2D")&&value>3&&value!=-1)return title_upload;
    return runtime_lookup?runtime_lookup(name,result):result;
}
static BOOL CALLBACK initialize(PINIT_ONCE,PVOID,PVOID*) {
    wchar_t path[32768];
    UINT n=GetSystemDirectoryW(path,32768);
    if(!n || n>32700) return FALSE;
    std::wstring system=std::wstring(path)+L"\\opengl32.dll";
    HMODULE real=LoadLibraryExW(system.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!real || real==self) return FALSE;
    for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i) {
        // Wine omits some Windows-only exports. Resolve those only if called.
        functions[i]=GetProcAddress(real,names[i]);
    }
    // Install forwarding early, but do file access and image decoding on the
    // texture-loading thread, not during Steam's early OpenGL resolution.
    auto upload_address=GetProcAddress(real,"glTexImage2D");
    auto query_address=GetProcAddress(real,"glGetIntegerv");
    std::memcpy(&texture_original,&upload_address,sizeof(texture_original));
    std::memcpy(&texture_query,&query_address,sizeof(texture_query));
    if(texture_original&&texture_query){auto hook=upload_texture;std::memcpy(&title_upload,&hook,sizeof(hook));}
    for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i) {
        if(title_upload&&!std::strcmp(names[i],"glTexImage2D"))functions[i]=title_upload;
        if(title_upload&&!std::strcmp(names[i],"wglGetProcAddress")&&functions[i]){
            std::memcpy(&original_gl_lookup,&functions[i],sizeof(original_gl_lookup));
            auto lookup=lookup_gl;std::memcpy(&functions[i],&lookup,sizeof(lookup));
        }
    }
    n=GetModuleFileNameW(self,path,32768);
    if(n && n<32768) {
        std::wstring runtime(path); runtime.resize(runtime.find_last_of(L"\\/"));
        runtime+=L"\\SSCMods\\runtime.dll";
        HMODULE module=LoadLibraryExW(runtime.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(module) {
            runtime_lookup=reinterpret_cast<decltype(runtime_lookup)>(GetProcAddress(module,"SscModLookupGL"));
            auto start=reinterpret_cast<void(WINAPI*)()>(GetProcAddress(module,"SscModInitialize"));
            if(start) start();
        }
    }
    return TRUE;
}
extern "C" FARPROC proxy_resolve(unsigned index) {
    if(!InitOnceExecuteOnce(&once,initialize,nullptr,nullptr) || index>=sizeof(names)/sizeof(names[0]) || !functions[index]) {
        MessageBoxW(nullptr,L"The system OpenGL library could not provide a required function. Remove the mod's opengl32.dll to restore the original game, and report your Proton version.",L"SSC Mod Menu loader",MB_OK|MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(),ERROR_PROC_NOT_FOUND);
        return nullptr;
    }
    return functions[index];
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {
    if(reason==DLL_PROCESS_ATTACH) {self=module; DisableThreadLibraryCalls(module);}
    return TRUE;
}
