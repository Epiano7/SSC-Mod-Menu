#pragma once
#include "privacy.h"
#include "compatibility.h"
#include "performance_metrics.h"
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
// Local rendering does not publish the selected palette or identify the user to peers.
inline bool local_cosmetics(){return cosmetics.load();}
inline bool rainbow=true;inline unsigned solid_rgb=0x55e20f;
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
  color_scope=local&&local_cosmetics()?(gradient?2:(!rainbow?1:0)):0;
 }
 void remote(const ssc_shared::Style& value){shared_style=value;identity_scope=2;color_scope=value.mode==ssc_shared::Mode::solid?3:value.mode==ssc_shared::Mode::gradient?4:0;}
 void inherit_remote(){if(previous_identity==2){identity_scope=2;color_scope=previous;shared_style=previous_style;}}
 ~ColorScope(){color_scope=previous;identity_scope=previous_identity;shared_style=previous_style;}
};
using Palette=float*(*)(uintptr_t,float*,float,float,unsigned char,unsigned char);
inline Palette native_palette=nullptr;
inline float* palette(uintptr_t context,float* output,float position,float lift,unsigned char gray,unsigned char balance){
 if(!color_scope||(color_scope>=3&&ssc_privacy::blocked()))return native_palette(context,output,position,lift,gray,balance);
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
 bot_layout=false;std::array<unsigned char,13124> code{};std::array<unsigned char,32> hash{},expected{};
 const char* hex="d9af6dadeef7a56ba8049a24c53b5073e9d27c918dbc9a76b100b1e33592e607";
 if(!ssc_compat::supports(1)||!read_bytes(image_base+0x279970,code.data(),code.size()))return;
 if(sodium_hex2bin(expected.data(),expected.size(),hex,64,nullptr,nullptr,nullptr)!=0)return;
 crypto_hash_sha256(hash.data(),code.data(),code.size());bot_layout=sodium_memcmp(hash.data(),expected.data(),32)==0;
}
inline bool private_visible(){return attached&&bot_layout&&ssc_auth::available();}
inline bool private_active(){return attached&&bot_layout&&ssc_auth::active();}
inline bool receive_colors=false;
inline bool render_access=false;
inline uint64_t render_access_at=0;
inline bool render_private_active(){auto now=GetTickCount64();return attached&&bot_layout&&render_access_at&&now>=render_access_at&&now-render_access_at<100&&render_access;}

