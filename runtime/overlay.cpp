#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include <bcrypt.h>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include "sound_module.h"
#include "cosmetic_adapter.h"
#include "presence_source.h"
#include "hud_editor.h"
#include "update_module.h"

namespace {
bool native_supported=false;
#include "menu_ui.h"

bool attach_sound() {
    auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    auto dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    const auto size=nt->OptionalHeader.SizeOfImage;
    auto module=GetModuleHandleW(L"openal32_x64.dll");
    auto expected=module?GetProcAddress(module,"alBufferData"):nullptr;if(!expected)return false;
    if(dir.VirtualAddress>=size||dir.Size>size-dir.VirtualAddress)return false;
    for(size_t offset=0;offset+sizeof(IMAGE_IMPORT_DESCRIPTOR)<=dir.Size;offset+=sizeof(IMAGE_IMPORT_DESCRIPTOR)){
        auto d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base+dir.VirtualAddress+offset);if(!d->Name)break;
        if(d->Name>=size||!memchr(base+d->Name,0,size-d->Name))return false;
        if(_stricmp(reinterpret_cast<char*>(base+d->Name),"openal32_x64.dll"))continue;
        if(!d->OriginalFirstThunk)return false;
        for(size_t i=0;;++i){size_t nr=d->OriginalFirstThunk+i*8,ar=d->FirstThunk+i*8;if(nr+8>size||ar+8>size)return false;
            auto n=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+nr),a=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+ar);if(!n->u1.AddressOfData)break;
            if(IMAGE_SNAP_BY_ORDINAL64(n->u1.Ordinal))continue;
            auto rva=n->u1.AddressOfData;if(rva+2>=size||!memchr(base+rva+2,0,size-rva-2))return false;
            if(strcmp(reinterpret_cast<char*>(base+rva+2),"alBufferData"))continue;
            if(a->u1.Function!=reinterpret_cast<ULONGLONG>(expected))return false;
            DWORD protection;
            if(!VirtualProtect(&a->u1.Function,8,PAGE_READWRITE,&protection))return false;
            ssc_sound::original=reinterpret_cast<ssc_sound::BufferData>(expected);
            auto prior=InterlockedCompareExchangePointer(reinterpret_cast<PVOID volatile*>(&a->u1.Function),reinterpret_cast<PVOID>(ssc_sound::buffer_data),reinterpret_cast<PVOID>(expected));
            DWORD unused;VirtualProtect(&a->u1.Function,8,protection,&unused);return prior==reinterpret_cast<PVOID>(expected);
        }
    }return false;
}

