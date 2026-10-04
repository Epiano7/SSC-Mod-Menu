#pragma once
#include "compatibility.h"
#include "cooldown_pulse.h"
#include <array>
namespace ssc_hud {
struct Item {const wchar_t* name;const char* key;float home_x,home_y,w,h,x,y,scale;};
inline std::array<Item,9> items={{{L"","team",.76f,.02f,.23f,.13f,.76f,.02f,1},
{L"Event feed","events",.76f,.18f,.23f,.23f,.76f,.18f,1},
{L"Minimap","map",.032f,.724f,.13f,.25f,.032f,.724f,1},
{L"Version / FPS","fps",.032f,.005f,.14f,.025f,.032f,.005f,1},
{L"Round timer","timer",.042f,.050f,.065f,.031f,.042f,.050f,1},
{L"Round / players","round",.032f,.084f,.115f,.032f,.032f,.084f,1},
{L"Money / syringes","currency",.91f,.022f,.085f,.07f,.91f,.022f,1},
{L"Weapons / ammo","weapons",.83f, .78f,.16f,.19f,.83f,.78f,1},
{L"Health / skills","health",.38f,.81f,.24f,.17f,.38f,.81f,1}}};
inline bool enabled=false,attached=false,hover_fade=true;
// Slot zero is retained only for legacy saved layouts; v0.992 removed its panel.
inline bool available(int index){return index>0&&index<int(items.size());}
inline int next_item(int index,int direction){
 for(size_t n=0;n<items.size();++n){index=(index+(direction<0?int(items.size())-1:1))%int(items.size());if(available(index))return index;}
 return 1;
}
inline int selected=1;
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
struct Hook {uintptr_t call,target;int item;unsigned stack_bytes;bool map_root=false;bool fade=false;bool skills_root=false;};
inline Hook hooks[]={
 {0x3b91bd,0x43a030,1,0},
 {0x3b8d49,0x423e60,2,0,true},{0x3b90a0,0x423e60,2,0,true},
 {0x1a9b43,0x1a9c00,3,0},
 {0x429df3,0x3b9a80,5,0},
 {0x3b9550,0x40b3a0,6,0,true},
 {0x3b9511,0x41a8f0,7,0},
 {0x3b94df,0x3eb0d0,8,0},{0x3b94f5,0x40e500,8,0,false,false,true},
 // These draws are called from the map routine but are not part of the map.
 {0x429d9f,0x416d70,-1,0},{0x429db8,0x418c10,-1,0},
 {0x429de3,0x3ea540,-1,0},{0x429e08,0x419ea0,-1,0},
 // Timer icon and its three text branches inside the round/player renderer.
 {0x3bc712,0x4855b0,4,128},{0x3bc939,0x480470,4,80},
 {0x3bc9e3,0x480470,4,80},{0x3bca78,0x480470,4,80},
 {0x43a157,0x447e00,-2,32,false,true},
 {0x3eb1d6,0x447e00,-2,32,false,true},
 {0x40b4b0,0x447e00,-2,32,false,true},
 {0x40eafe,0x447e00,-2,32,false,true},
 {0x41a9e8,0x447e00,-2,32,false,true},
 {0x423f7e,0x447e00,-2,32,false,true},
 {0x3b9cac,0x447e00,-2,32,false,true},
 {0x3bb11e,0x447e00,-2,32,false,true},
 {0x3ea658,0x447e00,-2,32,false,true},
 {0x416fde,0x447e00,-2,32,false,true},
 {0x418ddc,0x447e00,-2,32,false,true},
 {0x41a03f,0x447e00,-2,32,false,true},
 {0x1c4c9f,0x1c22e0,-3,112},
 {0x1c4d9a,0x1c22e0,-3,112},
 {0x1c62c1,0x1c22e0,-3,112},
 {0x1c641a,0x1c22e0,-3,112},
 {0x1c6540,0x1c22e0,-3,112},
 {0x1c75e5,0x1c22e0,-3,112},
 {0x1c76d2,0x1c22e0,-3,112},
 {0x4856cf,0x1c22e0,-3,112},
 {0x486e6f,0x1c22e0,-3,112},
 // Screen-wide progress strips share the currency function, but are not counters.
 {0x40b84b,0x4855b0,-1,128},{0x40b978,0x4855b0,-1,128},{0x40baa9,0x485d60,-1,120},
 // Local skill readiness and only its icon draw (not health/background).
 {0x40edae,0x9629d0,-4,0},{0x40f522,0x486d00,-5,80}
};
inline uintptr_t game_base=0;
inline bool changed(const Item& item){return item.scale!=1||item.x!=item.home_x||item.y!=item.home_y;}
#include "hud_geometry.h"
inline thread_local const float* map_projection=nullptr;
inline thread_local float skill_pulse=0;
inline thread_local int skill_icon=-1;
struct Scope {unsigned stack_bytes;unsigned alpha_offset;float alpha;bool skills_root;bool active;bool root;const float* previous;GLfloat projection[16];int previous_item;bool capture;};
static_assert(sizeof(Scope)<=0xa0);
static_assert(offsetof(Scope,alpha_offset)==4&&offsetof(Scope,alpha)==8);
inline void begin(Scope& scope,const Hook& hook){
 scope={};scope.stack_bytes=hook.stack_bytes;scope.previous=map_projection;scope.previous_item=active_item;scope.skills_root=hook.skills_root;
 if(scope.skills_root){skill_pulse=0;skill_icon=-1;}
 if(hook.fade||hook.item<=-3)return;
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
 if(scope.skills_root){skill_pulse=0;skill_icon=-1;}
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
  width=*reinterpret_cast<const int*>(game_base+ssc_compat::resolve(0xfab9f0));height=*reinterpret_cast<const int*>(game_base+ssc_compat::resolve(0xfab9f4));
 }
 if(width<=0||height<=0){width=view_w;height=view_h;}
 if(editing&&active_item>=0)return std::abs(alpha);
 const float native_feather=feather;auto original=reinterpret_cast<Fade>(game_base+ssc_compat::resolve(0x447e00));
 // Move only the proximity test away; preserve native global/status opacity.
 if(!hover_fade)return original(object,alpha,-1000000.f,-1000000.f,0,0,std::max(1.f,feather),global);
 if(enabled&&active_item>=0&&width>0&&height>0){const auto& a=items[active_item];
  if(measured[active_item]){x=a.x*width;y=a.y*height;w=a.w*a.scale*width;h=a.h*a.scale*height;feather*=a.scale;}
  else {x=(x-a.home_x*width)*a.scale+a.x*width;y=(y-a.home_y*height)*a.scale+a.y*height;w*=a.scale;h*=a.scale;feather*=a.scale;}
 }
 float result=original(object,alpha,x,y,w,h,feather,global);
 // Reveal the whole native health/skill region, preserving its normal opacity,
 // backgrounds, key labels and status fades instead of brightening only an icon.
 if(active_item==8){float pulse=ssc_cooldown::tracker.reveal(GetTickCount64(),ssc_cooldown::settings);
  if(pulse>0){float normal=original(object,alpha,-1000000.f,-1000000.f,0,0,std::max(1.f,feather),global);
   result+=(normal-result)*pulse;}}
 if(enabled&&active_item==5&&measured[4]){const auto& t=items[4];result=std::min(result,original(object,alpha,t.x*width,t.y*height,t.w*t.scale*width,t.h*t.scale*height,native_feather*t.scale,global));}
 return result;
}
using Ready=bool(__cdecl*)(uintptr_t,uintptr_t);
template<class T> inline T native_field(uintptr_t object,size_t offset){T v;std::memcpy(&v,reinterpret_cast<const void*>(object+offset),sizeof(v));return v;}
inline bool __cdecl skill_ready_hook(uintptr_t skill,uintptr_t actor){
    const bool ready=reinterpret_cast<Ready>(game_base+ssc_compat::resolve(0x9629d0))(skill,actor);
    skill_pulse=0;skill_icon=-1;
    if(!ssc_cooldown::settings.enabled){ssc_cooldown::tracker.clear();return ready;}
    // Arguments are native-owned records at a fingerprint-validated HUD call site.
    // Preserve the game's result; only observe the local living player's cooldown.
    const int local=native_field<int>(game_base+ssc_compat::resolve(0xe18628),0);
    if(!skill||!actor||local<0||native_field<int>(actor,0x78)!=local||native_field<unsigned char>(actor,0)!=0||
       !native_field<unsigned char>(actor,0x81)){ssc_cooldown::tracker.clear();return ready;}
    ssc_cooldown::Sample sample{actor,native_field<int>(skill,0),native_field<float>(skill,0x220),native_field<float>(skill,0x224),ready};
    skill_icon=native_field<int>(skill,0x10);
    skill_pulse=ssc_cooldown::tracker.observe(sample,GetTickCount64(),ssc_cooldown::settings);
    return ready;
}
inline void prepare_skill_alpha(Scope& scope,const unsigned char* stack){
    if(!ssc_cooldown::settings.enabled||editing||skill_pulse<=0||!stack)return;
    int icon;float alpha;std::memcpy(&icon,stack+0x10,4);std::memcpy(&alpha,stack+0x28,4);
    if(icon==skill_icon&&std::isfinite(alpha)&&alpha>=0&&alpha<skill_pulse){scope.alpha_offset=0x28;scope.alpha=skill_pulse;}
    skill_pulse=0;skill_icon=-1;
}
extern "C" void ssc_hud_bridge();
extern "C" __attribute__((used,noinline)) uintptr_t ssc_hud_before(unsigned index,Scope* scope,const unsigned char* stack){
    const auto& hook=hooks[index];begin(*scope,hook);
    if(hook.item==-3)capture_queued_rectangle(stack);
    if(hook.item==-4)return reinterpret_cast<uintptr_t>(skill_ready_hook);
    // The group fade hook now restores the complete native appearance.
    return hook.fade?reinterpret_cast<uintptr_t>(fade_hook):game_base+hook.target;
}
extern "C" __attribute__((used,noinline)) void ssc_hud_after(Scope* scope){end(*scope);}
inline bool attach(){
 if(!ssc_compat::supports(2))return false;
 for(auto& hook:hooks){hook.call=ssc_compat::resolve(uint32_t(hook.call));hook.target=ssc_compat::resolve(uint32_t(hook.target));}
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
