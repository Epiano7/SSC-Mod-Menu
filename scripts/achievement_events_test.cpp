#include "../runtime/presence_source.h"
#include "../runtime/achievement_loadout.h"
#include "../runtime/achievement_account_stats.h"
#include "../runtime/achievement_native_events.h"
#include <cassert>
#include <fstream>
#include <iostream>

using namespace ssc_achievements;
namespace events=ssc_achievements::native_events;
constexpr size_t stride=0x34b8,image_size=0x1000000;
static unsigned native_awards=0,native_deaths=0;
template<class T> void write(uintptr_t address,T value){std::memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
void award_passthrough(uintptr_t stats,int id){assert(stats==1&&id>0);++native_awards;}
void death_passthrough(uintptr_t actor,unsigned char next,unsigned char flags,unsigned char source){
 assert(next==1&&flags==4&&source==3);
 write(actor,uint8_t(1));++native_deaths;
}
int main(int argc,char** argv){
 assert(argc==2);
 auto* fixture=static_cast<unsigned char*>(VirtualAlloc(nullptr,image_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 assert(fixture);
 std::ifstream input(argv[1],std::ios::binary);
 assert(input.read(reinterpret_cast<char*>(fixture),image_size));
 auto base=reinterpret_cast<uintptr_t>(fixture);
 fixture[0x901bb0]^=1;fixture[0x8d43c7]^=1;
 events::attach(base);assert(!events::award_attached&&!events::death_attached);
 fixture[0x901bb0]^=1;fixture[0x8d43c7]^=1;
 events::attach(base);assert(events::award_attached&&events::death_attached);
 events::original_award=award_passthrough;
 events::original_death=death_passthrough;
 events::tracking=true;

 std::vector<unsigned char> world_data(0x10000),actors(stride*2);
 auto world=reinterpret_cast<uintptr_t>(world_data.data()),actor=reinterpret_cast<uintptr_t>(actors.data());
 write(base+0xfb78ac,2);write(base+0xfb78c0,0.0);write(base+0xdef5d0,world);write(base+0xe27138,actor);write(base+0xe23678,0);
 write(world+0x128,6);write(world+0x6a39,uint8_t(1));write(world+0x160,2);
 write(world+0x3e0,2);write(world+0x46c,3);write(world+0x3c0,-1.f);
 write(world+0xa658,actor);write(world+0xa660,actor+actors.size());write(actor+0x81,uint8_t(1));
 assert(ssc_combat::multiplayer(world));assert(ssc_combat::context(base).active);
 events::award(1,2);assert(!events::golden_bullet.exchange(false));
 write(world+0x160,1);events::award(1,0x1b);assert(events::golden_bullet.exchange(false));
 write(world+0x160,0);assert(ssc_combat::multiplayer(world));write(world+0x160,2);
 write(world+0x2d2,uint8_t(1));events::award(1,0x1b);assert(!events::golden_bullet.exchange(false));write(world+0x2d2,uint8_t(0));
 write(world+0x121,uint8_t(1));events::award(1,0x1b);assert(!events::golden_bullet.exchange(false));write(world+0x121,uint8_t(0));
 write(world+0x485,uint8_t(1));assert(!ssc_combat::multiplayer(world));write(world+0x485,uint8_t(0));
 events::award(1,0x1b);assert(events::golden_bullet.exchange(false)&&native_awards==5);

 auto death=[&](float overtime,bool survivor,bool expected,bool stale=false){
  write(actor,uint8_t(0));write(actor+0x8a4,int(!survivor));write(actor+0x954,int(native_deaths));
  const auto now=GetTickCount64();
  events::capture_live_form(actor,world,1,stale?now-501:now);
  write(actor+0x8a4,1); // Same packet already contains the new form.
  write(world+0x3cc,overtime);
  events::death(actor,1,4,3);
  assert(events::survivor_death.exchange(false)==expected);
 };
 death(114.99f,true,false);
 death(115.f,true,true);
 death(117.6f,true,true);
 death(120.99f,true,true);
 death(121.f,true,false);
 death(117.f,false,false);
 death(117.f,true,false,true);
 write(world+0x6a39,uint8_t(0));death(117.f,true,false);write(world+0x6a39,uint8_t(1));
 assert(native_deaths==8);

 account_stats::attach(base);
 assert(account_stats::profile_supported&&account_stats::xp_supported&&account_stats::cash_supported);
 write(base+0xe091f8,uintptr_t(1));write(base+0xe09418,100000000.0);write(base+0xe09524,11849);
 write(base+0xe08ba8,123456);assert(account_stats::player_id(base)==123456);
 account_stats::Historical history;
 assert(!account_stats::history(base,history));
 write(base+0xfb78ac,1);
 assert(account_stats::history(base,history)&&history.xp==100000000&&history.kills==11849);
 write(base+0xe09418,std::numeric_limits<double>::quiet_NaN());
 assert(!account_stats::history(base,history));
 write(base+0xe09418,100000000.0);

 Engine imported;
 for(auto kind:{Kind::xp,Kind::kills,Kind::cash})imported.capability(kind,true);
 assert(account_stats::import(imported,history,1795000000));
 for(auto id:{"xp_100000","xp_100000000","kills_100","kills_5000","kills_10000"})
  assert(imported.ledger.items[index(id)].unlocked_at==1795000000);
 assert(imported.notice(1)&&imported.notice(1)->item==index("xp_100000000"));
 assert(imported.notice(6001)&&imported.notice(6001)->item==index("kills_10000"));
 assert(!imported.notice(12001));
 Ledger restored;
 assert(Ledger::parse(imported.ledger.json(),restored)&&restored.account_stats_imported);
 imported.replace(restored);
 assert(!account_stats::import(imported,history,1795000001)&&!imported.has_notice());
 imported.activity(Kind::cash,300000,1795000002);
 assert(imported.notice(1)&&imported.notice(1)->item==index("cash_10000"));
 assert(imported.notice(6001)&&imported.notice(6001)->item==index("cash_100000"));
 assert(imported.notice(12001)&&imported.notice(12001)->item==index("cash_300000"));
 assert(!imported.notice(18001));

 Frame gain;gain.valid=gain.active=true;gain.session=world;gain.actor=actor;gain.round=1;
 account_stats::Gain delta;
 assert(delta.observe(gain,100,10)==0);
 assert(delta.observe(gain,600,20)==500);
 assert(delta.observe(gain,100,30)==0);
 assert(delta.observe(gain,600,40)==0);
 assert(delta.observe(gain,800,50)==200);
 gain.round=2;
 assert(delta.observe(gain,800,100)==0);
 assert(delta.observe(gain,0,110)==0);
 assert(delta.observe(gain,550,120)==550);
 gain.valid=false;
 assert(delta.observe(gain,800,130)==0);
 gain.valid=true;gain.actor=actor+stride;
 assert(delta.observe(gain,2000,140)==0);
 assert(delta.observe(gain,3000,150)==1000);


 Engine engine;
 for(auto kind:{Kind::invisibility,Kind::syringes,Kind::syringes_match})engine.capability(kind,true);
 Frame f;f.valid=f.active=true;f.session=world;f.actor=actor;f.round=1;
 LoadoutProgress progress;
 auto observe=[&]{progress.observe(f,loadout(actor,[](auto where,auto& value){return ssc_names::read(where,value);}),engine,1795000000);};
 auto get=[&](const char* id){return engine.ledger.items[index(id)];};
 std::array<unsigned char,0x240> skill{};
 std::array<int,4> applied={7,9,12,10};
 const auto skill_address=reinterpret_cast<uintptr_t>(skill.data()),applied_address=reinterpret_cast<uintptr_t>(applied.data());
 write(actor+0x10b8,skill_address);write(actor+0x10c0,skill_address+skill.size());write(skill_address,7);
 write(actor+0x3c0,applied_address);write(actor+0x3c8,applied_address);write(actor+0x8d0,4);
 observe();assert(!get("draft_invisibility").unlocked_at&&!get("syringes_match_6").unlocked_at);
 write(skill_address+0x1ec,1);write(actor+0x3c8,applied_address+8);
 observe();observe();
 assert(get("draft_invisibility").unlocked_at&&get("syringes_10").value==2&&get("syringes_match_6").unlocked_at);
 ++f.round;write(actor+0x3c8,applied_address);observe();
 write(actor+0x3c8,applied_address+4);observe();
 assert(get("syringes_10").value==3&&progress.syringes_used==3);
 f.valid=false;write(actor+0x3c8,applied_address+16);observe();
 assert(get("syringes_10").value==3&&progress.syringes_used==0);
 VirtualFree(fixture,0,MEM_RELEASE);
 std::cout<<"PASS exact game guards, award passthrough, practice exclusion, survivor overtime boundaries, automatic account import, tier notices, multiplayer cash and XP gains, confirmed draft and syringe progression\n";
}
