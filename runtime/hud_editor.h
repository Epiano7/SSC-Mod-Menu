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
 {0x3b935d,0x43a090,1,0},
 {0x3b8ee9,0x423ec0,2,0,true},{0x3b9240,0x423ec0,2,0,true},
 {0x1a9bf3,0x1a9cb0,3,0},
 {0x429e53,0x3b9c20,5,0},
 {0x3b96f0,0x40b400,6,0,true},
 {0x3b96b1,0x41a950,7,0},
 {0x3b967f,0x3eb130,8,0},{0x3b9695,0x40e560,8,0,false,false,true},
 // These draws are called from the map routine but are not part of the map.
 {0x429dff,0x416dd0,-1,0},{0x429e18,0x418c70,-1,0},
 {0x429e43,0x3ea5a0,-1,0},{0x429e68,0x419f00,-1,0},
 // Timer icon and its three text branches inside the round/player renderer.
 {0x3bc8b2,0x485610,4,128},{0x3bcad9,0x4804d0,4,80},
 {0x3bcb83,0x4804d0,4,80},{0x3bcc18,0x4804d0,4,80},
 {0x43a1b7,0x447e60,-2,32,false,true},
 {0x3eb236,0x447e60,-2,32,false,true},
 {0x40b510,0x447e60,-2,32,false,true},
 {0x40eb5e,0x447e60,-2,32,false,true},
 {0x41aa48,0x447e60,-2,32,false,true},
 {0x423fde,0x447e60,-2,32,false,true},
 {0x3b9e4c,0x447e60,-2,32,false,true},
 {0x3bb2be,0x447e60,-2,32,false,true},
 {0x3ea6b8,0x447e60,-2,32,false,true},
 {0x41703e,0x447e60,-2,32,false,true},
 {0x418e3c,0x447e60,-2,32,false,true},
 {0x41a09f,0x447e60,-2,32,false,true},
 {0x1c4d4f,0x1c2390,-3,112},
 {0x1c4e4a,0x1c2390,-3,112},
 {0x1c638c,0x1c2390,-3,112},
 {0x1c64e5,0x1c2390,-3,112},
 {0x1c660b,0x1c2390,-3,112},
 {0x1c76d2,0x1c2390,-3,112},
 {0x1c77bf,0x1c2390,-3,112},
 {0x48572f,0x1c2390,-3,112},
 {0x486ecf,0x1c2390,-3,112},
 // Screen-wide progress strips share the currency function, but are not counters.
 {0x40b8ab,0x485610,-1,128},{0x40b9d8,0x485610,-1,128},{0x40bb09,0x485dc0,-1,120},
 // Local skill readiness and only its icon draw (not health/background).
 {0x40ee0e,0x962bd0,-4,0},{0x40f582,0x486d60,-5,80}
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
  width=*reinterpret_cast<const int*>(game_base+ssc_compat::resolve(0xfaca10));height=*reinterpret_cast<const int*>(game_base+ssc_compat::resolve(0xfaca14));
 }
 if(width<=0||height<=0){width=view_w;height=view_h;}
 if(editing&&active_item>=0)return std::abs(alpha);
 const float native_feather=feather;auto original=reinterpret_cast<Fade>(game_base+ssc_compat::resolve(0x447e60));
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
    const bool ready=reinterpret_cast<Ready>(game_base+ssc_compat::resolve(0x962bd0))(skill,actor);
    skill_pulse=0;skill_icon=-1;
    if(!ssc_cooldown::settings.enabled){ssc_cooldown::tracker.clear();return ready;}
    // Arguments are native-owned records at a fingerprint-validated HUD call site.
    // Preserve the game's result; only observe the local living player's cooldown.
    const int local=native_field<int>(game_base+ssc_compat::resolve(0xe19638),0);
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
