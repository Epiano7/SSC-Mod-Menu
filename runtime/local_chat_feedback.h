#pragma once
#include "planned_native_guards.h"
#include "presence_source.h"
#include <array>
#include <atomic>
#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>
namespace ssc_local_chat {
// Borrowed input strings are never handed to a native destructor. The receive
// handler deep-copies them with the game's allocator into its own chat history.
struct BorrowedString {uintptr_t data=0,unused=0;size_t size=0,capacity=16;};
struct Message {
 int slot=-1,team=-1;BorrowedString sender,body;
 std::array<float,3> sender_color{1.f,1.f,1.f},body_color{1.f,1.f,1.f};
 int badge=0;uint8_t has_badge=0,visible=1,team_chat=0,rainbow=0,special=0,padding[3]{};float duration=0;
};
static_assert(sizeof(Message)==0x70&&offsetof(Message,sender)==8&&offsetof(Message,body)==0x28&&offsetof(Message,duration)==0x6c);
using Receive=void(*)(uintptr_t,const Message*,uint8_t,float,int,float,float,uint8_t);
inline Receive receive=nullptr;inline uintptr_t image_base=0;
struct Pending {uintptr_t world=0;std::string sender,body;uint64_t at=0;};
inline std::mutex mutex;inline std::deque<Pending> pending;inline std::atomic<uint64_t> delivered{0};
inline bool attach(uintptr_t base){
 // These exact native receive/copy/append fingerprints are verified before
 // hooks. No outgoing sender, packet builder or transport is used here.
 if(!ssc_planned_native::tts)return false;
 image_base=base;receive=reinterpret_cast<Receive>(base+0x31b070);return true;
}
inline std::wstring wide(const std::string& s){if(s.empty()||s.size()>8192)return {};int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0);if(n<=0)return {};std::wstring out(size_t(n),L' ');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),out.data(),n);return out;}
inline std::string utf8(const std::wstring& s){if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0,nullptr,nullptr);if(n<=0)return {};std::string out(size_t(n),' ');WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s.data(),int(s.size()),out.data(),n,nullptr,nullptr);return out;}
inline std::string safe_sender(const std::string& sender){auto value=wide(sender);value.erase(std::remove_if(value.begin(),value.end(),[](wchar_t c){return c<32||c==127;}),value.end());if(value.size()>24){value.resize(24);if(value.back()>=0xd800&&value.back()<=0xdbff)value.pop_back();}auto out=utf8(value);return out.empty()?"JARVIS":out;}
inline std::vector<std::string> lines(const std::string& text){
 auto value=wide(text);std::vector<std::string> out;if(value.size()>2048)value.resize(2048);
 for(auto& ch:value)if(ch==L'\r'||ch==L'\t')ch=L' ';else if(ch<32&&ch!=L'\n')ch=L' ';
 size_t at=0;while(at<value.size()&&out.size()<40){while(at<value.size()&&(value[at]==L' '||value[at]==L'\n'))++at;if(at==value.size())break;
 auto nl=value.find(L'\n',at);size_t end=std::min(at+64,nl==std::wstring::npos?value.size():nl);
 if(end<value.size()&&end!=nl){auto space=value.rfind(L' ',end);if(space!=std::wstring::npos&&space>at)end=space;}
 if(end>at&&end<value.size()&&value[end-1]>=0xd800&&value[end-1]<=0xdbff)--end;
 if(end==at){break;}
 auto part=value.substr(at,end-at);while(!part.empty()&&part.back()==L' ')part.pop_back();
 auto line=utf8(part);if(!line.empty())out.push_back(std::move(line));at=end;
 }return out;
}
inline void enqueue(const std::string& sender,const std::string& text,uint64_t now){
 if(!receive||!image_base){return;}
 uintptr_t world=0;if(!ssc_names::read(image_base+ssc_compat::resolve(0xdec5d0),world)||!world)return;
 auto name=safe_sender(sender);auto split=lines(text);std::lock_guard<std::mutex> lock(mutex);
 for(auto& body:split){pending.push_back({world,name,std::move(body),now});}
 while(pending.size()>64)pending.pop_front();
}
inline bool valid_world(uintptr_t world){
 uintptr_t current=0,first=0,last=0,capacity=0;using ssc_names::read;
 return receive&&world&&read(image_base+ssc_compat::resolve(0xdec5d0),current)&&current==world&&read(world+0xecc0,first)&&read(world+0xecc8,last)&&read(world+0xecd0,capacity)&&last>=first&&capacity>=last&&(!first?(!last&&!capacity):true)&&(last-first)%0x70==0&&(capacity-first)%0x70==0&&(capacity-first)/0x70<=65536&&(last-first)/0x70<4096;
}
// All three guarded native typed-chat constructors take their lifetime from
// game+0xfb9358 (e.g. the load at 0x22372d). Read it for each reply so changes
// to the game's normal chat duration are followed instead of inventing a timer.
inline float normal_message_duration(){float value=0;if(!ssc_planned_native::commands||!ssc_names::read(image_base+ssc_compat::resolve(0xfb9358),value)||!std::isfinite(value)||value<0||value>300)return 0;return value;}
inline void show_received_reply(uintptr_t world,size_t before,const Pending& line,float duration){
 uintptr_t first=0,last=0;using ssc_names::read;
 if(!valid_world(world)||!read(world+0xecc0,first)||!read(world+0xecc8,last)||(last-first)/sizeof(Message)!=before+1)return;
 auto record=last-sizeof(Message);int slot=0;if(!read(record,slot)||slot!=-1||ssc_rpc::native_string(record+8)!=line.sender||ssc_rpc::native_string(record+0x28)!=line.body)return;
 // The silent SERVER receive path zeroes the popup timer before copying the
 // row. Restore only this verified, newly appended local reply's native timer.
 auto target=record+offsetof(Message,duration);MEMORY_BASIC_INFORMATION memory{};
 if(!VirtualQuery(reinterpret_cast<void*>(target),&memory,sizeof memory)||memory.State!=MEM_COMMIT||(memory.Protect&PAGE_GUARD))return;
 auto protection=memory.Protect&0xff;auto region=reinterpret_cast<uintptr_t>(memory.BaseAddress);
 if((protection!=PAGE_READWRITE&&protection!=PAGE_WRITECOPY)||target<region||target-region>memory.RegionSize||sizeof(float)>memory.RegionSize-(target-region))return;
 std::memcpy(reinterpret_cast<void*>(target),&duration,sizeof duration);
}
// Called only by the guarded native chat-update hook on the game thread.
// UI actions enqueue replies; they never mutate a history vector on render/input threads.
inline void pump(uintptr_t world,uint64_t now){
 std::vector<Pending> ready;{std::lock_guard<std::mutex> lock(mutex);for(auto it=pending.begin();it!=pending.end();){if(it->world!=world||now<it->at||now-it->at>10000)it=pending.erase(it);else if(ready.size()<8){ready.push_back(std::move(*it));it=pending.erase(it);}else ++it;}}
 for(const auto& line:ready){if(!valid_world(world))break;uintptr_t first=0,last=0;if(!ssc_names::read(world+0xecc0,first)||!ssc_names::read(world+0xecc8,last))break;size_t before=(last-first)/sizeof(Message);float duration=normal_message_duration();Message row;row.sender={reinterpret_cast<uintptr_t>(line.sender.c_str()),0,line.sender.size(),std::max(size_t(16),line.sender.size())};row.body={reinterpret_cast<uintptr_t>(line.body.c_str()),0,line.body.size(),std::max(size_t(16),line.body.size())};
 // -1 is the native generic SERVER category, with no player identity, badge,
 // team marker, rainbow effect, speech bubble or sound notification.
 receive(world,&row,0,1.f,0,1.f,1.f,0);show_received_reply(world,before,line,duration);++delivered;
 }
}
}
