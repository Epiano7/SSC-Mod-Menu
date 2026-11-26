#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <array>
#include <fstream>
#include <iterator>
#include <cassert>
#include <iostream>
namespace ssc_names {inline size_t reads=0;template<class T>bool read(uintptr_t p,T& out){++reads;MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;auto a=reinterpret_cast<uintptr_t>(m.BaseAddress);if(p<a||p-a>m.RegionSize||sizeof(T)>m.RegionSize-(p-a))return false;std::memcpy(&out,reinterpret_cast<void*>(p),sizeof out);return true;}template<class T>void put(uintptr_t p,T value){std::memcpy(reinterpret_cast<void*>(p),&value,sizeof value);}}
namespace ssc_auto {struct Observation{bool valid=true,active=true,ending=false;uintptr_t actor=0,session=0;int round=1,last_round=3;bool preparing=false;};inline Observation tracking_observation;inline uint64_t tracking_at=0;}
#include "geri_mode.h"
#include "challenge_native.h"
namespace fixture {
using namespace ssc_challenge;
using ssc_names::put;
inline std::vector<unsigned char> image(0xfc0000), actor_bytes(0x34b8), world_bytes(0xb000);
inline std::array<uintptr_t,1> context{};
inline std::array<int,54> profile_mutations{};
inline std::array<int,9> draft_offers{};
inline uintptr_t actor=0,profile=0,world=0;
inline int rank=2,rarity=1,bonus=0,registered=0,submissions=0,prepares=0;
inline bool buy_accepted=true,blocked=false;
inline auto mutations_begin(){return reinterpret_cast<uintptr_t>(profile_mutations.data());}
inline auto offers_begin(){return reinterpret_cast<uintptr_t>(draft_offers.data());}

void populate_profile(){
 put<uintptr_t>(profile+0x320,mutations_begin());
 put<uintptr_t>(profile+0x328,mutations_begin());
 put<uintptr_t>(profile+0x330,mutations_begin()+sizeof(profile_mutations));
 put<uintptr_t>(profile+0x4b8,offers_begin());
 put<uintptr_t>(profile+0x4c0,offers_begin()+sizeof(draft_offers));
 put<uintptr_t>(profile+0x4c8,offers_begin()+sizeof(draft_offers));
 draft_offers={7,1,0, -1,-1,0, 3,2,0};
 put<int>(profile+0x2b0,2);
 put<int>(profile+0x11a0,0);
 int available=-1;assert(ssc_names::read(actor+0x8d0,available));
 put<int>(profile+0x830,available);
}
void setup(){
 game=reinterpret_cast<uintptr_t>(image.data());profile=game+0xe09e98;
 actor=reinterpret_cast<uintptr_t>(actor_bytes.data());world=reinterpret_cast<uintptr_t>(world_bytes.data());
 context[0]=actor;panel_context=reinterpret_cast<uintptr_t>(context.data());
 ssc_auto::tracking_observation={true,true,false,actor,world,1,3};
 random={true,true,false};choices_supported=true;picker.reset();commit={};prepared={};reroller.reset();
 rank=2;rarity=1;bonus=registered=submissions=prepares=0;blocked=false;buy_accepted=true;
 put<unsigned char>(game+0xfb9a20,1);put<int>(game+0xfb9a24,2);put<int>(game+0xfba230,4);
 put<int>(game+0xe23678,0);put<uintptr_t>(world+0xa658,actor);put<uintptr_t>(world+0xa660,actor+0x34b8);
 put<int>(actor+0x350,2);put<int>(actor+0x1240,0);put<int>(actor+0x8d0,2);
 put<int>(actor+0x1300,-1);put<int>(actor+0x337c,-1);
 put<int>(actor+0x7dc,0);put<unsigned char>(actor+0x84,0);put<float>(actor+0x88,-1.f);
 populate_profile();
 native_prepare=+[](uintptr_t w,uintptr_t p,int slot){assert(w==world&&p==profile&&slot==0);++prepares;populate_profile();};
 native_submit=+[](uintptr_t c){assert(c==panel_context);int pending=-1,live=-1,staged=-1;ssc_names::read(actor+0x337c,pending);ssc_names::read(actor+0x8d0,live);ssc_names::read(profile+0x830,staged);assert(pending==7&&live==2&&staged==1);++submissions;};
 native_mutation_cost=+[](uintptr_t a,int id){assert(a==actor&&id==7);return 1;};
 native_mutation_blocked=+[](){return blocked;};
 native_is_mutated=+[](uintptr_t p,int id){assert(p==profile&&id==7);uintptr_t end=0;ssc_names::read(p+0x328,end);return end>mutations_begin();};
 native_stim_rank=+[](uintptr_t p,int id){assert(p==profile&&id==7);return rank;};
 native_stim_rarity=+[](uintptr_t a,int c,int id){assert(a==actor+0xa0&&c==2&&id==7);return rarity;};
 native_add_mutation=+[](uintptr_t p,const int* id){assert(p==profile+0x320&&*id==7);profile_mutations[0]=*id;put<uintptr_t>(profile+0x328,mutations_begin()+4);};
 native_buy=+[](uintptr_t p,int id,uintptr_t third,int amount,unsigned char a,unsigned char b,unsigned char c,int levels,unsigned char d){
  assert(p==profile&&id==7&&!third&&!amount&&a==1&&b==1&&!c&&!d&&levels>0);
  assert(native_is_mutated(p,id)&&draft_offers[1]==1+levels);
  if(!buy_accepted)return false;
  bonus=levels;return true;
 };
 native_record_stim_bonus=+[](uintptr_t p,int id,int levels){assert(p==profile&&id==7&&levels==bonus);registered=levels;};
}
Offer offer(){
 Offer o;o.actor=actor;o.world=world;o.round=1;o.class_id=2;o.syringes=2;
 o.owned_known=true;o.acquired[7]=rank;o.allocated[7]=rank;o.present[7]=true;o.stims={7};
 return o;
}
void select(const Offer& o,uint64_t now){
 picker.tick(o,now,[](size_t count,size_t& choice){assert(count==1);choice=0;return true;},
   [&](bool skill,uintptr_t choice){assert(!skill&&choice==7);return queue_stim(o,int(choice),now);});
}
int queued(){int p=-2;assert(ssc_names::read(actor+0x337c,p));return p;}
}
int main(){
 using namespace ssc_challenge;
 using namespace fixture;
 assert(stim_bonus_levels(2,1,4,2)==2&&stim_bonus_levels(5,1,4,2)==1&&stim_bonus_levels(6,1,4,2)==0);
 assert(stim_bonus_levels(2,4,4,2)==-1);
 setup();
 auto o=offer();select(o,100);
 assert(queued()==7&&bonus==2&&registered==2&&submissions==1&&picker.pending);
 o.syringes=1;
 assert(!picker.settled(o,200));
 o.mutated[7]=true;
 assert(!picker.settled(o,700));
 o.acquired[7]+=1;
 assert(!picker.settled(o,16000));
 o.acquired[7]+=1;
 assert(picker.settled(o,16100)&&submissions==1);
 setup();rank=5;select(offer(),100);
 assert(queued()==7&&bonus==1&&registered==1&&submissions==1);
 setup();rank=6;select(offer(),100);
 assert(queued()==7&&bonus==0&&registered==0&&submissions==1);
 setup();auto stale=offer();put<int>(actor+0x8d0,1);select(stale,100);
 assert(queued()==-1&&submissions==0&&!picker.pending);
 setup();blocked=true;select(offer(),100);
 assert(queued()==-1&&submissions==0&&!picker.pending);
 setup();buy_accepted=false;select(offer(),100);
 assert(queued()==-1&&submissions==0&&prepares==1&&draft_offers[1]==1&&!native_is_mutated(profile,7));
 int restored=-1;assert(ssc_names::read(profile+0x830,restored)&&restored==2);
 setup();put<unsigned char>(game+0xfb9a20,0);select(offer(),100);
 assert(queued()==7&&bonus==0&&registered==0&&submissions==0);
 std::cout<<"PASS native syringe bonus, capped levels, split confirmations, no duplicate spends, profile recovery, and legacy queued mutation\n";
}
