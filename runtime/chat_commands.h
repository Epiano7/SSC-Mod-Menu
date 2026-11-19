#pragma once
#include "quick_chat.h"
#include "chat_commands_core.h"
#include "chat_completion.h"
#include <mutex>
#include <deque>
#include "kos_native.h"
#include "planned_native_guards.h"
#include "local_chat_feedback.h"
namespace ssc_commands {
inline Router router;
inline bool attached=false;
inline std::string last_reply;
inline std::string sender_name="JARVIS";
inline std::atomic<uint64_t> revision{0};
inline std::string (*execute)(const std::string&,const std::string&)=nullptr;
using Sender=ssc_chat::Sender;
inline Sender original_sender=nullptr;
inline std::string (*team_prefix)()=+[](){return ssc_planned_native::commands?ssc_rpc::native_string(ssc_chat::image_base+0xfba3b0):std::string{};};
inline void reply(const std::string& text){if(text.empty())return;last_reply=text.substr(0,2048);++revision;ssc_local_chat::enqueue(sender_name,text,GetTickCount64());}
inline void typed_send(uintptr_t world,unsigned char transport,uintptr_t actor,const void* message){
 if(!attached){original_sender(world,transport,actor,message);return;}
 Router kos_router;kos_router.enabled=true;kos_router.bindings={{"kos","kos"}};Router slash_router=router;slash_router.prefix='/';
 const auto original_text=ssc_rpc::native_string(reinterpret_cast<uintptr_t>(message));
 auto prefix=team_prefix();const bool valid_prefix=!prefix.empty()&&prefix.size()<=32&&prefix.find_first_of("\r\n")==std::string::npos;
 const bool originally_team=valid_prefix&&original_text.compare(0,prefix.size(),prefix)==0;
 const auto text=originally_team?original_text.substr(prefix.size()):original_text;
 const auto first=text.find_first_not_of(" \t");
 auto end=first==std::string::npos?first:text.find_first_of(" \t",first);auto name=first==std::string::npos?std::string{}:ssc_kos::canonical(text.substr(first,end==std::string::npos?end:end-first));const bool legacy_kos=ssc_kos::enabled&&name=="!kos";const bool slash=first!=std::string::npos&&text[first]=='/';if(legacy_kos)kos_router.prefix='!';
 if(!router.enabled&&!legacy_kos&&!slash&&!(first!=std::string::npos&&text.compare(first,2,"./")==0)){original_sender(world,transport,actor,message);return;}
 auto& active_router=legacy_kos?kos_router:slash?(router.enabled?slash_router:kos_router):router;
 if(slash&&!router.enabled&&!ssc_kos::enabled)kos_router.enabled=false;
 const bool prefixed=first!=std::string::npos&&text[first]==active_router.prefix;
 // This wrapper is used ONLY at both reviewed local typed-chat call sites.
 // Never forward a private command if local actor validation is unavailable.
 if(!ssc_chat::local_actor(actor)){if(prefixed){reply("Local player unavailable");return;}original_sender(world,transport,actor,message);return;}
 auto result=active_router.route(text,execute);
 if(result.consumed){if(result.action=="team"&&!valid_prefix){router.channel=Channel::unchanged;reply("Team chat unavailable on this game build");}else reply(result.reply);return;}
 // The native sender flag controls a transport path, NOT the chat audience.
 // The game's packet constructor recognizes and removes this exact prefix.
 if(result.channel==Channel::team||(result.channel==Channel::unchanged&&originally_team)){if(!valid_prefix){reply("Team chat unavailable; message was not sent");return;}if(result.outgoing.compare(0,prefix.size(),prefix)!=0)result.outgoing=prefix+result.outgoing;}
 else if(result.channel==Channel::public_chat&&valid_prefix&&result.outgoing.compare(0,prefix.size(),prefix)==0)result.outgoing.erase(0,prefix.size());
 if(result.outgoing==original_text){original_sender(world,transport,actor,message);return;}
 ssc_chat::NativeString literal{reinterpret_cast<uintptr_t>(result.outgoing.data()),0,result.outgoing.size(),std::max(size_t(16),result.outgoing.size())};
 original_sender(world,transport,actor,&literal);
}
inline bool attach(uintptr_t base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){
 if(!ssc_planned_native::commands)return false;
 constexpr uint32_t calls[]={0x223540,0x2ea2da};
 for(auto rva:calls){std::array<unsigned char,5> bytes{};int32_t rel=0;if(!ssc_names::read_bytes(base+rva,bytes.data(),5)||bytes[0]!=0xe8)return false;std::memcpy(&rel,bytes.data()+1,4);if(base+rva+5+rel!=base+0x223650)return false;}
 unsigned char* bridge=nullptr;for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};auto target=reinterpret_cast<uintptr_t>(typed_send);std::memcpy(bridge,jump,6);std::memcpy(bridge+6,&target,8);DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,14);
 DWORD protections[2]{};for(int i=0;i<2;++i)if(!VirtualProtect(reinterpret_cast<void*>(base+calls[i]),5,PAGE_EXECUTE_READWRITE,&protections[i])){for(int j=i-1;j>=0;--j){DWORD unused;VirtualProtect(reinterpret_cast<void*>(base+calls[j]),5,protections[j],&unused);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 original_sender=reinterpret_cast<Sender>(base+0x223650);
 for(auto rva:calls){int32_t rel=int32_t(reinterpret_cast<uintptr_t>(bridge)-(base+rva+5));std::memcpy(reinterpret_cast<void*>(base+rva+1),&rel,4);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(base+rva),5);}
 for(int i=1;i>=0;--i){DWORD unused;VirtualProtect(reinterpret_cast<void*>(base+calls[i]),5,protections[i],&unused);}return true;
}
}

#include "chat_shortcut.h"
