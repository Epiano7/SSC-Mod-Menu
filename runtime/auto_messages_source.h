#pragma once
#include "quick_chat.h"
#include "auto_messages.h"
#include "combat_stats.h"
namespace ssc_auto {
inline void sample_weapon(uintptr_t record,Values& values){
 using ssc_names::read;
 int tier=-1,type=-1,ammo=-1,mag=-1,rpm=-1,projectiles=-1,extra=-1;float reload=-1;
 static const char* types[]={"Melee","Explosive","Pistol","SMG","Shotgun","Rifle","LMG","Sniper","Misc"};
 if(read(record+0x218,type)&&type>=0&&type<int(std::size(types))){values["weaponType"]=types[type];if(type<2)values["weaponTier"]="0";else if(read(record+0x214,tier)&&tier>=0&&tier<=2)values["weaponTier"]=std::to_string(tier+1);}
 if(read(record+0x278,ammo)&&ammo>=0&&ammo<=1000000)values["ammoRemaining"]=std::to_string(ammo);
 if(read(record+0x4f0,mag)&&mag>=0&&mag<=10000)values["weaponMagazineSize"]=std::to_string(mag);
 if(read(record+0x458,rpm)&&rpm>=0&&rpm<=100000)values["weaponFireRate"]=std::to_string(rpm);
 if(read(record+0x454,reload)&&std::isfinite(reload)&&reload>=0&&reload<=3600){char text[32];std::snprintf(text,sizeof(text),"%.2f",double(reload));values["weaponReloadSeconds"]=text;}
 if(read(record+0x448,projectiles)&&read(record+0x44c,extra)&&projectiles>=0&&extra>=0&&int64_t(projectiles)*(int64_t(extra)+1)<=10000)values["weaponProjectilesPerShot"]=std::to_string(int64_t(projectiles)*(int64_t(extra)+1));
}

// Fingerprint both producers/consumers of world+6aa0 and the native send route.
inline constexpr uint32_t native_dependencies[][3]={{0x88c210,0x2ac720,0},{0x2e80f0,0x221960,0},{0x82dda0,0x8000a0,0},{0x22a170,0x895ad0,0}};
inline bool supported(){return ssc_compat::supports(32)&&ssc_compat::supports(4)&&ssc_compat::supports(8);}
// HUD survival timer (mode 6): 3b9d69 uses ceil(world+3bc) while positive,
// then switches to -world+3cc for overtime. The gameplay countdown at +3bc
// slows in overtime and must never be used as the displayed negative timer.
// Guard the HUD selector/formatter as well as the nominal-duration helper.
inline bool recording_clock_supported(uintptr_t base){
 static uintptr_t checked=0;static bool valid=false;if(checked==base)return valid;checked=base;valid=false;
 struct Span{uint32_t rva,length;uint64_t hash;};const Span spans[]={{0x240920,247,0x38010e2bcab0ebefULL},{0x22f220,36,0x87ef3746199cfe29ULL},{0x3b9d69,91,0x3c848a7ce6f644d6ULL},{0x3bc717,70,0xb6d602facca87efcULL}};
 for(const auto& span:spans){uint64_t hash=14695981039346656037ULL;for(uint32_t i=0;i<span.length;++i){unsigned char byte=0;if(!ssc_names::read(base+span.rva+i,byte))return false;hash=(hash^byte)*1099511628211ULL;}if(hash!=span.hash)return false;}
 float minimum=0,step=0,sign=0;double half=0;
 valid=ssc_names::read(base+0xc76920,minimum)&&minimum==30&&ssc_names::read(base+0xc767a4,step)&&step==5&&ssc_names::read(base+0xc75d20,half)&&half==.5&&ssc_names::read(base+0xc7726c,sign)&&sign==-1;return valid;
}
inline double recording_hud_countdown(float remaining,float overtime){
 if(!std::isfinite(remaining)||remaining>7200||remaining<-14400)return NAN;
 if(remaining>0)return std::ceil(remaining);
 if(!std::isfinite(overtime)||overtime<0||overtime>14400)return NAN;
 return -std::floor(double(overtime));
}
inline double recording_round_seconds(float duration,double remaining){
 if(!std::isfinite(duration)||!std::isfinite(remaining)||duration<0||duration>7200)return -1;
 double elapsed=std::floor(std::max(30.f,duration)/5.f+.5)*5-remaining;return elapsed>=0&&elapsed<=14400?elapsed:-1;
}
inline Observation sample(uintptr_t base,uintptr_t& actor,uintptr_t& world){
 Observation o;actor=world=0;if(!supported())return o;using ssc_names::read;
 int session=0,mode=-1,raw_round=0,last=0,local=-1,slot=-1,kind=-1;double joining=0;float countdown=0,ending=0;uintptr_t first=0,end=0,steam=0;unsigned char active=0,survival=0;
 if(!read(base+ssc_compat::resolve(0xfac614),session)||session!=2||!read(base+ssc_compat::resolve(0xfac628),joining)||!std::isfinite(joining)||joining>0)return o;
 o.connected=true;
 if(!read(base+ssc_compat::resolve(0xde45d0),world)||!world)return o;
 if(!read(world+0x128,mode)||mode!=6||!read(world+0x485,survival)||survival||!read(world+0x3e0,raw_round)||raw_round<1||raw_round>100||!read(world+0x46c,last)||last<raw_round-1||last>100||!read(world+0x3c0,countdown)||!std::isfinite(countdown)||!read(world+0x6ae8,ending)||!std::isfinite(ending))return o;
 if(!read(world+0xa618,first)||!read(world+0xa620,end)||!first||end<first||(end-first)%0x3478||(end-first)/0x3478>2048||!read(base+ssc_compat::resolve(0xe1c0a8),steam)||first!=steam||!read(base+ssc_compat::resolve(0xe18628),local)||local<0||uintptr_t(local)>=(end-first)/0x3478)return o;
 actor=first+uintptr_t(local)*0x3478;
 if(!read(actor+0x78,slot)||slot!=local||!read(actor+0x7dc,kind)||kind!=0||!read(actor+0x81,active)||!active)return o;
 o.actor=actor;o.valid=true;o.active=raw_round>=2&&countdown<=0&&ending<=0;o.ending=raw_round>=2&&(ending>0||countdown>0);o.preparing=countdown>0&&ending<=0;o.session=world;o.round=raw_round-1;o.last_round=last;o.values["roundNumber"]=std::to_string(o.round);
 if(ssc_compat::supports(2)&&o.round>=1&&recording_clock_supported(base)){
  float duration=0,remaining=0,overtime=NAN;
  if(read(world+0x488,duration)&&read(world+0x3bc,remaining)&&(remaining>0||read(world+0x3cc,overtime))){
   double displayed=recording_hud_countdown(remaining,overtime);
   o.recording_round_seconds=recording_round_seconds(duration,displayed);
   if(o.recording_round_seconds>=0)o.recording_countdown_seconds=displayed;
  }
 }
 o.values["roundsTotal"]=std::to_string(o.last_round);o.values["roundsRemaining"]=std::to_string(std::max(0,o.last_round-o.round));
 int level=-1,class_id=-1;float hp=0,max_hp=0;
 // Current XP-bar consumer (compatibility group 2, RVA 40b3a0) reads
 // actor+878 as cumulative round XP before subtracting the level threshold.
 if(ssc_compat::supports(2)){float xp=-1;if(read(actor+0x878,xp)&&std::isfinite(xp)&&xp>=0&&xp<=100000000)o.values["xp"]=std::to_string(xp);}

 if(read(actor+0x870,level)&&level>=0&&level<=10000)o.values["level"]=std::to_string(level);
 if(read(actor+0x13a4,hp)&&std::isfinite(hp)&&hp>=0&&hp<=1000000)o.values["healthRemaining"]=std::to_string(int(std::ceil(hp)));
 else o.eligible=false;
 if(hp<=0)o.eligible=false;
 if(read(actor+0x858,max_hp)&&std::isfinite(max_hp)&&max_hp>0&&max_hp<=1000000)o.values["maxHealth"]=std::to_string(int(std::ceil(max_hp)));
 int deaths=-1;float damage=-1;
 if(read(actor+0x954,deaths)&&deaths>=0&&deaths<=1000000)o.death_total=deaths;
 if(read(actor+0xd14,damage)&&std::isfinite(damage)&&damage>=0&&damage<=100000000)o.damage_total=damage;
 float shield=0;
 if(read(actor+0x13a8,shield)&&std::isfinite(shield)&&shield>=0&&shield<=1000000){o.values["shieldRemaining"]=std::to_string(int(std::ceil(shield)));if(max_hp>0&&max_hp<=1000000&&std::isfinite(max_hp))o.values["shieldPercent"]=std::to_string(int(std::lround(100.0*shield/max_hp)));}
 if(max_hp>0&&max_hp<=1000000&&std::isfinite(max_hp)&&std::isfinite(hp)&&hp>=0)o.values["healthPercent"]=std::to_string(int(std::lround(100.0*hp/max_hp)));
 if(o.values.count("healthRemaining")&&o.values.count("maxHealth"))o.values["healthMissing"]=std::to_string(std::max(0,int(std::ceil(max_hp-hp))));
 // Same actor fields displayed by the validated native currency HUD.
 if(ssc_compat::supports(2)){float money=-1;int syringes=-1;if(read(actor+0x8c0,money)&&std::isfinite(money)&&money>=0&&money<=100000000)o.values["money"]=std::to_string(int(money));if(read(actor+0x8d0,syringes)&&syringes>=0&&syringes<=1000000)o.values["syringes"]=std::to_string(syringes);}
 uintptr_t weapons=0,weapons_end=0;int equipped=-1;
 if(read(actor+0x12a0,weapons)&&read(actor+0x12a8,weapons_end)&&weapons_end>=weapons&&(weapons_end-weapons)%0x748==0&&(weapons_end-weapons)/0x748<=16&&(weapons||!weapons_end)){
  o.values["weaponCount"]=std::to_string((weapons_end-weapons)/0x748);
  for(int i=0;i<3;++i){auto key="weapon"+std::to_string(i+1);if(uintptr_t(i)>=(weapons_end-weapons)/0x748)o.values[key]="None";else {auto name=ssc_rpc::native_string(weapons+uintptr_t(i)*0x748+0x48);if(printable(name))o.values[key]=name;}}
  if(read(actor+0x2998,equipped)&&equipped>=0)for(auto record=weapons;record<weapons_end;record+=0x748){int id=-1;if(read(record+0x20c,id)&&id==equipped){auto name=ssc_rpc::native_string(record+0x48);if(!name.empty()&&printable(name)){o.values["weaponName"]=name;sample_weapon(record,o.values);}break;}}
 }
 uintptr_t classes=0,classes_end=0;
 if(read(actor+0x350,class_id)&&class_id>=0&&read(base+ssc_compat::resolve(0xe11a38),classes)&&read(base+ssc_compat::resolve(0xe11a40),classes_end)&&classes&&classes_end>=classes&&(classes_end-classes)%0x98==0&&(classes_end-classes)/0x98<=512&&uintptr_t(class_id)<(classes_end-classes)/0x98){auto name=ssc_rpc::native_string(classes+uintptr_t(class_id)*0x98+0x18);if(!name.empty())o.values["className"]=name;}
 return o;
}
using NativeSender=void(*)(uintptr_t,unsigned char,uintptr_t,const void*);
inline bool send_native(uintptr_t base,uintptr_t world,uintptr_t actor,const std::string& text,NativeSender sender=nullptr){
 if(!supported()||!printable(text)||!actor||!world)return false;
 float cooldown=0;unsigned char wheel=0;
 if(!ssc_names::read(actor+0x26ec,cooldown)||!std::isfinite(cooldown)||cooldown>0||!ssc_names::read(actor+0x26a2,wheel)||wheel)return false;
 // The native client chat-input path calls this with flag=0. It deep-copies the
 // temporary native-layout string into its own outbound queue, then shows it locally.
 ssc_chat::NativeString message{reinterpret_cast<uintptr_t>(text.data()),0,text.size(),std::max(size_t(16),text.size())};
 if(!sender)sender=reinterpret_cast<NativeSender>(base+ssc_compat::resolve(0x221960));
 sender(world,0,actor,&message);
 // Engine::dispatch limits automatic messages. Observe the native wheel's
 // cooldown above, but never impose an additional lockout on manual input.
 return true;
}
inline void diagnose(const Observation& o,uint64_t now){
 static std::string previous;
 if(diagnostic_dir.empty())return;
 try{
  Json state={{"connected",o.connected},{"valid",o.valid},{"round",o.round},{"active",o.active},{"ending",o.ending},{"preparing",o.preparing},{"tracker_running",round_tracker.running},{"tracker_prepared",round_tracker.prepared},{"timing_complete",round_tracker.complete},{"damage_baseline_available",round_tracker.start_damage>=0},{"damage_available",o.values.count("damageTaken")!=0},{"pending",engine.pending.size()},{"status",std::string(status.begin(),status.end())}};
  auto signature=state.dump();if(signature==previous)return;previous=signature;
  state["tick"]=now;state["damage_counter"]=o.damage_total;state["damage_baseline"]=round_tracker.start_damage;
  auto path=diagnostic_dir/L"auto-message-diagnostics.jsonl";
  std::error_code ec;if(std::filesystem::file_size(path,ec)>131072&&!ec){auto old=diagnostic_dir/L"auto-message-diagnostics.previous.jsonl";std::filesystem::remove(old,ec);std::filesystem::rename(path,old,ec);}
  std::ofstream file(path,std::ios::app);file<<state.dump()<<"\n";
 }catch(...){}
}
// The client collision pass skips remotely simulated actors. Its partial contact
// totals must not be published as complete outgoing hits or damage (including 0).
inline void append_combat_values(Values& values,const ssc_combat::Counter& counter){
 if(!counter.complete)return;
 values["shotsFired"]=std::to_string(counter.totals.shots);
 values["projectilesFired"]=std::to_string(counter.totals.projectiles);
}
// Shared observations feed recording without enabling message dispatch.
inline Observation tracking_observation;
inline uint64_t tracking_at=0;
inline bool tracking_enabled=false;
inline void tick(uint64_t now,bool recording=false){
 static uint64_t last=0;const bool requested=enabled||recording;
 if(requested!=tracking_enabled){last=0;tracking_enabled=requested;round_tracker.reset();tracking_observation={};tracking_at=0;std::lock_guard<std::mutex> lock(ssc_combat::mutex);ssc_combat::counter.reset();}
 if(last&&now>=last&&now-last<100)return;
 if(last&&(now<last||now-last>2000)){std::lock_guard<std::mutex> lock(ssc_combat::mutex);ssc_combat::counter.reset();}
 last=now;
 ssc_combat::enabled.store(requested&&ssc_combat::attached,std::memory_order_relaxed);
 if(!requested){tracking_observation={};tracking_at=0;engine.reset();round_tracker.reset();std::lock_guard<std::mutex> lock(ssc_combat::mutex);ssc_combat::counter.reset();return;}
 try{auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));uintptr_t actor=0,world=0;auto o=sample(base,actor,world);round_tracker.observe(o,now);
 if(ssc_combat::attached){std::lock_guard<std::mutex> lock(ssc_combat::mutex);ssc_combat::Context c;c.valid=o.valid;c.active=o.active;c.ending=o.ending;c.preparing=o.preparing;c.session=o.session;c.actor=actor;c.round=o.round;ssc_combat::counter.observe(c);if(o.valid)append_combat_values(o.values,ssc_combat::counter);}
 tracking_observation=o;tracking_at=now;
 if(enabled){engine.observe(o,now);if(o.valid)engine.dispatch(now,[&](const std::string& text){return send_native(base,world,actor,text);});diagnose(o,now);}else engine.reset();}
 catch(...){tracking_observation={};tracking_at=0;enabled=false;ssc_combat::enabled=false;engine.reset();round_tracker.reset();status=L"Auto messages paused after a data error";}
}
}
