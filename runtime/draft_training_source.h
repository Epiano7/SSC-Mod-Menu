#pragma once
#include "draft_advisor.h"
#include "auto_messages_source.h"
#include "draft_training_native.h"
namespace ssc_training {
inline void tick(uint64_t now,const std::filesystem::path& directory){
 static uintptr_t session=0,actor=0;static int number=-1,finished=-1;static uint64_t sampled=0,metric_at=0;
 const bool access=ssc_advisor::private_access();service().configure(directory,enabled,access);
 service().capture=!enabled?0:!access?1:!ssc_advisor::attached?2:3;
 if(!enabled){round.discard();session=actor=0;number=finished=-1;return;}
 if(!access||!ssc_advisor::attached){if(round.active)service().submit(round.finish(now,false));session=actor=0;number=finished=-1;return;}
 const auto& o=ssc_auto::tracking_observation;
 if(!ssc_auto::tracking_at||now<ssc_auto::tracking_at||now-ssc_auto::tracking_at>500||!o.valid){if(round.active)service().submit(round.finish(now,false));return;}
 if(sampled==ssc_auto::tracking_at)return;
 sampled=ssc_auto::tracking_at;
 if(session!=o.session||actor!=o.actor||number!=o.round){if(round.active)service().submit(round.finish(now,false));session=o.session;actor=o.actor;number=o.round;finished=-1;metric_at=0;}
 if(!round.active&&o.active&&!o.ending&&finished!=number){auto id=random_id();if(!id.empty()){round.begin(id,o.round,now,ssc_auto::round_tracker.complete);have_native=false;}}
 if(!round.active)return;
 service().capture=4;
 ssc_advisor::Build b;uintptr_t a=0,w=0;if(ssc_advisor::build_snapshot(ssc_freecam::observed_context,b,a,w)&&a==o.actor&&w==o.session){round.build(b,now);capture_native(b,now);}
 if(!metric_at||now-metric_at>=10000||o.ending){metric_at=now;Json values=Json::object();
  // Numeric allowlist only. Do not copy the values map or ordinary recordings.
  for(const auto* key:{"level","xp","healthRemaining","maxHealth","shieldRemaining","money","syringes","roundDeaths","damageTaken","timeAlive","deadTime","shotsFired","projectilesFired"}){
   auto it=o.values.find(key);if(it==o.values.end())continue;
   try{size_t used=0;double v=std::stod(it->second,&used);if(used==it->second.size()&&std::isfinite(v)&&v>=0&&v<=100000000)values[key]=v;}catch(...){}
  }round.metrics(values,now);
 }
 if(o.ending){service().submit(round.finish(now,true));finished=number;}
}
}
