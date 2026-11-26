#pragma once
#include "achievement_core.h"
#include "combat_stats.h"

namespace ssc_achievements {
struct LoadoutSnapshot {int purchased_invisibility=-1,unused_syringes=-1;bool stims_known=false;uint64_t stimmed=0;};
template<class Read> LoadoutSnapshot loadout(uintptr_t actor,Read read){
 LoadoutSnapshot result;
 int unused=-1;
 if(read(actor+0x8d0,unused)&&unused>=0&&unused<=1000)result.unused_syringes=unused;
 uintptr_t first=0,last=0;
 if(read(actor+0x10b8,first)&&read(actor+0x10c0,last)&&last>=first&&(last-first)%0x240==0&&(last-first)/0x240<=54&&(first||!last)){
  int purchased=0;bool valid=true;
  for(auto item=first;item<last;item+=0x240){
   int id=-1;
   if(!read(item,id)||id<0||id>=54){valid=false;break;}
   if(id==7){valid=read(item+0x1ec,purchased)&&purchased>=0&&purchased<=100;break;}
  }
  if(valid)result.purchased_invisibility=purchased;
 }
 if(read(actor+0x3c0,first)&&read(actor+0x3c8,last)&&last>=first&&(last-first)%4==0&&(last-first)/4<=54&&(first||!last)){
  result.stims_known=true;
  for(auto item=first;item<last;item+=4){
   int id=-1;
   if(!read(item,id)||id<0||id>=54){result.stims_known=false;break;}
   result.stimmed|=uint64_t(1)<<id;
  }
  uintptr_t observed_first=0,observed_last=0;
  if(!read(actor+0x3c0,observed_first)||!read(actor+0x3c8,observed_last)||observed_first!=first||observed_last!=last)result.stims_known=false;
 }
 return result;
}
struct LoadoutProgress {
 uintptr_t world=0,actor=0;
 int round=0,invisibility=-1;
 uint64_t seen=0,syringes_used=0;
 bool stim_baseline=false;
 void observe(const Frame& f,const LoadoutSnapshot& current,Engine& engine,uint64_t utc){
  if(!f.valid){*this={};return;}
  if(f.session!=world||f.actor!=actor||f.round<round){*this={};world=f.session;actor=f.actor;}
  if(round!=f.round){round=f.round;invisibility=-1;stim_baseline=false;seen=0;}
  if(!f.active||f.ending)return;
  if(current.purchased_invisibility>=0){
   if(invisibility>=0&&current.purchased_invisibility>invisibility)engine.event(Kind::invisibility,1,utc);
   invisibility=current.purchased_invisibility;
  }
  if(current.stims_known){
   unsigned uses=0;
   for(uint64_t added=current.stimmed&~seen;added;added&=added-1)++uses;
   if(stim_baseline&&uses){syringes_used+=uses;engine.activity(Kind::syringes,uses,utc);}
   stim_baseline=true;seen|=current.stimmed;
  }
  if(current.unused_syringes>=0)engine.event(Kind::syringes_match,syringes_used+unsigned(current.unused_syringes),utc);
 }
};
}
