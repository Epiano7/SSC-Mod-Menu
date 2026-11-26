#pragma once
#include "achievement_core.h"
#include "planned_native_guards.h"

namespace ssc_achievements::account_stats {
inline bool profile_supported=false,xp_supported=false,cash_supported=false;
inline void attach(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(!ssc_compat::supports(2))return;
 using ssc_planned_native::check;
 // The native account-totals consumer and profile XP update on build 26950674.
 profile_supported=check(base,{0x630c54,0x25,0x798d160120edc38bULL})&&check(base,{0x2afdbf,0x2f,0x703ff84151eaa8d5ULL});
 // The native XP bar uses cumulative round XP. Cash is the replicated gross earned counter.
 xp_supported=check(base,{0x40df7f,0x46,0xa0029346e0c0ed98ULL});
 cash_supported=check(base,{0x26c867,0x1b,0x08fb101fdcb4dd63ULL})&&check(base,{0x89ea26,6,0x197d729cc78433dbULL});
}
inline int player_id(uintptr_t base){
 uintptr_t loaded=0;
 int id=0;
 const auto offset=ssc_compat::resolve(0xe08ba8);
 return offset&&ssc_names::read(base+ssc_compat::resolve(0xe091f8),loaded)&&loaded&&
        ssc_names::read(base+offset,id)&&id>0?id:0;
}
struct Historical {uint64_t xp=0,kills=0;};
template<class Read> bool history(uintptr_t base,Read read,Historical& out){
 if(!profile_supported)return false;
 int phase=0,kills=-1;
 uintptr_t loaded=0;
 double xp=-1;
 if(!read(base+ssc_compat::resolve(0xfb7734),phase)||phase!=1||
    !read(base+0xe091f8,loaded)||!loaded||!read(base+0xe09418,xp)||
    !std::isfinite(xp)||xp<0||xp>10000000000.||!read(base+0xe09524,kills)||kills<0||kills>1000000000)return false;
 out={uint64_t(xp),uint64_t(kills)};
 return true;
}
inline bool history(uintptr_t base,Historical& out){
 return history(base,[](uintptr_t address,auto& value){return ssc_names::read(address,value);},out);
}

// Credit increases seen in the same eligible round. Rebase at joins and round
// resets, and ignore decreases during play so replicated values cannot replay XP.
struct Gain {
 uintptr_t session=0,actor=0;
 int round=0;
 uint64_t origin=0,peak=0;
 bool active=false,can_reset=false;
 uint64_t observe(const Frame& f,uint64_t value,uint64_t now){
  if(!f.valid||f.round<=0){*this={};return 0;}
  if(!f.active&&!f.ending)return 0;
  if(session!=f.session||actor!=f.actor||round!=f.round){
   const bool next_round=session==f.session&&actor==f.actor&&f.round==round+1;
   *this={f.session,f.actor,f.round,now,value,f.active,next_round};
   return 0;
  }
  if(!active){if(f.active){active=true;peak=value;origin=now;}return 0;}
  if(value<peak){
   // A round change can be replicated before its fresh counters.
   if(can_reset&&now>=origin&&now-origin<=2000){peak=value;can_reset=false;}
   return 0;
  }
  const auto delta=value-peak;
  if(delta){peak=value;can_reset=false;}
  return delta;
 }
};
inline bool import(Engine& engine,const Historical& totals,uint64_t utc){
 if(engine.ledger.account_stats_imported||!utc)return false;
 engine.lifetime(Kind::xp,totals.xp,utc,true);
 engine.lifetime(Kind::kills,totals.kills,utc,true);
 engine.ledger.account_stats_imported=true;
 ++engine.ledger.revision;
 return true;
}
}
