#pragma once
// Test-only simulated game memory and native request callback.
#include "../runtime/freecam.h"
struct FreecamSessionFixture {
 std::vector<unsigned char> image=std::vector<unsigned char>(0xde95d8),world=std::vector<unsigned char>(0x6ba0),actor=std::vector<unsigned char>(0x3000),context=std::vector<unsigned char>(0x2400);
 inline static int requests=0,requested=-1;
 template<class T> static void set(std::vector<unsigned char>& bytes,size_t offset,T value){std::memcpy(bytes.data()+offset,&value,sizeof value);}
 static void request(uintptr_t ctx){
  ++requests;requested=-1;
  for(auto off:{0x2300,0x2360,0x23c0}){int v;std::memcpy(&v,reinterpret_cast<void*>(ctx+off),4);if(v>=0)requested=v;v=-1;std::memcpy(reinterpret_cast<void*>(ctx+off),&v,4);}
 }
 FreecamSessionFixture(bool offline,uint64_t now){
  using namespace ssc_freecam;stop();state={};attached=true;base=reinterpret_cast<uintptr_t>(image.data());observed_context=reinterpret_cast<uintptr_t>(context.data());native_spectator_request=request;requests=0;requested=-1;
  set(image,0xde95d0,reinterpret_cast<uintptr_t>(world.data()));set(context,0,reinterpret_cast<uintptr_t>(actor.data()));
  set(actor,0x13a4,100.f);set(actor,0x78,7);set(actor,0x81,uint8_t(1));set(world,0x6b98,7);
  set(world,0x160,1);set(world,0x2d2,uint8_t(offline));set(world,0x6a38,uint8_t(offline));set(world,0x6a39,uint8_t(!offline));
  for(auto off:{0x2300,0x2360,0x23c0})set(context,off,-1);
  state.observe(reinterpret_cast<uintptr_t>(actor.data()),reinterpret_cast<uintptr_t>(world.data()),100,200,1000,1000,now);
 }
 ~FreecamSessionFixture(){using namespace ssc_freecam;stop();state={};base=observed_context=0;native_spectator_request=nullptr;attached=false;}
};

