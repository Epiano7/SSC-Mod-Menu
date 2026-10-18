#pragma once
#include <windows.h>
#include <atomic>
#include <cstring>
#include "vehicle_controls.h"
namespace ssc_vehicle {
inline std::atomic<bool> enabled{false},blocked{true};
inline bool attached=false;
inline uintptr_t image=0,last_world=0;
inline Mapper mapper;
using Control=void(*)(uintptr_t);
inline Control original=nullptr;
template<class T> bool read(uintptr_t p,T& value){
 MEMORY_BASIC_INFORMATION m{};
 if(!p||!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_NOACCESS|PAGE_GUARD))||p<reinterpret_cast<uintptr_t>(m.BaseAddress)||sizeof(T)>m.RegionSize-(p-reinterpret_cast<uintptr_t>(m.BaseAddress)))return false;
 std::memcpy(&value,reinterpret_cast<void*>(p),sizeof value);return true;
}
inline bool context(uintptr_t car,uintptr_t& actor,float& heading){
 uintptr_t world=0,first=0,end=0,linked=0,seats=0,seats_end=0,driver=0;
 unsigned char host=0,transport=1,dedicated=1,solo=0,survival=0,active=0;
 int humans=0,mode=-1,slot=-1,kind=-1,actor_mode=-1;float yaw=0,health=0;
 if(!read(image+0xde85d0,world)||!world)return false;
 if(world!=last_world){mapper.reset();last_world=world;}
 // Initial adapter is deliberately local-authority solo only: the multiplayer
 // command serializer must be reviewed before this can affect online controls.
 if(!read(world+0x6a38,host)||!read(world+0x6a39,transport)||!read(world+0x120,dedicated)||!read(world+0x2d2,solo)||!read(world+0x160,humans)||!read(world+0x485,survival)||!read(world+0x128,mode)||host!=1||dedicated!=0||humans!=1||!(transport==0||solo==1||(mode==6&&survival==1)))return false;
 if(!read(world+0xa628,first)||!read(world+0xa630,end)||!read(world+0x6b98,slot)||!first||end<first||slot<0||slot>=128||uintptr_t(slot+1)>(end-first)/0x3488)return false;
 actor=first+uintptr_t(slot)*0x3488;
 if(!read(actor+0x2ad8,linked)||linked!=car||!read(actor+0x7dc,kind)||kind!=0||!read(actor+0x81,active)||active!=1||!read(actor+0x8a0,actor_mode)||actor_mode==3||!read(actor+0x13a4,health)||!std::isfinite(health)||health<=0)return false;
 // Native 9dd400 consumes seat zero's controls, not every occupant's.
 if(!read(car+0x368,seats)||!read(car+0x370,seats_end)||!seats||seats_end<seats||seats_end-seats<0x68||seats_end-seats>16*0x68||!read(seats+0x60,driver)||driver!=actor)return false;
 if(!read(car+0x90,heading)||!std::isfinite(heading)||!read(image+0xfafc40,yaw)||!std::isfinite(yaw)||std::abs(std::remainder(yaw,360.f))>.01f)return false;
 // 76db20 rotates world velocity into car-local space; 9d4f70 tests
 // its X component for forward/reverse. Car forward is local +X.
 // 76d8f0 maps it to (cos(angle), sin(angle)); screen-up is -Y.
 heading=std::remainder(90.f+std::remainder(heading,360.f),360.f);return true;
}
inline void control(uintptr_t car){
 uintptr_t actor=0;float heading=0;unsigned char flags[4]{};
 if(enabled.load()&&!blocked.load()&&context(car,actor,heading)&&read(car+0x14f,flags)&&flags[0]<=1&&flags[1]<=1&&flags[2]<=1&&flags[3]<=1){
  auto out=mapper.buttons({flags[0]!=0,flags[1]!=0,flags[2]!=0,flags[3]!=0},heading,actor,car);
  unsigned char mapped[]={static_cast<unsigned char>(out.forward),static_cast<unsigned char>(out.reverse),static_cast<unsigned char>(out.left),static_cast<unsigned char>(out.right)};
  std::memcpy(reinterpret_cast<void*>(car+0x14f),mapped,sizeof mapped);
 }else if(!enabled.load()||blocked.load()||mapper.vehicle==car)mapper.reset();
 // Original routine still owns gears, braking, wheel steering and traction.
 if(original)original(car);
}
inline uint64_t hash(const unsigned char* p,size_t n){uint64_t h=14695981039346656037ULL;for(size_t i=0;i<n;++i)h=(h^p[i])*1099511628211ULL;return h;}
inline bool matches(const unsigned char* p,size_t n){
 if(n<0xfafc44||hash(p+0x9df170,0x230)!=0x331fc76d40db16c8ULL||hash(p+0x9d6cc0,0x980)!=0x85004542e1434359ULL)return false;
 int32_t rel=0;std::memcpy(&rel,p+0x9df391,4);return p[0x9df390]==0xe8&&int64_t(0x9df395)+rel==0x9d6cc0;
}
inline bool attach(){
 auto p=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));
 auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(p);if(!p||dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(p+dos->e_lfanew);if(nt->Signature!=IMAGE_NT_SIGNATURE||!matches(p,nt->OptionalHeader.SizeOfImage))return false;
 auto origin=reinterpret_cast<uintptr_t>(p);unsigned char* bridge=nullptr;
 for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((origin+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};auto target=reinterpret_cast<uintptr_t>(control);
 std::memcpy(bridge,jump,6);std::memcpy(bridge+6,&target,8);DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}
 FlushInstructionCache(GetCurrentProcess(),bridge,14);auto site=p+0x9df390;
 if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}
 image=origin;original=reinterpret_cast<Control>(p+0x9d6cc0);
 int32_t relative=int32_t(reinterpret_cast<uintptr_t>(bridge)-reinterpret_cast<uintptr_t>(site+5));std::memcpy(site+1,&relative,4);
 DWORD ignored=0;VirtualProtect(site,5,old,&ignored);FlushInstructionCache(GetCurrentProcess(),site,5);return true;
}
}
