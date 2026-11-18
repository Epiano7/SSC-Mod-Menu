#pragma once
#include "chat_shortcut_guards.h"
#include "chat_popup_layout.h"
#include <atomic>
#include <cmath>
#include <mutex>
namespace ssc_commands {
inline bool popup_anchor_supported=false;
inline bool shortcut_supported=false;inline uintptr_t shortcut_base=0;
using ChatUpdate=bool(*)(uintptr_t);using SetChatText=void(*)(uintptr_t,const char*);
inline ChatUpdate original_chat_update=nullptr;inline SetChatText set_chat_text=nullptr;
struct SlashRequest {uintptr_t world=0,actor=0;uint64_t at=0;std::wstring text=L"/";bool processing=false;};
inline std::mutex slash_mutex;
struct CommandInput {uintptr_t world=0,actor=0;bool submit=false;std::wstring replacement;};
inline CommandInput command_input;inline std::vector<Completion> completions;inline size_t completion_index=0;inline std::wstring completion_source;inline uintptr_t completion_world=0;inline uint64_t completion_at=0;inline std::atomic<bool> command_return_held{false},completion_key_held{false};
inline std::vector<Completion> completion_view(){std::lock_guard<std::mutex> lock(slash_mutex);return completions;}
inline SlashRequest slash_request;inline std::atomic<bool> slash_pending{false},slash_enter_held{false};inline std::atomic<int> slash_key_held{0};
inline bool shortcut_enabled(){return shortcut_supported&&attached;}
template<class T>bool shortcut_write(uintptr_t p,T value){static_assert(sizeof(T)==1||sizeof(T)==4);MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&PAGE_GUARD)||!((m.Protect&0xff)==PAGE_READWRITE||(m.Protect&0xff)==PAGE_WRITECOPY))return false;auto b=reinterpret_cast<uintptr_t>(m.BaseAddress);if(p<b||p-b>m.RegionSize||sizeof(T)>m.RegionSize-(p-b))return false;std::memcpy(reinterpret_cast<void*>(p),&value,sizeof value);return true;}
inline void cancel_slash(){std::lock_guard<std::mutex> lock(slash_mutex);slash_request={};command_input={};completions.clear();completion_source.clear();slash_pending=false;slash_key_held=0;slash_enter_held=false;command_return_held=false;completion_key_held=false;}
inline bool chat_context(uintptr_t& world,uintptr_t& actor,bool require_closed){
 using ssc_names::read;uintptr_t first=0,last=0;int slot=-1,world_slot=-1;unsigned char active=2;float cooldown=-1;
 if(!shortcut_enabled()||!read(shortcut_base+0xdec5d0,world)||!world||!read(world+0xa658,first)||!read(world+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048||!read(shortcut_base+0xe20678,slot)||slot<0||uintptr_t(slot)>=(last-first)/0x34b8||!read(world+0x6ba0,active)||active>1)return false;
 actor=first+uintptr_t(slot)*0x34b8;if(!read(world+0x6b98,world_slot)||world_slot!=slot||!ssc_chat::local_actor(actor))return false;
 if(!require_closed)return true;
 if(active||!read(world+0x6ba4,cooldown)||!std::isfinite(cooldown)||cooldown>0)return false;
 for(auto rva:{0xfb4718u,0xfb346au,0xfb3990u}){unsigned char modal=2;if(!read(shortcut_base+rva,modal)||modal!=0)return false;}
 return true;
}
inline bool input_text(uintptr_t world,std::wstring& text){
 using ssc_names::read;auto input=world+0x6bb0;uint64_t length=0,cap=0;uintptr_t data=input+0x1c8;unsigned char active=0;
 if(!read(world+0x6ba0,active)||active!=1||!read(input+0x1d8,length)||!read(input+0x1e0,cap)||length>256||cap<length||cap>65536)return false;
 if(cap>=8&&(!read(input+0x1c8,data)||!data))return false;
 text.resize(size_t(length));return text.empty()||ssc_names::read_bytes(data,text.data(),text.size()*sizeof(wchar_t));
}
struct CompletionPopup {std::vector<Completion> items;size_t selected=0;ChatAnchor anchor;};
inline CompletionPopup completion_popup(int width,int height){
 CompletionPopup out;uintptr_t world=0;{std::lock_guard<std::mutex> lock(slash_mutex);out.items=completions;out.selected=completion_index;world=completion_world;}
 if(out.items.empty()||!popup_anchor_supported||!shortcut_enabled())return {};
 uintptr_t current=0,actor=0;std::wstring input;
 if(!chat_context(current,actor,false)||current!=world||!input_text(world,input)||input.empty()||input.front()!=L'/')return {};
 using ssc_names::read;int x=0,y=0,w=0,h=0,ax=0,ay=0,lw=0,lh=0;auto p=world+0x6bb0;
 if(!read(p+0x14,x)||!read(p+0x18,y)||!read(p+0x1c,w)||!read(p+0x20,h)||!read(p+0x70,ax)||!read(p+0x74,ay)||!read(shortcut_base+ssc_compat::resolve(0xfb3ae8),lw)||!read(shortcut_base+ssc_compat::resolve(0xfb3aec),lh))return {};
 out.anchor=chat_anchor(x,y,w,h,ax,ay,lw,lh,width,height);if(!out.anchor.valid)return {};return out;
}
inline std::string input_utf8(const std::wstring& text){int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0,nullptr,nullptr);if(n<=0)return {};std::string out(size_t(n),'\0');return WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),out.data(),n,nullptr,nullptr)?out:std::string{};}
inline void replace_input(uintptr_t world,const std::wstring& text){if(!set_chat_text)return;auto out=input_utf8(text);set_chat_text(world+0x6bb0,out.c_str());auto n=uint32_t(text.size());for(auto offset:{0x170u,0x174u,0x7cu})shortcut_write(world+0x6bb0+offset,n);}
inline void update_completion(uintptr_t world){
 std::wstring text;if(!router.enabled||!input_text(world,text)||text.empty()||text[0]!=L'/'){std::lock_guard<std::mutex> lock(slash_mutex);completions.clear();completion_source.clear();return;}
 auto now=GetTickCount64();{std::lock_guard<std::mutex> lock(slash_mutex);if(world==completion_world&&text==completion_source&&now>=completion_at&&now-completion_at<500)return;}
 std::vector<std::string> names;for(const auto& p:ssc_kos::roster()){auto name=p.target.player.handle;if(names.size()<64&&std::find(names.begin(),names.end(),name)==names.end())names.push_back(std::move(name));}
 auto next=complete(router,input_utf8(text),names);std::lock_guard<std::mutex> lock(slash_mutex);if(world!=completion_world||text!=completion_source)completion_index=0;completions=std::move(next);completion_source=text;completion_world=world;completion_at=now;
}
inline bool queue_command_key(WPARAM key){
 uintptr_t world=0,actor=0;std::wstring text;if(!chat_context(world,actor,false)||!input_text(world,text))return false;
 auto utf8=input_utf8(text);auto first=utf8.find_first_not_of(" \t");if(first==std::string::npos||utf8[first]!='/')return false;
 std::lock_guard<std::mutex> lock(slash_mutex);
 if(key==VK_RETURN){command_input={world,actor,true,{}};command_return_held=true;return true;}
 if(utf8[first]!='/'||completions.empty()||world!=completion_world||text!=completion_source)return false;
 if(key==VK_UP||key==VK_DOWN){completion_index=(completion_index+completions.size()+(key==VK_UP?-1:1))%completions.size();completion_key_held=true;return true;}
 if(key==VK_TAB){auto& selected=completions[completion_index%completions.size()];command_input={world,actor,false,ssc_chat::widen(selected.text)};completion_key_held=true;return true;}return false;
}
inline bool process_command_input(uintptr_t world,unsigned char transport){
 CommandInput pending;{std::lock_guard<std::mutex> lock(slash_mutex);if(!command_input.world)return false;pending=command_input;command_input={};}
 uintptr_t current=0,actor=0;std::wstring text;if(!chat_context(current,actor,false)||current!=world||pending.world!=world||pending.actor!=actor||!input_text(world,text))return false;
 if(!pending.submit){replace_input(world,pending.replacement);update_completion(world);return true;}
 auto utf8=input_utf8(text);auto first=utf8.find_first_not_of(" \t");if(first==std::string::npos||utf8[first]!='/')return true;
 ssc_chat::NativeString literal{reinterpret_cast<uintptr_t>(utf8.data()),0,utf8.size(),std::max(size_t(16),utf8.size())};typed_send(world,transport,actor,&literal);
 replace_input(world,{});shortcut_write(world+0x6ba0,static_cast<unsigned char>(0));{std::lock_guard<std::mutex> lock(slash_mutex);completions.clear();completion_source.clear();}return true;
}
inline uintptr_t public_chat_press(){
 using ssc_names::read;uintptr_t first=0,last=0,cap=0,found=0;if(!read(shortcut_base+0xfba310,first)||!read(shortcut_base+0xfba318,last)||!read(shortcut_base+0xfba320,cap)||!first||last<first||cap<last||(last-first)%0x150||(last-first)/0x150>256)return 0;
 for(auto p=first;p<last;p+=0x150){int id=-1;if(!read(p+0x148,id))return 0;if(id==34){if(found)return 0;found=p+0x14c;}}return found;
}
inline bool arm_slash(){uintptr_t world=0,actor=0;if(!chat_context(world,actor,true)||!public_chat_press())return false;std::lock_guard<std::mutex> lock(slash_mutex);if(slash_pending.load())return false;slash_request={world,actor,GetTickCount64(),L"/",false};slash_pending=true;return true;}
inline bool translated_slash(WPARAM key,LPARAM data){BYTE state[256]{};if(!GetKeyboardState(state)||(state[VK_CONTROL]&0x80)||(state[VK_MENU]&0x80)||(state[VK_LWIN]&0x80)||(state[VK_RWIN]&0x80))return false;wchar_t text[4]{};return ToUnicodeEx(UINT(key),UINT((data>>16)&0xff),state,text,4,4,GetKeyboardLayout(0))==1&&text[0]==L'/';}
inline bool slash_input(UINT message,WPARAM key,LPARAM data,bool blocked){
 if((message==WM_KEYUP||message==WM_SYSKEYUP)&&key==VK_RETURN&&command_return_held.exchange(false))return true;
 if((message==WM_KEYUP||message==WM_SYSKEYUP)&&(key==VK_TAB||key==VK_UP||key==VK_DOWN)&&completion_key_held.exchange(false))return true;
 if(message==WM_CHAR&&((key==13&&command_return_held.load())||(key==9&&completion_key_held.load())))return true;
 if(message==WM_KILLFOCUS||message==WM_DESTROY||(message==WM_ACTIVATEAPP&&!key)){cancel_slash();return false;}
 if(blocked||!shortcut_enabled()){if(slash_pending.load()||slash_key_held.load())cancel_slash();return false;}
 if((message==WM_KEYUP||message==WM_SYSKEYUP)&&int(key)==slash_key_held.load()){slash_key_held=0;return true;}
 if((message==WM_KEYUP||message==WM_SYSKEYUP)&&key==VK_RETURN&&slash_enter_held.exchange(false))return true;
 if(message==WM_KEYDOWN||message==WM_SYSKEYDOWN){
  if((key==VK_RETURN||key==VK_TAB||key==VK_UP||key==VK_DOWN)&&queue_command_key(key))return true;
  if(int(key)==slash_key_held.load()&&slash_key_held.load())return true;
  if(slash_pending.load()&&key==VK_ESCAPE){cancel_slash();return true;}
  if(slash_pending.load()&&key==VK_RETURN){slash_enter_held=true;return true;}
  if(!(data&(1LL<<30))&&translated_slash(key,data)&&arm_slash()){slash_key_held=int(key);return true;}
 }
 if(message!=WM_CHAR)return false;
 if(key==L'/'&&slash_key_held.load())return true;
 if(slash_pending.load()){std::lock_guard<std::mutex> lock(slash_mutex);if(slash_pending.load()){if(key==8){if(!slash_request.text.empty()){auto ch=slash_request.text.back();slash_request.text.pop_back();if(ch>=0xdc00&&ch<=0xdfff&&!slash_request.text.empty()&&slash_request.text.back()>=0xd800&&slash_request.text.back()<=0xdbff)slash_request.text.pop_back();}}else if(key>=32&&key<=0xffff&&slash_request.text.size()<256)slash_request.text.push_back(wchar_t(key));return true;}}
 if(key==L'/'&&!(data&(1LL<<30))&&!(GetKeyState(VK_CONTROL)&0x8000)&&!(GetKeyState(VK_MENU)&0x8000)&&arm_slash())return true;
 return false;
}
inline bool shortcut_chat_update_for(uintptr_t world,unsigned char transport){
 struct DeliverReplies {uintptr_t world;~DeliverReplies(){ssc_local_chat::pump(world,GetTickCount64());}} deliver{world};
 if(process_command_input(world,transport))return false;
 update_completion(world);
 if(!slash_pending.load())return original_chat_update(world);
 SlashRequest pending;bool delegate=false;{std::lock_guard<std::mutex> lock(slash_mutex);if(!slash_pending.load()||slash_request.processing)delegate=true;else{pending=slash_request;slash_request.processing=true;}}if(delegate)return original_chat_update(world);
 uintptr_t current=0,actor=0;auto now=GetTickCount64();unsigned char old=2;auto press=public_chat_press();
 if(!chat_context(current,actor,true)||current!=world||pending.world!=world||pending.actor!=actor||now<pending.at||now-pending.at>1000||!press||!ssc_names::read(press,old)||old!=0||!shortcut_write(press,static_cast<unsigned char>(1))){cancel_slash();return original_chat_update(world);}
 struct Restore{uintptr_t p;~Restore(){if(public_chat_press()==p)shortcut_write(p,static_cast<unsigned char>(0));}} restore{press};
 (void)original_chat_update(world);
 std::wstring text;bool owned=false;{std::lock_guard<std::mutex> lock(slash_mutex);if(slash_pending.load()&&slash_request.world==world){text=slash_request.text;owned=true;}slash_request={};slash_pending=false;}if(!owned)return false;
 unsigned char active=0;if(!ssc_names::read(world+0x6ba0,active)||active!=1||!set_chat_text)return false;
 auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0,nullptr,nullptr);if(count<0||(!text.empty()&&count==0))return false;std::string utf8(size_t(count),'\0');if(count)WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),int(text.size()),utf8.data(),count,nullptr,nullptr);
 const auto input=world+0x6bb0;set_chat_text(input,utf8.c_str());uint64_t length=0;if(ssc_names::read(input+0x1d8,length)&&length<=256){shortcut_write(input+0x170,uint32_t(length));shortcut_write(input+0x174,uint32_t(length));shortcut_write(input+0x7c,uint32_t(length));}
 // Opening the command field never submits a message, even with Return held.
 return false;
}
inline bool shortcut_chat_update(uintptr_t world){return shortcut_chat_update_for(world,0);}
inline bool shortcut_chat_update_single(uintptr_t world){return shortcut_chat_update_for(world,1);}
inline bool attach_shortcut(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 auto read=[](uintptr_t p,auto& v){return ssc_names::read(p,v);};if(!attached||!validate_chat_shortcut(base,read))return false;
 constexpr uint32_t sites[]={0x223411,0x2e9f14};for(auto rva:sites){std::array<unsigned char,5> bytes{};int32_t rel=0;if(!ssc_names::read_bytes(base+rva,bytes.data(),5)||bytes[0]!=0xe8)return false;std::memcpy(&rel,bytes.data()+1,4);if(base+rva+5+rel!=base+0x317f70)return false;}
 unsigned char* bridge=nullptr;for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;const unsigned char jump[]={0xff,0x25,0,0,0,0};const uintptr_t targets[]={reinterpret_cast<uintptr_t>(shortcut_chat_update_single),reinterpret_cast<uintptr_t>(shortcut_chat_update)};for(int i=0;i<2;++i){std::memcpy(bridge+i*16,jump,6);std::memcpy(bridge+i*16+6,&targets[i],8);}DWORD old=0;if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,32);
 DWORD protection[2]{};for(int i=0;i<2;++i)if(!VirtualProtect(reinterpret_cast<void*>(base+sites[i]),5,PAGE_EXECUTE_READWRITE,&protection[i])){for(int j=i-1;j>=0;--j){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+sites[j]),5,protection[j],&ignored);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 popup_anchor_supported=validate_popup_anchor(base,read);shortcut_base=base;original_chat_update=reinterpret_cast<ChatUpdate>(base+0x317f70);set_chat_text=reinterpret_cast<SetChatText>(base+0x9a46d0);for(int i=0;i<2;++i){auto rva=sites[i];int32_t rel=int32_t(reinterpret_cast<uintptr_t>(bridge+i*16)-(base+rva+5));std::memcpy(reinterpret_cast<void*>(base+rva+1),&rel,4);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(base+rva),5);}for(int i=1;i>=0;--i){DWORD ignored;VirtualProtect(reinterpret_cast<void*>(base+sites[i]),5,protection[i],&ignored);}return true;
}
}
