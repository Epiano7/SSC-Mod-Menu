#pragma once
#include "privacy.h"
#include "chat_send_budget.h"
#include "presence_source.h"
#include "wheel_images.h"
#include <array>
#include <cstdio>
#include <random>
#include <fstream>
#include <filesystem>
#include "third_party/json.hpp"
namespace ssc_chat {
struct Slot {std::string text;int icon=0;bool custom=false;std::vector<std::string> variants{};};
inline std::array<Slot,9> slots={{{"",0},{"",1},{"",2},{"",3},{"",4},{"",5},{"",6},{"",7},{"",8}}};
inline std::array<int,9> order={{0,1,2,3,4,5,6,7,8}};
inline const wchar_t* names[]={L"Go go go!",L"Wow!",L"Yes",L"No",L"Well played",L"Thanks!",L"Hi!",L"GLHF",L"GG"};
inline bool enabled=false,attached=false;
inline int selected=0,auto_timing=0;
inline std::string auto_text="GG";
inline std::wstring status;
inline bool valid_text(const std::string& s){if(s.size()>100)return false;for(unsigned char c:s)if(c<32||c>126)return false;return s.rfind("XL_",0)!=0&&s.rfind("XL100",0)!=0;}
inline void reset(){for(int i=0;i<9;++i){slots[i]={"",i};order[i]=i;}}
inline void move(int direction){int to=std::clamp(selected+direction,0,8);std::swap(order[selected],order[to]);selected=to;}
// Native wheel: eight entries, starting at -180 degrees. GLHF/GG share the last slot.
inline std::array<int,8> visible_order(bool round_ended){std::array<int,8> result{};int n=0;for(int i=0;i<9;++i)if(order[i]!=(round_ended?7:8))result[n++]=i;return result;}
inline double preview_angle(int index){return (index*45.0-180.0)*3.141592653589793/180.0;}
inline void move_visible(int direction,bool round_ended){auto visible=visible_order(round_ended);auto it=std::find(visible.begin(),visible.end(),selected);if(it==visible.end())return;int next=std::clamp(int(it-visible.begin())+direction,0,7);std::swap(order[selected],order[visible[next]]);selected=visible[next];}
inline std::wstring widen(const std::string& s){return std::wstring(s.begin(),s.end());}
inline std::wstring preview(){if(auto_text.empty())return L"Enter a message";if(auto_text.find('{')!=std::string::npos||auto_text.find('}')!=std::string::npos)return L"Statistic variables are not available in this preview";return widen(auto_text);}
constexpr size_t max_messages=12;
constexpr size_t max_custom_send_length=80;
inline std::string short_message(const std::string& text){return text.substr(0,max_custom_send_length);}
inline bool valid_slot(const Slot& slot){if(!valid_text(slot.text)||slot.icon<0||slot.icon>8||slot.variants.size()>=max_messages)return false;if(!slot.variants.empty()&&slot.text.empty())return false;for(auto& v:slot.variants)if(v.empty()||!valid_text(v))return false;return true;}
inline size_t message_count(const Slot& slot){return 1+slot.variants.size();}
inline std::string& message_at(Slot& slot,size_t i){return i?slot.variants.at(i-1):slot.text;}
inline const std::string& message_at(const Slot& slot,size_t i){return i?slot.variants.at(i-1):slot.text;}
inline void remove_message(Slot& slot,size_t i){if(i>=message_count(slot))return;if(!i){if(slot.variants.empty())slot.text.clear();else{slot.text=slot.variants.front();slot.variants.erase(slot.variants.begin());}}else slot.variants.erase(slot.variants.begin()+i-1);}
inline void make_first(Slot& slot,size_t i){if(i&&i<message_count(slot)){auto text=message_at(slot,i);remove_message(slot,i);slot.variants.insert(slot.variants.begin(),slot.text);slot.text=text;}}
inline const std::string& choose_message(const Slot& slot){static std::mt19937 engine(std::random_device{}());return message_at(slot,std::uniform_int_distribution<size_t>(0,message_count(slot)-1)(engine));}
inline bool save(const std::filesystem::path& dir){try{for(auto& slot:slots)if(!valid_slot(slot))return false;nlohmann::json j={{"schema",2},{"enabled",enabled},{"order",order},{"auto_text",auto_text},{"auto_timing",auto_timing}};j["slots"]=nlohmann::json::array();for(auto& slot:slots)j["slots"].push_back({{"text",slot.text},{"variants",slot.variants},{"icon",slot.icon},{"custom",slot.custom}});auto tmp=dir/L"quick-chat.json.tmp";std::ofstream out(tmp);out<<j.dump(2);out.close();return out&&MoveFileExW(tmp.c_str(),(dir/L"quick-chat.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}catch(...){return false;}}
inline void load(const std::filesystem::path& dir){ssc_wheel_images::load(dir);try{auto p=dir/L"quick-chat.json";if(!std::filesystem::exists(p))return;if(std::filesystem::file_size(p)>32768)return;std::ifstream in(p);nlohmann::json j;in>>j;int schema=j.at("schema");if(schema!=1&&schema!=2)return;auto next=slots;auto ord=j.at("order").get<std::array<int,9>>();auto sorted=ord;std::sort(sorted.begin(),sorted.end());for(int i=0;i<9;++i){if(sorted[i]!=i)return;const auto& row=j.at("slots").at(i);next[i]={row.at("text").get<std::string>(),row.at("icon").get<int>(),row.value("custom",false)};if(schema==2)next[i].variants=row.value("variants",std::vector<std::string>{});if(!valid_slot(next[i]))return;}auto text=j.value("auto_text",std::string("GG"));int timing=j.value("auto_timing",0);if(!valid_text(text)||timing<0||timing>2)return;bool on=j.value("enabled",false);slots=next;order=ord;auto_text=text;auto_timing=timing;enabled=on;}catch(...){status=L"Could not load quick message settings";}}
inline uintptr_t stock_atlas(){auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));return ssc_compat::supports(16)?base+ssc_compat::resolve(0xe2e580):0;}
// Native records are owned by the game. Never put a mod allocator's buffer in them.
using Builder=void(*)(uintptr_t);
using Assign=void*(*)(uintptr_t,const void*,size_t,size_t);
inline Builder original=nullptr;inline Assign assign=nullptr;inline uintptr_t image_base=0;
struct NativeString {uintptr_t pointer=0,padding=0;size_t size=0,capacity=16;};
inline constexpr uintptr_t spam_meter_offset=0x26f0;
static_assert(spam_meter_offset>=0x26d0+sizeof(NativeString),"Spam meter overlaps native chat string");
template<class Read>bool validate_spam_layout(uintptr_t base,Read read){
 std::array<unsigned char,8> load{},store{};std::array<unsigned char,10> lock{};double cost=0;float threshold=0;
 const std::array<unsigned char,8> expected_load={0xf3,0x0f,0x10,0x83,0xf0,0x26,0,0},expected_store={0xf3,0x0f,0x11,0x8b,0xf0,0x26,0,0};
 const std::array<unsigned char,10> expected_lock={0xc7,0x83,0xf4,0x26,0,0,0,0,0,0x41};
 return read(base+0x816307,load)&&load==expected_load&&read(base+0x816325,store)&&store==expected_store&&read(base+0x81632f,lock)&&lock==expected_lock&&read(base+0xc80688,cost)&&cost==double(ssc_chat_limit::stock_cost)&&read(base+0xc80e60,threshold)&&threshold==ssc_chat_limit::threshold;
}
inline bool local_actor(uintptr_t actor){uintptr_t world=0,first=0,last=0,steam=0;int slot=-1;return ssc_names::read(image_base+ssc_compat::resolve(0xdef5d0),world)&&world&&ssc_names::read(world+0xa658,first)&&ssc_names::read(world+0xa660,last)&&ssc_names::read(image_base+ssc_compat::resolve(0xe27138),steam)&&first==steam&&ssc_names::read(image_base+ssc_compat::resolve(0xe23678),slot)&&slot>=0&&first&&last>=first&&(last-first)%0x34b8==0&&uintptr_t(slot)<(last-first)/0x34b8&&actor==first+uintptr_t(slot)*0x34b8;}
inline bool native_spam_meter(uintptr_t actor,ssc_chat_limit::Meter& value){
 // Group 16 fingerprints the whole native wheel function, including the counter read/write instructions and lockout; also verify the
 // exact non-overlapping field layout at attachment. Never use unguarded
 // fields, its +2 charge, threshold and lockout. Never use unguarded offsets.
 if(!attached||!ssc_compat::supports(16)||!local_actor(actor)||
    !ssc_names::read(actor+spam_meter_offset,value)||!ssc_chat_limit::valid(value))return false;
 MEMORY_BASIC_INFORMATION page{};const auto address=actor+spam_meter_offset;
 if(!VirtualQuery(reinterpret_cast<void*>(address),&page,sizeof(page))||page.State!=MEM_COMMIT||
    (page.Protect&(PAGE_GUARD|PAGE_NOACCESS))||address<reinterpret_cast<uintptr_t>(page.BaseAddress)||
    address-reinterpret_cast<uintptr_t>(page.BaseAddress)>page.RegionSize||
    sizeof(value)>page.RegionSize-(address-reinterpret_cast<uintptr_t>(page.BaseAddress)))return false;
 return (page.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))!=0;
}
inline bool native_spam_available(uintptr_t actor){ssc_chat_limit::Meter value;return native_spam_meter(actor,value)&&ssc_chat_limit::available(value);}
inline void charge_native_spam(uintptr_t actor,bool wheel){
 ssc_chat_limit::Meter value;if(!native_spam_meter(actor,value))return;
 value=ssc_chat_limit::charge(value,wheel);
 std::memcpy(reinterpret_cast<void*>(actor+spam_meter_offset),&value,sizeof(value));
}

inline uintptr_t mapped_actor=0,mapped_first=0;inline std::vector<int> mapped_slots;
inline unsigned mapped_custom_text=0;
inline uintptr_t pending_actor=0;inline int pending_slot=-1;inline bool pending_custom_text=false;
inline void clear_pending(){pending_actor=0;pending_slot=-1;pending_custom_text=false;}
// This call belongs only to native wheel selection, not chat-box or automatic messages.
inline void* select_message(uintptr_t destination,const void* source,size_t start,size_t count){
 clear_pending();auto at=reinterpret_cast<uintptr_t>(source);
 if(mapped_actor&&destination==mapped_actor+0x26d0&&at>=mapped_first&&(at-mapped_first)%0x48==0){size_t i=(at-mapped_first)/0x48;if(i<mapped_slots.size()){pending_actor=mapped_actor;pending_slot=mapped_slots[i];pending_custom_text=(mapped_custom_text&(1u<<i))!=0;}}
 return assign(destination,source,start,count);
}
using Sender=void(*)(uintptr_t,unsigned char,uintptr_t,const void*);
inline Sender original_send=nullptr;
inline void send_message(uintptr_t world,unsigned char team,uintptr_t actor,const void* message){
 int id=pending_slot;uintptr_t owner=pending_actor;const bool customized=pending_custom_text;clear_pending();
 if(ssc_privacy::blocked()&&id>=0&&actor==owner)return; // Never send an already-customized wheel entry.
 if(customized&&(actor!=owner||!enabled||!native_spam_available(actor)))return;
 try{if(customized&&enabled&&actor==owner&&id>=0&&id<9){
  const auto& slot=slots[id];if(valid_slot(slot)&&!slot.variants.empty()&&ssc_rpc::native_string(reinterpret_cast<uintptr_t>(message))==short_message(slot.text)){
   const auto text=short_message(choose_message(slot));NativeString chosen{reinterpret_cast<uintptr_t>(text.data()),0,text.size(),std::max(size_t(16),text.size())};original_send(world,team,actor,&chosen);charge_native_spam(actor,true);return;
  }
 }}catch(...){status=L"Could not choose a message; sending the wheel label";}
 original_send(world,team,actor,message);
 if(customized)charge_native_spam(actor,true);
}
inline bool apply(uintptr_t actor){
 mapped_actor=mapped_first=0;mapped_slots.clear();mapped_custom_text=0;clear_pending();
 uintptr_t first=0,last=0;if(ssc_privacy::blocked()||!enabled||!assign||!ssc_names::read(actor+0x2690,first)||!ssc_names::read(actor+0x2698,last)||!first||last<first||(last-first)%0x48||last-first>9*0x48)return false;
 struct Entry{int id;std::string text;};std::vector<Entry> entries;
 for(auto at=first;at<last;at+=0x48){int id=-1;if(!ssc_names::read(at+0x24,id)||id<0||id>8)return false;auto text=ssc_rpc::native_string(at);if(text.empty())return false;entries.push_back({id,text});}
 std::vector<Entry> ordered;for(int id:order){auto it=std::find_if(entries.begin(),entries.end(),[&](const Entry& e){return e.id==id;});if(it!=entries.end())ordered.push_back(*it);}
 if(ordered.size()!=entries.size())return false;
 for(const auto& entry:ordered)if(!valid_slot(slots[entry.id]))return false;
 for(size_t i=0;i<ordered.size();++i){auto& e=ordered[i];auto& config=slots[e.id];const auto text=config.text.empty()?e.text:short_message(config.text);NativeString source{reinterpret_cast<uintptr_t>(text.data()),0,text.size(),std::max(size_t(16),text.size())};auto at=first+i*0x48;assign(at,&source,0,size_t(-1));ssc_wheel_images::fallback[e.id]=config.icon;int icon=config.custom&&!ssc_wheel_images::images[e.id].empty()?ssc_wheel_images::tag+e.id:0x200+config.icon;std::memcpy(reinterpret_cast<void*>(at+0x28),&icon,4);}
 mapped_actor=actor;mapped_first=first;for(const auto& entry:ordered){if(!slots[entry.id].text.empty())mapped_custom_text|=1u<<mapped_slots.size();mapped_slots.push_back(entry.id);}
 return true;
}
inline void build(uintptr_t actor){mapped_actor=mapped_first=0;mapped_slots.clear();mapped_custom_text=0;clear_pending();original(actor);if(enabled&&!ssc_privacy::blocked()&&local_actor(actor)){try{apply(actor);}catch(...){enabled=false;status=L"Custom wheel paused after a data error";}}}
// Bound only the wheel's center label; the outbound message stays untouched.
using LabelDraw=uintptr_t(*)(uintptr_t,float,float,float,float,uintptr_t,float,float,float,float,float,int,int,uintptr_t,uintptr_t,uintptr_t,float,float,uintptr_t);
inline LabelDraw original_label=nullptr;
inline uintptr_t draw_label(uintptr_t a,float x,float y,float width,float height,uintptr_t text,float scale,float r,float g,float b,float alpha,int align,int vertical,uintptr_t start,uintptr_t clip,uintptr_t limit,float p17,float p18,uintptr_t p19){
 return original_label(a,x,y,enabled?std::min(width,220.f):width,height,text,scale,r,g,b,alpha,align,vertical,start,clip,limit,p17,p18,p19);
}
// Use the game's line splitter so its existing chat-row layout reserves the
// right number of lines. Changing only the final draw width would overlap rows.
using ChatSplit=void(*)(uintptr_t,uintptr_t,float,int,uintptr_t,float);
inline ChatSplit original_chat_split=nullptr;
inline float chat_viewport_width=0;
inline void (*chat_layout_diagnostic)(const char*)=nullptr;
inline float native_client_width(){
 RECT rect{};auto window=WindowFromDC(wglGetCurrentDC());
 return window&&GetClientRect(window,&rect)?float(rect.right-rect.left):chat_viewport_width;
}
inline float (*chat_client_width)()=native_client_width;
inline float body_wrap_width(float width,float scale,float client){
 if(!std::isfinite(width)||!std::isfinite(scale)||!std::isfinite(client)||width<=0||scale<=0||client<320)return width;
 return std::min(width,std::max(160.f,client*.30f)/scale);
}
inline void split_chat(uintptr_t font,uintptr_t text,float width,int limit,uintptr_t output,float scale){
 const float client=chat_client_width(),wrapped=body_wrap_width(width,scale,client);
 static bool reported=false;
 if(!reported&&client>=320&&chat_layout_diagnostic){char line[160];std::snprintf(line,sizeof(line),"Chat layout: viewport=%.1f scale=%.3f native_width=%.1f wrapped_width=%.1f",client,scale,width,wrapped);chat_layout_diagnostic(line);reported=true;}
 original_chat_split(font,text,wrapped,limit,output,scale);
}
inline bool attach(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(!ssc_compat::supports(17)||!validate_spam_layout(base,[](uintptr_t p,auto& value){return ssc_names::read(p,value);}))return false;
 struct Hook{uint32_t call,target,reserved;};Hook hooks[]={{0x81637f,0x811730,0},{0x3cc595,0x811730,0},{0x3cccc3,0x4887a0,0},{0x3cce14,0x482090,0},{0x8116f1,0x245b0,0},{0x816302,0x223cd0,0},{0x6ad1af,0x3a9980,0},{0x6ad428,0x3a9980,0}};
 constexpr int hook_count=int(std::size(hooks));
 for(auto& h:hooks){h.call=ssc_compat::resolve(h.call);h.target=ssc_compat::resolve(h.target);auto p=reinterpret_cast<const unsigned char*>(base+h.call);int32_t rel;std::memcpy(&rel,p+1,4);if(p[0]!=0xe8||base+h.call+5+rel!=base+h.target)return false;}
 unsigned char* bridge=nullptr;for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(bridge,jump,6);auto entry=reinterpret_cast<uintptr_t>(build);std::memcpy(bridge+6,&entry,8);std::memcpy(bridge+32,jump,6);auto icon_entry=reinterpret_cast<uintptr_t>(ssc_wheel_images::draw);std::memcpy(bridge+38,&icon_entry,8);std::memcpy(bridge+64,jump,6);auto label_entry=reinterpret_cast<uintptr_t>(draw_label);std::memcpy(bridge+70,&label_entry,8);std::memcpy(bridge+96,jump,6);auto select_entry=reinterpret_cast<uintptr_t>(select_message);std::memcpy(bridge+102,&select_entry,8);std::memcpy(bridge+128,jump,6);auto send_entry=reinterpret_cast<uintptr_t>(send_message);std::memcpy(bridge+134,&send_entry,8);DWORD old;
 std::memcpy(bridge+160,jump,6);auto split_entry=reinterpret_cast<uintptr_t>(split_chat);std::memcpy(bridge+166,&split_entry,8);
 std::memcpy(bridge+192,jump,6);std::memcpy(bridge+198,&split_entry,8);
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 DWORD protections[hook_count]{};for(int i=0;i<hook_count;++i)if(!VirtualProtect(reinterpret_cast<void*>(base+hooks[i].call),5,PAGE_EXECUTE_READWRITE,&protections[i])){for(int j=i-1;j>=0;--j){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+hooks[j].call),5,protections[j],&ignored);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 image_base=base;original=reinterpret_cast<Builder>(base+ssc_compat::resolve(0x811730));assign=reinterpret_cast<Assign>(base+ssc_compat::resolve(0x245b0));
 original_send=reinterpret_cast<Sender>(base+ssc_compat::resolve(0x223cd0));
 original_label=reinterpret_cast<LabelDraw>(base+ssc_compat::resolve(0x482090));
 original_chat_split=reinterpret_cast<ChatSplit>(base+ssc_compat::resolve(0x3a9980));
 ssc_wheel_images::original=reinterpret_cast<ssc_wheel_images::Draw>(base+ssc_compat::resolve(0x4887a0));
 for(int i=0;i<hook_count;++i){int32_t rel=int32_t(reinterpret_cast<uintptr_t>(bridge+(i>=2?(i-1)*32:0))-(base+hooks[i].call+5));std::memcpy(reinterpret_cast<void*>(base+hooks[i].call+1),&rel,4);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(base+hooks[i].call),5);}
 // Hooks may share a page: finish every write before restoring protection,
 // then unwind in reverse so the original RX protection wins.
 for(int i=hook_count-1;i>=0;--i){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+hooks[i].call),5,protections[i],&ignored);}return true;
}
}

