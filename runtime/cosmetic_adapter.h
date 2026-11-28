#pragma once
#include "privacy.h"
#include "cosmetic_animation.h"
#include "compatibility.h"
#include "roster_sweep.h"
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
inline bool rainbow=true;inline unsigned solid_rgb=0x55e7ef;
inline bool gradient=false;inline int gradient_count=3;
inline ssc_cosmetic_animation::Settings animation;
inline float local_brightness(){return ssc_cosmetic_animation::brightness(double(GetTickCount64())/1000.,animation);}
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
  color_scope=local&&local_cosmetics()?(gradient?2:(!rainbow?1:(animation.breathing?5:0))):0;
 }
 void remote(const ssc_shared::Style& value){shared_style=value;identity_scope=2;color_scope=value.mode==ssc_shared::Mode::solid?3:value.mode==ssc_shared::Mode::gradient?4:0;}
 void local_override(unsigned rgb){shared_style.colors[0]=rgb;color_scope=6;identity_scope=2;}
 void inherit_remote(){if(previous_identity==2){identity_scope=2;color_scope=previous;shared_style=previous_style;}}
 ~ColorScope(){color_scope=previous;identity_scope=previous_identity;shared_style=previous_style;}
};
using Palette=float*(*)(uintptr_t,float*,float,float,unsigned char,unsigned char);
inline Palette native_palette=nullptr;
inline float* palette(uintptr_t context,float* output,float position,float lift,unsigned char gray,unsigned char balance){
 if(!color_scope||((color_scope==3||color_scope==4)&&ssc_privacy::blocked()))return native_palette(context,output,position,lift,gray,balance);
 if(color_scope==5){auto result=native_palette(context,output,position,lift,gray,balance);const float factor=local_brightness();for(int k=0;k<3;++k)output[k]*=factor;return result;}
 auto color=gradient_rgb(position/255.f);
 if(color_scope==6){for(int k=0;k<3;++k)output[k]=float((shared_style.colors[0]>>(16-8*k))&255)/255;return output;}
 if(color_scope==1)for(int k=0;k<3;++k)color[k]=float((solid_rgb>>(16-8*k))&255)/255;
 if(color_scope==3||color_scope==4){
  float t=position/255.f;t=(t-std::floor(t))*shared_style.count;unsigned a=std::min(unsigned(t),shared_style.count-1),b=(a+1)%shared_style.count;t-=a;
  for(int k=0;k<3;++k){unsigned shift=16-8*k;float first=float((shared_style.colors[a]>>shift)&255)/255,second=float((shared_style.colors[b]>>shift)&255)/255;color[k]=first+(second-first)*t;}
 }
 if(color_scope==1||color_scope==2){const float factor=local_brightness();for(auto& component:color)component*=factor;}
 for(int k=0;k<3;++k){output[k]=color[k];}return output;
}

inline void solid(float& r,float& g,float& b,float& phase){if(!rainbow&&!gradient){r=float((solid_rgb>>16)&255)/255;g=float((solid_rgb>>8)&255)/255;b=float(solid_rgb&255)/255;const float factor=local_brightness();r*=factor;g*=factor;b*=factor;phase=0;}}
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
 const char* hex="074b08a52ea598810d1bafe6af8591cceb7b994dcbcb3b2712f1ae7f5ff0d723";
 if(!ssc_compat::supports(1)||!read_bytes(image_base+0x27a8c0,code.data(),code.size()))return;
 if(sodium_hex2bin(expected.data(),expected.size(),hex,64,nullptr,nullptr,nullptr)!=0)return;
 crypto_hash_sha256(hash.data(),code.data(),code.size());bot_layout=sodium_memcmp(hash.data(),expected.data(),32)==0;
}
inline bool private_visible(){return attached&&bot_layout&&ssc_auth::available();}
inline bool private_active(){return attached&&bot_layout&&ssc_auth::active();}
inline bool receive_colors=false;
inline bool render_access=false;
inline uint64_t render_access_at=0;
inline bool render_private_active(){auto now=GetTickCount64();return attached&&bot_layout&&render_access_at&&now>=render_access_at&&now-render_access_at<100&&render_access;}

inline bool (*local_effect_active)()=nullptr;
inline bool (*local_actor_tint)(uintptr_t,unsigned&)=nullptr;
inline void (*score_observer)(uintptr_t,float,float,float,float)=nullptr;
inline bool appearance_enabled(){return local_cosmetics()||receive_colors||render_private_active()||(local_effect_active&&local_effect_active());}

