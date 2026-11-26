#pragma once
#include "performance_metrics.h"
#include <cstdint>

namespace ssc_names {
inline constexpr uintptr_t roster_stride=0x34b8;
struct ActorRange {
 uintptr_t world=0,first=0,last=0;
 bool operator==(const ActorRange& other)const {
  return world==other.world&&first==other.first&&last==other.last;
 }
 explicit operator bool()const {
  return world&&first&&last>=first&&(last-first)%roster_stride==0&&size()<=2048;
 }
 size_t size()const {return (last-first)/roster_stride;}
 bool contains(uintptr_t actor)const {
  return actor>=first&&actor<last&&(actor-first)%roster_stride==0;
 }
};

// A scan never carries an actor pointer into a different world or vector.
struct RosterSweep {
 ActorRange range;
 uintptr_t next=0;
 uint64_t started_at=0;
 bool started=false;

 bool synchronize(const ActorRange& source) {
  if(range==source)return false;
  *this={};
  range=source;
  return true;
 }

 template<class Visit>
 bool step(uint64_t now,uint64_t interval,double budget_ms,Visit visit) {
  if(!range)return false;
  if(!next){
   if(started&&now>=started_at&&now-started_at<interval)return false;
   started=true;
   started_at=now;
   next=range.first;
  }
  const double deadline=ssc_perf::clock_ms()+budget_ms;
  for(unsigned count=0;next<range.last;++count){
   if(count&&(count==96||ssc_perf::clock_ms()>=deadline))return false;
   const auto actor=next;
   next+=roster_stride;
   visit(actor);
  }
  next=0;
  return true;
 }
};
}
