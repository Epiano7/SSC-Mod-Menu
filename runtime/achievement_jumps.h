#pragma once
#include "combat_stats.h"
#include "planned_native_guards.h"

namespace ssc_achievements::jumps {
// Verified on Steam build 26820987. The queue call runs only after the
// simulation accepts the launch. R15d is the pad index, or -1 for Jump.
inline constexpr ssc_planned_native::Guard guards[]={
 {0x8026cb,0x41f,0x71201550e05d54fbULL},
 {0x802c72,0x62,0x61613bbbe877ea4eULL},
 {0x80322c,0x2b,0xbb8983c1639bed5bULL},
 {0x1b0670,0x98,0xcac8932c3d3dbeb3ULL},
 {0x2eb226,0x23,0x49be129062132d7bULL}
};
inline constexpr uintptr_t completion_site=0x803248,queue_rva=0x1b0670;
using Queue=void(*)(uintptr_t,const int*);
inline Queue original_queue=nullptr;
inline uintptr_t image_base=0;
inline bool attached=false;
inline std::atomic<bool> enabled{false},pending{false};
inline std::mutex mutex;
struct Launches {
 uintptr_t world=0,actor=0;
 int round=0;
 float tick=-1;
 uint64_t count=0;

 void accept(const ssc_combat::Context& context,float clock){
  // The native world clock advances by simulation ticks. A replayed or
  // repeated launch in the same round must not increment the lifetime total.
  if(world!=context.session||actor!=context.actor||round!=context.round){
   world=context.session;actor=context.actor;round=context.round;tick=-1;
  }
  if(clock<=tick)return;
  tick=clock;
  ++count;
 }
};
inline Launches launches;

inline uint64_t take_progress(bool tracking,bool reset=false){
 if(enabled.load(std::memory_order_relaxed)==tracking&&!pending.load(std::memory_order_acquire)&&!reset)return 0;
 std::lock_guard<std::mutex> lock(mutex);
 const auto count=launches.count;
 if(reset)launches={};
 else launches.count=0;
 enabled=tracking;
 pending=false;
 return tracking?count:0;
}

inline void record_launch(uintptr_t actor,int pad){
 if(pad!=-1||!actor||!enabled.load(std::memory_order_relaxed))return;
 using ssc_names::read;
 const auto context=ssc_combat::context(image_base);
 if(!context.valid||!context.active||context.ending||context.actor!=actor||!ssc_combat::multiplayer(context.session))return;
 int slot=-1,world_slot=-2;
 uintptr_t vehicle=0;
 unsigned char request=0,dead=1;
 float upward=0,clock=0;
 if(!read(actor+0x78,slot)||!read(context.session+0x6b98,world_slot)||slot!=world_slot||
    !read(actor+0x3375,request)||request!=1||!read(actor,dead)||dead!=0||
    !read(actor+0x2ae0,vehicle)||vehicle||!read(actor+0x54,upward)||!std::isfinite(upward)||upward<=0||
    !read(context.session+0x434,clock)||!std::isfinite(clock)||clock<0)return;
 std::lock_guard<std::mutex> lock(mutex);
 if(!enabled.load(std::memory_order_relaxed))return;
 launches.accept(context,clock);
 if(launches.count)pending.store(true,std::memory_order_release);
}

extern "C" __attribute__((used,noinline)) inline void ssc_achievement_jump_completed(uintptr_t vector,const int* cue,uintptr_t actor,int pad){
 original_queue(vector,cue);
 try{record_launch(actor,pad);}catch(...){/* Leave the native launch and cue intact. */}
}
extern "C" void ssc_achievement_jump_bridge();
asm(".text\n.globl ssc_achievement_jump_bridge\n.def ssc_achievement_jump_bridge; .scl 2; .type 32; .endef\n"
    "ssc_achievement_jump_bridge:\nmovq %rdi, %r8\nmovl %r15d, %r9d\njmp ssc_achievement_jump_completed\n");

inline bool attach(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(attached)return true;
 if(!ssc_compat::supports(2)||!ssc_compat::supports(4))return false;
 for(const auto& guard:guards)if(!ssc_planned_native::check(base,guard))return false;
 const auto at=base+completion_site;
 auto* call=reinterpret_cast<unsigned char*>(at);
 int32_t relative=0;
 std::memcpy(&relative,call+1,sizeof relative);
 if(call[0]!=0xe8||at+5+relative!=base+queue_rva)return false;
 unsigned char* bridge=nullptr;
 for(uintptr_t distance=0x10000;distance<0x60000000&&!bridge;distance+=0x10000)
  bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+distance)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};
 const auto target=reinterpret_cast<uintptr_t>(ssc_achievement_jump_bridge);
 std::memcpy(bridge,jump,sizeof jump);
 std::memcpy(bridge+sizeof jump,&target,sizeof target);
 DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)||!VirtualProtect(call,5,PAGE_EXECUTE_READWRITE,&old)){
  VirtualFree(bridge,0,MEM_RELEASE);
  return false;
 }
 FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 image_base=base;
 original_queue=reinterpret_cast<Queue>(base+queue_rva);
 relative=int32_t(reinterpret_cast<intptr_t>(bridge)-static_cast<intptr_t>(at+5));
 std::memcpy(call+1,&relative,sizeof relative);
 FlushInstructionCache(GetCurrentProcess(),call,5);
 DWORD ignored=0;
 VirtualProtect(call,5,old,&ignored);
 return attached=true;
}
}
