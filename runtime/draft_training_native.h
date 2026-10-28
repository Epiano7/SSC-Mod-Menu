#pragma once
#include "draft_advisor.h"
namespace ssc_training {
struct NativeDraft {int counter=0,player_class=0;std::vector<int> offers,previous;bool operator==(const NativeDraft& b)const{return counter==b.counter&&player_class==b.player_class&&offers==b.offers&&previous==b.previous;}};
// Build 26271617: reviewed current/previous vectors at +4b8/+4d0;
// +5ec remains the generation counter, NOT remaining rerolls.
// This guard permits observations only; it does not certify reroll predictions.
inline bool native_supported(){static uintptr_t checked=0;static bool valid=false;auto base=ssc_advisor::game;if(!base)return false;if(checked!=base){checked=base;valid=ssc_creator::hash(reinterpret_cast<unsigned char*>(base+0x922ab0),0x5b16)==0xb9d1992739edcc57ULL;}return valid;}
template<class Reader>bool read_native_draft(uintptr_t context,NativeDraft& out,Reader read){
 NativeDraft value;if(!read(context+0x2b0,value.player_class)||value.player_class<1||value.player_class>8||!read(context+0x5ec,value.counter)||value.counter<0||value.counter>1024)return false;
 for(int which=0;which<2;++which){auto address=context+(which?0x4d0:0x4b8);uintptr_t first=0,end=0,cap=0;
  if(!read(address,first)||!read(address+8,end)||!read(address+16,cap)||end<first||cap<end||(end-first)%12||(end-first)/12>3||(!first&&end))return false;
  auto& ids=which?value.previous:value.offers;for(auto p=first;p<end;p+=12){int id=-1;if(!read(p,id)||id<-1||id>=54)return false;ids.push_back(id);}
  uintptr_t verify_first=0,verify_end=0;if(!read(address,verify_first)||!read(address+8,verify_end)||first!=verify_first||end!=verify_end)return false;
 }out=std::move(value);return true;
}
inline NativeDraft last_native;inline bool have_native=false;
inline void capture_native(const ssc_advisor::Build& build,uint64_t now){
 if(!round.active||!native_supported())return;
 NativeDraft a,b;auto read=[](uintptr_t p,auto& v){return ssc_advisor::read(p,v);};auto context=ssc_advisor::game+0xe04e98;
 if(!read_native_draft(context,a,read)||!read_native_draft(context,b,read)||!(a==b)||a.player_class!=build.player_class+1)return;
 if(have_native&&a==last_native)return;
 last_native=a;have_native=true;
 round.append({{"type",4},{"offers",a.offers},{"previous",a.previous},{"reroll_counter",a.counter}},now);
}
}
