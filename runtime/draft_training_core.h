#pragma once
#include "advisor_core.h"
#include "third_party/json.hpp"
#include <string>
namespace ssc_training {
using Json=nlohmann::json;
inline constexpr size_t max_bytes=262144,max_events=512;
inline Json build_json(const ssc_advisor::Build& b){
 Json skills=Json::array(),mutations=Json::array();
 for(int i=0;i<54;++i){if(b.levels[size_t(i)])skills.push_back({i,b.levels[size_t(i)]});if(b.mutated[size_t(i)])mutations.push_back(i);}
 return {{"class",b.player_class+1},{"skills",skills},{"mutations",mutations}};
}
// No arbitrary strings, account identifiers, native pointers, or machine data
// enter this schema. Missing coverage must never be interpreted as zero.
class Round {
 Json document,events;ssc_advisor::Build previous;bool have_build=false,saturated=false;
 Json last_advice[2];int pending[2]={-1,-1};uint64_t start=0;size_t bytes=0;
 struct Cached {ssc_advisor::Build build;std::array<int,54> offers{};std::array<ssc_advisor::Score,54> scores{};unsigned count=0;int best=-1;bool api_valid=false,api_used=false;};Cached cached[2];
public:
 bool active=false;
 void begin(const std::string& id,int number,uint64_t now,bool complete){
  start=now;bytes=0;active=true;have_build=false;saturated=false;events=Json::array();pending[0]=pending[1]=-1;last_advice[0]=last_advice[1]=nullptr;
  document={{"schema",1},{"round_id",id},{"round",number},{"game_build",26075336},{"collector",1},{"model",1},{"start_observed",complete},{"truncated",false}};
 }
 void append(Json event,uint64_t now){
  if(!active||saturated)return;
  event["ms"]=now>=start?now-start:0;const auto size=event.dump().size();
  if(events.size()>=max_events||bytes+size>max_bytes-4096){document["truncated"]=true;saturated=true;return;}
  bytes+=size+1;events.push_back(std::move(event));
 }
 void build(const ssc_advisor::Build& b,uint64_t now){
  if(!active||saturated||b.player_class<0||b.player_class>=8)return;
  if(have_build&&ssc_advisor::same_build(previous,b))return;
  if(have_build){
   int changes=0,id=-1,kind=-1;bool clean=b.player_class==previous.player_class;
   for(int i=0;i<54;++i){auto n=size_t(i);if(b.levels[n]!=previous.levels[n]){++changes;id=i;kind=0;clean=clean&&b.levels[n]==previous.levels[n]+1;}if(b.mutated[n]!=previous.mutated[n]){++changes;id=i;kind=1;clean=clean&&b.mutated[n];}}
   Json choice={{"type",2},{"skill",nullptr},{"kind",nullptr},{"followed",nullptr},{"advice_event",nullptr}};
   if(clean&&changes==1&&kind>=0&&pending[kind]>=0){auto& a=last_advice[kind];bool offered=false;for(const auto& c:a["candidates"])if(c[0]==id)offered=true;
    if(offered){choice["skill"]=id;choice["kind"]=kind;choice["followed"]=a["best"]==id;choice["advice_event"]=pending[kind];}
   }
   // type 2 is inference from a build delta, NEVER an asserted click.
   append(std::move(choice),now);pending[0]=pending[1]=-1;
  }
  previous=b;have_build=true;append({{"type",0},{"build",build_json(b)}},now);
 }
 void advice(bool mutation,const ssc_advisor::Build& b,const std::array<int,54>& offers,unsigned count,int best,const std::array<ssc_advisor::Score,54>& scores,bool api_valid,bool api_used,uint64_t now){
  if(!active||saturated||count>54||best<0||best>=54)return;
  const int kind=mutation?1:0;const auto& old=cached[kind];
  bool same=pending[kind]>=0&&ssc_advisor::same_build(old.build,b)&&old.count==count&&old.best==best&&old.api_valid==api_valid&&old.api_used==api_used;
  for(unsigned i=0;i<count;++i){int id=offers[i];if(id<0||id>=54)return;if(old.offers[i]!=id||old.scores[size_t(id)].value!=scores[size_t(id)].value||old.scores[size_t(id)].evidence!=scores[size_t(id)].evidence)same=false;}
  if(same)return; // No JSON allocation/serialization for unchanged render frames.
  build(b,now);Json candidates=Json::array();bool found=false;
  for(unsigned i=0;i<count;++i){int id=offers[i];if(id<0||id>=54||!std::isfinite(scores[size_t(id)].value))return;candidates.push_back({id,scores[size_t(id)].value,scores[size_t(id)].evidence});found=found||id==best;}
  if(!found)return;
  Json a={{"type",1},{"kind",kind},{"build",build_json(b)},{"candidates",candidates},{"best",best},{"api_available",api_valid},{"api_used",api_used}};
  if(a==last_advice[kind]&&pending[kind]>=0)return;
  last_advice[kind]=a;cached[kind]={b,offers,scores,count,best,api_valid,api_used};const auto index=events.size();append(a,now);pending[kind]=events.size()>index?int(index):-1;
 }
 void metrics(const Json& numbers,uint64_t now){append({{"type",3},{"values",numbers}},now);}
 Json finish(uint64_t now,bool ended){
  if(!active)return nullptr;
  active=false;document["duration_ms"]=now>=start?now-start:0;document["end_observed"]=ended;document["events"]=std::move(events);return std::move(document);
 }
 void discard(){active=false;document=nullptr;events=nullptr;}
};
}
