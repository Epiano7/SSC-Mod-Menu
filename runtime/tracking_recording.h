#pragma once
#include "auto_messages_source.h"
#include "local_recorder.h"
namespace ssc_record {
// Only scalar observations belong in recordings. Never serialize native pointers,
// account identifiers, chat rules, or authorization state.
inline Json tracking_snapshot(const ssc_auto::Observation& o,bool complete){
 Json metrics=Json::object(),availability=Json::object();
 for(const auto* key:ssc_auto::variables){
  metrics[key]=nullptr;
  if(ssc_auto::unavailable_stat(key)){availability[key]="incomplete_online_coverage";continue;}
  auto it=o.values.find(key);
  if(!o.valid||it==o.values.end()){availability[key]="not_observed";continue;}
  const std::string name=key;
  if(name=="className"||name=="weaponName"||name=="weaponType"||name=="weapon1"||name=="weapon2"||name=="weapon3")metrics[key]=it->second;
  else {try{size_t used=0;double value=std::stod(it->second,&used);if(used!=it->second.size()||!std::isfinite(value)||value<0){availability[key]="invalid_observation";continue;}metrics[key]=value;}catch(...){availability[key]="invalid_observation";continue;}}
  availability[key]="observed";
 }
 for(const auto* key:{"damageDealt","healingDone","healingReceived","shieldRestored","roundKills","assists","reloads","skillsUsed","throwablesUsed","accuracy"}){
  metrics[key]=nullptr;availability[key]="authoritative_source_unverified";
 }
 return {{"round",o.round},{"round_end_observed",o.valid&&o.ending},{"full_round_timing",o.valid&&complete},{"metrics",metrics},{"availability",availability},{"ability_breakdown",nullptr},{"ability_breakdown_unavailable_reason","source_attribution_unverified"}};
}
inline void track_observation(uint64_t now){
 static uintptr_t session=0,actor=0;static int round=-1;static uint64_t sampled=0;
 auto& s=state();
 if(!s.enabled){session=actor=0;round=-1;sampled=0;return;}
 const auto& o=ssc_auto::tracking_observation;
 if(!ssc_auto::tracking_at||now<ssc_auto::tracking_at||now-ssc_auto::tracking_at>500||!o.valid){
  end_round();session=actor=0;round=-1;sampled=0;return;
 }
 if(sampled==ssc_auto::tracking_at)return;
 sampled=ssc_auto::tracking_at;
 if(session!=o.session||actor!=o.actor||round!=o.round){end_round();session=o.session;actor=o.actor;round=o.round;}
 if(o.active&&!o.ending)round_state(true);
 if(s.active)s.tracking=tracking_snapshot(o,ssc_auto::round_tracker.complete);
 if(!o.active||o.ending)end_round();
}
}
