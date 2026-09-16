#pragma once
#include <array>
namespace ssc_hud {
struct Item {const wchar_t* name;const char* key;float home_x,home_y,w,h,x,y,scale;};
inline std::array<Item,9> items={{{L"Team roster","team",.76f,.02f,.23f,.13f,.76f,.02f,1},
{L"Event feed","events",.76f,.18f,.23f,.23f,.76f,.18f,1},
{L"Minimap","map",.032f,.724f,.13f,.25f,.032f,.724f,1},
{L"Version / FPS","fps",.032f,.005f,.14f,.025f,.032f,.005f,1},
{L"Round timer","timer",.042f,.050f,.065f,.031f,.042f,.050f,1},
{L"Round / players","round",.032f,.084f,.115f,.032f,.032f,.084f,1},
{L"Money / syringes","currency",.91f,.022f,.085f,.07f,.91f,.022f,1},
{L"Weapons / ammo","weapons",.83f, .78f,.16f,.19f,.83f,.78f,1},
{L"Health / skills","health",.38f,.81f,.24f,.17f,.38f,.81f,1}}};
inline bool enabled=false,attached=false,hover_fade=true;
inline int selected=0;
inline void constrain(Item& item){
    if(!std::isfinite(item.scale))item.scale=1;
    item.scale=std::clamp(item.scale,.25f,6.f);
    if(!std::isfinite(item.x))item.x=item.home_x;
    if(!std::isfinite(item.y))item.y=item.home_y;
    float right=1-item.w*item.scale,bottom=1-item.h*item.scale;
    item.x=std::clamp(item.x,std::min(0.f,right),std::max(0.f,right));item.y=std::clamp(item.y,std::min(0.f,bottom),std::max(0.f,bottom));
}
inline void reset(){for(auto& item:items){item.x=item.home_x;item.y=item.home_y;item.scale=1;}}
inline std::array<float,16> transform(const Item& item){
    float ax=2*item.home_x-1,ay=1-2*item.home_y,bx=2*item.x-1,by=1-2*item.y;
    return {item.scale,0,0,0,0,item.scale,0,0,0,0,1,0,bx-item.scale*ax,by-item.scale*ay,0,1};
}
using Draw=void(__cdecl*)(uintptr_t);
inline std::array<Draw,2> originals{};
template<int Index> void __cdecl draw(uintptr_t object){
    const auto& item=items[Index];
    if(!enabled||(item.scale==1&&item.x==item.home_x&&item.y==item.home_y)){originals[Index](object);return;}
    GLint mode=GL_MODELVIEW;GLfloat projection[16];glGetIntegerv(GL_MATRIX_MODE,&mode);glGetFloatv(GL_PROJECTION_MATRIX,projection);
    auto matrix=transform(item);glMatrixMode(GL_PROJECTION);glLoadMatrixf(matrix.data());glMultMatrixf(projection);glMatrixMode(mode);
    originals[Index](object);
    GLint after=GL_MODELVIEW;glGetIntegerv(GL_MATRIX_MODE,&after);glMatrixMode(GL_PROJECTION);glLoadMatrixf(projection);glMatrixMode(after);
}
// Hook only validated call sites. Renderer hooks forward 14/20 native arguments.
struct Hook {uintptr_t call,target;int item;unsigned stack_bytes;bool map_root=false;bool fade=false;};
inline constexpr Hook hooks[]={
 {0x3b395b,0x434290,0,0},{0x3b3967,0x4355e0,1,0},
 {0x3b34de,0x41e080,2,0,true},{0x3b3839,0x41e080,2,0,true},
 {0x1a7893,0x1a7950,3,0},
 {0x424003,0x3b4230,5,0},
 {0x3b3cf1,0x405660,6,0,true},
 {0x3b3cb2,0x414b30,7,0},
 {0x3b3c80,0x3e5260,8,0},{0x3b3c96,0x4087c0,8,0},
 // These draws are called from the map routine but are not part of the map.
 {0x423faf,0x410fe0,-1,0},{0x423fc8,0x412e80,-1,0},
 {0x423ff3,0x3e46d0,-1,0},{0x424018,0x4140f0,-1,0},
 // Timer icon and its three text branches inside the round/player renderer.
 {0x3b6b6b,0x47fce0,4,128},{0x3b6d94,0x47b010,4,80},
 {0x3b6e3f,0x47b010,4,80},{0x3b6ed5,0x47b010,4,80},
 {0x435707,0x443390,-2,32,false,true},
 {0x3e5366,0x443390,-2,32,false,true},
 {0x405770,0x443390,-2,32,false,true},
 {0x408dbe,0x443390,-2,32,false,true},
 {0x414c28,0x443390,-2,32,false,true},
 {0x41e19e,0x443390,-2,32,false,true},
 {0x3b4460,0x443390,-2,32,false,true},
 {0x3b56d8,0x443390,-2,32,false,true},
 {0x3e47e8,0x443390,-2,32,false,true},
 {0x41124e,0x443390,-2,32,false,true},
 {0x41304c,0x443390,-2,32,false,true},
 {0x41428f,0x443390,-2,32,false,true},
 {0x4348ca,0x443390,-2,32,false,true},
 {0x1c2516,0x1bfae0,-3,112},
 {0x1c2611,0x1bfae0,-3,112},
 {0x1c3b38,0x1bfae0,-3,112},
 {0x1c3c91,0x1bfae0,-3,112},
 {0x1c3db7,0x1bfae0,-3,112},
 {0x1c4e59,0x1bfae0,-3,112},
 {0x1c4f46,0x1bfae0,-3,112},
 {0x47fdf9,0x1bfae0,-3,112},
 {0x48156f,0x1bfae0,-3,112},
 // Screen-wide progress strips share the currency function, but are not counters.
 {0x405b0b,0x47fce0,-1,128},{0x405c38,0x47fce0,-1,128},{0x405d69,0x480460,-1,120}
};
inline uintptr_t game_base=0;
inline bool changed(const Item& item){return item.scale!=1||item.x!=item.home_x||item.y!=item.home_y;}
#include "hud_geometry.h"
inline thread_local const float* map_projection=nullptr;
struct Scope {unsigned stack_bytes;bool active;bool root;const float* previous;GLfloat projection[16];int previous_item;bool capture;};
static_assert(sizeof(Scope)<=0xa0);
inline void begin(Scope& scope,const Hook& hook){
 scope={};scope.stack_bytes=hook.stack_bytes;scope.previous=map_projection;scope.previous_item=active_item;
 if(hook.fade||hook.item==-3)return;
 active_item=hook.item;
 if(hook.item>=0&&capture_frame&&(!measured[hook.item]||editing||enabled))scope.capture=start_capture();
 if(!enabled)return;
 const bool modified=hook.item>=0&&changed(items[hook.item]);
 const bool root_needed=hook.map_root&&(changed(items[2])||changed(items[4])||changed(items[5]));
 if(!modified&&!map_projection&&!root_needed)return;
 GLint mode;glGetIntegerv(GL_MATRIX_MODE,&mode);glGetFloatv(GL_PROJECTION_MATRIX,scope.projection);
 scope.active=true;scope.root=hook.map_root;
 const float* base=map_projection?map_projection:scope.projection;
 if(scope.root)map_projection=scope.projection;
 glMatrixMode(GL_PROJECTION);
 if(modified){auto matrix=transform(items[hook.item]);glLoadMatrixf(matrix.data());glMultMatrixf(base);}
 else glLoadMatrixf(base);
 glMatrixMode(mode);
}
inline void end(Scope& scope){
 if(scope.active){GLint mode;glGetIntegerv(GL_MATRIX_MODE,&mode);glMatrixMode(GL_PROJECTION);glLoadMatrixf(scope.projection);glMatrixMode(mode);}
 if(scope.capture)stop_capture();
 active_item=scope.previous_item;map_projection=scope.previous;
}
using Fade= float(__cdecl*)(uintptr_t,float,float,float,float,float,float,char);
inline void fade_rectangle(const Item& item,float& x,float& y,float& w,float& h,float& feather){
 x=(x-item.home_x*view_w)*item.scale+item.x*view_w;
 y=(y-item.home_y*view_h)*item.scale+item.y*view_h;
 w*=item.scale;h*=item.scale;feather*=item.scale;
}
inline int fade_w=0,fade_h=0;
inline float __cdecl fade_hook(uintptr_t object,float alpha,float x,float y,float w,float h,float feather,char global){
 // Native mouse coordinates use the game's logical canvas, not the GL viewport.
 int width=fade_w?fade_w:view_w,height=fade_h?fade_h:view_h;
 if(game_base==reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
  width=*reinterpret_cast<const int*>(game_base+0xfa1750);height=*reinterpret_cast<const int*>(game_base+0xfa1754);
 }
 if(width<=0||height<=0){width=view_w;height=view_h;}
 if(editing&&active_item>=0)return std::abs(alpha);
 const float native_feather=feather;auto original=reinterpret_cast<Fade>(game_base+0x443390);
 // Move only the proximity test away; preserve native global/status opacity.
 if(!hover_fade)return original(object,alpha,-1000000.f,-1000000.f,0,0,std::max(1.f,feather),global);
 if(enabled&&active_item>=0&&width>0&&height>0){const auto& a=items[active_item];
  if(measured[active_item]){x=a.x*width;y=a.y*height;w=a.w*a.scale*width;h=a.h*a.scale*height;feather*=a.scale;}
  else {x=(x-a.home_x*width)*a.scale+a.x*width;y=(y-a.home_y*height)*a.scale+a.y*height;w*=a.scale;h*=a.scale;feather*=a.scale;}
 }
 float result=original(object,alpha,x,y,w,h,feather,global);
 if(enabled&&active_item==5&&measured[4]){const auto& t=items[4];result=std::min(result,original(object,alpha,t.x*width,t.y*height,t.w*t.scale*width,t.h*t.scale*height,native_feather*t.scale,global));}
 return result;
}
extern "C" void ssc_hud_bridge();
extern "C" __attribute__((used,noinline)) uintptr_t ssc_hud_before(unsigned index,Scope* scope,const unsigned char* stack){begin(*scope,hooks[index]);if(hooks[index].item==-3)capture_queued_rectangle(stack);return hooks[index].fade?reinterpret_cast<uintptr_t>(fade_hook):game_base+hooks[index].target;}
extern "C" __attribute__((used,noinline)) void ssc_hud_after(Scope* scope){end(*scope);}
inline bool attach(){
 const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 for(const auto& hook:hooks){auto p=reinterpret_cast<const unsigned char*>(base+hook.call);int32_t relative;std::memcpy(&relative,p+1,4);if(p[0]!=0xe8||base+hook.call+5+relative!=base+hook.target)return false;}
 unsigned char* bridge=nullptr;
 for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!bridge)return false;
 const uintptr_t entry=reinterpret_cast<uintptr_t>(ssc_hud_bridge);
 for(unsigned i=0;i<std::size(hooks);++i){auto p=bridge+i*32;p[0]=0x41;p[1]=0xba;std::memcpy(p+2,&i,4);const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(p+6,jump,6);std::memcpy(p+12,&entry,8);}
 DWORD old;if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 DWORD protection[std::size(hooks)]{};
 for(unsigned i=0;i<std::size(hooks);++i)if(!VirtualProtect(reinterpret_cast<void*>(base+hooks[i].call),5,PAGE_EXECUTE_READWRITE,&protection[i])){for(int j=int(i)-1;j>=0;--j){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+hooks[j].call),5,protection[j],&ignored);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 game_base=base;
 for(unsigned i=0;i<std::size(hooks);++i){int32_t rel=static_cast<int32_t>(reinterpret_cast<uintptr_t>(bridge+i*32)-(base+hooks[i].call+5));std::memcpy(reinterpret_cast<void*>(base+hooks[i].call+1),&rel,4);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(base+hooks[i].call),5);}
 for(int i=int(std::size(hooks))-1;i>=0;--i){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+hooks[i].call),5,protection[i],&ignored);}return true;
}
}
