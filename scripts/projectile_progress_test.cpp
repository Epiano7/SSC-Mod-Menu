#include "../runtime/auto_messages_source.h"
#include "../runtime/achievement_core.h"
#include <cassert>
#include <iostream>
static int native_calls=0,result=1;
static int native_spawn(uintptr_t,unsigned char,uintptr_t,unsigned char,uintptr_t,uintptr_t,float,float,unsigned char,unsigned char,unsigned char){++native_calls;return result;}
template<class T>void put(unsigned char* p,size_t at,T v){std::memcpy(p+at,&v,sizeof v);}
int main(){
 using namespace ssc_combat;
 auto* base=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1000000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(base);
 std::vector<unsigned char> world(0xb0000),actor(0x34b8),weapon(0x748),foreign(0x34b8);
 auto w=reinterpret_cast<uintptr_t>(world.data()),a=reinterpret_cast<uintptr_t>(actor.data()),gun=reinterpret_cast<uintptr_t>(weapon.data());
 put(base,0xfb78ac,2);put(base,0xfb78c0,0.0);put(base,0xdef5d0,w);put(base,0xe27138,a);put(base,0xe23678,0);
 put(world.data(),0x128,6);put(world.data(),0x6a39,uint8_t(1));put(world.data(),0x160,2);put(world.data(),0x3e0,2);put(world.data(),0x46c,3);put(world.data(),0x3c0,-1.f);put(world.data(),0x6ae8,0.f);
 put(world.data(),0xa658,a);put(world.data(),0xa660,a+0x34b8);put(actor.data(),0x78,0);put(actor.data(),0x7dc,0);put(actor.data(),0x81,static_cast<unsigned char>(1));
 put(actor.data(),0x12a0,gun);put(actor.data(),0x12a8,gun+0x748);put(weapon.data(),0x218,2);
 put(actor.data(),0x870,5);put(actor.data(),0x13a4,100.f);put(actor.data(),0x858,100.f);
 uintptr_t observed_actor=0,observed_world=0;auto basic=ssc_auto::sample(reinterpret_cast<uintptr_t>(base),observed_actor,observed_world,false);auto rich=ssc_auto::sample(reinterpret_cast<uintptr_t>(base),observed_actor,observed_world,true);assert(basic.valid&&rich.valid&&basic.values.at("level")==rich.values.at("level")&&!basic.values.count("weaponCount")&&rich.values.at("weaponCount")=="1");
 image_base=reinterpret_cast<uintptr_t>(base);original_spawn=native_spawn;enabled=false;achievement_bullets_enabled=true;
 auto fire=[&](uintptr_t who=0,uintptr_t what=0){return spawn(who?who:a,0,what?what:gun,0,0,0,0,0,0,0,0);};
 assert(fire()==1&&achievement_bullets.exchange(0)==1);
 for(int type:{0,1,8,9,-1}){put(weapon.data(),0x218,type);fire();assert(!achievement_bullets.exchange(0));}
 for(int type=2;type<=7;++type){put(weapon.data(),0x218,type);result=8;assert(fire()==8&&achievement_bullets.exchange(0)==8);}
 result=1;fire(reinterpret_cast<uintptr_t>(foreign.data()));assert(!achievement_bullets.exchange(0));
 fire(a,gun+4);assert(!achievement_bullets.exchange(0));
 put(world.data(),0x3e0,1);fire();assert(!achievement_bullets.exchange(0));put(world.data(),0x3e0,2);
 put(world.data(),0x6ae8,1.f);fire();assert(!achievement_bullets.exchange(0));put(world.data(),0x6ae8,0.f);
 result=0;assert(fire()==0&&!achievement_bullets.exchange(0));result=-1;assert(fire()==-1&&!achievement_bullets.exchange(0));result=10001;fire();assert(!achievement_bullets.exchange(0));
 result=1;achievement_bullets_enabled=false;fire();assert(!achievement_bullets.exchange(0));assert(native_calls==20);
 ssc_achievements::Engine e;e.capability(ssc_achievements::Kind::bullets,true);e.activity(ssc_achievements::Kind::bullets,99999,1795000000);assert(e.ledger.items[ssc_achievements::index("bullets_100000")].unlocked_at==0);
 e.activity(ssc_achievements::Kind::bullets,1,1795000000);assert(e.ledger.items[ssc_achievements::index("bullets_100000")].unlocked_at);
 auto saved=e.ledger.json();ssc_achievements::Ledger restored;assert(ssc_achievements::Ledger::parse(saved,restored));e.replace(restored);e.activity(ssc_achievements::Kind::bullets,400000,1795000001);
 assert(e.ledger.items[ssc_achievements::index("bullets_500000")].unlocked_at);assert(e.ledger.items[ssc_achievements::index("bullets_1000000")].value==500000);
 original_spawn=nullptr;image_base=0;VirtualFree(base,0,MEM_RELEASE);
 std::cout<<"PASS real spawn-wrapper attribution, native passthrough, six gun categories, explosives/melee exclusions, foreign actors, invalid weapons, warmup/end states, zero/invalid counts, pause and progress across reload\n";
}
