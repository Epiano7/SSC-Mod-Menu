#pragma once
#include "compatibility.h"
#include "private_auth.h"
#include "shared_cosmetics.h"
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
inline bool gradient=false;inline int gradient_count=3;
inline std::array<unsigned,3> gradient_colors{5623039,16737996,16759620};
inline std::array<float,3> gradient_rgb(float phase){
 if(!std::isfinite(phase))phase=0;
 const int count=std::clamp(gradient_count,2,3);float t=(phase-std::floor(phase))*count;int a=std::min(int(t),count-1),b=(a+1)%count;t-=a;
 std::array<float,3> color{};for(int k=0;k<3;++k){int shift=16-8*k;float first=float((gradient_colors[a]>>shift)&255)/255,second=float((gradient_colors[b]>>shift)&255)/255;color[k]=first+(second-first)*t;}return color;
}
inline thread_local int color_scope=0;
// -1 means a shared text path with no owner attribution; 0/1 are explicit
// other/local decisions from actor, sender or account-specific adapters.
inline thread_local int identity_scope=-1;
inline thread_local ssc_shared::Style shared_style;
struct ColorScope {
 int previous,previous_identity;
 ssc_shared::Style previous_style=shared_style;
 explicit ColorScope(bool local,bool attributed=true):previous(color_scope),previous_identity(identity_scope){
  if(attributed)identity_scope=local?1:0;
  color_scope=local&&cosmetics.load()?(gradient?2:(!rainbow?1:0)):0;
 }
 void remote(const ssc_shared::Style& value){shared_style=value;identity_scope=2;color_scope=value.mode==ssc_shared::Mode::solid?3:value.mode==ssc_shared::Mode::gradient?4:0;}
 void inherit_remote(){if(previous_identity==2){identity_scope=2;color_scope=previous;shared_style=previous_style;}}
 ~ColorScope(){color_scope=previous;identity_scope=previous_identity;shared_style=previous_style;}
};
using Palette=float*(*)(uintptr_t,float*,float,float,unsigned char,unsigned char);
inline Palette native_palette=nullptr;
inline float* palette(uintptr_t context,float* output,float position,float lift,unsigned char gray,unsigned char balance){
 if(!color_scope)return native_palette(context,output,position,lift,gray,balance);
 auto color=gradient_rgb(position/255.f);
 if(color_scope==1)for(int k=0;k<3;++k)color[k]=float((solid_rgb>>(16-8*k))&255)/255;
 if(color_scope==3||color_scope==4){
  float t=position/255.f;t=(t-std::floor(t))*shared_style.count;unsigned a=std::min(unsigned(t),shared_style.count-1),b=(a+1)%shared_style.count;t-=a;
  for(int k=0;k<3;++k){unsigned shift=16-8*k;float first=float((shared_style.colors[a]>>shift)&255)/255,second=float((shared_style.colors[b]>>shift)&255)/255;color[k]=first+(second-first)*t;}
 }
 for(int k=0;k<3;++k){output[k]=color[k];}return output;
}

