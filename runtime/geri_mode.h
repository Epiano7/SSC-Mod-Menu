#pragma once
#include "compatibility.h"
#include <atomic>
namespace ssc_geri {
inline std::atomic<bool> enabled{false};
inline bool attached=false;
using Panel=void(*)(uintptr_t);
inline Panel original=nullptr;
// This call encloses the skill draft/reroll/syringe UI, not the currency HUD.
// Suppress its rendering and its own interaction handlers together. No actor,
// inventory, draft offer, skill-point or syringe data is modified.
inline void panel(uintptr_t context){if(!enabled.load()&&original)original(context);}
// The world-click dispatcher tests this separately from drawing the panel.
// Returning false skips only the skill-panel hit region; its next inventory and
// other UI tests still run normally. No persistent game UI/actor state is changed.
using HitTest=bool(*)(uintptr_t);
inline HitTest input_original=nullptr;
inline bool captures_input(uintptr_t actor){
 return !enabled.load()&&input_original&&input_original(actor);
}
inline bool attach(){
 if(!ssc_compat::supports(128))return false;
 const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
 struct Site {uint32_t call,target;uintptr_t hook;};
 const Site sites[]={
  {0x3b8f3d,0x3d78b0,reinterpret_cast<uintptr_t>(panel)},
  {0x7d11a5,0x7f4af0,reinterpret_cast<uintptr_t>(captures_input)}
 };
 unsigned char* addresses[2]{};uintptr_t targets[2]{};int32_t saved[2]{};
 for(size_t i=0;i<2;++i){
  const auto call=ssc_compat::resolve(sites[i].call),target=ssc_compat::resolve(sites[i].target);
  if(!call||!target)return false;
  addresses[i]=reinterpret_cast<unsigned char*>(base+call);targets[i]=base+target;
  std::memcpy(&saved[i],addresses[i]+1,4);
  if(addresses[i][0]!=0xe8||base+call+5+saved[i]!=targets[i])return false;
 }
 unsigned char* bridge=nullptr;
 for(uintptr_t distance=0x10000;distance<0x60000000&&!bridge;distance+=0x10000)
  bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+distance)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};
 for(size_t i=0;i<2;++i){std::memcpy(bridge+16*i,jump,6);std::memcpy(bridge+16*i+6,&sites[i].hook,8);}
 DWORD old=0;if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}
 FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 // Acquire both pages before changing either call: failed installation leaves
 // the original rendering AND input behavior together.
 DWORD protection[2]{};
 for(size_t i=0;i<2;++i){
  if(!VirtualProtect(addresses[i],5,PAGE_EXECUTE_READWRITE,&protection[i])){
   for(size_t j=0;j<i;++j){DWORD ignored=0;VirtualProtect(addresses[j],5,protection[j],&ignored);}
   VirtualFree(bridge,0,MEM_RELEASE);return false;
  }
 }
 original=reinterpret_cast<Panel>(targets[0]);input_original=reinterpret_cast<HitTest>(targets[1]);
 for(size_t i=0;i<2;++i){
  const auto relative=static_cast<int32_t>(reinterpret_cast<uintptr_t>(bridge+16*i)-reinterpret_cast<uintptr_t>(addresses[i]+5));
  std::memcpy(addresses[i]+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),addresses[i],5);
 }
 for(size_t i=2;i>0;--i){DWORD ignored=0;VirtualProtect(addresses[i-1],5,protection[i-1],&ignored);}
 return true;
}
}
