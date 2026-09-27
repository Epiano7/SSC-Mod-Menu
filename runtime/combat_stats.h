#pragma once
#include "presence_source.h"
#include <mutex>
namespace ssc_combat {
struct Context {bool valid=false,active=false,ending=false,preparing=false;uintptr_t session=0,actor=0;int round=0;};
struct Totals {uint64_t shots=0,projectiles=0,hits=0,player_hits=0,npc_hits=0;double impact=0;};
struct Counter {
 uintptr_t session=0;int round=0;bool prepared=false,running=false,complete=false,finished=false;Totals totals;
 void reset(){*this=Counter{};}
 void observe(const Context& c){
  if(!c.valid){reset();return;}
  if(session!=c.session||round!=c.round){bool carry=session==c.session&&c.round==round+1&&prepared;reset();session=c.session;round=c.round;prepared=carry;}
  if(c.ending){if(running)finished=true;running=false;prepared=true;return;}
  if(!c.active){prepared=c.preparing;return;}
  if(!running&&prepared&&!finished){running=complete=true;prepared=false;totals={};}
 }
 bool accepts(const Context& c){observe(c);return running&&complete&&c.active&&!c.ending;}
 void fired(const Context& c,int count){if(!accepts(c)||count<=0||count>10000)return;if(totals.projectiles>100000000-uint64_t(count)){complete=false;return;}++totals.shots;totals.projectiles+=unsigned(count);}
 void hit(const Context& c,int kind,float damage){if(!accepts(c)||kind<0||kind>3||!std::isfinite(damage)||damage<=0||damage>1000000)return;if(totals.hits>=100000000||totals.impact+damage>1e12){complete=false;return;}++totals.hits;if(kind<2)++totals.player_hits;else ++totals.npc_hits;totals.impact+=damage;}
};
inline Counter counter;inline std::mutex mutex;inline std::atomic<bool> enabled{false};inline bool attached=false;inline uintptr_t image_base=0;
inline Context context(uintptr_t base){
 using ssc_names::read;Context c;int phase=0,mode=-1,raw=0,last=0,local=-1,slot=-1,kind=-1;double joining=0;float countdown=0,ending=0;unsigned char survival=0,active=0;uintptr_t first=0,end=0,registry=0;
 if(!read(base+ssc_compat::resolve(0xfab604),phase)||phase!=2||!read(base+ssc_compat::resolve(0xfab618),joining)||!std::isfinite(joining)||joining>0)return c;
 if(!read(base+ssc_compat::resolve(0xde35d0),c.session)||!c.session||!read(c.session+0x128,mode)||mode!=6||!read(c.session+0x485,survival)||survival||!read(c.session+0x3e0,raw)||raw<1||raw>101||!read(c.session+0x46c,last)||last<1||last>100||raw-1>last||!read(c.session+0x3c0,countdown)||!std::isfinite(countdown)||!read(c.session+0x6ae8,ending)||!std::isfinite(ending))return {};
 if(!read(c.session+0xa618,first)||!read(c.session+0xa620,end)||!first||end<first||(end-first)%0x3478||(end-first)/0x3478>2048||!read(base+ssc_compat::resolve(0xe1b088),registry)||registry!=first||!read(base+ssc_compat::resolve(0xe17608),local)||local<0||uintptr_t(local)>=(end-first)/0x3478)return {};
 c.actor=first+uintptr_t(local)*0x3478;if(!read(c.actor+0x78,slot)||slot!=local||!read(c.actor+0x7dc,kind)||kind!=0||!read(c.actor+0x81,active)||!active)return {};
 c.valid=true;c.round=raw-1;c.active=raw>=2&&countdown<=0&&ending<=0;c.ending=raw>=2&&(countdown>0||ending>0);c.preparing=countdown>0&&ending<=0;return c;
}
inline bool owns_weapon(uintptr_t actor,uintptr_t weapon){uintptr_t begin=0,end=0;return ssc_names::read(actor+0x12a0,begin)&&ssc_names::read(actor+0x12a8,end)&&begin&&end>=begin&&(end-begin)%0x748==0&&(end-begin)/0x748<=16&&weapon>=begin&&weapon<end&&(weapon-begin)%0x748==0;}
using Spawn=int(*)(uintptr_t,unsigned char,uintptr_t,unsigned char,uintptr_t,uintptr_t,float,float,unsigned char,unsigned char,unsigned char);
using Append=uintptr_t(*)(uintptr_t,const float*);
inline Spawn original_spawn=nullptr;inline Append original_append=nullptr;
inline int spawn(uintptr_t actor,unsigned char p2,uintptr_t weapon,unsigned char p4,uintptr_t p5,uintptr_t p6,float p7,float p8,unsigned char p9,unsigned char p10,unsigned char p11){
 const auto result=original_spawn(actor,p2,weapon,p4,p5,p6,p7,p8,p9,p10,p11);
 if(enabled.load(std::memory_order_relaxed)&&result>0){try{auto c=context(image_base);if(c.valid&&actor==c.actor&&owns_weapon(actor,weapon)){std::lock_guard<std::mutex> lock(mutex);counter.fired(c,result);}}catch(...){enabled=false;}}
 return result;
}
extern "C" __attribute__((used,noinline)) inline uintptr_t ssc_combat_hit(uintptr_t vector,const float* values,uintptr_t shooter,uintptr_t victim,uintptr_t projectile){
 const auto result=original_append(vector,values);
 if(enabled.load(std::memory_order_relaxed)){try{auto c=context(image_base);if(c.valid&&shooter==c.actor&&victim!=shooter&&projectile){int kind=-1;float damage=0;if(ssc_names::read(victim+0x7dc,kind)&&ssc_names::read(reinterpret_cast<uintptr_t>(values)+4,damage)){std::lock_guard<std::mutex> lock(mutex);counter.hit(c,kind,damage);}}}catch(...){enabled=false;}}
 return result;
}
extern "C" void ssc_combat_hit_bridge();
asm(".text\n.globl ssc_combat_hit_bridge\n.def ssc_combat_hit_bridge; .scl 2; .type 32; .endef\n.seh_proc ssc_combat_hit_bridge\nssc_combat_hit_bridge:\nsubq $0x38, %rsp\n.seh_stackalloc 0x38\n.seh_endprologue\nmovq %rdi, 0x20(%rsp)\nmovq %rsi, %r8\nmovq %rbx, %r9\ncall ssc_combat_hit\naddq $0x38, %rsp\nret\n.seh_endproc\n");
inline bool attach(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(attached)return true;
 if(!ssc_compat::supports(64))return false;
 struct Hook{uint32_t call,target,reserved;};Hook hooks[]={{0x2f4dad,0x7eb720,0},{0x7e3fde,0x1ae330,0}};
 for(auto& h:hooks){h.call=ssc_compat::resolve(h.call);h.target=ssc_compat::resolve(h.target);auto p=reinterpret_cast<const unsigned char*>(base+h.call);int32_t rel;std::memcpy(&rel,p+1,4);if(p[0]!=0xe8||base+h.call+5+rel!=base+h.target)return false;}
 unsigned char* bridge=nullptr;for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;
 uintptr_t entries[]={reinterpret_cast<uintptr_t>(spawn),reinterpret_cast<uintptr_t>(ssc_combat_hit_bridge)};for(int i=0;i<2;++i){const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(bridge+i*32,jump,6);std::memcpy(bridge+i*32+6,&entries[i],8);}DWORD old;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 DWORD protections[2]{};for(int i=0;i<2;++i)if(!VirtualProtect(reinterpret_cast<void*>(base+hooks[i].call),5,PAGE_EXECUTE_READWRITE,&protections[i])){for(int j=i-1;j>=0;--j){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+hooks[j].call),5,protections[j],&ignored);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 image_base=base;original_spawn=reinterpret_cast<Spawn>(base+ssc_compat::resolve(0x7eb720));original_append=reinterpret_cast<Append>(base+ssc_compat::resolve(0x1ae330));
 for(int i=0;i<2;++i){int32_t rel=int32_t(reinterpret_cast<uintptr_t>(bridge+i*32)-(base+hooks[i].call+5));std::memcpy(reinterpret_cast<void*>(base+hooks[i].call+1),&rel,4);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(base+hooks[i].call),5);}
 for(int i=1;i>=0;--i){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+hooks[i].call),5,protections[i],&ignored);}attached=true;return true;
}
}
