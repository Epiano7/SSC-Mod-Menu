#pragma once
#include "compatibility.h"
#include "private_auth.h"
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
inline bool rainbow=true;inline unsigned solid_rgb=0x55ccff;
inline void solid(float& r,float& g,float& b,float& phase){if(!rainbow){r=float((solid_rgb>>16)&255)/255;g=float((solid_rgb>>8)&255)/255;b=float(solid_rgb&255)/255;phase=-1;}}
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
inline unsigned bot_rgb=0xffbc41,bot_alpha=255;
inline bool parse_bot_color(const std::wstring& text,unsigned& rgb,unsigned& alpha){
 const size_t start=!text.empty()&&text[0]==L'#'?1:0,n=text.size()-start;
 if(n!=6&&n!=8)return false;
 uint32_t value=0;
 for(size_t i=start;i<text.size();++i){auto c=text[i];unsigned d;
  if(c>=L'0'&&c<=L'9')d=c-L'0';else if(c>=L'a'&&c<=L'f')d=c-L'a'+10;else if(c>=L'A'&&c<=L'F')d=c-L'A'+10;else return false;
  value=(value<<4)|d;
 }
 alpha=n==8?value&255:255;rgb=n==8?value>>8:value;return true;
}
inline std::wstring bot_color_hex(){wchar_t value[10];swprintf(value,10,L"#%06X%02X",bot_rgb,bot_alpha);return value;}
inline void bot_tint(float& r,float& g,float& b,float& alpha,float& phase){
 r=float((bot_rgb>>16)&255)/255;g=float((bot_rgb>>8)&255)/255;b=float(bot_rgb&255)/255;
 alpha*=float(bot_alpha)/255;phase=-1;
}
inline bool bot_layout=false;
inline bool bot_range(int slot,int kind,int start,int offset,int count,size_t total){
 return kind==1&&start>0&&start<=256&&offset>=0&&offset<=2048&&count>0&&count<=2048&&size_t(start+offset+count)<=total&&slot>=start+offset&&slot<start+offset+count;
}
// Exact current spawn routine only; changed game layouts fail closed for this
// private feature without affecting public cosmetics. The reviewed routine constructs the
// generated-player range from the world start, offset and count fields.
inline void validate_bot_layout(){
 bot_layout=false;std::array<unsigned char,0x3304> code{};std::array<unsigned char,32> hash{},expected{};
 const char* hex="ba342f909e53a6f11ed342886f74f2b4a359c93c9796e9ea260502325f7d984e";
 if(!ssc_compat::supports(1)||!read_bytes(image_base+0x2782f0,code.data(),code.size()))return;
 if(sodium_hex2bin(expected.data(),expected.size(),hex,64,nullptr,nullptr,nullptr)!=0)return;
 crypto_hash_sha256(hash.data(),code.data(),code.size());bot_layout=sodium_memcmp(hash.data(),expected.data(),32)==0;
}
inline bool private_visible(){return attached&&bot_layout&&ssc_auth::available();}
inline bool private_active(){return attached&&bot_layout&&ssc_auth::active();}
struct Identity {bool local=false,bot=false;};
// Native MSVC account string, not actor+0x798 (the editable display name).
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
    return account_string(image_base+ssc_compat::resolve(0xdfd1c8),own)&&account_string(account,candidate)&&own==candidate;
}
inline bool native_phase(float& phase){
    float speed=0,time=0;
    if(!read(image_base+ssc_compat::resolve(0xfaa094),speed)||!read(image_base+ssc_compat::resolve(0xfaaaf0),time)||!std::isfinite(time)||!std::isfinite(speed))return false;
    float candidate=std::fmod(time*speed,1.f);if(!std::isfinite(candidate))return false;if(candidate<0)candidate+=1.f;phase=candidate;return true;
}
inline Identity identify(uintptr_t actor){
    Identity answer;uintptr_t world=0,first=0,last=0,registry=0;int slot=-1,kind=-1,local_slot=-1;unsigned char active=0;
    if(!read(image_base+ssc_compat::resolve(0xde35d0),world)||!world||!read(world+0xa618,first)||!read(world+0xa620,last)||!first||last<first||(last-first)%0x3478||(last-first)/0x3478>2048)return answer;
    if(actor<first||actor>=last||(actor-first)%0x3478||!read(actor+0x78,slot)||slot<0||uintptr_t(slot)!=(actor-first)/0x3478||!read(actor+0x7dc,kind)||!read(actor+0x81,active)||!active)return answer;
    if(bot_layout&&ssc_auth::active()){
        int start=0,offset=0,count=0;
        if(read(image_base+0xfad7bc,start)&&read(world+0x190,offset)&&read(world+0x28c,count))answer.bot=bot_range(slot,kind,start,offset,count,(last-first)/0x3478);
    }
    // e03370 is used by the native own-player formatter; bf48 may be spectated.
    answer.local=kind==0&&read(image_base+ssc_compat::resolve(0xe17608),local_slot)&&slot==local_slot&&
        read(image_base+ssc_compat::resolve(0xe1b088),registry)&&registry==first&&local_account(actor+0x6f0);
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
    if(!account_string(image_base+ssc_compat::resolve(0xdfd1c8),own_key)||!account_string(text,candidate)||own_key!=candidate)return false;
    uintptr_t world=0,first=0,last=0;int slot=-1;
    if(!read(image_base+ssc_compat::resolve(0xde35d0),world)||!read(world+0xa618,first)||!read(world+0xa620,last)||
       !first||last<first||(last-first)%0x3478||(last-first)/0x3478>2048||
       !read(image_base+ssc_compat::resolve(0xe17608),slot)||slot<0||size_t(slot)>=(last-first)/0x3478)return false;
    const auto own=first+size_t(slot)*0x3478;
    if(!identify(own).local)return false;
    for(auto p=first;p<last;p+=0x3478){unsigned char active=0;
        if(!read(p+0x81,active))return false;
        if(p!=own&&active&&((account_string(p+0x798,candidate)&&candidate==own_key)||(account_string(p+0x6f0,candidate)&&candidate==own_key)))return false;
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
    if(drawing_local_widget&&cosmetics.load()&&local_account(reinterpret_cast<uintptr_t>(name))&&native_phase(phase)){++cosmetic_draws;solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline void __cdecl overhead(uintptr_t actor,float alpha,unsigned char p3,uintptr_t p4,unsigned char p5,unsigned char p6){
    struct Scope{uintptr_t previous;Scope(uintptr_t p):previous(drawing_actor){drawing_actor=p;}~Scope(){drawing_actor=previous;}} scope(actor);
    native_overhead(actor,alpha,p3,p4,p5,p6);
}
inline float __cdecl draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
    if(!cosmetics.load()&&!private_active())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    auto identity=identify(drawing_actor);
    if(identity.bot&&private_active())bot_tint(r,g,b,alpha,phase);
    if(cosmetics.load()&&identity.local){++cosmetic_draws;solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
extern "C" float __cdecl ssc_score_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t row){
    if(!cosmetics.load()&&!private_active())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    int slot=-1;uintptr_t world=0,first=0,last=0;
    Identity identity;
    if(read(row+0x2c,slot)&&slot>=0&&read(image_base+ssc_compat::resolve(0xde35d0),world)&&read(world+0xa618,first)&&read(world+0xa620,last)&&last>=first&&size_t(slot)<(last-first)/0x3478)identity=identify(first+size_t(slot)*0x3478);
    if(identity.bot&&private_active())bot_tint(r,g,b,alpha,phase);
    if(cosmetics.load()&&identity.local&&native_phase(phase)){++cosmetic_draws;solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline float __cdecl account_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
    if(cosmetics.load()&&local_account(reinterpret_cast<uintptr_t>(name))){if(rainbow)native_phase(phase);else solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline bool attach(){
    if(!ssc_compat::supports(1))return false;
    image_base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    native_predicate=reinterpret_cast<Predicate>(image_base+ssc_compat::resolve(0x3aa1a0));native_overhead=reinterpret_cast<Overhead>(image_base+ssc_compat::resolve(0x8466c0));native_draw=reinterpret_cast<Draw>(image_base+ssc_compat::resolve(0x47f990));
    native_widget=reinterpret_cast<Widget>(image_base+ssc_compat::resolve(0x9990f0));
    struct Site{uint32_t call,target;uintptr_t hook;};
    // Whitelist presentation calls only. Do not hook the predicate globally:
    // 5fb3b0/5fd316/6834a3/6b6db7 also service pass-related UI/cache logic.
    Site sites[]={
        {0x4197f7,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x41987b,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x621cd4,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x63b527,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x6aa0cc,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x6aa354,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x6aa572,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x9e0873,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x9abe4b,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},
        {0x9abfef,0x47f990,reinterpret_cast<uintptr_t>(account_draw)},

        {0x33c1f4,0x8466c0,reinterpret_cast<uintptr_t>(overhead)},
        {0x846b76,0x3aa1a0,reinterpret_cast<uintptr_t>(predicate)},
        {0x41960f,0x3aa1a0,reinterpret_cast<uintptr_t>(predicate)},
        {0x439565,0x3aa1a0,reinterpret_cast<uintptr_t>(predicate)},
        {0x62197c,0x3aa1a0,reinterpret_cast<uintptr_t>(predicate)},
        {0x63b1ee,0x3aa1a0,reinterpret_cast<uintptr_t>(predicate)},
        {0x6a8589,0x3aa1a0,reinterpret_cast<uintptr_t>(predicate)},
        {0x9e07d5,0x3aa1a0,reinterpret_cast<uintptr_t>(predicate)},
        {0x9abd7d,0x3aa1a0,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x9abf27,0x3aa1a0,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x6082a1,0x9990f0,reinterpret_cast<uintptr_t>(local_widget)},
        {0x99b589,0x47f990,reinterpret_cast<uintptr_t>(widget_draw)},
        {0x8473f8,0x47f990,reinterpret_cast<uintptr_t>(draw)},
        {0x847484,0x47f990,reinterpret_cast<uintptr_t>(draw)},
        {0x43603e,0x47f990,reinterpret_cast<uintptr_t>(ssc_score_bridge)},
        {0x4360c3,0x47f990,reinterpret_cast<uintptr_t>(ssc_score_bridge)}};
    for(auto& site:sites){site.call=ssc_compat::resolve(uint32_t(site.call));site.target=ssc_compat::resolve(uint32_t(site.target));}
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