struct Identity {bool local=false,bot=false;std::string account;uintptr_t actor=0,style_actor=0;};
struct CloneSpan {uint32_t rva,size;uint64_t hash;};
// Reviewed replicated owner-slot decoder, native owner name/level lookup, and
// reset behavior. This is an extra feature guard, not a regenerated baseline.
inline constexpr CloneSpan clone_spans[]={{0x8d5b2e,61,0x73c57c86f214c541ULL},{0x84e88a,27,0x20b31c855b2eed83ULL},{0x84f434,27,0xbabed9f94dc5f7ceULL},{0x787d05,17,0x11ec0b5da91a1244ULL}};
inline bool clone_layout=false;
inline bool clone_layout_supported(uintptr_t base){
 for(const auto& span:clone_spans){std::array<unsigned char,64> bytes{};if(span.size>bytes.size()||!read_bytes(base+span.rva,bytes.data(),span.size))return false;uint64_t hash=14695981039346656037ULL;for(uint32_t i=0;i<span.size;++i)hash=(hash^bytes[i])*1099511628211ULL;if(hash!=span.hash)return false;}return true;
}
// Native MSVC account string, not actor+0x798 (the editable display name).
inline bool account_string(uintptr_t object,std::string& value){
    std::array<unsigned char,32> header{};size_t length=0,capacity=0;uintptr_t data=object;
    if(!read_bytes(object,header.data(),header.size()))return false;
    std::memcpy(&length,header.data()+16,8);std::memcpy(&capacity,header.data()+24,8);
    if(!length||length>256||capacity<length)return false;
    std::array<unsigned char,256> characters;
    if(capacity>15){std::memcpy(&data,header.data(),8);if(!read_bytes(data,characters.data(),length))return false;}
    else {if(length>15)return false;std::memcpy(characters.data(),header.data(),length);}
    value.clear();value.reserve(length);
    for(size_t i=0;i<length;++i){auto c=characters[i];if(c<32)return false;value.push_back(c>='A'&&c<='Z'?char(c+32):char(c));}
    return true;
}
// Snapshot only the checked local account string for the next render frame.
inline thread_local std::string render_local_account;
inline thread_local uintptr_t render_account_image=0;
inline thread_local uint64_t render_account_at=0;
inline thread_local bool render_account_read=false;
inline bool local_account_key(const std::string& candidate){
    if(candidate.empty())return false;
    if(render_account_image&&render_account_image==image_base&&render_account_at&&GetTickCount64()-render_account_at<100){
        if(!render_account_read)render_account_read=account_string(image_base+ssc_compat::resolve(0xe091e8),render_local_account);
        return render_account_read&&render_local_account==candidate;
    }
    std::string own;
    return account_string(image_base+ssc_compat::resolve(0xe091e8),own)&&own==candidate;
}
inline bool local_account(uintptr_t account){
    std::string candidate;
    return account_string(account,candidate)&&local_account_key(candidate);
}
inline bool native_phase(float& phase){
    float speed=0,time=0;
    if(!read(image_base+ssc_compat::resolve(0xfb6314),speed)||!read(image_base+ssc_compat::resolve(0xfb6d70),time)||!std::isfinite(time)||!std::isfinite(speed))return false;
    float candidate=std::fmod(time*speed,1.f);if(!std::isfinite(candidate))return false;if(candidate<0)candidate+=1.f;phase=candidate;return true;
}
inline bool local_phase(float& phase){
 float speed=0,time=0;
 if(!read(image_base+ssc_compat::resolve(0xfb6314),speed)||!read(image_base+ssc_compat::resolve(0xfb6d70),time)||!std::isfinite(time)||!std::isfinite(speed))return false;
 phase=ssc_cosmetic_animation::phase(double(time)*speed,animation);return true;
}
inline ActorRange actor_range(){
 ActorRange range;
 if(!read(image_base+ssc_compat::resolve(0xdef5d0),range.world)||!range.world||
    !read(range.world+0xa658,range.first)||!read(range.world+0xa660,range.last)||!range)return {};
 return range;
}
inline Identity identify_direct(uintptr_t actor,const ActorRange& range,bool allow_inactive){
    Identity answer;const auto first=range.first,world=range.world;uintptr_t registry=0;int slot=-1,kind=-1,local_slot=-1;unsigned char active=0;
    if(!range.contains(actor)||!read(actor+0x78,slot)||slot<0||uintptr_t(slot)!=(actor-first)/0x34b8||!read(actor+0x7dc,kind)||!read(actor+0x81,active)||(!active&&!allow_inactive))return answer;
    answer.actor=actor;
    if(kind==1&&render_private_active()){
        int start=0,offset=0,count=0;
        if(read(image_base+0xfb9a84,start)&&read(world+0x190,offset)&&read(world+0x28c,count))answer.bot=bot_range(slot,kind,start,offset,count,range.size());
    }
    // e03370 is used by the native own-player formatter; bf48 may be spectated.
    std::string account;
    bool account_read=false;
    if(kind==0&&read(image_base+ssc_compat::resolve(0xe23678),local_slot)&&slot==local_slot&&
        read(image_base+ssc_compat::resolve(0xe27138),registry)&&registry==first){
        account_read=account_string(actor+0x6f0,account);
        answer.local=account_read&&local_account_key(account);
    }
    // A hidden remote identity must not be recovered for shared name styling.
    unsigned char anonymous=2;
    if(kind==0&&(answer.local||(read(actor+0x209,anonymous)&&anonymous==0))&&
       (account_read||account_string(actor+0x6f0,account))&&ssc_shared::account_valid(account))answer.account=std::move(account);
    return answer;
}
inline Identity identify_direct(uintptr_t actor,bool allow_inactive=false){
 return identify_direct(actor,actor_range(),allow_inactive);
}
inline Identity identify(uintptr_t actor,const ActorRange& range,bool allow_inactive){
 auto answer=identify_direct(actor,range,allow_inactive);if(!answer.actor||!clone_layout)return answer;
 int kind=-1;uintptr_t owner=0;
 if(!read(actor+0x7dc,kind)||kind!=1||!read(actor+0x2e8,owner)||!owner)return answer;
 // Native ownership is one actor pointer into the current actor vector. Never
 // infer it from names and never follow recursive/stale/foreign owner chains.
 answer.local=false;answer.bot=false;answer.account.clear();
 auto parent=identify_direct(owner,range,false);uintptr_t grandparent=0;int owner_kind=-1;unsigned char anonymous=2;
 if(owner==actor||!parent.actor||!read(owner+0x7dc,owner_kind)||(owner_kind!=0&&owner_kind!=1)||!read(owner+0x2e8,grandparent)||grandparent)return answer;
 if(owner_kind==0&&!parent.local&&(!read(owner+0x209,anonymous)||anonymous!=0))return answer;
 answer.local=parent.local;answer.bot=parent.bot;answer.account=std::move(parent.account);answer.style_actor=owner;return answer;
}
inline Identity identify(uintptr_t actor,bool allow_inactive=false){
 return identify(actor,actor_range(),allow_inactive);
}
// The service worker publishes immutable, expiring style data. Rendering never
// waits for its mutex, and only refreshes observations once per account per second.
inline bool (*lookup_shared)(const std::string&,ssc_shared::Style&)=nullptr;
inline std::shared_ptr<const ssc_shared::Cache::View> shared_frame_view;
inline std::map<std::string,uint64_t> frame_styles;
inline uint64_t shared_frame_at=0,shared_view_at=0,shared_frame_privacy=0;
inline void begin_render_frame(bool receive,bool active){
 const auto tick=GetTickCount64(),privacy=ssc_privacy::generation.load();
 if(!receive||receive!=receive_colors||privacy!=shared_frame_privacy){
  shared_frame_view.reset();
  frame_styles.clear();
  shared_view_at=0;
 }
 shared_frame_privacy=privacy;
 receive_colors=receive;
 render_account_image=attached&&(receive||local_cosmetics())?image_base:0;
 render_account_at=tick;
 render_account_read=false;
 shared_frame_at=tick;
 if(receive&&!lookup_shared){
  if(ssc_shared::service().render_view(shared_frame_view))shared_view_at=tick;
  else if(tick-shared_view_at>=100)shared_frame_view.reset();
 }
 render_access=active;
 render_access_at=tick;
}
inline void begin_render_frame(bool receive){begin_render_frame(receive,private_active());}
inline bool frame_shared(const std::string& key,ssc_shared::Style& style){
 if(ssc_privacy::blocked()||!receive_colors)return false;
 ssc_perf::Scope timing(ssc_perf::sharing);
 if(lookup_shared)return lookup_shared(key,style);
 auto seen=frame_styles.find(key);
 if(seen==frame_styles.end()&&frame_styles.size()>=128){
  for(auto it=frame_styles.begin();it!=frame_styles.end();){
   if(shared_frame_at-it->second>=30000)it=frame_styles.erase(it);else ++it;
  }
 }
 if((seen==frame_styles.end()?frame_styles.size()<128:shared_frame_at-seen->second>=1000)&&
    ssc_shared::service().observe_render(key,shared_frame_at))frame_styles[key]=shared_frame_at;
 return shared_frame_view&&shared_frame_view->get(key,shared_frame_at,style);
}
inline bool shared_account(const std::string& key,ssc_shared::Style& style){
 return receive_colors&&attached&&ssc_shared::account_valid(key)&&!local_account_key(key)&&frame_shared(key,style);
}
inline bool shared_account(uintptr_t string,ssc_shared::Style& style){
 std::string key;
 return receive_colors&&attached&&account_string(string,key)&&shared_account(key,style);
}
inline void shared_tint(ColorScope& scope,const ssc_shared::Style& value,float& r,float& g,float& b,float& phase){
 scope.remote(value);if(value.mode==ssc_shared::Mode::solid){auto c=value.colors[0];r=float((c>>16)&255)/255;g=float((c>>8)&255)/255;b=float(c&255)/255;phase=0;}
 else if(!native_phase(phase))phase=0;
}
inline void shared_actor_tint(ColorScope& scope,const Identity& identity,float& r,float& g,float& b,float& phase){
 unsigned rgb=0;if(local_actor_tint&&identity.actor&&local_actor_tint(identity.style_actor?identity.style_actor:identity.actor,rgb)){scope.local_override(rgb);r=float((rgb>>16)&255)/255;g=float((rgb>>8)&255)/255;b=float(rgb&255)/255;phase=0;return;}
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
inline bool formatted_identity_current(const FormattedLabel& entry){
 uintptr_t world=0,first=0,last=0;std::string account,display;
 if(!entry.actor||!read(image_base+ssc_compat::resolve(0xdef5d0),world)||world!=entry.world||
    !read(world+0xa658,first)||!read(world+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048||
    entry.actor<first||entry.actor>=last||(entry.actor-first)%0x34b8)return false;
 account_string(entry.actor+0x6f0,account);
 return account==entry.account&&account_string(entry.actor+0x798,display)&&display==entry.display;
}
inline void remember_formatted_label(const std::string& label,const FormattedLabel& identity){
 auto add=[&](const std::string& key){
  auto found=formatted_labels.find(key);
  if(found==formatted_labels.end()){if(formatted_labels.size()<2048)formatted_labels.emplace(key,identity);return;}
  auto& previous=found->second;if(!previous.actor)return; // A real live-label collision stays ambiguous.
  if(previous.actor==identity.actor||!formatted_identity_current(previous))previous=identity;
  else previous.actor=0;
 };
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
 if(read(image_base+ssc_compat::resolve(0xdef5d0),world)&&world&&account_string(result,label)&&account_string(actor+0x798,display)){
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
 auto& entry=found->second;
 if(!formatted_identity_current(entry)){
  // The actor vector can move while the world and cached chat rows survive.
  // Rebind only a native-attributed label to its exact, unique account/display;
  // never infer ownership from a prefix or a substring of arbitrary chat text.
  if(entry.account.empty())return 0;
  auto current=shared_label_index.find(entry.account);if(current==shared_label_index.end()||!current->second)return 0;
  auto rebound=entry;rebound.actor=current->second;if(!formatted_identity_current(rebound))return 0;
  entry=std::move(rebound);
 }
 return entry.actor;
}
inline Identity generated_label_identity(const std::string& label){
 if(!attached||label.empty())return {};
 auto actor=label_actor(label);
 return actor?identify(actor,true):Identity{};
}
inline Identity generated_label_identity(uintptr_t text){
 std::string label;
 return attached&&account_string(text,label)?generated_label_identity(label):Identity{};
}
inline RosterSweep label_sweep;
inline std::map<std::string,uintptr_t> pending_labels;
inline bool labels_readable=true;
inline void refresh_shared_labels(uint64_t now,bool receive){
 if(!attached||(!receive&&!local_cosmetics())){
  shared_label_index.clear();formatted_labels.clear();shared_label_world=0;shared_label_refresh=0;
  label_sweep={};pending_labels.clear();return;
 }
 const auto range=actor_range();
 if(range.world!=shared_label_world)formatted_labels.clear();
 if(!shared_label_refresh&&label_sweep.started){label_sweep={};pending_labels.clear();}
 if(label_sweep.synchronize(range)){pending_labels.clear();shared_label_index.clear();}
 shared_label_world=range.world;
 const bool beginning=!label_sweep.next;
 auto add=[](const std::string& label,uintptr_t actor){
  if(label.empty())return;
  auto result=pending_labels.emplace(label,actor);
  if(!result.second&&result.first->second!=actor)result.first->second=0;
 };
 const bool ready=label_sweep.step(now,250,.28,[&](uintptr_t actor){
  if(beginning&&actor==range.first){labels_readable=true;pending_labels.clear();}
  if(!labels_readable)return;
  int kind=-1;
  if(!read(actor+0x7dc,kind)){labels_readable=false;shared_label_index.clear();return;}
  if(kind!=0&&kind!=1)return;
  std::string key,display;
  if(account_string(actor+0x6f0,key))add(key,actor);
  if(account_string(actor+0x798,display))add(display,actor);
 });
 if(!shared_label_refresh&&label_sweep.started)shared_label_refresh=now?now:1;
 if(ready){
  ++shared_label_scans;shared_label_refresh=now?now:1;
  if(labels_readable)shared_label_index.swap(pending_labels);
  else shared_label_index.clear();
  pending_labels.clear();
 }
}
inline bool shared_text(const std::string& label,ssc_shared::Style& value){
 // Generic HUD text must only perform an index lookup, never a roster walk.
 if(!attached||label.empty())return false;
 auto actor=label_actor(label);if(!actor)return false;
 uintptr_t world=0;if(!read(image_base+ssc_compat::resolve(0xdef5d0),world)||world!=shared_label_world)return false;
 auto identity=identify(actor,true);if(identity.local||identity.bot||identity.account.empty())return false;
 // A reused actor slot or renamed player must not inherit a stale label.
 std::string display;if(identity.account!=label&&(!account_string(actor+0x798,display)||display!=label)&&formatted_labels.find(label)==formatted_labels.end())return false;
 return frame_shared(identity.account,value);
}
inline bool bot_text(const std::string& label){
 if(!attached)return false;
 auto actor=label_actor(label);if(!actor)return false;
 uintptr_t world=0;if(!read(image_base+ssc_compat::resolve(0xdef5d0),world)||world!=shared_label_world)return false;
 if(!identify(actor,true).bot||!render_private_active())return false;
 std::string current;return (account_string(actor+0x798,current)&&current==label)||(account_string(actor+0x6f0,current)&&current==label)||formatted_labels.find(label)!=formatted_labels.end();
}
inline bool bot_narrow_text(uintptr_t object){std::string label;return render_private_active()&&account_string(object,label)&&bot_text(label);}
inline bool wide_label(uintptr_t object,std::string& label){
 std::array<unsigned char,32> header{};size_t length=0,capacity=0;uintptr_t data=object;
 if(!read_bytes(object,header.data(),header.size()))return false;
 std::memcpy(&length,header.data()+16,8);std::memcpy(&capacity,header.data()+24,8);
 if(!length||length>256||capacity<length)return false;
 std::array<wchar_t,256> text;
 if(capacity>7){std::memcpy(&data,header.data(),8);if(!read_bytes(data,text.data(),length*sizeof(wchar_t)))return false;}
 else {if(length>7)return false;std::memcpy(text.data(),header.data(),length*sizeof(wchar_t));}
 std::array<char,1024> bytes;int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(length),bytes.data(),int(bytes.size()),nullptr,nullptr);if(n<=0)return false;
 label.assign(bytes.data(),size_t(n));for(auto& c:label)if(c>='A'&&c<='Z')c+=32;
 return true;
}
inline RosterSweep account_sweep;
inline std::map<std::string,uintptr_t> pending_accounts,observed_accounts;
inline uint64_t account_privacy_revision=0;
inline void refresh_shared_accounts(uint64_t now,bool receive,uint64_t privacy_revision){
 if(!attached||!receive||privacy_revision!=account_privacy_revision){
  account_sweep={};pending_accounts.clear();observed_accounts.clear();
  account_privacy_revision=privacy_revision;
  if(!attached||!receive)return;
 }
 const auto range=actor_range();
 if(account_sweep.synchronize(range)){pending_accounts.clear();observed_accounts.clear();}
 const bool ready=account_sweep.step(now,1000,.28,[&](uintptr_t actor){
  int kind=-1;
  if(pending_accounts.size()>=128||!read(actor+0x7dc,kind)||(kind!=0&&kind!=1))return;
  auto identity=identify(actor,range,false);
  if(!identity.account.empty()&&!identity.local)pending_accounts.emplace(std::move(identity.account),actor);
 });
 if(ready){observed_accounts.swap(pending_accounts);pending_accounts.clear();}
}
inline std::vector<std::string> shared_accounts(){
 std::vector<std::string> keys;
 const auto current=actor_range();
 if(!current||!(current==account_sweep.range)||ssc_privacy::blocked())return keys;
 keys.reserve(observed_accounts.size());
 // Recheck before handing identities to the worker. An actor may leave,
 // become anonymous, or reuse a slot during a multi-frame scan.
 for(const auto& entry:observed_accounts){
  const auto identity=identify(entry.second,current,false);
  if(!identity.local&&identity.account==entry.first)keys.push_back(entry.first);
 }
 return keys;
}
// Observe the original game result, never the cosmetic override returned below.
inline std::string native_pass_account;
inline bool native_pass_value=false;
inline uint64_t native_pass_observed=0;
inline void observe_native_pass(bool own,unsigned char result){
 if(!own)return;
 std::string account;if(!account_string(image_base+ssc_compat::resolve(0xe091e8),account))return;
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
    if(!account_string(image_base+ssc_compat::resolve(0xe091e8),own_key)||!account_string(text,candidate)||own_key!=candidate)return false;
    uintptr_t world=0,first=0,last=0;int slot=-1;
    if(!read(image_base+ssc_compat::resolve(0xdef5d0),world)||!read(world+0xa658,first)||!read(world+0xa660,last)||
       !first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048||
       !read(image_base+ssc_compat::resolve(0xe23678),slot)||slot<0||size_t(slot)>=(last-first)/0x34b8)return false;
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
 if(appearance.own&&local_cosmetics()){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline void __cdecl local_widget(uintptr_t widget,float p2,void* p3,void* p4,char p5,float p6,float p7,float p8,float p9,void* p10,float p11,void* p12,void* p13){
    struct Scope{bool old=drawing_local_widget;Scope(){drawing_local_widget=true;}~Scope(){drawing_local_widget=old;}} scope;
    native_widget(widget,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13);
}
inline float __cdecl widget_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    const bool own=drawing_local_widget&&local_account(reinterpret_cast<uintptr_t>(name));ColorScope scope(own,drawing_local_widget);
    if(own&&local_cosmetics()){++cosmetic_draws;if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);}
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
    if(local_cosmetics()&&identity.local){++cosmetic_draws;if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
extern "C" float __cdecl ssc_score_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t row){
 const bool color_active=appearance_enabled();
    int slot=-1;uintptr_t world=0,first=0,last=0;
    Identity identity;
    if(read(row+0x2c,slot)&&slot>=0&&read(image_base+ssc_compat::resolve(0xdef5d0),world)&&read(world+0xa658,first)&&read(world+0xa660,last)&&last>=first&&size_t(slot)<(last-first)/0x34b8)identity=identify(first+size_t(slot)*0x34b8);
        ColorScope scope(identity.local);
    if(color_active)shared_actor_tint(scope,identity,r,g,b,phase);
    if(color_active&&identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
    if(color_active&&local_cosmetics()&&identity.local){++cosmetic_draws;if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);}
    const float drawn_width=native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    if(score_observer&&identity.actor&&alpha>0&&std::isfinite(drawn_width)&&drawn_width>0){const float left=x-(align==1?drawn_width*.5f:align==2?drawn_width:0);score_observer(identity.actor,left,y,size,drawn_width);}
    return drawn_width;
}
inline float __cdecl account_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
    std::string label;
    const bool valid=account_string(reinterpret_cast<uintptr_t>(name),label);
    const bool own=valid&&(local_account_key(label)||generated_label_identity(label).local);
    ColorScope scope(own);
    if(valid&&!own&&render_private_active()&&bot_text(label))bot_tint(r,g,b,alpha,phase);
    ssc_shared::Style remote;if(valid&&!own&&receive_colors&&shared_text(label,remote))shared_tint(scope,remote,r,g,b,phase);
    if(local_cosmetics()&&own){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);}
    return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// These call sites pass the canonical account string from a leaderboard row,
// not its rank, display-name approximation, or the current match roster.
inline float canonical_account_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 std::string key;
 const bool valid=account_string(reinterpret_cast<uintptr_t>(name),key);
 const bool own=valid&&local_account_key(key);
 ColorScope scope(own);
 ssc_shared::Style remote;if(valid&&!own&&shared_account(key,remote))shared_tint(scope,remote,r,g,b,phase);
 if(own&&local_cosmetics()){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
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
 if(identity.local&&local_cosmetics()){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Chat labels are formatted (for example "Name:"). R13 still points to
// the original sender record whose leading string is used by the native
// account predicate. Never authorize a name tint by stripping display text.
extern "C" void ssc_chat_name_bridge();
extern "C" void ssc_actor_name_bridge();
extern "C" float ssc_chat_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t sender){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 std::string key;
 const bool valid=account_string(sender,key);
 const bool own=valid&&(local_account_key(key)||generated_label_identity(key).local);
 ColorScope scope(own);
 ssc_shared::Style remote;if(valid&&!own&&shared_account(key,remote))shared_tint(scope,remote,r,g,b,phase);
 if(own&&local_cosmetics()){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Actor-attached name labels have a separate render path from overhead names.
extern "C" float ssc_actor_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t actor){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const auto identity=identify(actor);const bool own=identity.local;ColorScope scope(own);
 shared_actor_tint(scope,identity,r,g,b,phase);
 if(identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
 if(own&&local_cosmetics()){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Showdown retains eliminated actors in its list; validate the registry and
// account even when the actor is no longer active. Native formatting stays intact.
extern "C" float ssc_showdown_name_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase,uintptr_t actor){
 if(!appearance_enabled())return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const auto identity=identify(actor,true);const bool own=identity.local;ColorScope scope(own);
 shared_actor_tint(scope,identity,r,g,b,phase);
 if(identity.bot&&render_private_active())bot_tint(r,g,b,alpha,phase);
 if(own&&local_cosmetics()){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 return native_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
// Main-menu account widget uses its stored wrapped label (+0xa8), not
// the optional single-line argument. Keep the native layout/ownership intact.
using Wrapped=int(*)(uintptr_t,float,float,float,float,void*,float,float,float,float,float,int,int,int,unsigned char,int,float,float,float);
inline Wrapped native_wrapped=nullptr;
inline int widget_wrapped(uintptr_t renderer,float x,float y,float width,float height,void* name,float size,float r,float g,float b,float alpha,int align,int vertical,int first,unsigned char flags,int limit,float start,float end,float phase){
 if(!appearance_enabled())return native_wrapped(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
 const bool own=drawing_local_widget&&local_account(reinterpret_cast<uintptr_t>(name));ColorScope scope(own,drawing_local_widget);
 if(own&&local_cosmetics()){++cosmetic_draws;if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);}
 return native_wrapped(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
}
// Shared native text adapters cover cached list/profile/leaderboard widgets as
// well as direct name labels. Compare the COMPLETE UTF-16 text with the current
// UTF-8 account name; never strip punctuation or recolor matching substrings.
// This is presentation matching, not private-module authorization. Explicit
// actor/sender ownership always wins, including an explicit non-local result.
struct WideAppearance {
 bool own=false,bot=false,remote=false;
 ssc_shared::Style style;
};
inline WideAppearance wide_appearance(uintptr_t text){
 WideAppearance value;
 const bool own_enabled=local_cosmetics();
 if(identity_scope>=0){
  value.own=own_enabled&&identity_scope==1;
  return value;
 }
 const bool indexed=!shared_label_index.empty()||!formatted_labels.empty();
 if(!own_enabled&&!indexed)return value;
 std::string label;
 if(!wide_label(text,label))return value;
 const auto actor=indexed?label_actor(label):0;
 if(own_enabled)value.own=actor?identify(actor,true).local:local_account_key(label);
 if(actor){
  if(render_private_active())value.bot=bot_text(label);
  if(!value.own&&receive_colors)value.remote=shared_text(label,value.style);
 }
 return value;
}
inline Draw native_wide_draw=nullptr;
inline float wide_draw(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
 if(!appearance_enabled())return native_wide_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
 const auto appearance=wide_appearance(reinterpret_cast<uintptr_t>(name));ColorScope scope(appearance.own);
 scope.inherit_remote();
 if(appearance.remote)shared_tint(scope,appearance.style,r,g,b,phase);
 if(appearance.bot)bot_tint(r,g,b,alpha,phase);
 if(appearance.own){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
 // The native callee consumes this MSVC string. Inspect before calling and
 // forward exactly once; do not free it or substitute a MinGW string object.
 return native_wide_draw(renderer,x,y,name,size,r,g,b,alpha,align,width,depth,shadow,phase);
}
inline Wrapped native_wide_wrapped=nullptr;
inline int wide_wrapped(uintptr_t renderer,float x,float y,float width,float height,void* name,float size,float r,float g,float b,float alpha,int align,int vertical,int first,unsigned char flags,int limit,float start,float end,float phase){
 if(!appearance_enabled())return native_wide_wrapped(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
 const auto appearance=wide_appearance(reinterpret_cast<uintptr_t>(name));ColorScope scope(appearance.own);
 scope.inherit_remote();
 if(appearance.remote)shared_tint(scope,appearance.style,r,g,b,phase);
 if(appearance.bot)bot_tint(r,g,b,alpha,phase);
 if(appearance.own){if(rainbow||gradient){if(!local_phase(phase))phase=0;}solid(r,g,b,phase);++cosmetic_draws;}
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
 if(!read_bytes(image_base+0x3bf5d0,code.data(),code.size()))return false;
 const char* hex="81095851909c5360181feb70c87b4ba412ffb82bd54ccbbb94c1ecdc48e3e136";
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
  {0x7e0130,0x220,"e3ae4840a304509736cd00c7a626ebdd300fa7c734627b3802d16e932c12cbfc"},
  {0x223de0,0x333,"b783dd2a5157b38e91aacb8473a0e71b48c7ac032530ce070c9dd6872bc421ec"},
  {0x3ad6f0,0x3c4,"9e037deaa3f234a2c493c42fee0b21d55849b1153c0fc1b8b4d4100d23a57e45"},
  {0x609730,0x17ac,"38340848de4c5631171474a942d41219296652f7ff9b716b501251df3314b739"},
  {0x60b510,0x20fb,"ee97adf2dcae9e2e714895498e57aa849670d06c7b635e265e78e1f8fe9ccffb"},
  {0x6294f0,0x3a3a,"870db63b4e45016466f2045e0fcefd61f07c3b9aef60bb9b37b32a8ed5c8c8e4"},
  {0x41b810,0x1243,"e4d80f9569973e7197ff1b3955c06b140241fff72eb7cda73c6ab06a409d9965"},
  {0x624330,0x51c0,"85a15fe78414049ec7ba0dfa3e82b04f1c6d23bed7e2a9ed1ea07d44acb225ab"},
  {0x63cc70,0x9468,"00a0adeef0174f0edc6e845dbdf5b164ad1ed4ef3a957f846caced154206398a"},
  {0x9e8cd6,0x72f,"60ef99f8340733607cab0df947fac07c40e5015c695b53a375dfaa055c81f400"},
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
    clone_layout=clone_layout_supported(image_base);
    {std::array<unsigned char,12105> code{};std::array<unsigned char,32> actual{},expected{};
     const char* hex="fee3a845bb4957e70cdf6842b3fb91ef6fe708a2120c4be4c664d036c327c93e";
     if(!read_bytes(image_base+0x3bc680,code.data(),code.size())||sodium_hex2bin(expected.data(),expected.size(),hex,64,nullptr,nullptr,nullptr))return false;
     crypto_hash_sha256(actual.data(),code.data(),code.size());if(sodium_memcmp(actual.data(),expected.data(),32))return false;}
    native_predicate=reinterpret_cast<Predicate>(image_base+ssc_compat::resolve(0x3ad6f0));native_overhead=reinterpret_cast<Overhead>(image_base+ssc_compat::resolve(0x84e5a0));native_draw=reinterpret_cast<Draw>(image_base+ssc_compat::resolve(0x482370));
    native_wrapped=reinterpret_cast<Wrapped>(image_base+ssc_compat::resolve(0x482500));
    native_wide_draw=reinterpret_cast<Draw>(image_base+ssc_compat::resolve(0x482fb0));
    native_wide_wrapped=reinterpret_cast<Wrapped>(image_base+ssc_compat::resolve(0x482690));
    native_palette=reinterpret_cast<Palette>(image_base+ssc_compat::resolve(0x156010));
    native_name_formatter=reinterpret_cast<NameFormatter>(image_base+0x7e0350);
    native_sender_formatter=reinterpret_cast<NameFormatter>(image_base+0x7e0130);
    native_widget=reinterpret_cast<Widget>(image_base+ssc_compat::resolve(0x9a1860));
    struct Site{uint32_t call,target;uintptr_t hook;bool exact=false;};
    // Whitelist presentation calls only. Do not hook the predicate globally:
    // 5fb3b0/5fd316/6834a3/6b6db7 also service pass-related UI/cache logic.
    Site sites[]={
        {0x223e2b,0x7e0130,reinterpret_cast<uintptr_t>(sender_name_formatter),true},
        {0x60a38f,0x3ad6f0,reinterpret_cast<uintptr_t>(native_pass_probe),true},
        {0x60c2f6,0x3ad6f0,reinterpret_cast<uintptr_t>(native_pass_probe),true},
        // Native formatter constructs the cached participant label from RBX actor.
        {0x3bcbce,0x7e0350,reinterpret_cast<uintptr_t>(list_name_formatter),true},
        // Showdown participant name, including native eliminated/score suffixes; RDI is its actor.
        {0x3bd557,0x482370,reinterpret_cast<uintptr_t>(ssc_showdown_name_bridge),true},
        // Modifier owner is formatted by 0x7de740; RBX retains its actor.
        {0x3c33a4,0x482370,reinterpret_cast<uintptr_t>(modifier_label_draw),true},
        {0x3c348d,0x482370,reinterpret_cast<uintptr_t>(modifier_label_draw),true},
        {0x3c35db,0x482370,reinterpret_cast<uintptr_t>(modifier_label_draw),true},
        // Shared narrow-to-wide conversion and direct UTF-16 text paths.
        {0x431442,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x431aac,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x431dd3,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x43cc7d,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x482496,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x48262b,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x486220,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x530a4f,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5320b0,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5325aa,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x535272,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x536f18,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5373b5,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x537b7c,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x537fd1,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5382a8,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x538548,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5387e8,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x538ce8,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5398ed,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x539dd1,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x545054,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x54530a,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x5465a3,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x546b26,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x547106,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x548480,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x54888e,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x548d5c,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x636e54,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6530dc,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x653249,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6533a0,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6534e9,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x653596,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65372f,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6538db,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x653b1f,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65b6c6,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65b862,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65b9f5,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65bded,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65bf9e,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65c146,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x65c2e4,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x661a38,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x662d8e,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x665e89,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x665f3b,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x66917d,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x6c29b8,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c2b70,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c3145,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c3228,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c36cd,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c39fb,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c3d1d,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c403f,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c66d9,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c6d0f,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c69f4,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c7082,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6c743f,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x6d957a,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x71bcbe,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        {0x9a90ba,0x482fb0,reinterpret_cast<uintptr_t>(wide_draw)},
        {0x9a9190,0x482690,reinterpret_cast<uintptr_t>(wide_wrapped)},
        // Both glyph palette paths (regular and alternate text rendering).
        {0x48bbd1,0x156010,reinterpret_cast<uintptr_t>(palette)},
        {0x48bc32,0x156010,reinterpret_cast<uintptr_t>(palette)},
        {0x3f82bc,0x482370,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f8375,0x482370,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f8408,0x482370,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f84dc,0x482370,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f8578,0x482370,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},
        {0x3f8633,0x482370,reinterpret_cast<uintptr_t>(ssc_actor_name_bridge)},

        {0x9a3bbf,0x482500,reinterpret_cast<uintptr_t>(widget_wrapped)},
        {0x48b519,0x156010,reinterpret_cast<uintptr_t>(palette)},
        {0x48b576,0x156010,reinterpret_cast<uintptr_t>(palette)},
        {0x41c6e5,0x482370,reinterpret_cast<uintptr_t>(account_draw)},
        {0x41c76a,0x482370,reinterpret_cast<uintptr_t>(ssc_modifier_name_bridge),true},
        {0x6291ba,0x482370,reinterpret_cast<uintptr_t>(canonical_account_draw),true},
        {0x62b8f8,0x482370,reinterpret_cast<uintptr_t>(canonical_account_draw),true},
        {0x642ddb,0x482370,reinterpret_cast<uintptr_t>(canonical_account_draw),true},
        {0x6af21c,0x482370,reinterpret_cast<uintptr_t>(account_draw)},
        {0x6af4a4,0x482370,reinterpret_cast<uintptr_t>(ssc_chat_name_bridge)},
        {0x6af6c2,0x482370,reinterpret_cast<uintptr_t>(account_draw)},
        {0x9e90c3,0x482370,reinterpret_cast<uintptr_t>(ssc_vehicle_name_bridge),true},
        {0x9b45bb,0x482370,reinterpret_cast<uintptr_t>(event_draw)},
        {0x9b475f,0x482370,reinterpret_cast<uintptr_t>(event_draw)},

        {0x33f48c,0x84e5a0,reinterpret_cast<uintptr_t>(overhead)},
        {0x84ea56,0x3ad6f0,reinterpret_cast<uintptr_t>(predicate)},
        {0x41c4fc,0x3ad6f0,reinterpret_cast<uintptr_t>(predicate)},
        {0x43c608,0x3ad6f0,reinterpret_cast<uintptr_t>(predicate)},
        {0x62668c,0x3ad6f0,reinterpret_cast<uintptr_t>(predicate)},
        {0x63ff9e,0x3ad6f0,reinterpret_cast<uintptr_t>(predicate)},
        {0x6ad733,0x3ad6f0,reinterpret_cast<uintptr_t>(predicate)},
        {0x9e9025,0x3ad6f0,reinterpret_cast<uintptr_t>(predicate)},
        {0x9b44ed,0x3ad6f0,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x9b4697,0x3ad6f0,reinterpret_cast<uintptr_t>(event_predicate)},
        {0x60c621,0x9a1860,reinterpret_cast<uintptr_t>(local_widget)},
        {0x9a3cf9,0x482370,reinterpret_cast<uintptr_t>(widget_draw)},
        {0x84f2d8,0x482370,reinterpret_cast<uintptr_t>(draw)},
        {0x84f364,0x482370,reinterpret_cast<uintptr_t>(draw)},
        {0x4390ad,0x482370,reinterpret_cast<uintptr_t>(ssc_score_bridge)},
        {0x439132,0x482370,reinterpret_cast<uintptr_t>(ssc_score_bridge)}};
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

