#include "../runtime/tracking_recording.h"
#include <cassert>
#include <iostream>
template<class T>void fixture_put(unsigned char* base,size_t at,T value){std::memcpy(base+at,&value,sizeof(value));}
void test_late_join_recovery(){
 using namespace ssc_auto;
 RoundTracker tracker;
 auto observation=[](int round,bool active,bool preparing,bool ending){
  Observation o;o.valid=o.connected=true;o.session=42;o.actor=9;o.round=round;
  o.active=active;o.preparing=preparing;o.ending=ending;o.values={{"level","5"},{"healthRemaining","100"}};
  o.damage_total=70;o.death_total=1;return o;
 };
 auto late=observation(1,true,false,false);tracker.observe(late,1000);assert(!tracker.complete&&!tracker.running);
 auto finish=observation(1,false,false,true);tracker.observe(finish,2000);assert(!tracker.prepared&&!tracker.complete);
 // Recorded failure: ending and preparing are both true during intermission.
 auto countdown=observation(1,false,true,true);tracker.observe(countdown,3000);assert(tracker.prepared&&!tracker.complete);
 auto armed=tracker;
 for(int n=2;n<=3;++n){
  auto start=observation(n,true,false,false);start.damage_total=0;start.death_total=0;tracker.observe(start,4000+(n-2)*4000);
  assert(tracker.complete&&tracker.running&&start.values.at("damageTaken")=="0");
  auto end=observation(n,false,true,true);end.damage_total=20;end.death_total=0;tracker.observe(end,5000+(n-2)*4000);
  assert(tracker.complete&&end.values.at("damageTaken")=="20"&&end.values.at("roundDeaths")=="0");
  assert(ssc_record::tracking_snapshot(end,tracker.complete)["full_round_timing"]==true);
  auto wait=observation(n,false,true,true);wait.damage_total=20;wait.death_total=0;tracker.observe(wait,7000+(n-2)*4000);
 }
 for(int mismatch=0;mismatch<4;++mismatch){
  auto copy=armed;auto next=observation(2,true,false,false);
  if(mismatch==0)next.actor=10;
  if(mismatch==1)next.session=43;
  if(mismatch==2)next.round=3;
  copy.observe(next,mismatch==3?6001:4000);assert(!copy.complete&&!copy.running);
 }
 // Missing the intermission entirely must not promote a later round to complete.
 RoundTracker missing;late=observation(1,true,false,false);missing.observe(late,1000);
 auto next=observation(2,true,false,false);missing.observe(next,2000);assert(!missing.complete);
}
void test_xp_source(const unsigned char* reviewed=nullptr){
 auto* base=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1000000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(base);
 std::vector<unsigned char> world(0xa680),actor(0x3478);auto a=reinterpret_cast<uintptr_t>(actor.data()),w=reinterpret_cast<uintptr_t>(world.data());
 fixture_put(base,0xfad634,2);fixture_put(base,0xde55d0,w);fixture_put(base,0xe1d0b8,a);fixture_put(base,0xe19638,0);
 fixture_put(world.data(),0x128,6);fixture_put(world.data(),0x3e0,2);fixture_put(world.data(),0x46c,3);fixture_put(world.data(),0xa618,a);fixture_put(world.data(),0xa620,a+actor.size());
 fixture_put(actor.data(),0x81,(unsigned char)1);fixture_put(actor.data(),0x870,5);fixture_put(actor.data(),0x13a4,100.f);fixture_put(actor.data(),0x858,100.f);
 uintptr_t got_actor=0,got_world=0;fixture_put(actor.data(),0x878,1250.5f);
 auto observation=ssc_auto::sample(reinterpret_cast<uintptr_t>(base),got_actor,got_world);assert(observation.valid&&observation.values.at("xp")=="1250.500000");
 for(float invalid:{-1.f,100000016.f,float(NAN),float(INFINITY)}){fixture_put(actor.data(),0x878,invalid);auto bad=ssc_auto::sample(reinterpret_cast<uintptr_t>(base),got_actor,got_world);assert(bad.values.count("xp")==0);}
 if(!reviewed)assert(!ssc_auto::recording_clock_supported(reinterpret_cast<uintptr_t>(base)));
 VirtualFree(base,0,MEM_RELEASE);
 if(reviewed){
  // Use a distinct allocation: guards cache their decision per module base.
  auto* native=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1000000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(native);
  for(auto span:std::vector<std::pair<size_t,size_t>>{{0x240cc0,247},{0x22f5c0,36},{0x3b9f09,91},{0x3bc8b7,70},{0xc78068,4},{0xc77eec,4},{0xc77468,8},{0xc789b4,4}})std::memcpy(native+span.first,reviewed+span.first,span.second);
  // Reset the guard's last checked base before testing a potentially reused allocation.
  assert(ssc_auto::recording_clock_supported(reinterpret_cast<uintptr_t>(reviewed)));
  fixture_put(native,0xfad634,2);fixture_put(native,0xde55d0,w);fixture_put(native,0xe1d0b8,a);fixture_put(native,0xe19638,0);
  fixture_put(world.data(),0x488,180.f);fixture_put(world.data(),0x3bc,-31.243f);fixture_put(world.data(),0x3cc,179.9f);
  auto native_o=ssc_auto::sample(reinterpret_cast<uintptr_t>(native),got_actor,got_world);
  assert(native_o.valid&&native_o.recording_countdown_seconds==-179&&native_o.recording_round_seconds==359);
  auto row=ssc_record::tracking_snapshot(native_o,false);assert(row["round_countdown_ms"]==-179000&&row["round_elapsed_ms"]==359000&&row["round_clock_source"]=="hud_survival_v1");
  fixture_put(world.data(),0x3bc,80.2f);fixture_put(world.data(),0x3cc,float(NAN));
  native_o=ssc_auto::sample(reinterpret_cast<uintptr_t>(native),got_actor,got_world);assert(native_o.recording_countdown_seconds==81);
  fixture_put(world.data(),0x3bc,-1.f);native_o=ssc_auto::sample(reinterpret_cast<uintptr_t>(native),got_actor,got_world);assert(std::isnan(native_o.recording_countdown_seconds));
  VirtualFree(native,0,MEM_RELEASE);
 }
}
int main(int argc,char**argv){
 using namespace ssc_record;
 if(argc==3&&std::string(argv[1])=="--native-clock"){
  auto path=std::filesystem::path(argv[2]);HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(file!=INVALID_HANDLE_VALUE);
  HANDLE mapping=CreateFileMappingW(file,nullptr,PAGE_READONLY|SEC_IMAGE_NO_EXECUTE,0,0,nullptr);assert(mapping);void* view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);assert(view);
  assert(ssc_auto::recording_clock_supported(reinterpret_cast<uintptr_t>(view)));test_xp_source(static_cast<const unsigned char*>(view));UnmapViewOfFile(view);CloseHandle(mapping);CloseHandle(file);
  std::cout<<"PASS: reviewed round-clock code and constants match current executable (read-only, no game launch)\n";return 0;
 }
 assert(argc==2);test_xp_source();test_late_join_recovery();
 ssc_auto::Observation o;o.valid=o.active=true;o.session=42;o.round=1;o.values={{"shotsFired","4"},{"level","8"},{"xp","123.5"},{"money","1200"},{"damageTaken","27"},{"bulletHits","9"},{"weaponName","Test weapon"}};
 // Independent HUD formatting cases, including deliberately divergent gameplay time.
 assert(ssc_auto::recording_hud_countdown(179.1f,NAN)==180);
 assert(ssc_auto::recording_hud_countdown(.1f,999)==1);
 assert(ssc_auto::recording_hud_countdown(0,0)==0);
 assert(ssc_auto::recording_hud_countdown(-31.243f,179.9f)==-179);
 assert(ssc_auto::recording_hud_countdown(-22,121.8f)==-121);
 assert(ssc_auto::recording_hud_countdown(-11,240.1f)==-240);
 for(float invalid:{-1.f,float(NAN),float(INFINITY),14401.f})assert(std::isnan(ssc_auto::recording_hud_countdown(-1,invalid)));
 assert(std::isnan(ssc_auto::recording_hud_countdown(NAN,120)));
 assert(ssc_auto::recording_round_seconds(180,ssc_auto::recording_hud_countdown(-31.243f,179.9f))==359);
 assert(ssc_auto::recording_round_seconds(300,225)==75);
 assert(ssc_auto::recording_round_seconds(300,-15)==315);
 assert(ssc_auto::recording_round_seconds(302,299)==1);
 assert(ssc_auto::recording_round_seconds(300,400)==-1);
 assert(ssc_auto::recording_round_seconds(NAN,0)==-1);
 assert(tracking_snapshot(o,true)["round_elapsed_ms"].is_null());
 o.recording_countdown_seconds=-15;assert(tracking_snapshot(o,false)["round_countdown_ms"]==-15000);o.recording_round_seconds=75;assert(tracking_snapshot(o,false)["round_elapsed_ms"]==75000);
 auto j=tracking_snapshot(o,true);assert(j["metrics"]["xp"]==123.5&&j["metrics"]["money"]==1200);assert(j["metrics"]["shotsFired"]==4&&j["metrics"]["level"]==8&&j["metrics"]["damageTaken"]==27);assert(j["metrics"]["bulletHits"].is_null()&&j["metrics"]["healingDone"].is_null()&&j["ability_breakdown"].is_null());assert(j["metrics"]["ammoRemaining"].is_null());
 for(auto value:{"nan","inf","12abc","-1",""}){o.values["xp"]=value;assert(tracking_snapshot(o,true)["metrics"]["xp"].is_null());o.values["level"]=value;assert(tracking_snapshot(o,true)["metrics"]["level"].is_null());}
 o.values["xp"]="123.5";o.values["level"]="8";o.valid=false;assert(tracking_snapshot(o,true)["metrics"]["shotsFired"].is_null());o.valid=true;
 // Changing local actors cannot carry a countdown baseline into another actor.
 ssc_combat::Counter c;ssc_combat::Context x;x.valid=x.preparing=true;x.session=1;x.actor=10;x.round=0;c.observe(x);x.round=1;x.active=true;x.preparing=false;c.fired(x,3);assert(c.complete&&c.totals.projectiles==3);x.actor=11;c.fired(x,4);assert(!c.complete&&c.totals.projectiles==0);
 c.reset();x.active=false;x.preparing=true;c.observe(x);x.active=true;c.fired(x,2);x.active=false;x.preparing=false;c.observe(x);x.active=true;c.fired(x,2);assert(!c.complete&&c.totals.projectiles==2);
 ssc_auto::RoundTracker rt;ssc_auto::Observation ro;ro.valid=true;ro.session=1;ro.actor=1;ro.round=0;ro.values={{"level","1"}};ro.damage_total=0;ro.death_total=0;rt.observe(ro,100);ro.active=true;ro.round=1;rt.observe(ro,200);assert(rt.complete);ro.actor=2;ro.values.clear();rt.observe(ro,300);assert(!rt.complete&&ro.values.count("damageTaken")==0);
 // Recording can consume shared observations with Auto Messages switched off.
 ssc_auto::enabled=false;auto dir=std::filesystem::path(argv[1]);start(dir);
 ssc_auto::tracking_observation=o;ssc_auto::tracking_at=1000;ssc_auto::round_tracker.complete=true;track_observation(1000);assert(state().active);auto match=state().match_id;assert(match.size()==32&&state().segment==1);assert(state().tracking["metrics"]["shotsFired"]==4);assert(ssc_auto::engine.pending.empty());
 o.ending=true;o.active=false;o.values["shotsFired"]="7";ssc_auto::tracking_observation=o;ssc_auto::tracking_at=1100;track_observation(1100);assert(!state().active&&state().tracking["round_end_observed"]==true&&state().tracking["metrics"]["shotsFired"]==7);
 o.round=2;o.active=true;o.ending=false;o.values.erase("shotsFired");ssc_auto::tracking_observation=o;ssc_auto::tracking_at=1200;track_observation(1200);assert(state().active&&state().tracking["metrics"]["shotsFired"].is_null());assert(state().match_id==match&&state().segment==2);
 track_observation(1800);assert(!state().active&&state().tracking["round_end_observed"]==false);stop();
 for(int i=0;i<500&&state().status!=0;++i){Sleep(10);}assert(state().status==0);
 int files=0;bool final=false;for(auto& entry:std::filesystem::directory_iterator(dir)){++files;std::ifstream f(entry.path());std::string line;while(std::getline(f,line)){auto e=Json::parse(line);if(e["event"]=="round_ended"&&e["tracking"]["round"]==1){assert(e["tracking"]["metrics"]["shotsFired"]==7);final=true;}}}assert(files==1&&final);
 // Actor replacement and a new countdown must not merge two players/matches.
 start(dir);o.actor=8;o.round=3;o.active=true;o.ending=false;o.connected=true;
 ssc_auto::tracking_observation=o;ssc_auto::tracking_at=2000;track_observation(2000);auto id1=state().match_id;
 o.actor=9;ssc_auto::tracking_observation=o;ssc_auto::tracking_at=2100;track_observation(2100);assert(state().match_id!=id1&&state().segment==1);auto id2=state().match_id;
 o.round=0;o.active=false;o.preparing=true;ssc_auto::tracking_observation=o;ssc_auto::tracking_at=2200;track_observation(2200);assert(state().match_id!=id2&&!state().active);stop();
 for(int i=0;i<500&&(state().written<4||state().status!=0);++i){Sleep(10);}assert(state().written>=4&&state().status==0);
 assert(std::distance(std::filesystem::directory_iterator(dir),std::filesystem::directory_iterator())==3);
 // One, two and three played rounds each produce exactly one physical file.
 for(int played=1;played<=3;++played){
  auto match_dir=dir/("match-"+std::to_string(played));start(match_dir);o.actor=90+played;o.session=42+played;o.connected=true;o.last_round=3;
  uint64_t tick=10000+played*1000;
  for(int round_number=1;round_number<=played;++round_number){
   o.round=round_number;o.active=true;o.ending=o.preparing=false;ssc_auto::tracking_observation=o;ssc_auto::tracking_at=++tick;track_observation(tick);assert(state().match_open&&state().active);
   auto began=state().started;sample(2,"BR","Fixture",GetTickCount64());
   o.active=false;o.ending=true;ssc_auto::tracking_observation=o;ssc_auto::tracking_at=++tick;track_observation(tick);
   assert(!state().active&&state().started==began);if(round_number<3)assert(state().match_open);else assert(!state().match_open);
  }
  o.valid=false;o.connected=false;ssc_auto::tracking_observation=o;ssc_auto::tracking_at=++tick;track_observation(tick);assert(!state().match_open&&state().enabled);stop();
  for(int i=0;i<500&&state().status!=0;++i){Sleep(10);}assert(state().status==0);
  int count=0,boundaries=0,headers=0,closes=0;for(auto& file:std::filesystem::directory_iterator(match_dir)){++count;std::ifstream f(file.path());std::string line;double previous=0;while(std::getline(f,line)){auto row=Json::parse(line);if(row["event"]=="round_ended")++boundaries;if(row["event"]=="recording_started")++headers;if(row["event"]=="recording_stopped")++closes;if(row.contains("elapsed_ms")){double time=row["elapsed_ms"];assert(time>=previous);previous=time;}}}
  assert(count==1&&headers==1&&closes==1&&boundaries==played);o.valid=true;
 }
 std::cout<<"PASS: numeric validation, missing vs zero, partial-hit suppression, actor isolation, interrupted rounds, recorder-only tracking, final snapshot, stale observations and round isolation\n";
}
