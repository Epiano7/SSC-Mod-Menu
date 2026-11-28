#pragma once
#include "achievement_core.h"
#include "combat_stats.h"
#include "kos_kill_tracking.h"

namespace ssc_achievements::airborne {
// Verified on Steam build 26992189. Both movement paths share the floor test
// and landing transition. The credited-kill call has its own KOS guards.
inline constexpr ssc_planned_native::Guard guards[]={
 {0x22ecc7,0x3e,0xce16ce5148934324ULL},
 {0x2ee073,0x3c,0x20c7b64748a62fb8ULL},
 {0x7ff6b0,0x220e,0x1e404028e22c7f42ULL},
 {0x7bc9d9,0xb3,0xb2b38f4b59ff4e16ULL},
 {0x7a9140,0x13d,0xed870680b0094b98ULL},
 {0x5835b0,0x236,0x18a8b56812ed792cULL}
};
inline constexpr uintptr_t movement_sites[]={0x22ece6,0x2ee092};
inline constexpr uintptr_t movement_rva=0x7ff6b0;
using Movement=int(*)(uintptr_t,float,unsigned char,unsigned char,unsigned char);
using Terrain=float(*)(uintptr_t,const float*,unsigned char,unsigned char,int);
using Allies=unsigned char(*)(uintptr_t,uintptr_t,float,unsigned char);
inline Movement original_move=nullptr;
inline Terrain terrain=nullptr;
inline Allies allies=nullptr;
inline uintptr_t image_base=0;
inline bool attached=false;
inline std::atomic<uintptr_t> tracked_actor{0};
inline std::atomic<bool> partial{false},progress_ready{false};
inline std::mutex mutex;
inline AirborneKnifeCombo combo;
struct KnifeDeath {uintptr_t actor;bool seen_dead=false;};
inline std::vector<KnifeDeath> knife_deaths;
inline uintptr_t match_world=0;
inline int match_round=0;
inline unsigned match_knives=0,pending_knives=0;

inline bool is_airborne(uintptr_t actor){
 using ssc_names::read;
 float clearance=0;
 unsigned char dead=1,contact=1;
 uintptr_t vehicle=0,map=0;
 std::array<float,3> position{};
 // The movement routine derives its airborne flag from current clearance
 // and contact. Its separate drop timer can be zero during a jump.
 if(!read(actor,dead)||dead!=0||!read(actor+0x1a28,contact)||contact!=0||
    !read(actor+0x2ae0,vehicle)||vehicle||
    !read(actor+0x2c,position)||!std::all_of(position.begin(),position.end(),[](float v){return std::isfinite(v);})||
    !read(image_base+0xdef5d8,map)||!map||
    !read(image_base+0xfb7e38,clearance)||!std::isfinite(clearance)||clearance<0||clearance>128)return false;
 const float floor=terrain(map,position.data(),0,0,-1);
 return std::isfinite(floor)&&position[2]-floor>clearance;
}

inline void check_landing(uintptr_t actor){
 if(tracked_actor.load(std::memory_order_relaxed)!=actor||!partial.load(std::memory_order_relaxed)||is_airborne(actor))return;
 std::lock_guard<std::mutex> lock(mutex);
 if(tracked_actor.load(std::memory_order_relaxed)==actor){combo.land();partial=false;}
}

inline int move(uintptr_t actor,float step,unsigned char first,unsigned char second,unsigned char third){
 check_landing(actor);
 const int result=original_move(actor,step,first,second,third);
 check_landing(actor);
 return result;
}

inline unsigned take_progress(uintptr_t actor,bool enabled){
 if(tracked_actor.load(std::memory_order_relaxed)==actor&&!progress_ready.load(std::memory_order_acquire))return 0;
 std::lock_guard<std::mutex> lock(mutex);
 const auto progress=combo.take_best();
 progress_ready=false;
 if(!enabled||tracked_actor.load(std::memory_order_relaxed)!=actor){
  combo.land();partial=false;tracked_actor=actor;
 }
 return enabled?progress:0;
}

inline unsigned take_match_knives(const ssc_names::ActorRange& range,int round,bool retain){
 std::lock_guard<std::mutex> lock(mutex);
 if(!retain||range.world!=match_world||round<match_round){knife_deaths.clear();match_world=0;match_round=0;match_knives=0;}
 else knife_deaths.erase(std::remove_if(knife_deaths.begin(),knife_deaths.end(),[&](KnifeDeath& item){
  unsigned char dead=0,active=0;
  if(!range.contains(item.actor)||!ssc_names::read(item.actor,dead)||dead>1||
     !ssc_names::read(item.actor+0x81,active)||active!=1)return true;
  if(dead)item.seen_dead=true;
  return !dead&&item.seen_dead;
 }),knife_deaths.end());
 const auto result=pending_knives;pending_knives=0;
 return result;
}
inline void credited_kill(uintptr_t killer,uintptr_t victim,uintptr_t world){
 if(killer!=tracked_actor.load(std::memory_order_relaxed)||!killer||!victim||victim==killer)return;
 const auto context=ssc_combat::context(image_base);
 if(!context.valid||!context.active||context.ending||context.session!=world||context.actor!=killer||!ssc_combat::multiplayer(world))return;
 const auto range=ssc_names::actor_range();
 if(range.world!=world||!range.contains(victim))return;
 using ssc_names::read;
 int source=-1,kind=-1,deaths=-1;
 if(!read(victim+0x173c,source)||source!=0||!read(victim+0x7dc,kind)||kind!=0||
    !read(killer+0x954,deaths)||deaths<0||allies(victim,killer,0,1))return;
 const bool airborne=is_airborne(killer);
 std::lock_guard<std::mutex> lock(mutex);
 if(killer!=tracked_actor.load(std::memory_order_relaxed))return;
 if(match_world!=world||context.round<match_round){match_world=world;match_knives=0;knife_deaths.clear();}
 if(match_round!=context.round){match_round=context.round;knife_deaths.clear();}
 if(match_knives<10&&knife_deaths.size()<32&&std::none_of(knife_deaths.begin(),knife_deaths.end(),[&](const auto& d){return d.actor==victim;})){
  unsigned char dead=0;
  ssc_names::read(victim,dead);
  knife_deaths.push_back({victim,dead==1});pending_knives=std::max(pending_knives,++match_knives);
 }
 combo.kill({world,killer,context.round,deaths},victim,source,airborne,true);
 if(combo.best)progress_ready.store(true,std::memory_order_release);
 partial=combo.count!=0&&combo.count<combo.victims.size();
}

inline bool attach(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(attached)return true;
 if(!ssc_kos::kill_tracking_supported||!ssc_compat::supports(2)||!ssc_compat::supports(4))return false;
 for(const auto& guard:guards)if(!ssc_planned_native::check(base,guard))return false;
 for(auto site:movement_sites){
  const auto* p=reinterpret_cast<const unsigned char*>(base+site);
  int32_t rel=0;std::memcpy(&rel,p+1,sizeof rel);
  if(p[0]!=0xe8||site+5+rel!=movement_rva)return false;
 }
 unsigned char* bridge=nullptr;
 for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)
  bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};
 const auto entry=reinterpret_cast<uintptr_t>(move);
 std::memcpy(bridge,jump,sizeof jump);
 std::memcpy(bridge+sizeof jump,&entry,sizeof entry);
 DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}
 FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 DWORD previous[2]{};
 for(size_t i=0;i<std::size(movement_sites);++i){
  if(VirtualProtect(reinterpret_cast<void*>(base+movement_sites[i]),5,PAGE_EXECUTE_READWRITE,&previous[i]))continue;
  while(i){--i;DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+movement_sites[i]),5,previous[i],&ignored);}
  VirtualFree(bridge,0,MEM_RELEASE);
  return false;
 }
 image_base=base;
 original_move=reinterpret_cast<Movement>(base+movement_rva);
 terrain=reinterpret_cast<Terrain>(base+0x5835b0);
 allies=reinterpret_cast<Allies>(base+0x7a9140);
 for(size_t i=0;i<std::size(movement_sites);++i){
  const auto site=base+movement_sites[i];
  const auto rel=int32_t(reinterpret_cast<intptr_t>(bridge)-static_cast<intptr_t>(site+5));
  std::memcpy(reinterpret_cast<void*>(site+1),&rel,sizeof rel);
  FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(site),5);
 }
 for(size_t i=std::size(movement_sites);i>0;--i){
  DWORD ignored;
  VirtualProtect(reinterpret_cast<void*>(base+movement_sites[i-1]),5,previous[i-1],&ignored);
 }
 ssc_kos::observe_credited_kill=credited_kill;
 return attached=true;
}
}
