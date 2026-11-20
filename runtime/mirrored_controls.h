#pragma once
#include "challenge_modes.h"
#include "movement_scramble.h"
#include "mirrored_control_guards.h"
#include <cstring>
namespace ssc_mirror {
inline uintptr_t image=0;
using Movement=void(*)(uintptr_t,unsigned char);
inline Movement original=nullptr;
inline bool requested(){return ssc_challenge::input_supported&&ssc_challenge::chaos.effects[8]&&ssc_challenge::active();}
// Only the four resolved on-foot direction flags are writable. Key bindings,
// keyboard state, chat text, mouse aim, actor position and vehicle input stay native.
template<class Read,class Write>bool reverse(uintptr_t actor,unsigned char blocked,Read read,Write write){
 if(blocked||!requested()||!actor)return false;
 uintptr_t world=0,first=0,last=0,steam_first=0,car=0;int local=-1,steam_slot=-2,slot=-3,kind=-1,mode=-1;unsigned char active=0;float health=0,blocked_movement=0;
 if(!read(image+0xded5d0,world)||!world||world!=ssc_challenge::session||!read(world+0x6b98,local)||local<0||local>=2048||!read(image+0xe21678,steam_slot)||local!=steam_slot||!read(world+0xa658,first)||!read(world+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048||uintptr_t(local)>=(last-first)/0x34b8||actor!=first+uintptr_t(local)*0x34b8||!read(image+0xe25138,steam_first)||first!=steam_first)return false;
 if(!read(actor+0x78,slot)||slot!=local||!read(actor+0x7dc,kind)||kind!=0||!read(actor+0x81,active)||active!=1||!read(actor+0x8a0,mode)||mode==3||!read(actor+0x2ae0,car)||car||!read(actor+0x13a4,health)||!std::isfinite(health)||health<=0||!read(actor+0x24a8,blocked_movement)||!std::isfinite(blocked_movement)||blocked_movement>0)return false;
 std::array<unsigned char,4> flags{};if(!read(actor+0x336c,flags))return false;for(auto v:flags)if(v>1)return false;
 return write(actor+0x336c,std::array<unsigned char,4>{flags[1],flags[0],flags[3],flags[2]});
}
inline void tick(uint64_t now,bool focused){const auto& c=ssc_challenge::chaos;const bool enabled=c.enabled&&c.effects[8]&&ssc_challenge::input_supported;scramble.tick(now,enabled,focused&&ssc_challenge::active(),false,60000,ssc_challenge::session,ssc_challenge::last_round,ssc_challenge::random_word);}
inline bool write_flags(uintptr_t p,const std::array<unsigned char,4>& flags){
 MEMORY_BASIC_INFORMATION m{};if(!VirtualQuery(reinterpret_cast<const void*>(p),&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&PAGE_GUARD)||!((m.Protect&0xff)==PAGE_READWRITE||(m.Protect&0xff)==PAGE_WRITECOPY))return false;
 auto begin=reinterpret_cast<uintptr_t>(m.BaseAddress);if(p<begin||p-begin>m.RegionSize||flags.size()>m.RegionSize-(p-begin))return false;
 std::memcpy(reinterpret_cast<void*>(p),flags.data(),flags.size());return true;
}
inline void movement(uintptr_t actor,unsigned char blocked){
 original(actor,blocked);
 reverse(actor,blocked,[](uintptr_t p,auto& v){return ssc_names::read(p,v);},write_flags);
}
template<class Read>bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;uint32_t i=0;std::array<unsigned char,64> bytes{};for(;g.size-i>=bytes.size();i+=uint32_t(bytes.size())){if(!read(base+g.rva+i,bytes))return false;for(auto b:bytes)h=(h^b)*1099511628211ULL;}for(;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
inline bool attach(){
 image=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));if(!validate(image,[](uintptr_t p,auto& v){return ssc_names::read(p,v);}))return false;
 auto site=reinterpret_cast<unsigned char*>(image+0x816672);int32_t before=0;std::memcpy(&before,site+1,4);if(site[0]!=0xe8||uintptr_t(site+5+before)!=image+0x816f60)return false;
 unsigned char* bridge=nullptr;for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((image+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(bridge,jump,6);auto target=reinterpret_cast<uintptr_t>(movement);std::memcpy(bridge+6,&target,8);DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,14);
 if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}
 original=reinterpret_cast<Movement>(image+0x816f60);int32_t rel=int32_t(reinterpret_cast<intptr_t>(bridge)-reinterpret_cast<intptr_t>(site+5));std::memcpy(site+1,&rel,4);FlushInstructionCache(GetCurrentProcess(),site,5);DWORD ignored;VirtualProtect(site,5,old,&ignored);return true;
}
}
