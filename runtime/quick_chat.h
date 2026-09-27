#pragma once
#include "presence_source.h"
#include "wheel_images.h"
#include <array>
#include <fstream>
#include <filesystem>
#include "third_party/json.hpp"
namespace ssc_chat {
struct Slot {std::string text;int icon=0;bool custom=false;};
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
inline bool save(const std::filesystem::path& dir){try{nlohmann::json j={{"schema",1},{"enabled",enabled},{"order",order},{"auto_text",auto_text},{"auto_timing",auto_timing}};j["slots"]=nlohmann::json::array();for(auto& slot:slots)j["slots"].push_back({{"text",slot.text},{"icon",slot.icon},{"custom",slot.custom}});auto tmp=dir/L"quick-chat.json.tmp";std::ofstream out(tmp);out<<j.dump(2);out.close();return out&&MoveFileExW(tmp.c_str(),(dir/L"quick-chat.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}catch(...){return false;}}
inline void load(const std::filesystem::path& dir){ssc_wheel_images::load(dir);try{auto p=dir/L"quick-chat.json";if(!std::filesystem::exists(p))return;if(std::filesystem::file_size(p)>16384)return;std::ifstream in(p);nlohmann::json j;in>>j;if(j.at("schema")!=1)return;auto next=slots;auto ord=j.at("order").get<std::array<int,9>>();auto sorted=ord;std::sort(sorted.begin(),sorted.end());for(int i=0;i<9;++i){if(sorted[i]!=i)return;next[i]={j.at("slots").at(i).at("text").get<std::string>(),j.at("slots").at(i).at("icon").get<int>(),j.at("slots").at(i).value("custom",false)};if(!valid_text(next[i].text)||next[i].icon<0||next[i].icon>8)return;}auto text=j.value("auto_text",std::string("GG"));int timing=j.value("auto_timing",0);if(!valid_text(text)||timing<0||timing>2)return;bool on=j.value("enabled",false);slots=next;order=ord;auto_text=text;auto_timing=timing;enabled=on;}catch(...){status=L"Could not load quick message settings";}}
inline uintptr_t stock_atlas(){auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));return ssc_compat::supports(16)?base+ssc_compat::resolve(0xe22470):0;}
// Native records are owned by the game. Never put a mod allocator's buffer in them.
using Builder=void(*)(uintptr_t);
using Assign=void*(*)(uintptr_t,const void*,size_t,size_t);
inline Builder original=nullptr;inline Assign assign=nullptr;inline uintptr_t image_base=0;
struct NativeString {uintptr_t pointer=0,padding=0;size_t size=0,capacity=16;};
inline bool local_actor(uintptr_t actor){uintptr_t world=0,first=0,last=0,steam=0;int slot=-1;return ssc_names::read(image_base+ssc_compat::resolve(0xde35d0),world)&&world&&ssc_names::read(world+0xa618,first)&&ssc_names::read(world+0xa620,last)&&ssc_names::read(image_base+ssc_compat::resolve(0xe1b088),steam)&&first==steam&&ssc_names::read(image_base+ssc_compat::resolve(0xe17608),slot)&&slot>=0&&first&&last>=first&&(last-first)%0x3478==0&&uintptr_t(slot)<(last-first)/0x3478&&actor==first+uintptr_t(slot)*0x3478;}

inline bool apply(uintptr_t actor){
 uintptr_t first=0,last=0;if(!enabled||!assign||!ssc_names::read(actor+0x2688,first)||!ssc_names::read(actor+0x2690,last)||!first||last<first||(last-first)%0x48||last-first>9*0x48)return false;
 struct Entry{int id;std::string text;};std::vector<Entry> entries;
 for(auto at=first;at<last;at+=0x48){int id=-1;if(!ssc_names::read(at+0x24,id)||id<0||id>8)return false;auto text=ssc_rpc::native_string(at);if(text.empty())return false;entries.push_back({id,text});}
 std::vector<Entry> ordered;for(int id:order){auto it=std::find_if(entries.begin(),entries.end(),[&](const Entry& e){return e.id==id;});if(it!=entries.end())ordered.push_back(*it);}
 if(ordered.size()!=entries.size())return false;
 for(size_t i=0;i<ordered.size();++i){auto& e=ordered[i];auto& config=slots[e.id];auto& text=config.text.empty()?e.text:config.text;if(!valid_text(config.text)||config.icon<0||config.icon>8)return false;NativeString source{reinterpret_cast<uintptr_t>(text.data()),0,text.size(),std::max(size_t(16),text.size())};auto at=first+i*0x48;assign(at,&source,0,size_t(-1));ssc_wheel_images::fallback[e.id]=config.icon;int icon=config.custom&&!ssc_wheel_images::images[e.id].empty()?ssc_wheel_images::tag+e.id:0x200+config.icon;std::memcpy(reinterpret_cast<void*>(at+0x28),&icon,4);}
 return true;
}
inline void build(uintptr_t actor){original(actor);if(enabled&&local_actor(actor)){try{apply(actor);}catch(...){enabled=false;status=L"Custom wheel paused after a data error";}}}
// Bound only the wheel's center label; the outbound message stays untouched.
using LabelDraw=uintptr_t(*)(uintptr_t,float,float,float,float,uintptr_t,float,float,float,float,float,int,int,uintptr_t,uintptr_t,uintptr_t,float,float,uintptr_t);
inline LabelDraw original_label=nullptr;
inline uintptr_t draw_label(uintptr_t a,float x,float y,float width,float height,uintptr_t text,float scale,float r,float g,float b,float alpha,int align,int vertical,uintptr_t start,uintptr_t clip,uintptr_t limit,float p17,float p18,uintptr_t p19){
 return original_label(a,x,y,enabled?std::min(width,220.f):width,height,text,scale,r,g,b,alpha,align,vertical,start,clip,limit,p17,p18,p19);
}
inline bool attach(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(!ssc_compat::supports(16))return false;
 struct Hook{uint32_t call,target,reserved;};Hook hooks[]={{0x80f03f,0x80a3d0,0},{0x3c97d5,0x80a3d0,0},{0x3c9f03,0x4862f0,0},{0x3ca054,0x47fbf0,0}};
 for(auto& h:hooks){h.call=ssc_compat::resolve(h.call);h.target=ssc_compat::resolve(h.target);auto p=reinterpret_cast<const unsigned char*>(base+h.call);int32_t rel;std::memcpy(&rel,p+1,4);if(p[0]!=0xe8||base+h.call+5+rel!=base+h.target)return false;}
 unsigned char* bridge=nullptr;for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(bridge,jump,6);auto entry=reinterpret_cast<uintptr_t>(build);std::memcpy(bridge+6,&entry,8);std::memcpy(bridge+32,jump,6);auto icon_entry=reinterpret_cast<uintptr_t>(ssc_wheel_images::draw);std::memcpy(bridge+38,&icon_entry,8);std::memcpy(bridge+64,jump,6);auto label_entry=reinterpret_cast<uintptr_t>(draw_label);std::memcpy(bridge+70,&label_entry,8);DWORD old;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 DWORD protections[4]{};for(int i=0;i<4;++i)if(!VirtualProtect(reinterpret_cast<void*>(base+hooks[i].call),5,PAGE_EXECUTE_READWRITE,&protections[i])){for(int j=i-1;j>=0;--j){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+hooks[j].call),5,protections[j],&ignored);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 image_base=base;original=reinterpret_cast<Builder>(base+ssc_compat::resolve(0x80a3d0));assign=reinterpret_cast<Assign>(base+ssc_compat::resolve(0x24820));
 original_label=reinterpret_cast<LabelDraw>(base+ssc_compat::resolve(0x47fbf0));
 ssc_wheel_images::original=reinterpret_cast<ssc_wheel_images::Draw>(base+ssc_compat::resolve(0x4862f0));
 for(int i=0;i<4;++i){int32_t rel=int32_t(reinterpret_cast<uintptr_t>(bridge+(i>=2?(i-1)*32:0))-(base+hooks[i].call+5));std::memcpy(reinterpret_cast<void*>(base+hooks[i].call+1),&rel,4);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(base+hooks[i].call),5);}
 // Hooks may share a page: finish every write before restoring protection,
 // then unwind in reverse so the original RX protection wins.
 for(int i=3;i>=0;--i){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+hooks[i].call),5,protections[i],&ignored);}return true;
}
}
