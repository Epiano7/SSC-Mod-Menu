#pragma once
#include "auto_messages_source.h"
#include "planned_native_guards.h"

namespace ssc_achievements::native_events {
inline uintptr_t image=0;
inline std::atomic<bool> tracking{false},golden_bullet{false},survivor_death{false};
inline bool award_attached=false,death_attached=false;
using Award=void(*)(uintptr_t,int);
using Death=void(*)(uintptr_t,unsigned char,unsigned char,unsigned char);
inline Award original_award=nullptr;
inline Death original_death=nullptr;
struct LiveForm {uintptr_t actor=0,world=0;int round=0,deaths=-1;uint64_t sampled=0;bool survivor=false;};
inline std::mutex form_mutex;
inline LiveForm last_live_form;
inline void capture_live_form(uintptr_t actor,uintptr_t world,int round,uint64_t now){
 LiveForm next;
 if(actor&&world){
  using ssc_names::read;
  unsigned char dead=1;int form=-1,zombie=-1,deaths=-1;
  if(read(actor,dead)&&dead==0&&read(actor+0x8a0,form)&&read(actor+0x8a4,zombie)&&read(actor+0x954,deaths)&&deaths>=0)
   next={actor,world,round,deaths,now,form!=3&&zombie==0};
 }
 std::lock_guard<std::mutex> lock(form_mutex);last_live_form=next;
}
inline bool was_survivor(const ssc_combat::Context& c,int deaths,uint64_t now){
 std::lock_guard<std::mutex> lock(form_mutex);
 const auto& f=last_live_form;
 return f.survivor&&f.actor==c.actor&&f.world==c.session&&f.round==c.round&&f.deaths==deaths&&
        now>=f.sampled&&now-f.sampled<=500;
}

inline void award(uintptr_t stats,int id){
 original_award(stats,id);
 if(id!=0x1b||!tracking.load(std::memory_order_relaxed))return;
 const auto c=ssc_combat::context(image);
 if(c.valid&&c.active&&!c.ending&&ssc_combat::multiplayer(c.session))golden_bullet.store(true,std::memory_order_release);
}
inline void death(uintptr_t actor,unsigned char next,unsigned char flags,unsigned char source){
 bool qualifies=false;
 if(tracking.load(std::memory_order_relaxed)){
  const auto c=ssc_combat::context(image);
  if(c.valid&&c.active&&!c.ending&&c.actor==actor&&ssc_combat::multiplayer(c.session)){
   using ssc_names::read;
   int deaths=-1;float remaining=0,overtime=0;unsigned char dead=1;
   // The incoming packet may already have written the victim's new form.
   if(read(actor,dead)&&dead==0&&read(actor+0x954,deaths)&&was_survivor(c,deaths,GetTickCount64())&&
      read(c.session+0x3bc,remaining)&&read(c.session+0x3cc,overtime)&&std::isfinite(remaining)&&std::isfinite(overtime)){
    const double clock=ssc_auto::recording_hud_countdown(remaining,overtime);
    qualifies=clock>=-120&&clock<=-115;
   }
  }
 }
 original_death(actor,next,flags,source);
 unsigned char dead=0;
 if(qualifies&&ssc_names::read(actor,dead)&&dead==1)survivor_death.store(true,std::memory_order_release);
}
struct Site {uintptr_t address;unsigned char opcode;};
template<size_t N> bool attach_calls(uintptr_t base,const Site (&sites)[N],uintptr_t target,uintptr_t observer){
 for(const auto& site:sites){
  const auto* p=reinterpret_cast<unsigned char*>(base+site.address);
  int32_t rel=0;std::memcpy(&rel,p+1,4);
  if(p[0]!=site.opcode||site.address+5+rel!=target)return false;
 }
 unsigned char* relay=nullptr;
 for(uintptr_t d=0x10000;d<0x60000000&&!relay;d+=0x10000)
  relay=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!relay)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};
 std::memcpy(relay,jump,sizeof jump);std::memcpy(relay+sizeof jump,&observer,sizeof observer);
 DWORD old=0;
 if(!VirtualProtect(relay,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(relay,0,MEM_RELEASE);return false;}
 FlushInstructionCache(GetCurrentProcess(),relay,4096);
 DWORD protect[N]{};
 size_t i=0;
 for(;i<N;++i)if(!VirtualProtect(reinterpret_cast<void*>(base+sites[i].address),5,PAGE_EXECUTE_READWRITE,&protect[i]))break;
 const bool ready=i==N;
 if(ready)for(const auto& site:sites){
  const auto call=base+site.address;
  const auto rel=int32_t(reinterpret_cast<intptr_t>(relay)-static_cast<intptr_t>(call+5));
  std::memcpy(reinterpret_cast<void*>(call+1),&rel,4);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(call),5);
 }
 while(i){--i;DWORD unused;VirtualProtect(reinterpret_cast<void*>(base+sites[i].address),5,protect[i],&unused);}
 if(!ready)VirtualFree(relay,0,MEM_RELEASE);
 return ready;
}
inline void attach(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(!ssc_compat::supports(2)||!ssc_compat::supports(4))return;
 image=base;
 using ssc_planned_native::Guard;
 constexpr Guard award_guards[]={{0x901e50,0x8b,0x2330fe3f110c219fULL},{0x317dce,0x74,0xdea08fb2a2e6cde6ULL},{0x1ae620,0x60,0xbcc68be8854766deULL}};
 constexpr Guard death_guards[]={{0x8d4667,0xa4,0xb1c7d6b46a9564f0ULL},{0x7fb290,0xa0,0xa7641b6d8ac5ca1bULL},
  {0x3bc964,0x5b,0x3d5aa4abfb1ebebbULL},{0x3bf16f,0x47,0x90b6b13777690f0eULL}};
 auto valid=[base](const auto& g){return ssc_planned_native::check(base,g);};
 if(!award_attached&&std::all_of(std::begin(award_guards),std::end(award_guards),valid)){
  original_award=reinterpret_cast<Award>(base+0x1ae620);
  const Site sites[]={{0x901ed8,0xe9},{0x317e3b,0xe8}};
  award_attached=attach_calls(base,sites,0x1ae620,reinterpret_cast<uintptr_t>(award));
 }
 if(!death_attached&&std::all_of(std::begin(death_guards),std::end(death_guards),valid)){
  original_death=reinterpret_cast<Death>(base+0x7fb290);
  const Site site[]={{0x8d4706,0xe8}};
  death_attached=attach_calls(base,site,0x7fb290,reinterpret_cast<uintptr_t>(death));
 }
}
}
