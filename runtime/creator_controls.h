#pragma once
#include "compatibility.h"
#include <GL/gl.h>
#include <atomic>
namespace ssc_creator {
inline std::atomic<bool> hide_crosshair{false};
inline std::atomic<bool> freecam_hide{false};
inline bool attached=false;
using AimDraw=void(*)(uintptr_t,unsigned char);
inline AimDraw original=nullptr;
// Retain native aim/collision calculations and balanced matrix operations.
// Only this world-space aim pass is masked; UI cursors and input are untouched.
struct WriteMask {
 using StencilSeparate=void(APIENTRY*)(GLenum,GLuint);
 GLboolean color[4]{},depth{};GLint stencil=0,back=0;StencilSeparate separate=nullptr;
 WriteMask(){auto proc=wglGetProcAddress("glStencilMaskSeparate");auto address=reinterpret_cast<uintptr_t>(proc);if(address>3&&address!=~uintptr_t(0))separate=reinterpret_cast<StencilSeparate>(proc);glGetBooleanv(GL_COLOR_WRITEMASK,color);glGetBooleanv(GL_DEPTH_WRITEMASK,&depth);glGetIntegerv(GL_STENCIL_WRITEMASK,&stencil);if(separate)glGetIntegerv(0x8ca5,&back);glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);glDepthMask(GL_FALSE);glStencilMask(0);}
 ~WriteMask(){glColorMask(color[0],color[1],color[2],color[3]);glDepthMask(depth);if(separate){separate(GL_FRONT,GLuint(stencil));separate(GL_BACK,GLuint(back));}else glStencilMask(GLuint(stencil));}
};
inline void draw(uintptr_t actor,unsigned char mode){
 if(!original)return;
 if(hide_crosshair.load(std::memory_order_relaxed)||freecam_hide.load(std::memory_order_relaxed)){WriteMask mask;original(actor,mode);}
 else original(actor,mode);
}
inline uint64_t hash(const unsigned char* bytes,size_t count){uint64_t h=14695981039346656037ULL;for(size_t i=0;i<count;++i)h=(h^bytes[i])*1099511628211ULL;return h;}
inline bool matches(const unsigned char* base,size_t size){
 // Independently reviewed Windows build 25847950. Unknown native code leaves
 // this optional visual feature unavailable without affecting public modules.
 if(size<0x8488c8||hash(base+0x844c20,0x3ca8)!=0x196baf33462dbfc3ULL||hash(base+0x3b8b50,0x6e)!=0x9c278909f361f592ULL)return false;
 int32_t relative=0;std::memcpy(&relative,base+0x3b8ba7,4);
 return base[0x3b8ba6]==0xe8&&int64_t(0x3b8bab)+relative==0x844c20;
}
inline bool attach(){
 auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
 if(!base||dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
 if(nt->Signature!=IMAGE_NT_SIGNATURE||!matches(base,nt->OptionalHeader.SizeOfImage))return false;
 unsigned char* bridge=nullptr;auto origin=reinterpret_cast<uintptr_t>(base);
 for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((origin+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};auto target=reinterpret_cast<uintptr_t>(draw);
 std::memcpy(bridge,jump,6);std::memcpy(bridge+6,&target,8);DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}
 FlushInstructionCache(GetCurrentProcess(),bridge,14);auto site=base+0x3b8ba6;
 if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}
 original=reinterpret_cast<AimDraw>(base+0x844c20);
 auto displacement=int32_t(bridge-(site+5));std::memcpy(site+1,&displacement,4);
 FlushInstructionCache(GetCurrentProcess(),site,5);DWORD ignored=0;VirtualProtect(site,5,old,&ignored);return true;
}
}
