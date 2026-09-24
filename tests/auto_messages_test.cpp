#include "../runtime/auto_messages_source.h"
#include <cassert>
#include <iostream>
static int native_sends=0;
static void capture_send(uintptr_t world,unsigned char flag,uintptr_t actor,const void* ptr){assert(world&&actor&&flag==0);auto& text=*static_cast<const ssc_chat::NativeString*>(ptr);assert(std::string(reinterpret_cast<const char*>(text.pointer),text.size)=="GG!");++native_sends;}
int main(int argc,char** argv){
 assert(ssc_auto::choice_event(0)==0&&ssc_auto::choice_event(1)==2&&ssc_auto::choice_event(2)==3&&ssc_auto::choice_event(3)==4);assert(ssc_auto::event_choice(1)==1&&ssc_auto::event_choice(2)==1);
using namespace ssc_auto;assert(argc==2);auto dir=std::filesystem::path(argv[1]);std::filesystem::create_directories(dir);
 std::string out,error;assert(format("Round [roundNumber]: {level}",examples(),out,error)&&out=="Round 3: 8");assert(!format("{roundKills}",examples(),out,error));assert(!format("[level}",examples(),out,error));assert(!format("{level}",{},out,error));assert(!format("GG\n",examples(),out,error));assert(!format(std::string(99,'x')+"{level}",examples(),out,error));
 Rule r;r.enabled=true;r.event=3;r.threshold=8;r.min_level=3;r.message="Level [level] in round {roundNumber}!";auto c=code(r);Rule imported;assert(decode(c,imported)&&!imported.enabled&&pack(imported)==pack(r));assert(c.size()<100);for(auto bad:{"SSC-AM2:abc","SSC-AM1:!!!!","SSC-AM1:","not a rule"})assert(!decode(bad,imported));assert(!decode(c+"A",imported));assert(!decode(std::string(2000,'a'),imported));
 // Reject impossible values and container-shaped messages.
 try{unpack(Json::array({9,1,0,"GG"}));assert(false);}catch(...){}
 rules={r};enabled=true;assert(save(dir));rules={{}};enabled=false;load(dir);assert(enabled&&rules.size()==1&&rules[0].enabled&&rules[0].message==r.message);
 {std::ofstream f(dir/"auto-messages.json");f<<"{broken";}load(dir);assert(rules[0].message==r.message);
 Observation o;o.valid=o.active=true;o.session=1;o.round=1;o.last_round=3;o.values=examples();o.values["roundNumber"]="1";o.values["level"]="7";
 Engine e;std::vector<std::string> sent;auto sink=[&](const std::string& s){sent.push_back(s);return true;};e.observe(o,1000);assert(e.pending.empty());o.values["level"]="8";e.observe(o,2000);e.dispatch(2000,sink);assert(sent.size()==1&&sent[0]=="Level 8 in round 1!");e.observe(o,3000);assert(e.pending.empty());
 // Once per threshold per round, even after stat drops and rises again.
 o.values["level"]="7";e.observe(o,4000);o.values["level"]="8";e.observe(o,5000);assert(e.pending.empty());
 rules={{true,0,1,0,"Round [roundNumber] done"},{true,1,1,0,"Third round"},{true,2,1,0,"Final round"}};e.reset();o.active=false;o.ending=true;e.observe(o,6000);assert(e.pending.empty()); // joining results isn't participation
 o.ending=false;o.active=true;e.observe(o,7000);o.ending=true;o.active=false;e.observe(o,8000);assert(e.pending.size()==1);e.observe(o,9000);assert(e.pending.size()==1);
 e.dispatch(14000,sink);assert(sent.back()=="Round 1 done");o.round=3;o.values["roundNumber"]="3";o.active=true;o.ending=false;e.observe(o,15000);o.ending=true;o.active=false;e.observe(o,16000);assert(e.pending.size()==3);e.dispatch(16000,sink);assert(e.pending.size()==2);e.dispatch(24000,sink);assert(sent.back()=="Third round");e.dispatch(34000,sink);assert(e.pending.empty()); // stale queued messages expire
 // Disconnect/abort never synthesizes round completion; disabled state drops queue.
 e.reset();o.active=true;o.ending=false;e.observe(o,40000);o.valid=false;e.observe(o,41000);assert(e.pending.empty());o.valid=true;o.active=false;o.ending=true;e.observe(o,42000);assert(e.pending.empty());
 rules={{true,4,30,0,"HP {healthRemaining}"}};e.reset();o.active=true;o.ending=false;o.values["healthRemaining"]="60";e.observe(o,50000);o.values["healthRemaining"]="20";e.observe(o,50100);assert(e.pending.size()==1);enabled=false;e.observe(o,50200);assert(e.pending.empty());
 // Missing variables and conditions suppress a message rather than inventing values.
 enabled=true;rules={{true,0,1,10,"Class {className}"}};e.reset();o.values["level"]="8";e.observe(o,60000);o.active=false;o.ending=true;e.observe(o,61000);assert(e.pending.empty());rules[0].min_level=0;e.reset();o.values.erase("className");o.active=true;o.ending=false;e.observe(o,62000);o.active=false;o.ending=true;e.observe(o,63000);assert(e.pending.empty());


 // Pending messages are cancelled if their rule is changed or disabled.
 rules={{true,0,1,0,"GG"}};e.reset();o.values=examples();o.active=true;o.ending=false;e.observe(o,70000);o.active=false;o.ending=true;e.observe(o,70100);assert(e.pending.size()==1);rules[0].enabled=false;e.dispatch(80000,sink);assert(e.pending.empty());
 // A spectator cannot arm rules just by observing a round; a player's lethal hit can cross a health threshold.
 rules={{true,4,30,0,"HP {healthRemaining}"}};e.reset();o.active=true;o.ending=false;o.eligible=false;o.values["healthRemaining"]="60";e.observe(o,90000);o.values["healthRemaining"]="20";e.observe(o,90100);assert(e.pending.empty());
 e.reset();o.eligible=true;o.values["healthRemaining"]="60";e.observe(o,91000);o.eligible=false;o.values["healthRemaining"]="0";e.observe(o,91100);assert(e.pending.size()==1);

 // AND/OR conditions, repeated sends, and v2 share fields.
 rules={{true,0,1,0,"GG"}};rules[0].name="Auto-GG";rules[0].repeats=3;rules[0].conditions={{0,0,9},{1,1,50}};Rule shared;assert(decode(code(rules[0]),shared)&&shared.name=="Auto-GG"&&shared.repeats==3&&shared.conditions.size()==2&&!shared.enabled);
 e.reset();o.values=examples();o.eligible=true;o.active=true;o.ending=false;e.observe(o,100000);o.active=false;o.ending=true;e.observe(o,100100);assert(e.pending.empty());
 rules[0].any=true;e.reset();o.active=true;o.ending=false;e.observe(o,110000);o.active=false;o.ending=true;e.observe(o,110100);assert(e.pending.size()==3);e.dispatch(110100,sink);assert(e.pending.size()==2);e.dispatch(120100,sink);assert(e.pending.size()==1);e.dispatch(130100,sink);assert(e.pending.empty());
 // Half-second spacing applies across rules and repeats, not per frame.
 e.reset();e.last_send=0;rules={{true,0,1,0,"GG"}};rules[0].repeats=3;o.active=true;o.ending=false;e.observe(o,140000);o.active=false;o.ending=true;e.observe(o,140100);auto count_before=sent.size();
 e.dispatch(140100,sink);e.dispatch(140599,sink);assert(sent.size()==count_before+1&&e.pending.size()==2);e.dispatch(140600,sink);assert(sent.size()==count_before+2);e.dispatch(141100,sink);assert(sent.size()==count_before+3&&e.pending.empty());
 assert(format("Level {currentLevel} / [level]",examples(),out,error)&&out=="Level 8 / 8");
 // Every exposed variable accepts both bracket forms; unavailable never becomes zero.
 for(auto key:variables){auto token=std::string(key);assert(format("["+token+"]",examples(),out,error));assert(format("{"+token+"}",examples(),out,error));assert(!format("["+token+"]",{},out,error));}
 for(int i=0;i<condition_count;++i){Rule condition_rule;condition_rule.conditions={{i,0,1}};assert(decode(code(condition_rule),shared)&&shared.conditions[0].stat==i);}
 Rule expanded;expanded.message="{damageTaken} damage / {timeAlive}s";expanded.conditions={{13,0,50}};assert(decode(code(expanded),shared)&&shared.conditions[0].stat==13);
 // Countdown baselines, active/dead time, immutable end totals, and gaps/late joins.
 RoundTracker tracker;Observation track;track.valid=track.eligible=true;track.session=3;track.round=2;track.values={{"level","3"}};track.death_total=4;track.damage_total=150.5;
 tracker.observe(track,1000);track.active=true;tracker.observe(track,1100);track.values["level"]="5";track.death_total=5;track.damage_total=200.75;track.eligible=false;tracker.observe(track,2100);assert(track.values.at("roundDeaths")=="1"&&track.values.at("damageTaken")=="51"&&track.values.at("levelsGained")=="2"&&track.values.at("timeAlive")=="1");
 track.active=false;track.ending=true;tracker.observe(track,3100);assert(track.values.at("roundTime")=="2"&&track.values.at("deadTime")=="1"&&track.values.at("alivePercent")=="52");track.values.clear();track.damage_total=300;tracker.observe(track,4100);assert(track.values.at("damageTaken")=="51"&&track.values.at("timeAlive")=="1");
 tracker.reset();track.values=examples();track.values.erase("roundTime");track.active=true;track.ending=false;tracker.observe(track,5000);assert(!track.values.count("roundTime"));
 tracker.reset();track.values={{"level","3"}};track.active=false;track.eligible=true;tracker.observe(track,6000);track.active=true;tracker.observe(track,6100);track.values={{"level","4"}};tracker.observe(track,10000);assert(!track.values.count("roundTime")&&!track.values.count("deadTime")&&!track.values.count("alivePercent")&&track.values.count("damageTaken"));
 track.valid=false;tracker.observe(track,10100);assert(!tracker.running);
 // Replay native countdown under the previous round, with zero HP and delayed counter reset.
 tracker.reset();track={};track.valid=true;track.eligible=false;track.preparing=true;track.session=4;track.round=0;track.values={{"level","8"}};track.damage_total=198;track.death_total=0;
 tracker.observe(track,20000);track.round=1;track.active=track.eligible=true;track.preparing=false;track.values={{"level","1"}};tracker.observe(track,20100);
 track.damage_total=0;track.values={{"level","1"}};tracker.observe(track,20600);
 track.damage_total=86;tracker.observe(track,21000);assert(track.values.at("damageTaken")=="86");
 rules={{true,0,1,0,"I took {damageTaken} damage this round!"}};e.reset();e.observe(track,21000);
 track.active=false;track.ending=true;track.damage_total=110;tracker.observe(track,22000);e.observe(track,22000);
 assert(e.pending.size()==1&&e.pending[0].text=="I took 110 damage this round!");
 track.ending=false;track.preparing=true;track.eligible=false;track.values.clear();tracker.observe(track,23000);
 track.round=2;track.active=track.eligible=true;track.preparing=false;tracker.observe(track,23100);
 track.damage_total=0;tracker.observe(track,23200);track.damage_total=25;tracker.observe(track,23500);assert(track.values.at("damageTaken")=="25");

 // Transient actor reconstruction must not erase an observed countdown or participation.
 tracker.reset();track={};track.valid=true;track.session=10;track.round=0;track.damage_total=12;tracker.observe(track,30000);
 auto unavailable=track;unavailable.valid=false;unavailable.connected=true;tracker.observe(unavailable,30100);
 track.round=1;track.active=true;tracker.observe(track,30200);assert(tracker.running);
 track.damage_total=22;track.values.clear();tracker.observe(track,34000);assert(track.values.at("damageTaken")=="10"&&!track.values.count("timeAlive"));
 e.reset();e.observe(track,34000);e.observe(unavailable,34100);track.active=false;track.ending=true;track.preparing=false;tracker.observe(track,34200);e.observe(track,34200);assert(e.pending.size()==1);
 track.values.clear();tracker.observe(track,35000);track.round=2;track.active=true;track.ending=false;tracker.observe(track,35100);assert(tracker.running);
 unavailable.connected=false;tracker.observe(unavailable,35200);e.observe(unavailable,35200);assert(!tracker.running&&e.pending.empty());
 // Exercise the actual game-memory reader against a bounded synthetic layout.
 std::vector<unsigned char> image(0xfb0000),world_mem(0xa600),actor_mem(0x3460),classes(0x98);
 auto base=reinterpret_cast<uintptr_t>(image.data()),world=reinterpret_cast<uintptr_t>(world_mem.data()),actor=reinterpret_cast<uintptr_t>(actor_mem.data()),cls=reinterpret_cast<uintptr_t>(classes.data());
 auto put=[](uintptr_t at,auto value){std::memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));};
 put(base+0xfa335c,2);put(base+0xfa3370,0.0);put(base+0xddb5d0,world);put(world+0x128,6);put(world+0x3e0,4);put(world+0x46c,5);put(world+0x3c0,0.f);put(world+0x6aa0,0.f);put(world+0xa5b8,actor);put(world+0xa5c0,actor+0x3460);put(base+0xe12de8,actor);put(base+0xe0f380,0);
 put(actor+0x78,0);put(actor+0x7c4,0);put(actor+0x81,uint8_t(1));put(actor+0x858,8);put(actor+0x138c,42.f);put(actor+0x840,150.f);put(actor+0x350,0);put(base+0xe18de8,cls);put(base+0xe18df0,cls+0x98);std::memcpy(classes.data()+0x18,"Test class",11);put(cls+0x28,size_t(10));put(cls+0x30,size_t(15));
 put(actor+0x8a8,250.75f);put(actor+0x8b8,2);put(actor+0x1390,75.f);put(actor+0x93c,2);put(actor+0xcfc,50.5f);
 uintptr_t a=0,w=0;auto observed=sample(base,a,w);assert(observed.valid&&observed.active&&!observed.ending&&observed.round==3&&observed.last_round==5&&observed.values.at("className")=="Test class");assert(a==actor&&w==world);assert(observed.values.at("money")=="250"&&observed.values.at("syringes")=="2"&&observed.values.at("healthMissing")=="108"&&observed.values.at("roundsRemaining")=="2"&&observed.values.at("weapon1")=="None");assert(observed.values.at("shieldRemaining")=="75"&&observed.values.at("shieldPercent")=="50"&&observed.values.at("healthPercent")=="28"&&observed.values.at("weaponCount")=="0"&&observed.death_total==2&&observed.damage_total==50.5);
 std::vector<unsigned char> weapon(0x748);auto wp=reinterpret_cast<uintptr_t>(weapon.data());put(actor+0x1288,wp);put(actor+0x1290,wp+0x748);put(actor+0x2980,7);put(wp+0x20c,7);std::memcpy(weapon.data()+0x48,"Test gun",9);put(wp+0x58,size_t(8));put(wp+0x60,size_t(15));observed=sample(base,a,w);assert(observed.values.at("weaponName")=="Test gun"&&observed.values.at("weaponCount")=="1");put(wp+0x214,1);put(wp+0x218,4);put(wp+0x278,7);put(wp+0x4f0,12);put(wp+0x458,180);put(wp+0x454,1.5f);put(wp+0x448,8);put(wp+0x44c,1);
 observed=sample(base,a,w);assert(observed.values.at("weaponTier")=="2"&&observed.values.at("weaponType")=="Shotgun"&&observed.values.at("ammoRemaining")=="7"&&observed.values.at("weaponMagazineSize")=="12"&&observed.values.at("weaponFireRate")=="180"&&observed.values.at("weaponReloadSeconds")=="1.50"&&observed.values.at("weaponProjectilesPerShot")=="16"&&observed.values.at("weapon1")=="Test gun"&&observed.values.at("weapon2")=="None");
 put(wp+0x278,-1);put(wp+0x454,std::numeric_limits<float>::infinity());put(wp+0x448,INT_MAX);put(wp+0x44c,INT_MAX);observed=sample(base,a,w);assert(!observed.values.count("ammoRemaining")&&!observed.values.count("weaponReloadSeconds")&&!observed.values.count("weaponProjectilesPerShot"));
 put(actor+0x2980,8);assert(!sample(base,a,w).values.count("weaponName"));put(actor+0x1390,std::numeric_limits<float>::quiet_NaN());assert(!sample(base,a,w).values.count("shieldRemaining"));
 put(world+0x3c0,10.f);observed=sample(base,a,w);assert(observed.valid&&observed.ending&&observed.preparing&&!observed.active);
 put(world+0x3e0,1);observed=sample(base,a,w);assert(observed.valid&&!observed.ending&&observed.preparing&&!observed.active&&observed.round==0);
 put(world+0x3e0,4);put(world+0x3c0,0.f);
 put(world+0x3e0,6);assert(sample(base,a,w).valid&&sample(base,a,w).round==5);put(world+0x3e0,7);assert(!sample(base,a,w).valid);put(world+0x3e0,4);
 put(world+0x6aa0,8.f);observed=sample(base,a,w);assert(observed.ending&&!observed.active&&observed.values.at("healthRemaining")=="42");
 put(world+0x6aa0,0.f);put(actor+0x138c,0.f);assert(!sample(base,a,w).eligible);put(actor+0x138c,42.f);put(base+0xfa3370,1.0);assert(!sample(base,a,w).valid);put(base+0xfa3370,0.0);put(actor+0x7c4,1);assert(!sample(base,a,w).valid);
 // An automatic message must not consume or change manual-wheel state.
 put(actor+0x26d4,0.f);put(actor+0x26d0,2.5f);put(actor+0x268a,uint8_t(0));auto original_actor=actor_mem;
 assert(send_native(base,world,actor,"GG!",capture_send)&&native_sends==1&&actor_mem==original_actor);
 put(actor+0x26d4,3.f);assert(!send_native(base,world,actor,"GG!",capture_send)&&native_sends==1);
 put(actor+0x26d4,0.f);put(actor+0x268a,uint8_t(1));assert(!send_native(base,world,actor,"GG!",capture_send)&&native_sends==1);
 put(actor+0x268a,uint8_t(0));assert(!send_native(base,world,actor,"",capture_send)&&native_sends==1);
 std::cout<<"PASS: template brackets, unavailable values, bounded share codes, settings, thresholds, round gating, deduplication, cooldown, expiry and disconnect cancellation\n";
}
