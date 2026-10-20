#include "../runtime/draft_training_core.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace ssc_training;Round r;ssc_advisor::Build b;b.player_class=0;std::array<int,54> offers{};offers[0]=2;offers[1]=7;auto scores=ssc_advisor::evaluate(b);
 r.begin(std::string(32,'a'),1,100,false);r.advice(false,b,offers,2,2,scores,false,false,101);r.advice(false,b,offers,2,2,scores,false,false,102);
 b.levels[7]=1;r.build(b,200);r.advice(false,b,offers,2,2,scores,true,true,210);b.levels[2]=1;b.levels[7]=2;r.build(b,300);
 auto j=r.finish(400,true);assert(j["events"].size()==7);assert(j["events"][2]["skill"]==7);assert(j["events"][2]["followed"]==false);assert(j["events"][2]["advice_event"]==1);assert(j["events"][5]["skill"].is_null());assert(j["start_observed"]==false);assert(j["end_observed"]==true);
 std::cout<<j.dump()<<"\n";
 r.begin(std::string(32,'b'),2,0,true);for(int i=0;i<2000;++i)r.metrics({{"xp",i}},i);j=r.finish(2000,false);assert(j["truncated"]==true);assert(j["events"].size()==max_events);assert(j.dump().size()<max_bytes);
 r.discard();assert(r.finish(3000,true).is_null());
}