inline bool appearance_enabled(){return local_cosmetics()||receive_colors||render_private_active();}

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
    return account_string(image_base+ssc_compat::resolve(0xe061e8),own)&&account_string(account,candidate)&&own==candidate;
}
inline bool native_phase(float& phase){
    float speed=0,time=0;
    if(!read(image_base+ssc_compat::resolve(0xfb31a4),speed)||!read(image_base+ssc_compat::resolve(0xfb3bf8),time)||!std::isfinite(time)||!std::isfinite(speed))return false;
    float candidate=std::fmod(time*speed,1.f);if(!std::isfinite(candidate))return false;if(candidate<0)candidate+=1.f;phase=candidate;return true;
}
inline Identity identify(uintptr_t actor,bool allow_inactive=false){
    Identity answer;uintptr_t world=0,first=0,last=0,registry=0;int slot=-1,kind=-1,local_slot=-1;unsigned char active=0;
    if(!read(image_base+ssc_compat::resolve(0xdec5d0),world)||!world||!read(world+0xa658,first)||!read(world+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048)return answer;
    if(actor<first||actor>=last||(actor-first)%0x34b8||!read(actor+0x78,slot)||slot<0||uintptr_t(slot)!=(actor-first)/0x34b8||!read(actor+0x7dc,kind)||!read(actor+0x81,active)||(!active&&!allow_inactive))return answer;
    if(kind==0){std::string key;if(account_string(actor+0x6f0,key)&&ssc_shared::account_valid(key))answer.account=key;}
    if(kind==1&&render_private_active()){
        int start=0,offset=0,count=0;
        if(read(image_base+0xfb68d4,start)&&read(world+0x190,offset)&&read(world+0x28c,count))answer.bot=bot_range(slot,kind,start,offset,count,(last-first)/0x34b8);
    }
    // e03370 is used by the native own-player formatter; bf48 may be spectated.
    answer.local=kind==0&&read(image_base+ssc_compat::resolve(0xe20678),local_slot)&&slot==local_slot&&
        read(image_base+ssc_compat::resolve(0xe24138),registry)&&registry==first&&local_account(actor+0x6f0);
    return answer;
}
inline bool (*lookup_shared)(const std::string&,ssc_shared::Style&)=+[](const std::string& key,ssc_shared::Style& value){return ssc_shared::service().get(key,value);};
struct FrameStyle {bool found=false;ssc_shared::Style style;};
inline std::map<std::string,FrameStyle> frame_styles;
inline void begin_render_frame(bool receive){
 receive_colors=receive;
 frame_styles.clear();render_access=private_active();render_access_at=GetTickCount64();
}
inline bool frame_shared(const std::string& key,ssc_shared::Style& style){
 if(ssc_privacy::blocked()||!receive_colors)return false;
 ssc_perf::Scope timing(ssc_perf::sharing);
 auto found=frame_styles.find(key);
 if(found==frame_styles.end()){
  FrameStyle result;result.found=lookup_shared(key,result.style);
  if(frame_styles.size()<128)found=frame_styles.emplace(key,result).first;
  else {style=result.style;return result.found;}
 }
 style=found->second.style;return found->second.found;
}
inline bool shared_account(uintptr_t string,ssc_shared::Style& style){
 std::string key;return receive_colors&&attached&&account_string(string,key)&&ssc_shared::account_valid(key)&&!local_account(string)&&frame_shared(key,style);
}
inline void shared_tint(ColorScope& scope,const ssc_shared::Style& value,float& r,float& g,float& b,float& phase){
 scope.remote(value);if(value.mode==ssc_shared::Mode::solid){auto c=value.colors[0];r=float((c>>16)&255)/255;g=float((c>>8)&255)/255;b=float(c&255)/255;phase=0;}
 else if(!native_phase(phase))phase=0;
}
inline void shared_actor_tint(ColorScope& scope,const Identity& identity,float& r,float& g,float& b,float& phase){
 ssc_shared::Style value;if(!identity.local&&!identity.bot&&!identity.account.empty()&&frame_shared(identity.account,value))shared_tint(scope,value,r,g,b,phase);
}
// Text-only labels (vehicle occupants, cached lists, compact event feed) have
// no owner pointer. Resolve complete labels to exactly one active actor; never
// strip prefixes or accept substrings, and reject duplicate display names.
inline std::map<std::string,uintptr_t> shared_label_index;
inline uintptr_t shared_label_world=0;
inline uint64_t shared_label_refresh=0,shared_label_scans=0;
struct FormattedLabel {uintptr_t actor=0,world=0;std::string account,display;};
inline std::map<std::string,FormattedLabel> formatted_labels;
inline void remember_formatted_label(const std::string& label,const FormattedLabel& identity){
 auto add=[&](const std::string& key){if(formatted_labels.size()>=2048)return;auto [it,added]=formatted_labels.emplace(key,identity);if(!added&&it->second.actor!=identity.actor)it->second.actor=0;};
 add(label);
 // Chat adds its separator to the complete native-formatted sender label.
 // Index those exact labels too; never strip prefixes from arbitrary text.
 if(!label.empty()&&label.back()!=':'){add(label+":");add(label+": ");}
}
using NameFormatter=uintptr_t(*)(uintptr_t,uintptr_t,unsigned char);
inline NameFormatter native_name_formatter=nullptr;
inline NameFormatter native_sender_formatter=nullptr;
inline void remember_native_name(uintptr_t actor,uintptr_t result){
 if(!local_cosmetics()&&!receive_colors&&!render_private_active())return;
 std::string label,account,display;uintptr_t world=0;
 account_string(actor+0x6f0,account); // Bots need not have a human account key.
 if(read(image_base+ssc_compat::resolve(0xdec5d0),world)&&world&&account_string(result,label)&&account_string(actor+0x798,display)){
  const auto identity=identify(actor,true);
  if(!identity.local&&!identity.bot&&identity.account.empty())return;
  if(world!=shared_label_world){formatted_labels.clear();shared_label_index.clear();shared_label_world=world;shared_label_refresh=0;}
  remember_formatted_label(label,FormattedLabel{actor,world,account,display});
 }
}
inline uintptr_t list_name_formatter(uintptr_t actor,uintptr_t output,unsigned char flags){
 auto result=native_name_formatter(actor,output,flags);remember_native_name(actor,result);
 return result;
}
inline uintptr_t sender_name_formatter(uintptr_t actor,uintptr_t output,unsigned char flags){
 auto result=native_sender_formatter(actor,output,flags);remember_native_name(actor,result);
 return result;
}
inline uintptr_t label_actor(const std::string& label){
 auto raw=shared_label_index.find(label);if(raw!=shared_label_index.end())return raw->second;
 auto found=formatted_labels.find(label);if(found==formatted_labels.end()||!found->second.actor)return 0;
 const auto& entry=found->second;uintptr_t world=0;std::string account,display;
 account_string(entry.actor+0x6f0,account);
 if(!read(image_base+ssc_compat::resolve(0xdec5d0),world)||world!=entry.world||account!=entry.account||!account_string(entry.actor+0x798,display)||display!=entry.display)return 0;
 return entry.actor;
}
inline Identity generated_label_identity(uintptr_t text){
 std::string label;if(!attached||!account_string(text,label))return {};
 auto actor=label_actor(label);return actor?identify(actor,true):Identity{};
}
inline void refresh_shared_labels(uint64_t now,bool receive){
 if(!attached||(!receive&&!local_cosmetics())){shared_label_index.clear();formatted_labels.clear();shared_label_world=0;shared_label_refresh=0;return;}
 uintptr_t world=0,first=0,last=0;
 if(!read(image_base+ssc_compat::resolve(0xdec5d0),world)||!world){shared_label_index.clear();shared_label_world=0;return;}
 if(world==shared_label_world&&shared_label_refresh&&now>=shared_label_refresh&&now-shared_label_refresh<250)return;
 if(world!=shared_label_world)formatted_labels.clear();
 shared_label_index.clear();shared_label_world=world;shared_label_refresh=now;++shared_label_scans;
 if(!read(world+0xa658,first)||!read(world+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048)return;
 auto add=[](const std::string& label,uintptr_t actor){if(label.empty())return;auto result=shared_label_index.emplace(label,actor);if(!result.second&&result.first->second!=actor)result.first->second=0;};
 for(auto actor=first;actor<last;actor+=0x34b8){int kind=-1;if(!read(actor+0x7dc,kind)){shared_label_index.clear();return;}if(kind!=0&&kind!=1)continue;
  std::string key,display;if(account_string(actor+0x6f0,key))add(key,actor);if(account_string(actor+0x798,display))add(display,actor);
 }
}
inline bool shared_text(const std::string& label,ssc_shared::Style& value){
 // Generic HUD text must only perform an index lookup, never a roster walk.
 if(!attached||label.empty())return false;
 auto actor=label_actor(label);if(!actor)return false;
 uintptr_t world=0;if(!read(image_base+ssc_compat::resolve(0xdec5d0),world)||world!=shared_label_world)return false;
 auto identity=identify(actor,true);if(identity.local||identity.bot||identity.account.empty())return false;
 // A reused actor slot or renamed player must not inherit a stale label.
 std::string display;if(identity.account!=label&&(!account_string(actor+0x798,display)||display!=label)&&formatted_labels.find(label)==formatted_labels.end())return false;
 return frame_shared(identity.account,value);
}
inline bool bot_text(const std::string& label){
 if(!attached)return false;
 auto actor=label_actor(label);if(!actor)return false;
 uintptr_t world=0;if(!read(image_base+ssc_compat::resolve(0xdec5d0),world)||world!=shared_label_world)return false;
 if(!identify(actor,true).bot||!render_private_active())return false;
 std::string current;return (account_string(actor+0x798,current)&&current==label)||(account_string(actor+0x6f0,current)&&current==label)||formatted_labels.find(label)!=formatted_labels.end();
}
inline bool bot_narrow_text(uintptr_t object){std::string label;return render_private_active()&&account_string(object,label)&&bot_text(label);}
inline bool shared_narrow_text(uintptr_t object,ssc_shared::Style& value){std::string label;return receive_colors&&account_string(object,label)&&shared_text(label,value);}
inline bool wide_label(uintptr_t object,std::string& label){
 std::array<unsigned char,32> header{};size_t length=0,capacity=0;uintptr_t data=object;
 if(!read_bytes(object,header.data(),header.size()))return false;
 std::memcpy(&length,header.data()+16,8);std::memcpy(&capacity,header.data()+24,8);
 if(!length||length>256||capacity<length)return false;
 std::array<wchar_t,256> text{};
 if(capacity>7){std::memcpy(&data,header.data(),8);if(!read_bytes(data,text.data(),length*sizeof(wchar_t)))return false;}
 else {if(length>7)return false;std::memcpy(text.data(),header.data(),length*sizeof(wchar_t));}
 std::array<char,1024> bytes{};int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(length),bytes.data(),int(bytes.size()),nullptr,nullptr);if(n<=0)return false;
 label.assign(bytes.data(),size_t(n));for(auto& c:label)if(c>='A'&&c<='Z')c+=32;
 return true;
}
inline bool shared_wide_text(uintptr_t object,ssc_shared::Style& value){std::string label;return receive_colors&&wide_label(object,label)&&shared_text(label,value);}
inline bool bot_wide_text(uintptr_t object){std::string label;return render_private_active()&&wide_label(object,label)&&bot_text(label);}
inline std::vector<std::string> shared_accounts(){
 std::set<std::string> keys;uintptr_t world=0,first=0,last=0;
 if(!attached||!read(image_base+ssc_compat::resolve(0xdec5d0),world)||!world||!read(world+0xa658,first)||!read(world+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048)return {};
 for(auto actor=first;actor<last&&keys.size()<128;actor+=0x34b8){auto identity=identify(actor);if(!identity.account.empty()&&!identity.local)keys.insert(identity.account);}
 return {keys.begin(),keys.end()};
}
// Observe the original game result, never the cosmetic override returned below.
inline std::string native_pass_account;
inline bool native_pass_value=false;
inline uint64_t native_pass_observed=0;
inline void observe_native_pass(bool own,unsigned char result){
 if(!own)return;
 std::string account;if(!account_string(image_base+ssc_compat::resolve(0xe061e8),account))return;
 native_pass_account=account;native_pass_value=result!=0;native_pass_observed=GetTickCount64();
}
inline unsigned char native_pass_probe(void* context,int id,void* owned_string){
 const bool own=local_account(reinterpret_cast<uintptr_t>(owned_string));
 auto result=native_predicate(context,id,owned_string);observe_native_pass(own,result);return result;
}
inline unsigned char __cdecl predicate(void* context,int id,void* owned_string){
    const bool own=local_account(reinterpret_cast<uintptr_t>(owned_string));
    ssc_shared::Style remote;bool apply=(local_cosmetics()&&own)||shared_account(reinterpret_cast<uintptr_t>(owned_string),remote)||bot_narrow_text(reinterpret_cast<uintptr_t>(owned_string));
    unsigned char result=native_predicate(context,id,owned_string); // original consumes this copy
    observe_native_pass(own,result);
    if(apply)++cosmetic_draws;
    return apply?1:result;
}
// The native compact event feed stores formatted names, not actor pointers.
// Only use its own account-key lookup when the current session proves the key
// belongs to the local actor and no other actor is displaying that same key.
inline bool unambiguous_event_account(uintptr_t text){
    std::string own_key,candidate;
    if(!account_string(image_base+ssc_compat::resolve(0xe061e8),own_key)||!account_string(text,candidate)||own_key!=candidate)return false;
    uintptr_t world=0,first=0,last=0;int slot=-1;
    if(!read(image_base+ssc_compat::resolve(0xdec5d0),world)||!read(world+0xa658,first)||!read(world+0xa660,last)||
       !first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048||
       !read(image_base+ssc_compat::resolve(0xe20678),slot)||slot<0||size_t(slot)>=(last-first)/0x34b8)return false;
    const auto own=first+size_t(slot)*0x34b8;
    if(!identify(own).local)return false;
    for(auto p=first;p<last;p+=0x34b8){unsigned char active=0;
        if(!read(p+0x81,active))return false;
        if(p!=own&&active&&((account_string(p+0x798,candidate)&&candidate==own_key)||(account_string(p+0x6f0,candidate)&&candidate==own_key)))return false;
    }
    return true;
}
// The event predicate receives the account string; the subsequent draw gets
// a separately formatted display label. Carry attribution across that pair.
struct EventAppearance {bool own=false,bot=false,remote=false;ssc_shared::Style style;};
inline thread_local EventAppearance event_appearance;
inline unsigned char __cdecl event_predicate(void* context,int id,void* owned_string){
 event_appearance={};auto text=reinterpret_cast<uintptr_t>(owned_string);
 event_appearance.own=local_cosmetics()&&unambiguous_event_account(text);
 event_appearance.bot=!event_appearance.own&&bot_narrow_text(text);
 event_appearance.remote=!event_appearance.own&&!event_appearance.bot&&shared_account(text,event_appearance.style);
 // Do not ask native code to persist rainbow RGB into the event record.
 return native_predicate(context,id,owned_string);
}
inline float __cdecl event_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 const auto appearance=event_appearance;event_appearance={};ColorScope scope(appearance.own);
 if(appearance.remote)shared_tint(scope,appearance.style,r,g,b,phase);
 if(appearance.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
 if(appearance.own&&local_cosmetics()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline void __cdecl local_widget(uintptr_t widget,float p2,void* p3,void* p4,char p5,float p6,float p7,float p8,float p9,void* p10,float p11,void* p12,void* p13){
    struct Scope{bool old=drawing_local_widget;Scope(){drawing_local_widget=true;}~Scope(){drawing_local_widget=old;}} scope;
    native_widget(widget,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13);
}
inline float __cdecl widget_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    const bool own=drawing_local_widget&&local_account(reinterpret_cast<uintptr_t>(name));ColorScope scope(own,drawing_local_widget);
    if(own&&local_cosmetics()){++cosmetic_draws;if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline void __cdecl overhead(uintptr_t actor,float alpha,unsigned char p3,uintptr_t p4,unsigned char p5,unsigned char p6){
    struct Scope{uintptr_t previous;Scope(uintptr_t p):previous(drawing_actor){drawing_actor=p;}~Scope(){drawing_actor=previous;}} scope(actor);
    native_overhead(actor,alpha,p3,p4,p5,p6);
}
inline float __cdecl draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    auto identity=identify(drawing_actor);ColorScope scope(identity.local);
    shared_actor_tint(scope,identity,r,g,b,phase);
    if(identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
    if(local_cosmetics()&&identity.local){++cosmetic_draws;if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
extern "C" float __cdecl ssc_score_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t row){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    int slot=-1;uintptr_t world=0,first=0,last=0;
    Identity identity;
    if(read(row+0x2c,slot)&&slot>=0&&read(image_base+ssc_compat::resolve(0xdec5d0),world)&&read(world+0xa658,first)&&read(world+0xa660,last)&&last>=first&&size_t(slot)<(last-first)/0x34b8)identity=identify(first+size_t(slot)*0x34b8);
    ColorScope scope(identity.local);
    shared_actor_tint(scope,identity,r,g,b,phase);
    if(identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
    if(local_cosmetics()&&identity.local){++cosmetic_draws;if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline float __cdecl account_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    const auto text=reinterpret_cast<uintptr_t>(name);const bool own=local_account(text)||generated_label_identity(text).local;ColorScope scope(own);
    if(!own&&bot_narrow_text(reinterpret_cast<uintptr_t>(name)))bot_tint(r,g,b,alpha,phase);
    ssc_shared::Style remote;if(!own&&shared_narrow_text(reinterpret_cast<uintptr_t>(name),remote))shared_tint(scope,remote,r,g,b,phase);
    if(local_cosmetics()&&own){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// These call sites pass the canonical account string from a leaderboard row,
// not its rank, display-name approximation, or the current match roster.
inline float canonical_account_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const auto account=reinterpret_cast<uintptr_t>(name);const bool own=local_account(account);ColorScope scope(own);
 ssc_shared::Style remote;if(!own&&shared_account(account,remote))shared_tint(scope,remote,r,g,b,phase);
 if(own&&local_cosmetics()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
extern "C" void ssc_vehicle_name_bridge();
extern "C" float ssc_vehicle_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t vehicle){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 uintptr_t occupants=0,actor=0;
 if(read(vehicle+0x368,occupants)&&occupants)read(occupants+0x60,actor);
 const auto identity=identify(actor,true);ColorScope scope(identity.local);
 shared_actor_tint(scope,identity,r,g,b,phase);
 if(identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
 if(identity.local&&local_cosmetics()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Chat labels are formatted (for example "Name:"). R13 still points to
// the original sender record whose leading string is used by the native
// account predicate. Never authorize a name tint by stripping display text.
extern "C" void ssc_chat_name_bridge();
extern "C" void ssc_actor_name_bridge();
extern "C" float ssc_chat_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t sender){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const bool own=local_account(sender)||generated_label_identity(sender).local;ColorScope scope(own);
 ssc_shared::Style remote;if(!own&&shared_account(sender,remote))shared_tint(scope,remote,r,g,b,phase);
 if(own&&local_cosmetics()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Actor-attached name labels have a separate render path from overhead names.
extern "C" float ssc_actor_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t actor){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const auto identity=identify(actor);const bool own=identity.local;ColorScope scope(own);
 shared_actor_tint(scope,identity,r,g,b,phase);
 if(identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
 if(own&&local_cosmetics()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Showdown retains eliminated actors in its list; validate the registry and
// account even when the actor is no longer active. Native formatting stays intact.
extern "C" float ssc_showdown_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t actor){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const auto identity=identify(actor,true);const bool own=identity.local;ColorScope scope(own);
 shared_actor_tint(scope,identity,r,g,b,phase);
 if(identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
 if(own&&local_cosmetics()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Main-menu account widget uses its stored wrapped label (+0xa8), not
// the optional single-line argument. Keep the native layout/ownership intact.
using Wrapped=int(*)(uintptr_t,float,float,float,float,void*,float,float,float,float,float,int,int,int,unsigned char,int,float,float,float);
inline Wrapped native_wrapped=nullptr;
inline int widget_wrapped(uintptr_t renderer,float x,float y,float width,float height,void* name,float size,float r,float g,float b,float alpha,int align,int vertical,int first,unsigned char flags,int limit,float start,float end,float phase){
 if(!appearance_enabled())return native_wrapped(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
 const bool own=drawing_local_widget&&local_account(reinterpret_cast<uintptr_t>(name));ColorScope scope(own,drawing_local_widget);
 if(own&&local_cosmetics()){++cosmetic_draws;if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);}
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
 std::string account;if(!account_string(image_base+ssc_compat::resolve(0xe061e8),account))return false;
 std::array<wchar_t,256> expected{},candidate{};
 const int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,account.data(),int(account.size()),expected.data(),int(expected.size()));
 if(count<=0||size_t(count)!=length)return false;
 if(capacity>7){std::memcpy(&data,header.data(),8);if(!read_bytes(data,candidate.data(),length*sizeof(wchar_t)))return false;}
 else{if(length>7)return false;std::memcpy(candidate.data(),header.data(),length*sizeof(wchar_t));}
 for(size_t i=0;i<length;++i){auto c=candidate[i];if(c>=L'A'&&c<=L'Z')c+=L'a'-L'A';if(c!=expected[i])return false;}
 return true;
}
inline bool shared_label_local(void* text){
 if(!local_cosmetics())return false;
 if(identity_scope>=0)return identity_scope==1;
 std::string label;if(wide_label(reinterpret_cast<uintptr_t>(text),label)){auto actor=label_actor(label);if(actor)return identify(actor,true).local;}
 return local_wide_label(reinterpret_cast<uintptr_t>(text));
}
inline Draw native_wide_draw=nullptr;
inline float wide_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_wide_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const bool bot=identity_scope<0&&bot_wide_text(reinterpret_cast<uintptr_t>(name));
 const bool own=shared_label_local(name);ssc_shared::Style remote;const bool remote_label=identity_scope<0&&!own&&shared_wide_text(reinterpret_cast<uintptr_t>(name),remote);ColorScope scope(own);
 scope.inherit_remote();
 if(remote_label)shared_tint(scope,remote,r,g,b,phase);
 if(bot)bot_tint(r,g,b,alpha,phase);
 if(own){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 // The native callee consumes this MSVC string. Inspect before calling and
 // forward exactly once; do not free it or substitute a MinGW string object.
 return native_wide_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline Wrapped native_wide_wrapped=nullptr;
inline int wide_wrapped(uintptr_t renderer,float x,float y,float width,float height,void* name,float size,float r,float g,float b,float alpha,int align,int vertical,int first,unsigned char flags,int limit,float start,float end,float phase){
 if(!appearance_enabled())return native_wide_wrapped(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
 const bool bot=identity_scope<0&&bot_wide_text(reinterpret_cast<uintptr_t>(name));
 const bool own=shared_label_local(name);ssc_shared::Style remote;const bool remote_label=identity_scope<0&&!own&&shared_wide_text(reinterpret_cast<uintptr_t>(name),remote);ColorScope scope(own);
 scope.inherit_remote();
 if(remote_label)shared_tint(scope,remote,r,g,b,phase);
 if(bot)bot_tint(r,g,b,alpha,phase);
 if(own){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_wide_wrapped(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
}
// Updated modifier panel no longer retains the actor in RBX. Its native
// formatter supplies the identity; the existing label cache verifies world,
// account and display identity again before applying any cosmetic color.
inline float modifier_label_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const auto identity=generated_label_identity(reinterpret_cast<uintptr_t>(name));ColorScope scope(identity.local);
 shared_actor_tint(scope,identity,r,g,b,phase);
 if(identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
 if(identity.local&&local_cosmetics()){if(rainbow||gradient){if(!native_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline bool modifier_layout_supported(){
 std::array<unsigned char,18105> code{};std::array<unsigned char,32> actual{},expected{};
 if(!read_bytes(image_base+0x3be5c0,code.data(),code.size()))return false;
 const char* hex="c7c71a3904cf391ab22fed1518dca145e92d041b8079acd70d3b7e27d44d9f28";
 if(sodium_hex2bin(expected.data(),expected.size(),hex,64,nullptr,nullptr,nullptr))return false;
 crypto_hash_sha256(actual.data(),code.data(),code.size());return sodium_memcmp(actual.data(),expected.data(),32)==0;
}
extern "C" void ssc_modifier_name_bridge();
extern "C" void ssc_showdown_name_bridge();
// New row/vehicle sites are exact-build presentation hooks. Validate the
// reviewed native bodies before installing any of them; never bless a mismatch.
inline bool account_surfaces_supported(){
 struct Guard{uint32_t rva,size;const char* hash;};
 const Guard guards[]={
  // Native auto-message/quick-react sender formatter and its reviewed caller.
  {0x7dd820,544,"6ef5dd783540bce3ef1bcf9c45b4db2feba15a39c8e29b85677aeed29110e404"},
  {0x2234f0,819,"ac3e526335d33f76e4095ca27162777ee03b2d8b412b7eccf446a53f7f420049"},
  {0x3ac4e0,0x3c4,"1e11eab59a6ebe4d79ccd6b2e04029da0441ae060bdc6c3803539e4329137ff2"},
  {0x6085c0,0x17ac,"930aa4314482668c490da882dc85786f73587306da15baea08e3d8f698b8d991"},
  {0x60a3a0,0x20fb,"45cbaf00d0c148904643c1a7f1e4084d825867fed4e07d0d3238f67ac7dfbf6c"},
  {0x628380,0x3a3a,"a227e1707fe22873ef63c8d9e2395bcdd02ac94f6ee43297a4f547bead5e99f8"},
  {0x41a790,0x1243,"b216b97dcfe69a7ff64a1480e2271ec8e575036e59c1855d43dcd5fe94416596"},
  {0x6231c0,0x51c0,"f19a4fc1d909ad48a7688616de841cef7a6dc5eea1edba05c8df7c760a99923e"},
  {0x63bb00,0x9468,"517bdd455433fce7f9e049b03835d490725936ccc930069c951661e429f953d9"},
  {0x9e5d56,0x72f,"e849096d7a2434dd61cffe889bc7eb427f8868cc38cabc755fc6c12eb20d82d6"},
 };
 for(const auto& guard:guards){
  std::vector<unsigned char> bytes(guard.size);std::array<unsigned char,32> actual{},expected{};
  if(!read_bytes(image_base+guard.rva,bytes.data(),bytes.size())||sodium_hex2bin(expected.data(),expected.size(),guard.hash,64,nullptr,nullptr,nullptr))return false;
  crypto_hash_sha256(actual.data(),bytes.data(),bytes.size());if(sodium_memcmp(actual.data(),expected.data(),32))return false;
 }
 return true;
}
inline bool attach(){
    if(!ssc_compat::supports(1))return false;
    image_base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    if(!modifier_layout_supported()||!account_surfaces_supported())return false;
    {std::array<unsigned char,12519> code{};std::array<unsigned char,32> actual{},expected{};
     const char* hex="27d0669c5ca0fc25bbce83d69ca1d1fd4caf053661b1513bd33a630844a07cea";
     if(!read_bytes(image_base+0x3bb4d0,code.data(),code.size())||sodium_hex2bin(expected.data(),expected.size(),hex,64,nullptr,nullptr,nullptr))return false;
     crypto_hash_sha256(actual.data(),code.data(),code.size());if(sodium_memcmp(actual.data(),expected.data(),32))return false;}
    native_predicate=reinterpret_cast<Predicate>(image_base+ssc_compat::resolve(0x3ac4e0));native_overhead=reinterpret_cast<Overhead>(image_base+ssc_compat::resolve(0x84b480));native_draw=reinterpret_cast<Draw>(image_base+ssc_compat::resolve(0x4812b0));
    native_wrapped=reinterpret_cast<Wrapped>(image_base+ssc_compat::resolve(0x481440));
    native_wide_draw=reinterpret_cast<Draw>(image_base+ssc_compat::resolve(0x481ef0));
    native_wide_wrapped=reinterpret_cast<Wrapped>(image_base+ssc_compat::resolve(0x4815d0));
    native_palette=reinterpret_cast<Palette>(image_base+ssc_compat::resolve(0x1557d0));
    native_name_formatter=reinterpret_cast<NameFormatter>(image_base+0x7dda40);
    native_sender_formatter=reinterpret_cast<NameFormatter>(image_base+0x7dd820);
    native_widget=reinterpret_cast<Widget>(image_base+ssc_compat::resolve(0x99e920));
    struct Site{uint32_t call,target;uintptr_t hook;bool exact=false;};
    // Whitelist presentation calls only. Do not hook the predicate globally:
    // 5fb3b0/5fd316/6834a3/6b6db7 also service pass-related UI/cache logic.
    Site sites[]={
        {0x22353b,0x7dd820,reinterpret_cast<uintptr_t>(sender_name_formatter),true},
        {0x60921f,0x3ac4e0,reinterpret_cast<uintptr_t>(native_pass_probe),true},
        {0x60b186,0x3ac4e0,reinterpret_cast<uintptr_t>(native_pass_probe),true},
        // Native formatter constructs the cached participant label from RBX actor.
        {0x3bba1e,0x7dda40,reinterpret_cast<uintptr_t>(list_name_formatter),true},
        // Showdown participant name, including native eliminated/score suffixes; RDI is its actor.
        {0x3bc3a6,0x4812b0,reinterpret_cast<uintptr_t>(ssc_showdown_name_bridge),true},
        // Modifier owner identity comes from these exact native formatter calls.
        {0x3c22e3,0x7dda40,reinterpret_cast<uintptr_t>(list_name_formatter),true},
        {0x3c23c9,0x7dda40,reinterpret_cast<uintptr_t>(list_name_formatter),true},
        {0x3c2519,0x7dda40,reinterpret_cast<uintptr_t>(list_name_formatter),true},
        {0x3c2394,0x4812b0,reinterpret_cast<uintptr_t>(modifier_label_draw),true},
        {0x3c247d,0x4812b0,reinterpret_cast<uintptr_t>(modifier_label_draw),true},
        {0x3c25cb,0x4812b0,reinterpret_cast<uintptr_t>(modifier_label_draw),true},
        // Shared narrow-to-wide conversion and direct UTF-16 text paths.
        {0x4303c2,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x430a2c,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x430d53,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x43bbfd,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x4813d6,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x48156b,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x485160,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x52f9ef,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x531050,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x53154a,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x534212,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x535eb8,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x536355,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x536b1c,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x536f71,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x537248,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5374e8,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x537788,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x537c88,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x53888d,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x538d71,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x543ff4,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5442aa,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x545543,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x545ac6,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5460a6,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x547420,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x54782e,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x547cfc,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x635ce4,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x651f6c,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6520d9,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x652230,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x652379,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x652426,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6525bf,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65276b,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6529af,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65a556,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65a6f2,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65a885,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65ac7d,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65ae2e,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65afd6,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65b174,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6608c8,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x661c1e,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x664d08,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x664dba,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x667ff8,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6c0d08,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c0ec0,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c1495,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c1578,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c1a1d,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c1d4b,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c206d,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c238f,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c4a29,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c4d44,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c505f,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c53d2,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c578f,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6d768a,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x7199be,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x9a617a,0x481ef0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x9a6250,0x4815d0,reinterpret_cast<uintptr_t>(wide_wrapped)},
        // Both glyph palette paths (regular and alternate text rendering).
        {0x48ab11,0x1557d0,reinterpret_cast<uintptr_t>(palette)},
        {0x48ab72,0x1557d0,reinterpret_cast<uintptr_t>(palette)},
        {0x3f72dc,0x4812b0,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f7395,0x4812b0,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f7428,0x4812b0,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f74fc,0x4812b0,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f7598,0x4812b0,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f7653,0x4812b0,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},

        {0x9a0c7f,0x481440,reinterpret_cast<uintptr_t>(widget_wrapped)},
        {0x48a459,0x1557d0,reinterpret_cast<uintptr_t>(palette)},
        {0x48a4b6,0x1557d0,reinterpret_cast<uintptr_t>(palette)},
        {0x41b665,0x4812b0,reinterpret_cast<uintptr_t>(account_draw)},
        {0x41b6ea,0x4812b0,reinterpret_cast<uintptr_t>(ssc_modifier_name_bridge),true},
        {0x62804a,0x4812b0,reinterpret_cast<uintptr_t>(canonical_account_draw),true},
        {0x62a788,0x4812b0,reinterpret_cast<uintptr_t>(canonical_account_draw),true},
        {0x641c6b,0x4812b0,reinterpret_cast<uintptr_t>(canonical_account_draw),true},
        {0x6adf0c,0x4812b0,reinterpret_cast<uintptr_t>(account_draw)},
        {0x6ae194,0x4812b0,reinterpret_cast<uintptr_t>(ssc_chat_name_bridge)},
        {0x6ae3b2,0x4812b0,reinterpret_cast<uintptr_t>(account_draw)},
        {0x9e6143,0x4812b0,reinterpret_cast<uintptr_t>(ssc_vehicle_name_bridge),true},
        {0x9b167b,0x4812b0,reinterpret_cast<uintptr_t>(event_draw)},
        {0x9b181f,0x4812b0,reinterpret_cast<uintptr_t>(event_draw)},

        {0x33e27c,0x84b480,reinterpret_cast<uintptr_t>(overhead)},
        {0x84b936,0x3ac4e0,reinterpret_cast<uintptr_t>(predicate)},
        {0x41b47c,0x3ac4e0,reinterpret_cast<uintptr_t>(predicate)},
        {0x43b588,0x3ac4e0,reinterpret_cast<uintptr_t>(predicate)},
        {0x62551c,0x3ac4e0,reinterpret_cast<uintptr_t>(predicate)},
        {0x63ee2e,0x3ac4e0,reinterpret_cast<uintptr_t>(predicate)},
        {0x6ac423,0x3ac4e0,reinterpret_cast<uintptr_t>(predicate)},
        {0x9e60a5,0x3ac4e0,reinterpret_cast<uintptr_t>(predicate)},
        {0x9b15ad,0x3ac4e0,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x9b1757,0x3ac4e0,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x60b4b1,0x99e920,reinterpret_cast<uintptr_t>(local_widget)},
        {0x9a0db9,0x4812b0,reinterpret_cast<uintptr_t>(widget_draw)},
        {0x84c1b8,0x4812b0,reinterpret_cast<uintptr_t>(draw)},
        {0x84c244,0x4812b0,reinterpret_cast<uintptr_t>(draw)},
        {0x43802d,0x4812b0,reinterpret_cast<uintptr_t>(ssc_score_bridge)},
        {0x4380b2,0x4812b0,reinterpret_cast<uintptr_t>(ssc_score_bridge)}};
    for(auto& site:sites){if(site.exact)continue;site.call=ssc_compat::resolve(uint32_t(site.call));site.target=ssc_compat::resolve(uint32_t(site.target));}
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

