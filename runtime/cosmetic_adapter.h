#pragma once
#include <atomic>
#include <array>
#include <string>
namespace ssc_names {
using Predicate=unsigned char(__cdecl*)(void*,int,void*);
using Overhead=void(__cdecl*)(uintptr_t,float,unsigned char,uintptr_t,unsigned char,unsigned char);
using Draw=float(__cdecl*)(void*,float,float,void*,float,float,float,float,float,int,float,unsigned char,unsigned char,float);
inline Predicate native_predicate=nullptr;
inline Overhead native_overhead=nullptr;
inline Draw native_draw=nullptr;
inline std::atomic<bool> cosmetics{false};
inline std::atomic<unsigned> cosmetic_draws{0};
inline bool attached=false;
inline uintptr_t image_base=0;
inline thread_local uintptr_t drawing_actor=0;
inline thread_local bool drawing_local_widget=false;
using Widget=void(__cdecl*)(uintptr_t,float,void*,void*,char,float,float,float,float,void*,float,void*,void*);
inline Widget native_widget=nullptr;
extern "C" void ssc_score_bridge();
inline bool read_bytes(uintptr_t address,void* result,size_t size){
    if(!address||!size||size>UINTPTR_MAX-address)return false;
    // Validate each memory region once, not each character. Do not cache
    // permissions or identity across draws: actors and strings can change.
    uintptr_t cursor=address;size_t remaining=size;
    while(remaining){
        MEMORY_BASIC_INFORMATION info{};
        if(!VirtualQuery(reinterpret_cast<void*>(cursor),&info,sizeof(info))||info.State!=MEM_COMMIT||(info.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return false;
        auto begin=reinterpret_cast<uintptr_t>(info.BaseAddress);
        if(cursor<begin||cursor-begin>=info.RegionSize)return false;
        auto chunk=std::min(remaining,info.RegionSize-(cursor-begin));
        cursor+=chunk;remaining-=chunk;
    }
    std::memcpy(result,reinterpret_cast<void*>(address),size);return true;
}
template<class T> bool read(uintptr_t address,T& result){return read_bytes(address,&result,sizeof(T));}
struct Identity {bool local=false;};
// Native MSVC account string, not actor+0x780 (the editable display name).
inline bool account_string(uintptr_t object,std::string& value){
    std::array<unsigned char,32> header{};size_t length=0,capacity=0;uintptr_t data=object;
    if(!read_bytes(object,header.data(),header.size()))return false;
    std::memcpy(&length,header.data()+16,8);std::memcpy(&capacity,header.data()+24,8);
    if(!length||length>256||capacity<length)return false;
    std::array<unsigned char,256> characters{};
    if(capacity>15){std::memcpy(&data,header.data(),8);if(!read_bytes(data,characters.data(),length))return false;}
    else {if(length>15)return false;std::memcpy(characters.data(),header.data(),length);}
    value.clear();value.reserve(length);
    for(size_t i=0;i<length;++i){auto c=characters[i];if(c<32)return false;value.push_back(c>='A'&&c<='Z'?char(c+32):char(c));}
    return true;
}
inline bool local_account(uintptr_t account){
    std::string own,candidate;
    return account_string(image_base+0xde8030,own)&&account_string(account,candidate)&&own==candidate;
}
inline bool native_phase(float& phase){
    float speed=0,time=0;
    if(!read(image_base+0xf94db4,speed)||!read(image_base+0xf95810,time)||!std::isfinite(time)||!std::isfinite(speed))return false;
    float candidate=std::fmod(time*speed,1.f);if(!std::isfinite(candidate))return false;if(candidate<0)candidate+=1.f;phase=candidate;return true;
}
inline Identity identify(uintptr_t actor){
    Identity answer;uintptr_t world=0,first=0,last=0,registry=0;int slot=-1,kind=-1,local_slot=-1;unsigned char active=0;
    if(!read(image_base+0xdce5d0,world)||!world||!read(world+0xa5a8,first)||!read(world+0xa5b0,last)||!first||last<first||(last-first)%0x3450||(last-first)/0x3450>2048)return answer;
    if(actor<first||actor>=last||(actor-first)%0x3450||!read(actor+0x78,slot)||slot<0||uintptr_t(slot)!=(actor-first)/0x3450||!read(actor+0x7c4,kind)||!read(actor+0x81,active)||!active)return answer;
    // e03370 is used by the native own-player formatter; bf48 may be spectated.
    answer.local=kind==0&&read(image_base+0xe02370,local_slot)&&slot==local_slot&&
        read(image_base+0xe05dc8,registry)&&registry==first&&local_account(actor+0x6d8);
    return answer;
}
inline unsigned char __cdecl predicate(void* context,int id,void* owned_string){
    bool apply=cosmetics.load()&&local_account(reinterpret_cast<uintptr_t>(owned_string));
    unsigned char result=native_predicate(context,id,owned_string); // original consumes this copy
    if(apply)++cosmetic_draws;
    return apply?1:result;
}
// The native compact event feed stores formatted names, not actor pointers.
// Only use its own account-key lookup when the current session proves the key
// belongs to the local actor and no other actor is displaying that same key.
inline bool unambiguous_event_account(uintptr_t text){
    std::string own_key,candidate;
    if(!account_string(image_base+0xde8030,own_key)||!account_string(text,candidate)||own_key!=candidate)return false;
    uintptr_t world=0,first=0,last=0;int slot=-1;
    if(!read(image_base+0xdce5d0,world)||!read(world+0xa5a8,first)||!read(world+0xa5b0,last)||
       !first||last<first||(last-first)%0x3450||(last-first)/0x3450>2048||
       !read(image_base+0xe02370,slot)||slot<0||size_t(slot)>=(last-first)/0x3450)return false;
    const auto own=first+size_t(slot)*0x3450;
    if(!identify(own).local)return false;
    for(auto p=first;p<last;p+=0x3450){unsigned char active=0;
        if(!read(p+0x81,active))return false;
        if(p!=own&&active&&((account_string(p+0x780,candidate)&&candidate==own_key)||(account_string(p+0x6d8,candidate)&&candidate==own_key)))return false;
    }
    return true;
}
inline unsigned char __cdecl event_predicate(void* context,int id,void* owned_string){
    const bool apply=cosmetics.load()&&unambiguous_event_account(reinterpret_cast<uintptr_t>(owned_string));
    const auto result=native_predicate(context,id,owned_string);
    if(apply)++cosmetic_draws;
    return apply?1:result;
}
inline void __cdecl local_widget(uintptr_t widget,float p2,void* p3,void* p4,char p5,float p6,float p7,float p8,float p9,void* p10,float p11,void* p12,void* p13){
    struct Scope{bool old=drawing_local_widget;Scope(){drawing_local_widget=true;}~Scope(){drawing_local_widget=old;}} scope;
    native_widget(widget,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13);
}
inline float __cdecl widget_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
    if(drawing_local_widget&&cosmetics.load()&&local_account(reinterpret_cast<uintptr_t>(name))&&native_phase(phase))++cosmetic_draws;
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline void __cdecl overhead(uintptr_t actor,float alpha,unsigned char p3,uintptr_t p4,unsigned char p5,unsigned char p6){
    struct Scope{uintptr_t previous;Scope(uintptr_t p):previous(drawing_actor){drawing_actor=p;}~Scope(){drawing_actor=previous;}} scope(actor);
    native_overhead(actor,alpha,p3,p4,p5,p6);
}
inline float __cdecl draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
    if(!cosmetics.load())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    auto identity=identify(drawing_actor);
    if(cosmetics.load()&&identity.local)++cosmetic_draws;
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
extern "C" float __cdecl ssc_score_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t row){
    if(!cosmetics.load())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    int slot=-1;uintptr_t world=0,first=0,last=0;
    Identity identity;
    if(read(row+0x2c,slot)&&slot>=0&&read(image_base+0xdce5d0,world)&&read(world+0xa5a8,first)&&read(world+0xa5b0,last)&&last>=first&&size_t(slot)<(last-first)/0x3450)identity=identify(first+size_t(slot)*0x3450);
    if(cosmetics.load()&&identity.local&&native_phase(phase))++cosmetic_draws;
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline bool attach(){
    image_base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    native_predicate=reinterpret_cast<Predicate>(image_base+0x3a4db0);native_overhead=reinterpret_cast<Overhead>(image_base+0x83aa30);native_draw=reinterpret_cast<Draw>(image_base+0x4799d0);
    native_widget=reinterpret_cast<Widget>(image_base+0x98cf90);
    struct Site{uint32_t call,target;uintptr_t hook;};
    // Whitelist presentation calls only. Do not hook the predicate globally:
    // 5fb3b0/5fd316/6834a3/6b6db7 also service pass-related UI/cache logic.
    const Site sites[]={
        {0x3381aa,0x83aa30,reinterpret_cast<uintptr_t>(overhead)},
        {0x83aed2,0x3a4db0,reinterpret_cast<uintptr_t>(predicate)},
        {0x4139ef,0x3a4db0,reinterpret_cast<uintptr_t>(predicate)},
        {0x433968,0x3a4db0,reinterpret_cast<uintptr_t>(predicate)},
        {0x434d95,0x3a4db0,reinterpret_cast<uintptr_t>(predicate)},
        {0x616d1c,0x3a4db0,reinterpret_cast<uintptr_t>(predicate)},
        {0x63053e,0x3a4db0,reinterpret_cast<uintptr_t>(predicate)},
        {0x69d759,0x3a4db0,reinterpret_cast<uintptr_t>(predicate)},
        {0x9d4551,0x3a4db0,reinterpret_cast<uintptr_t>(predicate)},
        {0x99fba9,0x3a4db0,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x99fd53,0x3a4db0,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x5fd651,0x98cf90,reinterpret_cast<uintptr_t>(local_widget)},
        {0x98f425,0x4799d0,reinterpret_cast<uintptr_t>(widget_draw)},
        {0x83b787,0x4799d0,reinterpret_cast<uintptr_t>(draw)},
        {0x83b813,0x4799d0,reinterpret_cast<uintptr_t>(draw)},
        {0x43044d,0x4799d0,reinterpret_cast<uintptr_t>(ssc_score_bridge)},
        {0x4304d2,0x4799d0,reinterpret_cast<uintptr_t>(ssc_score_bridge)}};
    for(auto site:sites){auto p=reinterpret_cast<unsigned char*>(image_base+site.call);int32_t relative;std::memcpy(&relative,p+1,4);if(p[0]!=0xe8||image_base+site.call+5+relative!=image_base+site.target)return false;}
    unsigned char* bridge=nullptr;
    for(uintptr_t distance=0x10000;distance<0x60000000&&!bridge;distance+=0x10000){uintptr_t address=(image_base+distance)&~uintptr_t(0xffff);bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>(address),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));}
    if(!bridge)return false;
    for(size_t i=0;i<std::size(sites);++i){const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(bridge+i*16,jump,6);std::memcpy(bridge+i*16+6,&sites[i].hook,8);}
    DWORD old;if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,4096);
    std::array<DWORD,std::size(sites)> protections{};size_t prepared=0;
    for(auto site:sites){if(!VirtualProtect(reinterpret_cast<void*>(image_base+site.call),5,PAGE_EXECUTE_READWRITE,&protections[prepared])){for(size_t i=prepared;i-->0;){DWORD unused;VirtualProtect(reinterpret_cast<void*>(image_base+sites[i].call),5,protections[i],&unused);}VirtualFree(bridge,0,MEM_RELEASE);return false;}++prepared;}
    for(size_t i=0;i<std::size(sites);++i){auto p=reinterpret_cast<unsigned char*>(image_base+sites[i].call);int32_t relative=static_cast<int32_t>(reinterpret_cast<uintptr_t>(bridge+i*16)-(image_base+sites[i].call+5));std::memcpy(p+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),p,5);}
    // Sites share pages: write all first, then unwind protections in reverse order.
    for(size_t i=std::size(sites);i-->0;){DWORD unused;VirtualProtect(reinterpret_cast<void*>(image_base+sites[i].call),5,protections[i],&unused);}
    return true;
}
}