template<class T> T gl_proc(const char* name) {
    auto p=wglGetProcAddress(name);auto v=reinterpret_cast<intptr_t>(p);
    return (v==0||v==1||v==2||v==3||v==-1)?nullptr:reinterpret_cast<T>(p);
}
void render(HDC dc) {
    HWND window=WindowFromDC(dc); HGLRC context=wglGetCurrentContext();
    if(!window||!context) return;
    if(!game_window) {
        if(GetWindowThreadProcessId(window,nullptr)!=GetCurrentThreadId()) {log("Window/render threads differ; input hook refused");return;}
        SetLastError(0);auto old=SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(window_proc));
        if(!old) {log("Window input hook failed");return;}
        previous_proc=reinterpret_cast<WNDPROC>(old);game_window=window;log("Render and input attached; Right Shift opens menu");
    }
    ULONGLONG now=GetTickCount64();float seconds=last_frame?float(now-last_frame)/1000.f:0.f;last_frame=now;
    static ULONGLONG diagnostics_at=0;
    if(developer_tools&&now-diagnostics_at>15000){diagnostics_at=now;char report[160];snprintf(report,sizeof(report),"Draw diagnostics: rainbow=%u audio=%u",ssc_names::cosmetic_draws.load(),ssc_sound::substituted.load());log(report);}
    static ULONGLONG presence_at=0;
    if(now-presence_at>=1000){presence_at=now;rpc_preview=native_supported?ssc_rpc::sample_steam(rpc_rating):ssc_rpc::Snapshot{};static int source_phase=-1;if(source_phase!=rpc_preview.phase){source_phase=rpc_preview.phase;log(source_phase==0?"RPC source: game status unavailable":source_phase==1?"RPC source: verified main menu":"RPC source: verified game session");}ssc_rpc::submit(rpc_requested,rpc_id,rpc_timer,rpc_preview);if(opened)dirty=true;}
    static ULONGLONG update_at=0;
    if(now-update_at>=1000){update_at=now;
        if(native_supported&&rpc_preview.phase==1&&!welcome_seen&&!opened){opened=true;show_manager(8);}
        if(native_supported&&rpc_preview.phase==1&&!ssc_update::checked)ssc_update::check(state_dir);
        if(ssc_update::poll(window,rpc_preview.phase==1||!native_supported))dirty=true;
        if(rpc_preview.phase!=1&&manager&&settings_page==7&&opened)close_menu(true);
        if(rpc_preview.phase==1&&ssc_update::available()&&!ssc_update::notified&&!opened){ssc_update::notified=true;opened=true;show_manager(7);}
    }
    RECT hud_client{};GetClientRect(window,&hud_client);if(ssc_hud::finish_frame(hud_client.right,hud_client.bottom,now)&&ssc_hud::editing)dirty=true;
    advance_animation(seconds);
    if(window!=game_window||(!opened&&visibility==0.f&&modal_visibility==0.f&&!clock_enabled))return;
    RECT client;GetClientRect(window,&client);int width=client.right,height=client.bottom;
    if(width<=0||height<=0)return;
    layout_panel(width,height);
    if(!owner_context) owner_context=context;
    if(context!=owner_context) return; // Never reuse texture IDs across GL contexts.
    using UseProgram=void(APIENTRY*)(GLuint);
    using ActiveTexture=void(APIENTRY*)(GLenum);
    using BindFramebuffer=void(APIENTRY*)(GLenum,GLuint);
    using BindBuffer=void(APIENTRY*)(GLenum,GLuint);
    auto use_program=gl_proc<UseProgram>("glUseProgram");
    auto active_texture=gl_proc<ActiveTexture>("glActiveTexture");
    auto bind_framebuffer=gl_proc<BindFramebuffer>("glBindFramebuffer");
    auto bind_buffer=gl_proc<BindBuffer>("glBindBuffer");
    GLint unpack_buffer=0;
    if(bind_buffer) glGetIntegerv(0x88EF,&unpack_buffer);
    GLint program=0,active=0,framebuffer=0,mode=0;
    glGetIntegerv(0x8B8D,&program);glGetIntegerv(0x84E0,&active);glGetIntegerv(GL_MATRIX_MODE,&mode);
    if(bind_framebuffer) glGetIntegerv(0x8CA6,&framebuffer);
    glPushAttrib(GL_ALL_ATTRIB_BITS);glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);
    if(use_program) use_program(0);
    if(bind_buffer) bind_buffer(0x88EC,0);
    if(bind_framebuffer) bind_framebuffer(0x8CA9,0); // Draw framebuffer only; do not disturb read binding.
    if(active_texture) {
        GLint units=1;glGetIntegerv(0x84E2,&units);
        for(int i=0;i<units;++i) {active_texture(0x84C0+i);glDisable(GL_TEXTURE_2D);glDisable(GL_TEXTURE_1D);}
        active_texture(0x84C0);
    }
    glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);
    using BlendEquation=void(APIENTRY*)(GLenum);
    if(auto eq=gl_proc<BlendEquation>("glBlendEquation"))eq(0x8006);
    glDisable(GL_COLOR_LOGIC_OP);
    for(int i=0;i<6;++i)glDisable(GL_CLIP_PLANE0+i);
    glDisable(GL_TEXTURE_GEN_S);glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_ALPHA_TEST);glDisable(GL_LIGHTING);glDisable(GL_CULL_FACE);glDisable(GL_FOG);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    glViewport(0,0,width,height);glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glMatrixMode(GL_TEXTURE);glPushMatrix();glLoadIdentity();
    glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    const float opacity=ease(visibility);
    if(modal_visibility>0.f&&!ssc_hud::editing) {
        glDisable(GL_TEXTURE_2D);glColor4f(.01f,.03f,.07f,.45f*ease(modal_visibility));
        glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(width,0);glVertex2i(width,height);glVertex2i(0,height);glEnd();
    }
    if(ssc_hud::editing){
        glDisable(GL_TEXTURE_2D);glLineWidth(1);
        for(int i=0;i<int(ssc_hud::items.size());++i)if(ssc_hud::visible(i,now)){
            const auto& a=ssc_hud::items[i];float x=a.x*width,y=a.y*height,w=a.w*a.scale*width,h=a.h*a.scale*height;
            if(i==ssc_hud::selected)glColor4f(.2f,.85f,1.f,1.f);else glColor4f(.7f,.8f,.9f,.65f);
            glBegin(GL_LINE_LOOP);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
            glBegin(GL_QUADS);glVertex2f(x+w-5,y+h-5);glVertex2f(x+w+5,y+h-5);glVertex2f(x+w+5,y+h+5);glVertex2f(x+w-5,y+h+5);glEnd();
        }
    }
    if(opened||visibility>0.f||clock_enabled) {
    if(!texture){glGenTextures(1,&texture);dirty=true;}
    glEnable(GL_TEXTURE_2D);glBindTexture(GL_TEXTURE_2D,texture);glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812F);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812F);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    bool menu_visible=opened||visibility>0.f;static bool was_menu=true;static int clock_second=-1;
    if(menu_visible!=was_menu){dirty=true;was_menu=menu_visible;}
    static ULONGLONG rainbow_preview_at=0;
    if(menu_visible&&manager&&settings_page==4&&now-rainbow_preview_at>=50){rainbow_preview_at=now;dirty=true;}
    if(menu_visible) {if(dirty){if(ssc_hud::editing)paint_live_hud_toolbar();else paint_panel();glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,raster_w,raster_h,0,0x80E1,GL_UNSIGNED_BYTE,pixels);dirty=false;}}
    else {
        SYSTEMTIME time;GetLocalTime(&time);
        if(dirty||clock_second!=time.wSecond) {
            clock_second=time.wSecond;prepare_canvas();std::memset(pixels,0,raster_w*raster_h*4);
            panel_w=310;panel_h=42;rectangle(0,0,panel_w,panel_h,RGB(8,21,41));
            wchar_t value[80];swprintf(value,80,L"TEST CLOCK  %02u:%02u:%02u",time.wHour,time.wMinute,time.wSecond);
            text(12,13,value);finish_canvas();glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,raster_w,raster_h,0,0x80E1,GL_UNSIGNED_BYTE,pixels);
        }
    }
    float x=menu_visible?float(panel_x):24.f,y=menu_visible?float(panel_y):24.f;
    float w=menu_visible?std::round(panel_w*draw_scale):310.f,h=menu_visible?std::round(panel_h*draw_scale):42.f;
    float u=std::round((menu_visible?panel_w:310.f)*raster_scale)/raster_w,v=std::round((menu_visible?panel_h:42.f)*raster_scale)/raster_h;
    glColor4f(1,1,1,menu_visible?opacity:1.f);
    glBegin(GL_QUADS);glTexCoord2f(0,0);glVertex2f(x,y);glTexCoord2f(u,0);glVertex2f(x+w,y);
    glTexCoord2f(u,v);glVertex2f(x+w,y+h);glTexCoord2f(0,v);glVertex2f(x,y+h);glEnd();
    }
    glMatrixMode(GL_TEXTURE);glPopMatrix();glMatrixMode(GL_MODELVIEW);glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(mode);
    glPopClientAttrib();glPopAttrib();
    if(bind_buffer) bind_buffer(0x88EC,unpack_buffer);
    if(bind_framebuffer) bind_framebuffer(0x8CA9,framebuffer);
    if(use_program) use_program(program);
    if(active_texture) active_texture(active);
}
BOOL WINAPI swap_hook(HDC dc) {render(dc);return original_swap(dc);}
bool accepted_build() {
    wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768)) return false;
    std::ifstream in(std::filesystem::path(path),std::ios::binary|std::ios::ate);
    if(!in||in.tellg()!=15261184) return false;
    in.seekg(0);std::vector<unsigned char> bytes(15261184);in.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(!in) return false;
    BCRYPT_ALG_HANDLE algorithm=nullptr;unsigned char digest[32];
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0) return false;
    BCRYPT_HASH_HANDLE hash=nullptr;
    NTSTATUS result=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0);
    if(result>=0) result=BCryptHashData(hash,bytes.data(),static_cast<ULONG>(bytes.size()),0);
    if(result>=0) result=BCryptFinishHash(hash,digest,32,0);
    if(hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm,0);if(result<0) return false;
    char hex[65];for(int i=0;i<32;++i) std::sprintf(hex+i*2,"%02X",digest[i]);
    return std::string(hex)=="11760857A577DFC2DCA0836614A250EBA31B309249DCC02F4D304A8A075A388A";
}
bool attach_swap() {
    auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
    auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>4096) return false;
    auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC) return false;
    auto size=nt->OptionalHeader.SizeOfImage;
    auto imports=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if(!imports.VirtualAddress || imports.VirtualAddress>=size || imports.Size>size-imports.VirtualAddress) return false;
    auto expected=GetProcAddress(GetModuleHandleW(L"gdi32.dll"),"SwapBuffers");
    for(DWORD offset=0;offset+sizeof(IMAGE_IMPORT_DESCRIPTOR)<=imports.Size;offset+=sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        auto d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base+imports.VirtualAddress+offset);
        if(!d->Name) break;
        if(d->Name>=size||!memchr(base+d->Name,0,size-d->Name)) return false;
        if(_stricmp(reinterpret_cast<char*>(base+d->Name),"GDI32.dll")) continue;
        if(!d->OriginalFirstThunk) return false;
        for(DWORD i=0;;++i) {
            size_t nr=d->OriginalFirstThunk+size_t(i)*8,ar=d->FirstThunk+size_t(i)*8;
            if(nr+8>size||ar+8>size) return false;
            auto name=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+nr);auto address=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+ar);
            if(!name->u1.AddressOfData) break;
            if(IMAGE_SNAP_BY_ORDINAL64(name->u1.Ordinal)) continue;
            size_t rva=name->u1.AddressOfData;
            if(rva+2>=size||!memchr(base+rva+2,0,size-rva-2)) return false;
            if(strcmp(reinterpret_cast<char*>(base+rva+2),"SwapBuffers")) continue;
            if(address->u1.Function!=reinterpret_cast<ULONGLONG>(expected)) {log("SwapBuffers already hooked; refusing conflict");return false;}
            DWORD old;if(!VirtualProtect(&address->u1.Function,8,PAGE_READWRITE,&old)) return false;
            original_swap=reinterpret_cast<Swap>(expected);
            auto prior=InterlockedCompareExchangePointer(reinterpret_cast<PVOID volatile*>(&address->u1.Function),reinterpret_cast<PVOID>(swap_hook),reinterpret_cast<PVOID>(expected));
            DWORD unused;VirtualProtect(&address->u1.Function,8,old,&unused);
            return prior==reinterpret_cast<PVOID>(expected);
        }
    }
    return false;
}
}
extern "C" __declspec(dllexport) void WINAPI SscModInitialize() {
    wchar_t path[32768];DWORD n=GetEnvironmentVariableW(L"SSCMODS_STATE_DIR",path,32768);
    if(n&&n<32768) state_dir=path;
    else {n=GetEnvironmentVariableW(L"LOCALAPPDATA",path,32768);if(!n||n>=32768)return;state_dir=std::filesystem::path(path)/L"SkillshotCityMod";}
    std::error_code error;std::filesystem::create_directories(state_dir,error);if(error)return;
    native_supported=accepted_build();
    load_settings();
    if(!native_supported){ssc_hud::enabled=false;cosmetic_requested=false;sound_requested=false;rpc_requested=false;log("Unsupported executable; compatibility menu only");log(attach_swap()?"Compatibility menu attached; Right Shift opens updater":"Compatibility menu unavailable");return;}
    ssc_names::cosmetics=cosmetic_requested;
    ssc_names::attached=ssc_names::attach();log(ssc_names::attached?"Tab and overhead name adapters attached":"Name adapters unavailable");
    try {
        wchar_t executable[32768];DWORD length=GetModuleFileNameW(nullptr,executable,32768);
        if(length&&length<32768){ssc_sound::initialize(std::filesystem::path(executable).parent_path(),state_dir,sound_requested);ssc_sound::attached=attach_sound();log(ssc_sound::attached?"Audio buffer adapter attached (WAV; restart applies changes)":"Audio adapter unavailable");}
    }catch(const std::exception&){log("Sound catalog unavailable; originals preserved");}
    ssc_hud::attached=ssc_hud::attach();log(ssc_hud::attached?"HUD draw groups attached":"HUD adapter unavailable");
    log(attach_swap()?"Exact build accepted; SwapBuffers import attached":"SwapBuffers attachment refused");
}
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,LPVOID) {if(reason==DLL_PROCESS_ATTACH)DisableThreadLibraryCalls(module);return TRUE;}
