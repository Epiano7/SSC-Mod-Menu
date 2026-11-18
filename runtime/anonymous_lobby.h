#pragma once
#include "privacy.h"
namespace ssc_privacy {
// Match the game's case-sensitive Community Cup title check. Read only the
// lobby title, never a player's real name or account ID. Bound malformed data.
template<class Read>Detection community_cup_anonymity(uintptr_t world,Read read){
 uintptr_t length=0,capacity=0,data=world+0x48;
 if(!world||!read(world+0x58,length)||!read(world+0x60,capacity)||length>capacity||capacity<15||length>256)return Detection::unavailable;
 if(length<13)return Detection::off;
 if(capacity>=16&&(!read(world+0x48,data)||!data))return Detection::unavailable;
 constexpr char marker[]="Community Cup";size_t matched=0;
 for(uintptr_t i=0;i<length;++i){unsigned char ch=0;if(!read(data+i,ch)||!ch)return Detection::unavailable;
  if(ch==static_cast<unsigned char>(marker[matched])){if(++matched==13)return Detection::on;}
  else matched=ch=='C'?1:0;
 }
 return Detection::off;
}
// Observe only the local actor's public anonymity bit; never read account IDs.
// The native name formatter and local actor layout must already be validated.
template<class Read>Detection lobby_anonymity(uintptr_t base,bool supported,Read read){
 if(!supported)return Detection::off;
 int phase=0;if(!read(base+0xfb5734,phase))return Detection::unavailable;if(phase!=2)return Detection::off;
 uintptr_t world=0,first=0,last=0,canonical=0;int slot=-1,world_slot=-2,stored=-3,kind=-1;unsigned char anonymous=2,active=0;
 if(!read(base+0xded5d0,world)||!world)return Detection::unavailable;
 if(!tournament_validated.load())return Detection::unavailable;
 auto cup=community_cup_anonymity(world,read);if(cup!=Detection::off)return cup;
 if(!read(base+0xe21678,slot)||slot<0||slot>=2048||!read(world+0x6b98,world_slot)||world_slot!=slot||!read(world+0xa658,first)||!read(world+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048||uintptr_t(slot)>=(last-first)/0x34b8||!read(base+0xe25138,canonical)||canonical!=first)return Detection::unavailable;
 auto actor=first+uintptr_t(slot)*0x34b8;
 if(!read(actor+0x78,stored)||stored!=slot||!read(actor+0x7dc,kind)||kind!=0||!read(actor+0x81,active)||active!=1||!read(actor+0x209,anonymous)||anonymous>1)return Detection::unavailable;
 return anonymous?Detection::on:Detection::off;
}
}
