#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <cassert>
#include <cstdio>
#include "../runtime/hud_editor.h"
using U=uintptr_t;
static float expected_alpha;
static int calls;
static float __cdecl draw(U a,U b,float x,float y,float w,float h,int icon,bool flag,int mode,float alpha,float r,float g,float blue,bool last){
 assert(a==11&&b==22&&x==33&&y==44&&w==55&&h==66&&icon==77&&flag&&mode==88&&alpha==expected_alpha&&r==.25f&&g==.5f&&blue==.75f&&!last);++calls;return 12.75f;
}
static bool ready_result=true;
static bool __cdecl ready(U,U){return ready_result;}
static int local_slot=4;
static float __cdecl native_fade(U,float,float x,float,float,float,float,char){return x< -999999?.8f:.12f;}
template<class T,size_t N> static void put(std::array<unsigned char,N>& a,size_t at,T v){std::memcpy(a.data()+at,&v,sizeof(v));}
int main(int argc,char** argv){
 unsigned index=0;for(unsigned i=0;i<std::size(ssc_hud::hooks);++i)if(ssc_hud::hooks[i].item==-5)index=i;assert(index);
 auto stub=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(stub);
 const U entry=reinterpret_cast<U>(ssc_hud::ssc_hud_bridge);const unsigned char code[]={0x41,0xba,0,0,0,0,0xff,0x25,0,0,0,0};
 std::memcpy(stub,code,12);std::memcpy(stub+2,&index,4);std::memcpy(stub+12,&entry,8);DWORD old;
 assert(VirtualProtect(stub,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),stub,4096);
 auto call=reinterpret_cast<decltype(&draw)>(stub);
 ssc_hud::enabled=false;ssc_hud::game_base=reinterpret_cast<U>(draw)-ssc_hud::hooks[index].target;
 auto invoke=[&](float original,float expected){expected_alpha=expected;assert(call(11,22,33,44,55,66,77,true,88,original,.25f,.5f,.75f,false)==12.75f);};
 ssc_cooldown::settings.enabled=true;ssc_hud::skill_icon=77;ssc_hud::skill_pulse=.5f;invoke(.1f,.1f);
 invoke(.1f,.1f); // A pulse never leaks to the next draw.
 ssc_hud::skill_icon=78;ssc_hud::skill_pulse=.5f;invoke(.1f,.1f);
 ssc_hud::skill_icon=77;ssc_hud::skill_pulse=.5f;invoke(.8f,.8f);
 ssc_hud::editing=true;ssc_hud::skill_icon=77;ssc_hud::skill_pulse=.5f;invoke(.1f,.1f);ssc_hud::editing=false;
 ssc_cooldown::settings.enabled=false;invoke(.1f,.1f);assert(calls==6);
 // Full region reveal preserves normal native opacity rather than forcing 1.0.
 ssc_cooldown::settings={};ssc_cooldown::tracker.clear();auto now=GetTickCount64();
 ssc_cooldown::Sample pulse{1,2,0,1,true};ssc_cooldown::tracker.observe(pulse,now,ssc_cooldown::settings);pulse.elapsed=1;ssc_cooldown::tracker.observe(pulse,now,ssc_cooldown::settings);
 ssc_hud::game_base=reinterpret_cast<U>(native_fade)-0x4447e0;ssc_hud::fade_w=1920;ssc_hud::fade_h=1080;ssc_hud::active_item=8;
 assert(std::abs(ssc_hud::fade_hook(0,1,0,0,100,100,20,1)-.8f)<.00001f);
 ssc_hud::active_item=7;assert(ssc_hud::fade_hook(0,1,0,0,100,100,20,1)==.12f);
 ssc_hud::active_item=8;ssc_cooldown::settings.enabled=false;assert(ssc_hud::fade_hook(0,1,0,0,100,100,20,1)==.12f);
 ssc_hud::active_item=-1;ssc_hud::fade_w=ssc_hud::fade_h=0;
 // Observe native-owned fields without mutating the native readiness result.
 std::array<unsigned char,0x240> skill{};std::array<unsigned char,0x2ad0> actor{};
 put(skill,0,9);put(skill,0x10,77);put(skill,0x220,1.f);put(skill,0x224,2.f);
 put(actor,0x78,4);put(actor,0x81,(unsigned char)1);
 U s=reinterpret_cast<U>(skill.data()),a=reinterpret_cast<U>(actor.data());
 ssc_hud::game_base=reinterpret_cast<U>(GetModuleHandleW(nullptr));ssc_compat::initialized=true;ssc_compat::available=7;
 ssc_compat::addresses[0x95c3a0]=uint32_t(reinterpret_cast<U>(ready)-ssc_hud::game_base);
 ssc_compat::addresses[0xe0f380]=uint32_t(reinterpret_cast<U>(&local_slot)-ssc_hud::game_base);
 ssc_cooldown::settings={};ssc_cooldown::tracker.clear();
 assert(ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==0);
 put(skill,0x220,2.f);assert(ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==1.f&&ssc_hud::skill_icon==77);
 ready_result=false;assert(!ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==0);ready_result=true;
 for(int invalid:{0,1,2}){ssc_cooldown::tracker.clear();put(skill,0x220,1.f);ssc_hud::skill_ready_hook(s,a);put(skill,0x220,2.f);
  if(invalid==0){put(actor,0x78,5);}
  if(invalid==1){put(actor,0x81,(unsigned char)0);}
  if(invalid==2){put(actor,0,(unsigned char)1);}
  assert(ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==0);
  put(actor,0x78,4);put(actor,0x81,(unsigned char)1);put(actor,0,(unsigned char)0);
 }
 // A vehicle attachment is not death/inactivity. Native readiness remains the gate.
 put(actor,0x2ac0,U(1));put(skill,0x220,1.f);ssc_hud::skill_ready_hook(s,a);put(skill,0x220,2.f);
 assert(ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==1.f);
 if(argc>1){
  // Map the executable without starting it or resolving imports. Only the
  // validated readiness leaf runs, against synthetic local actor/world records.
  HMODULE image=LoadLibraryExA(argv[1],nullptr,DONT_RESOLVE_DLL_REFERENCES);assert(image);
  auto bytes=reinterpret_cast<unsigned char*>(image);auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(bytes);
  auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(bytes+dos->e_lfanew);auto sec=IMAGE_FIRST_SECTION(nt);std::vector<ssc_compat::Section> sections;
  for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)sections.push_back({sec[i].VirtualAddress,sec[i].Misc.VirtualSize,(sec[i].Characteristics&IMAGE_SCN_MEM_EXECUTE)!=0});
  assert(ssc_compat::inspect(bytes,nt->OptionalHeader.SizeOfImage,sections,nullptr,ssc_compat::function_specs,std::size(ssc_compat::function_specs),ssc_compat::binding_specs,std::size(ssc_compat::binding_specs))==15);
  ssc_hud::game_base=reinterpret_cast<U>(image);
  std::array<unsigned char,0x600> world{};std::array<unsigned char,0x3460> player{};
  U world_address=reinterpret_cast<U>(world.data());std::memcpy(bytes+ssc_compat::resolve(0xddb5d0),&world_address,sizeof(world_address));
  std::memcpy(bytes+ssc_compat::resolve(0xe0f380),&local_slot,sizeof(local_slot));
  put(player,0x78,local_slot);put(player,0x81,(unsigned char)1);a=reinterpret_cast<U>(player.data());
  skill={};put(skill,0,0);put(skill,0x10,77);put(skill,0x224,2.f);put(skill,0x220,1.f);ssc_cooldown::tracker.clear();
  assert(!ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==0);
  put(skill,0x220,2.f);assert(ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==1.f);
  put(skill,0x220,0.f);assert(!ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==0);
  put(world,0x128,6);put(world,0x3e0,2);put(world,0x3c0,1.f);put(skill,0x220,2.f);
  assert(!ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==0); // Native round-start gate wins.
  put(world,0x3c0,0.f);assert(ssc_hud::skill_ready_hook(s,a)&&ssc_hud::skill_pulse==0);
  FreeLibrary(image);std::puts("PASS: current game native cooldown predicate, completion, recast and round-start gate without launching the game");
 }
 VirtualFree(stub,0,MEM_RELEASE);std::puts("PASS: native skill readiness observation and real bridge opacity override preserve all other arguments/results");
}