inline void solid(float& r,float& g,float& b,float& phase){if(!rainbow&&!gradient){r=float((solid_rgb>>16)&255)/255;g=float((solid_rgb>>8)&255)/255;b=float(solid_rgb&255)/255;phase=0;}}
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
 const char* hex="53dde98879222f7a4e6b7d3b6eccce71c8bd364d5490efc94c09fd44dd99eabb";
 if(!ssc_compat::supports(1)||!read_bytes(image_base+0x278680,code.data(),code.size()))return;
 if(sodium_hex2bin(expected.data(),expected.size(),hex,64,nullptr,nullptr,nullptr)!=0)return;
 crypto_hash_sha256(hash.data(),code.data(),code.size());bot_layout=sodium_memcmp(hash.data(),expected.data(),32)==0;
}
inline bool private_visible(){return attached&&bot_layout&&ssc_auth::available();}
inline bool private_active(){return attached&&bot_layout&&ssc_auth::active();}
struct Identity {bool local=false,bot=false;std::string account;};
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
    return account_string(image_base+ssc_compat::resolve(0xdfe1e8),own)&&account_string(account,candidate)&&own==candidate;
}
inline bool native_phase(float& phase){
    float speed=0,time=0;
    if(!read(image_base+ssc_compat::resolve(0xfab0a4),speed)||!read(image_base+ssc_compat::resolve(0xfabb00),time)||!std::isfinite(time)||!std::isfinite(speed))return false;
    float candidate=std::fmod(time*speed,1.f);if(!std::isfinite(candidate))return false;if(candidate<0)candidate+=1.f;phase=candidate;return true;
}
inline Identity identify(uintptr_t actor){
    Identity answer;uintptr_t world=0,first=0,last=0,registry=0;int slot=-1,kind=-1,local_slot=-1;unsigned char active=0;
    if(!read(image_base+ssc_compat::resolve(0xde45d0),world)||!world||!read(world+0xa618,first)||!read(world+0xa620,last)||!first||last<first||(last-first)%0x3478||(last-first)/0x3478>2048)return answer;
    if(actor<first||actor>=last||(actor-first)%0x3478||!read(actor+0x78,slot)||slot<0||uintptr_t(slot)!=(actor-first)/0x3478||!read(actor+0x7dc,kind)||!read(actor+0x81,active)||!active)return answer;
    if(kind==0){std::string key;if(account_string(actor+0x6f0,key)&&ssc_shared::account_valid(key))answer.account=key;}
    if(kind==1&&bot_layout&&ssc_auth::active()){
        int start=0,offset=0,count=0;
        if(read(image_base+0xfae7cc,start)&&read(world+0x190,offset)&&read(world+0x28c,count))answer.bot=bot_range(slot,kind,start,offset,count,(last-first)/0x3478);
    }
    // e03370 is used by the native own-player formatter; bf48 may be spectated.
    answer.local=kind==0&&read(image_base+ssc_compat::resolve(0xe18628),local_slot)&&slot==local_slot&&
        read(image_base+ssc_compat::resolve(0xe1c0a8),registry)&&registry==first&&local_account(actor+0x6f0);
    return answer;
}
inline bool shared_account(uintptr_t string,ssc_shared::Style& style){
 std::string key;return attached&&account_string(string,key)&&ssc_shared::account_valid(key)&&!local_account(string)&&ssc_shared::service().get(key,style);
}
inline void shared_tint(ColorScope& scope,const ssc_shared::Style& value,float& r,float& g,float& b,float& phase){
 scope.remote(value);if(value.mode==ssc_shared::Mode::solid){auto c=value.colors[0];r=float((c>>16)&255)/255;g=float((c>>8)&255)/255;b=float(c&255)/255;phase=0;}
 else if(!native_phase(phase))phase=0;
}
inline void shared_actor_tint(ColorScope& scope,const Identity& identity,float& r,float& g,float& b,float& phase){
 ssc_shared::Style value;if(!identity.local&&!identity.bot&&!identity.account.empty()&&ssc_shared::service().get(identity.account,value))shared_tint(scope,value,r,g,b,phase);
}
inline std::vector<std::string> shared_accounts(){
 std::set<std::string> keys;uintptr_t world=0,first=0,last=0;
 if(!attached||!read(image_base+ssc_compat::resolve(0xde45d0),world)||!world||!read(world+0xa618,first)||!read(world+0xa620,last)||!first||last<first||(last-first)%0x3478||(last-first)/0x3478>2048)return {};
 for(auto actor=first;actor<last&&keys.size()<32;actor+=0x3478){auto identity=identify(actor);if(!identity.account.empty()&&!identity.local)keys.insert(identity.account);}
 return {keys.begin(),keys.end()};
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
    if(!account_string(image_base+ssc_compat::resolve(0xdfe1e8),own_key)||!account_string(text,candidate)||own_key!=candidate)return false;
    uintptr_t world=0,first=0,last=0;int slot=-1;
    if(!read(image_base+ssc_compat::resolve(0xde45d0),world)||!read(world+0xa618,first)||!read(world+0xa620,last)||
       !first||last<first||(last-first)%0x3478||(last-first)/0x3478>2048||
       !read(image_base+ssc_compat::resolve(0xe18628),slot)||slot<0||size_t(slot)>=(last-first)/0x3478)return false;
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
    const bool own=drawing_local_widget&&local_account(reinterpret_cast<uintptr_t>(name));ColorScope scope(own,drawing_local_widget);
    if(own&&cosmetics.load()){++cosmetic_draws;if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline void __cdecl overhead(uintptr_t actor,float alpha,unsigned char p3,uintptr_t p4,unsigned char p5,unsigned char p6){
    struct Scope{uintptr_t previous;Scope(uintptr_t p):previous(drawing_actor){drawing_actor=p;}~Scope(){drawing_actor=previous;}} scope(actor);
    native_overhead(actor,alpha,p3,p4,p5,p6);
}
inline float __cdecl draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
    auto identity=identify(drawing_actor);ColorScope scope(identity.local);
    shared_actor_tint(scope,identity,r,g,b,phase);
    if(identity.bot&&private_active())bot_tint(r,g,b,alpha,phase);
    if(cosmetics.load()&&identity.local){++cosmetic_draws;if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
extern "C" float __cdecl ssc_score_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t row){
    int slot=-1;uintptr_t world=0,first=0,last=0;
    Identity identity;
    if(read(row+0x2c,slot)&&slot>=0&&read(image_base+ssc_compat::resolve(0xde45d0),world)&&read(world+0xa618,first)&&read(world+0xa620,last)&&last>=first&&size_t(slot)<(last-first)/0x3478)identity=identify(first+size_t(slot)*0x3478);
    ColorScope scope(identity.local);
    shared_actor_tint(scope,identity,r,g,b,phase);
    if(identity.bot&&private_active())bot_tint(r,g,b,alpha,phase);
    if(cosmetics.load()&&identity.local){++cosmetic_draws;if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline float __cdecl account_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
    const bool own=local_account(reinterpret_cast<uintptr_t>(name));ColorScope scope(own);
    if(cosmetics.load()&&own){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Chat labels are formatted (for example "Name:"). R13 still points to
// the original sender record whose leading string is used by the native
// account predicate. Never authorize a name tint by stripping display text.
extern "C" void ssc_chat_name_bridge();
extern "C" void ssc_actor_name_bridge();
extern "C" float ssc_chat_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t sender){
 const bool own=local_account(sender);ColorScope scope(own);
 ssc_shared::Style remote;if(!own&&shared_account(sender,remote))shared_tint(scope,remote,r,g,b,phase);
 if(own&&cosmetics.load()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Actor-attached name labels have a separate render path from overhead names.
extern "C" float ssc_actor_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t actor){
 const auto identity=identify(actor);const bool own=identity.local;ColorScope scope(own);
 shared_actor_tint(scope,identity,r,g,b,phase);
 if(own&&cosmetics.load()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Main-menu account widget uses its stored wrapped label (+0xa8), not
// the optional single-line argument. Keep the native layout/ownership intact.
using Wrapped=int(*)(uintptr_t,float,float,float,float,void*,float,float,float,float,float,int,int,int,unsigned char,int,float,float,float);
inline Wrapped native_wrapped=nullptr;
inline int widget_wrapped(uintptr_t renderer,float x,float y,float width,float height,void* name,float size,float r,float g,float b,float alpha,int align,int vertical,int first,unsigned char flags,int limit,float start,float end,float phase){
 const bool own=drawing_local_widget&&local_account(reinterpret_cast<uintptr_t>(name));ColorScope scope(own,drawing_local_widget);
 if(own&&cosmetics.load()){++cosmetic_draws;if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
 return native_wrapped(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
}
// Shared native text adapters cover cached list/profile/leaderboard widgets as
// well as direct name labels. Compare the COMPLETE UTF-16 text with the current
// UTF-8 account name; never strip punctuation or recolor matching substrings.
// This is presentation matching, not private-module authorization. Explicit
// actor/sender ownership always wins, including an explicit non-local result.
inline bool local_wide_label(uintptr_t object){
 std::array<unsigned char,32> header{};size_t length=0,capacity=0;uintptr_t data=object;
 if(!read_bytes(object,header.data(),header.size()))return false;
 std::memcpy(&length,header.data()+16,8);std::memcpy(&capacity,header.data()+24,8);
 if(!length||length>256||capacity<length)return false;
 std::string account;if(!account_string(image_base+ssc_compat::resolve(0xdfe1e8),account))return false;
 std::array<wchar_t,256> expected{},candidate{};
 const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,account.data(),int(account.size()),expected.data(),int(expected.size()));
 if(count<=0||size_t(count)!=length)return false;
 if(capacity>7){std::memcpy(&data,header.data(),8);if(!read_bytes(data,candidate.data(),length*sizeof(wchar_t)))return false;}
 else{if(length>7)return false;std::memcpy(candidate.data(),header.data(),length*sizeof(wchar_t));}
 for(size_t i=0;i<length;++i){auto c=candidate[i];if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';if(c!=expected[i])return false;}
 return true;
}
inline bool shared_label_local(void* text){
 return cosmetics.load()&&(identity_scope>=0?identity_scope==1:local_wide_label(reinterpret_cast<uintptr_t>(text)));
}
inline Draw native_wide_draw=nullptr;
inline float wide_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 const bool own=shared_label_local(name);ColorScope scope(own);
 scope.inherit_remote();
 if(own){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 // The native callee consumes this MSVC string. Inspect before calling and
 // forward exactly once; do not free it or substitute a MinGW string object.
 return native_wide_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline Wrapped native_wide_wrapped=nullptr;
inline int wide_wrapped(uintptr_t renderer,float x,float y,float width,float height,void* name,float size,float r,float g,float b,float alpha,int align,int vertical,int first,unsigned char flags,int limit,float start,float end,float phase){
 const bool own=shared_label_local(name);ColorScope scope(own);
 scope.inherit_remote();
 if(own){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_wide_wrapped(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
}
inline bool attach(){
    if(!ssc_compat::supports(1))return false;
    image_base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    native_predicate=reinterpret_cast<Predicate>(image_base+ssc_compat::resolve(0x3aa660));native_overhead=reinterpret_cast<Overhead>(image_base+ssc_compat::resolve(0x8474e0));native_draw=reinterpret_cast<Draw>(image_base+ssc_compat::resolve(0x480470));
    native_wrapped=reinterpret_cast<Wrapped>(image_base+ssc_compat::resolve(0x480600));
    native_wide_draw=reinterpret_cast<Draw>(image_base+ssc_compat::resolve(0x4810b0));
    native_wide_wrapped=reinterpret_cast<Wrapped>(image_base+ssc_compat::resolve(0x480790));
    native_palette=reinterpret_cast<Palette>(image_base+ssc_compat::resolve(0x1545d0));
    native_widget=reinterpret_cast<Widget>(image_base+ssc_compat::resolve(0x999ca0));
    struct Site{uint32_t call,target;uintptr_t hook;};
    // Whitelist presentation calls only. Do not hook the predicate globally:
    // 5fb3b0/5fd316/6834a3/6b6db7 also service pass-related UI/cache logic.
    Site sites[]={
        // Shared narrow-to-wide conversion and direct UTF-16 text paths.
        {0x42e592,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x42ebfc,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x42ef23,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x439f0d,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x480596,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x48072b,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x484310,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x52dd9f,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x52f400,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x52f8fa,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5325c2,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x534268,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x534705,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x534ecc,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x535321,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5355f8,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x535898,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x535b38,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x536038,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x536c3d,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x537121,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5423a4,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x54265a,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5438f3,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x543e76,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x544456,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5457d0,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x545bde,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5460ac,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x632e74,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x64f01c,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x64f189,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x64f2e0,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x64f429,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x64f4d6,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x64f66f,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x64f81b,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x64fa5f,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x657383,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65751f,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6576b2,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x657aaa,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x657c5b,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x657e03,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x657fa1,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65d7e8,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65eb3e,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x661c28,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x661cda,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x664f18,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6bdc58,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6bde10,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6be3e5,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6be4c8,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6be96d,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6bec9b,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6befbd,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6bf2df,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c1979,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c1c94,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c1faf,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c2322,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c26df,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6d42fa,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x71661e,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x9a14fa,0x4810b0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x9a15d0,0x480790,reinterpret_cast<uintptr_t>(wide_wrapped)},
        // Both glyph palette paths (regular and alternate text rendering).
        {0x489ad1,0x1545d0,reinterpret_cast<uintptr_t>(palette)},
        {0x489b32,0x1545d0,reinterpret_cast<uintptr_t>(palette)},
        {0x3f5675,0x480470,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f572e,0x480470,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f57c1,0x480470,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f5895,0x480470,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f5931,0x480470,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f59ec,0x480470,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},

        {0x99bfff,0x480600,reinterpret_cast<uintptr_t>(widget_wrapped)},
        {0x489419,0x1545d0,reinterpret_cast<uintptr_t>(palette)},
        {0x489476,0x1545d0,reinterpret_cast<uintptr_t>(palette)},
        {0x419b27,0x480470,reinterpret_cast<uintptr_t>(account_draw)},
        {0x419bab,0x480470,reinterpret_cast<uintptr_t>(account_draw)},
        {0x622a84,0x480470,reinterpret_cast<uintptr_t>(account_draw)},
        {0x63c2d7,0x480470,reinterpret_cast<uintptr_t>(account_draw)},
        {0x6aae7c,0x480470,reinterpret_cast<uintptr_t>(account_draw)},
        {0x6ab104,0x480470,reinterpret_cast<uintptr_t>(ssc_chat_name_bridge)},
        {0x6ab322,0x480470,reinterpret_cast<uintptr_t>(account_draw)},
        {0x9e1423,0x480470,reinterpret_cast<uintptr_t>(account_draw)},
        {0x9ac9fb,0x480470,reinterpret_cast<uintptr_t>(account_draw)},
        {0x9acb9f,0x480470,reinterpret_cast<uintptr_t>(account_draw)},

        {0x33c6b4,0x8474e0,reinterpret_cast<uintptr_t>(overhead)},
        {0x847996,0x3aa660,reinterpret_cast<uintptr_t>(predicate)},
        {0x41993f,0x3aa660,reinterpret_cast<uintptr_t>(predicate)},
        {0x439895,0x3aa660,reinterpret_cast<uintptr_t>(predicate)},
        {0x62272c,0x3aa660,reinterpret_cast<uintptr_t>(predicate)},
        {0x63bf9e,0x3aa660,reinterpret_cast<uintptr_t>(predicate)},
        {0x6a9339,0x3aa660,reinterpret_cast<uintptr_t>(predicate)},
        {0x9e1385,0x3aa660,reinterpret_cast<uintptr_t>(predicate)},
        {0x9ac92d,0x3aa660,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x9acad7,0x3aa660,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x609051,0x999ca0,reinterpret_cast<uintptr_t>(local_widget)},
        {0x99c139,0x480470,reinterpret_cast<uintptr_t>(widget_draw)},
        {0x848218,0x480470,reinterpret_cast<uintptr_t>(draw)},
        {0x8482a4,0x480470,reinterpret_cast<uintptr_t>(draw)},
        {0x43636e,0x480470,reinterpret_cast<uintptr_t>(ssc_score_bridge)},
        {0x4363f3,0x480470,reinterpret_cast<uintptr_t>(ssc_score_bridge)}};
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
