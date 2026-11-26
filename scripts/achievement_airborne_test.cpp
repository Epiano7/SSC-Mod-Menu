#include "../runtime/achievement_airborne.h"
#include <cassert>
#include <fstream>
#include <iostream>

namespace flight=ssc_achievements::airborne;
using ssc_achievements::Engine;
using ssc_achievements::Kind;
constexpr size_t actor_size=0x34b8;
constexpr size_t image_size=0x1000000;
static int movement_calls=0,cue_calls=0;
static bool landing=false,friendly=false;

template<class T>void write(uintptr_t address,T value){std::memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
static int native_move(uintptr_t actor,float seconds,unsigned char first,unsigned char second,unsigned char third){
 assert(seconds==0.01f&&first==1&&second==0&&third==1);
 ++movement_calls;
 if(landing){
  write(actor+0x34,0.f);
  write(actor+0x2e74,1.f);
  write(actor+0x1a28,uint16_t(0));
 }
 return 42;
}
static float native_floor(uintptr_t map,const float* xy,unsigned char first,unsigned char second,int filter){
 assert(map&&xy&&first==0&&second==0&&filter==-1);
 return 0.f;
}
static unsigned char native_allies(uintptr_t victim,uintptr_t killer,float range,unsigned char active){
 assert(victim!=killer&&range==0&&active==1);
 return friendly;
}
static void native_cue(uintptr_t,int cue){assert(cue==0x21);++cue_calls;}

int main(int argc,char** argv){
 assert(argc==2);
 auto* fixture=static_cast<unsigned char*>(VirtualAlloc(nullptr,image_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 assert(fixture);
 std::ifstream input(argv[1],std::ios::binary);
 assert(input.read(reinterpret_cast<char*>(fixture),image_size));
 auto base=reinterpret_cast<uintptr_t>(fixture);

 // The checked fixture patches both real call sites. A changed guard keeps all
 // original call targets and the KOS observer intact.
 ssc_planned_native::kos=true;
 assert(ssc_kos::attach_kill_tracking(base));
 std::array<std::array<unsigned char,5>,2> original{};
 for(size_t i=0;i<2;++i)std::memcpy(original[i].data(),fixture+flight::movement_sites[i],5);
 fixture[flight::guards[2].rva]^=1;
 assert(!flight::attach(base));
 assert(!ssc_kos::observe_credited_kill);
 for(size_t i=0;i<2;++i)assert(!std::memcmp(original[i].data(),fixture+flight::movement_sites[i],5));
 fixture[flight::guards[2].rva]^=1;
 assert(flight::attach(base)&&ssc_kos::observe_credited_kill==flight::credited_kill);
 for(size_t i=0;i<2;++i){
  auto at=base+flight::movement_sites[i];
  int32_t relative=0;
  std::memcpy(&relative,reinterpret_cast<const void*>(at+1),4);
  assert(at+5+relative!=base+flight::movement_rva);
  uintptr_t target=0;
  std::memcpy(&target,reinterpret_cast<const void*>(at+5+relative+6),8);
  assert(target==reinterpret_cast<uintptr_t>(flight::move));
 }

 // Exercise the installed callbacks with deterministic native stand-ins.
 // The saved game code is used for validation only.
 flight::original_move=native_move;flight::terrain=native_floor;flight::allies=native_allies;
 ssc_kos::original_kill_cue=native_cue;ssc_kos::accept_kills=false;
 ssc_names::image_base=base;
 std::vector<unsigned char> world_data(0x10000), actors(4*actor_size);
 const auto world=reinterpret_cast<uintptr_t>(world_data.data()), killer=reinterpret_cast<uintptr_t>(actors.data());
 auto actor=[&](int slot){return killer+slot*actor_size;};
 write(base+0xfb7734,2);write(base+0xfb7748,0.0);
 write(base+0xdef5d0,world);write(base+0xe27138,killer);write(base+0xe23678,0);
 write(base+0xdef5d8,world);write(base+0xfb5cb4,0.01f);
 write(world+0x128,6);write(world+0x6a39,uint8_t(1));write(world+0x160,2);write(world+0x3e0,2);write(world+0x46c,3);write(world+0x3c0,-1.f);
 write(world+0xa658,killer);write(world+0xa660,killer+actors.size());
 for(int slot=0;slot<4;++slot){
  write(actor(slot)+0x78,slot);write(actor(slot)+0x81,uint8_t(1));
  write(actor(slot)+0x209,uint8_t(1)); // Anonymous names never enter this path.
 }
 auto sky=[&](){write(killer+0x34,20.f);write(killer+0x2e74,-1.f);write(killer+0x1a28,uint16_t(0x100));};
 auto kill=[&](int slot){ssc_kos::ssc_kos_credited_cue(killer,0x21,actor(slot),world);};
 auto move=[&](){assert(flight::move(killer,0.01f,1,0,1)==42);};
 Engine engine;
 engine.capability(Kind::air_knife,true);
 auto submit=[&](){engine.airborne_knives(flight::take_progress(killer,true),true,true,1795000000);};
 auto progress=[&](){return engine.ledger.items[ssc_achievements::index("air_knife_3")];};
 auto reset=[&](){flight::take_progress(0,false);assert(!flight::take_progress(killer,true));sky();};

 reset();kill(1);kill(1);kill(2);
 assert(flight::combo.count==2);
 landing=true;move();landing=false;
 assert(flight::combo.count==0);
 kill(3); // The game's positive landing timer cannot credit this kill.
 assert(flight::combo.count==0);
 sky();kill(3);submit();
 assert(progress().value==2&&!progress().unlocked_at);

 reset();kill(1);kill(2);
 write(killer+0x34,0.f); // Current contact beats a stale falling flag.
 move();
 sky();kill(3);
 assert(flight::combo.count==1);
 submit();assert(!progress().unlocked_at);

 reset();kill(1);kill(2);write(killer+0x954,1);kill(3);
 assert(flight::combo.count==1);
 write(world+0x3e0,3);kill(2);assert(flight::combo.count==1);
 write(world+0x3e0,1);kill(3);assert(flight::combo.count==1);
 write(world+0x3e0,2);
 reset();friendly=true;kill(1);friendly=false;
 write(actor(2)+0x7dc,1);kill(2);write(actor(2)+0x7dc,0);
 write(actor(3)+0x173c,1);kill(3);write(actor(3)+0x173c,0);
 kill(0);
 assert(flight::combo.count==0);
 flight::take_progress(0,false);kill(1);
 assert(flight::combo.count==0);
 assert(cue_calls>0&&!ssc_kos::accept_kills);

 reset();kill(1);kill(2);kill(3);
 assert(flight::combo.count==3);
 landing=true;move();landing=false;
 submit();assert(progress().value==3&&progress().unlocked_at&&movement_calls==3);
 // Ordinary jumps can have no special aerial-spawn timer. The native
 // contact and floor checks still define a single flight.
 Engine jump_engine;
 jump_engine.capability(Kind::air_knife,true);
 reset();
 write(killer+0x2e74,0.f);
 write(killer+0x1a29,uint8_t(0)); // Movement's cached airborne byte may lag the position.
 write(killer+0x54,4.f);
 kill(1);
 assert(flight::combo.count==1);
 write(killer+0x54,0.f);
 move();kill(2);
 assert(flight::combo.count==2);
 write(killer+0x54,-4.f);
 move();kill(3);
 jump_engine.airborne_knives(flight::take_progress(killer,true),true,true,1795000000);
 assert(jump_engine.ledger.items[ssc_achievements::index("air_knife_3")].unlocked_at);

 reset();write(killer+0x2e74,0.f);kill(1);kill(2);
 landing=true;move();landing=false;
 assert(flight::combo.count==0);
 kill(3);
 assert(flight::combo.count==0);
 sky();write(killer+0x2e74,0.f);kill(3);
 assert(flight::combo.count==1);
 flight::take_match_knives({},0,false);
 engine.capability(Kind::melee_match,true);
 for(unsigned round=1;round<=3;++round){
  write(world+0x3e0,int(round)+1);sky();
  for(int slot=1;slot<=3;++slot){kill(slot);kill(slot);}
  const auto count=flight::take_match_knives(ssc_names::actor_range(),round,true);
  assert(count==round*3);
  engine.event(Kind::melee_match,count,1795000000);
 }
 assert(!engine.ledger.items[ssc_achievements::index("melee_match_10")].unlocked_at);
 write(actor(1),uint8_t(1));
 flight::take_match_knives(ssc_names::actor_range(),3,true);
 write(actor(1),uint8_t(0));
 flight::take_match_knives(ssc_names::actor_range(),3,true);
 kill(1);
 engine.event(Kind::melee_match,flight::take_match_knives(ssc_names::actor_range(),3,true),1795000000);
 assert(engine.ledger.items[ssc_achievements::index("melee_match_10")].unlocked_at);
 auto saved=engine.ledger.json();
 ssc_achievements::Ledger restored;
 assert(ssc_achievements::Ledger::parse(saved,restored));
 assert(restored.items[ssc_achievements::index("air_knife_3")].unlocked_at==1795000000);
 VirtualFree(fixture,0,MEM_RELEASE);
 std::cout<<"PASS exact guards, both movement call sites, KOS-independent native credit, distinct enemies, ordinary jump ascent/apex/descent, landing before next frame, floor contact, life/round boundaries and unlock persistence\n";
}
