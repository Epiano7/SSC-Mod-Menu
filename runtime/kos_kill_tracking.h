#pragma once
#include "kos_native.h"
#include <atomic>
#include <mutex>

namespace ssc_kos {
// Steam build 26820987. The native human-kill cue is queued only after
// resolving a credited local killer and ruling out self-kills and clones.
// It is called from both the replicated death and game simulation paths.
inline constexpr ssc_planned_native::Guard kill_guards[]={
 {0x31ab30,0x1106,0x7fec96df319fad19ULL},
 {0x22cd86,0x3d,0x240887c746ebc16eULL},
 {0x8d4667,0xa4,0xb1c7d6b46a9564f0ULL},
 {0x1d9fe0,64,0xc7c352f88988b6bbULL}
};
inline constexpr uintptr_t kill_cue_site=0x31ac54;
inline constexpr uintptr_t native_queue_cue=0x1d9fe0;
inline bool kill_tracking_supported=false;

struct CreditedKill {
 ssc_names::ActorRange range;
 uintptr_t victim;
 int round;
 Player player;
};
struct ObservedDeath {
 uintptr_t victim;
 std::string account;
 bool seen_dead=false;
};
inline std::atomic<bool> accept_kills{false},kills_pending{false};
inline std::mutex kill_mutex;
inline std::vector<CreditedKill> pending_kills;
inline std::vector<ObservedDeath> recent_deaths;
inline ssc_names::ActorRange death_range;
inline int death_round=0;

inline void collect_kill(uintptr_t killer,uintptr_t victim,uintptr_t world){
 using ssc_names::read;
 const auto range=ssc_names::actor_range();
 if(!range.contains(killer)||!range.contains(victim)||range.world!=world||killer==victim||tournament_anonymous(world))return;
 const auto base=ssc_names::image_base;
 const auto killer_slot=int((killer-range.first)/ssc_names::roster_stride);
 int local=-1,world_local=-1,round=0,kind=-1;
 uintptr_t registry=0;
 if(!read(base+ssc_compat::resolve(0xe23678),local)||local!=killer_slot||
    !read(world+0x6b98,world_local)||world_local!=local||
    !read(base+ssc_compat::resolve(0xe27138),registry)||registry!=range.first||
    !read(killer+0x7dc,kind)||kind!=0||!read(world+0x3e0,round))return;
 auto target=player(victim,range,false);
 if(!eligible(target.target.player))return;
 std::lock_guard<std::mutex> lock(kill_mutex);
 if(accept_kills.load(std::memory_order_relaxed)&&pending_kills.size()<List::capacity){
  pending_kills.push_back({range,victim,round,std::move(target.target.player)});
  kills_pending.store(true,std::memory_order_release);
 }
}

// Both native paths may notify for the same death. Count the first notification
// and keep the latch until the actor has been observed dead and then alive again.
inline bool process_kills(const ssc_names::ActorRange& range,int round,bool enabled_now){
 std::vector<CreditedKill> batch;
 {
  std::lock_guard<std::mutex> lock(kill_mutex);
  batch.swap(pending_kills);
  kills_pending.store(false,std::memory_order_release);
 }
 if(!enabled_now||!(death_range==range)||round!=death_round){
  recent_deaths.clear();
  death_range=range;
  death_round=round;
 }
 if(!enabled_now)return false;
 bool changed=false;
 for(const auto& kill:batch){
  if(!(kill.range==range)||round!=kill.round)continue;
  if(std::any_of(recent_deaths.begin(),recent_deaths.end(),[&](const ObservedDeath& death){
    return death.victim==kill.victim&&death.account==kill.player.account;
  }))continue;
  if(recent_deaths.size()>=List::capacity)continue;
  recent_deaths.push_back({kill.victim,kill.player.account});
  changed=list.spend(kill.player)||changed;
 }
 recent_deaths.erase(std::remove_if(recent_deaths.begin(),recent_deaths.end(),[&](ObservedDeath& death){
  unsigned char dead=0,active=0;
  if(!range.contains(death.victim)||!ssc_names::read(death.victim+0x81,active)||!active||
     !ssc_names::read(death.victim,dead)||dead>1)return true;
  if(dead)death.seen_dead=true;
  return !dead&&death.seen_dead;
 }),recent_deaths.end());
 return changed;
}

inline bool tick_kills(uint64_t now){
 const bool tracking=kill_tracking_supported&&enabled&&!list.entries().empty();
 const bool was_tracking=accept_kills.exchange(tracking,std::memory_order_relaxed);
 static uint64_t next_death_check=0;
 if(!tracking){
  if(was_tracking)process_kills({},0,false);
  return false;
 }
 if(!kills_pending.load(std::memory_order_acquire)&&(recent_deaths.empty()||now<next_death_check))return false;
 next_death_check=now+100;
 const auto range=ssc_names::actor_range();
 int round=0;
 const bool ready=range&&ssc_names::read(range.world+0x3e0,round)&&!tournament_anonymous(range.world);
 if(!process_kills(range,round,ready))return false;
 save();
 return true;
}

using QueueCue=void(*)(uintptr_t,int);
inline QueueCue original_kill_cue=nullptr;
inline void (*observe_credited_kill)(uintptr_t,uintptr_t,uintptr_t)=nullptr;
extern "C" __attribute__((used,noinline)) inline void ssc_kos_credited_cue(uintptr_t killer,int cue,uintptr_t victim,uintptr_t world){
 original_kill_cue(killer,cue);
 if(cue!=0x21)return;
 if(observe_credited_kill){try{observe_credited_kill(killer,victim,world);}catch(...){/* Keep native cue and KOS available. */}}
 if(!accept_kills.load(std::memory_order_relaxed))return;
 try{collect_kill(killer,victim,world);}catch(...){/* Leave the game's kill cue intact. */}
}
extern "C" void ssc_kos_credited_bridge();
asm(".text\n.globl ssc_kos_credited_bridge\n.def ssc_kos_credited_bridge; .scl 2; .type 32; .endef\n"
    "ssc_kos_credited_bridge:\nmovq %r14, %r8\nmovq %r15, %r9\njmp ssc_kos_credited_cue\n");

inline bool attach_kill_tracking(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(kill_tracking_supported)return true;
 if(!ssc_planned_native::kos||!ssc_compat::supports(4))return false;
 for(const auto& guard:kill_guards)if(!ssc_planned_native::check(base,guard))return false;
 const auto at=base+kill_cue_site;
 auto* call=reinterpret_cast<unsigned char*>(at);
 int32_t relative=0;
 std::memcpy(&relative,call+1,sizeof relative);
 if(call[0]!=0xe8||at+5+relative!=base+native_queue_cue)return false;

 unsigned char* bridge=nullptr;
 for(uintptr_t distance=0x10000;distance<0x60000000&&!bridge;distance+=0x10000)
  bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+distance)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};
 const auto target=reinterpret_cast<uintptr_t>(ssc_kos_credited_bridge);
 std::memcpy(bridge,jump,sizeof jump);
 std::memcpy(bridge+sizeof jump,&target,sizeof target);
 DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)||!VirtualProtect(call,5,PAGE_EXECUTE_READWRITE,&old)){
  VirtualFree(bridge,0,MEM_RELEASE);
  return false;
 }
 FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 original_kill_cue=reinterpret_cast<QueueCue>(base+native_queue_cue);
 relative=static_cast<int32_t>(reinterpret_cast<intptr_t>(bridge)-static_cast<intptr_t>(at+5));
 std::memcpy(call+1,&relative,sizeof relative);
 FlushInstructionCache(GetCurrentProcess(),call,5);
 DWORD ignored=0;
 VirtualProtect(call,5,old,&ignored);
 kill_tracking_supported=true;
 return true;
}
}
