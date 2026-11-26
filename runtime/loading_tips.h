#pragma once
#include "challenge_modes.h"
#include "loading_tip_guards.h"
#include <atomic>
#include <mutex>
#include <filesystem>
#include <fstream>
namespace ssc_loading_tips {
inline std::atomic<bool> enabled{true};inline bool attached=false;inline uintptr_t image=0;
inline constexpr const char* messages[]={
 "Tip: if you see Mang on a roof, run",
 "BREAKING NEWS: Jyxalag crashes out for the 30th time on live television about losing to invis, boycotts SSC",
 "say that again",
 "SSC mod staff has conducted an investigation on themselves, found no wrongdoing",
 "BREAKING NEWS: a mutant named BOXCAT somehow got flashbangs again and is on the run after using them on Jynxalag and Chill",
 "-- .- -.. . / -.-- --- ..- / .-.. --- --- -.- -.-.--",
 "9/10 doctors recommend using radiate as part of a healthy bedtime routine!",
 "BREAKING NEWS: Due to popular community demand, bots WILL be nerfed in the next patch (for real this time)",
 "yeah thats the problem oshiri is that the game is really just a game of luck. if you get dealt bad skills and aren't the lucky one that finds the good merchant, your pretty much destined to be at a huge disadvantage all match. after you fart putter across the map to the next merchant, meanwhile being most likely killed by said players that got the better guns. I feel like making all shops carry the same weapons per round would be a great idea. Why make it 100% about luck? Mouthbreathers that live on this game most likely have everything mapped out, and just play the same game over and over repeating their steps to power level and gain advantage over those just trying to enjoy a game. Having to hold a trigger the whole match is pretty annoying as well. Can't we just move at one speed? A normal speed? It should not take over a full minute to cross the map fart puttering. If I have to go find a better gun your telling me I am spending half the round solely fart puttering? This game has an amazing template, although I feel as if it has catered to the top players that open their pocketbooks, and effort needs to be made to make it a fun game for ALL, not just those that play 50 matches per day and have the map and synergies memorized.",
 "(0_0)",
 "(Well, there is a man here.)",
 "snibble:32",
 "BREAKING NEWS: 808 iron monkeys have taken over the city",
 "BREAKING NEWS: mutant blinded after getting flashbanged on 1.5 brightness",
 "W.D. Caster is recruiting! Call 248-434-5508 to apply!",
 "WIRK_pGdIdA",
 "1iOqIC3LrdY",
 "iOztnsBPrAA",
 "FIXING NEWS: sorry for the delay",
 "who is jix",
 "BREAKING NEWS: Chill quits SSC and uninstalls, then reinstalls within two hours"
};

inline constexpr size_t message_count=sizeof(messages)/sizeof(messages[0]);
inline constexpr int long_tip=8; // Stable ID: new tips are appended after the long paragraph.
inline std::once_flag selection;inline int selected=-1;
template<class Next>int choose(size_t native_count,Next next){if(!native_count||native_count>50||native_count+message_count>54)return -1;size_t index=0;if(!ssc_challenge::uniform(native_count+message_count,next,index)||index<native_count)return -1;return int(index-native_count);}
template<class Read>bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
struct Layout {float x=0,y=0,width=0,height=0,size=0;int lines=0;float clip_y=0,clip_height=0;};
template<class Measure> Layout fit_bottom(int w,int h,float base_size,Measure measure){
 Layout out;if(w<320||h<240||w>16384||h>16384||!std::isfinite(base_size)||base_size<=0)return out;
 const float margin=std::max(12.f,w*.025f),top=h*.68f,bottom=h*.97f;
 out.x=margin;out.y=top;out.width=w-2*margin;out.height=bottom-top;out.clip_y=top;out.clip_height=bottom-top;
 float lo=.025f,hi=std::clamp(base_size*h/720.f*.82f,.025f,8.f);int count=0;
 auto fits=[&](float size){count=measure(out.width,size);return count>0&&count<=10000&&(count+2)*32.f*size<=out.height;};
 if(!fits(lo))return Layout{};
 for(int i=0;i<16;++i){float mid=(lo+hi)*.5f;if(fits(mid))lo=mid;else hi=mid;}
 out.size=lo;out.lines=measure(out.width,out.size);// Native glyphs extend above their line origin: keep that padding inside a fixed clip box.
 out.y+=32.f*out.size;out.height=bottom-out.y;return out;
}
struct BottomClip {
 GLboolean was=GL_FALSE;GLint previous[4]{};bool active=false;
 BottomClip(const Layout& l,int w,int h){GLint vp[4]{};glGetIntegerv(GL_VIEWPORT,vp);if(vp[2]<=0||vp[3]<=0)return;was=glIsEnabled(GL_SCISSOR_TEST);glGetIntegerv(GL_SCISSOR_BOX,previous);
  int left=vp[0]+int(std::ceil(l.x/w*vp[2])),right=vp[0]+int(std::floor((l.x+l.width)/w*vp[2]));
  const float clip_y=l.clip_height>0?l.clip_y:l.y,clip_height=l.clip_height>0?l.clip_height:l.height;
  int bottom=vp[1]+int(std::ceil((h-clip_y-clip_height)/h*vp[3])),top=vp[1]+int(std::floor((h-clip_y)/h*vp[3]));
  if(was){left=std::max(left,previous[0]);right=std::min(right,previous[0]+previous[2]);bottom=std::max(bottom,previous[1]);top=std::min(top,previous[1]+previous[3]);}
  glEnable(GL_SCISSOR_TEST);glScissor(left,bottom,std::max(0,right-left),std::max(0,top-bottom));active=true;
 }
 ~BottomClip(){if(active){glScissor(previous[0],previous[1],previous[2],previous[3]);if(!was)glDisable(GL_SCISSOR_TEST);}}
};
inline ssc_names::Wrapped original=nullptr;
inline uintptr_t cached_renderer=0;inline int cached_w=0,cached_h=0;inline float cached_base=0;inline Layout cached_layout;
inline int wrapped(uintptr_t renderer,float x,float y,float width,float height,void* name,float size,float r,float g,float b,float alpha,int align,int vertical,int first,unsigned char flags,int limit,float start,float end,float phase){
 std::call_once(selection,[]{uintptr_t begin=0,last=0;if(ssc_names::read(image+0xfb65e8,begin)&&ssc_names::read(image+0xfb65f0,last)&&begin&&last>=begin&&(last-begin)%4==0)selected=choose(size_t((last-begin)/4),ssc_challenge::random_word);});
 if(!enabled.load(std::memory_order_relaxed)||selected<0||size_t(selected)>=message_count)return original(renderer,x,y,width,height,name,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
 const char* text=messages[selected];const size_t n=std::strlen(text);ssc_chat::NativeString literal{reinterpret_cast<uintptr_t>(text),0,n,std::max(size_t(16),n)};
 if(selected!=long_tip)return original(renderer,x,y,width,height,&literal,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
 int w=0,h=0;if(!ssc_names::read(image+0xfb6ae8,w)||!ssc_names::read(image+0xfb6aec,h)||w<320||h<240)return original(renderer,x,y,width,height,&literal,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
 if(cached_renderer!=renderer||cached_w!=w||cached_h!=h||cached_base!=size||cached_layout.size<=0){cached_layout=fit_bottom(w,h,size,[&](float line_width,float scale){return original(renderer,0,0,line_width,0,&literal,scale,r,g,b,0,0,0,0,1,9999,start,end,phase);});cached_renderer=renderer;cached_w=w;cached_h=h;cached_base=size;}
 const auto& l=cached_layout;if(l.size<=0)return original(renderer,x,y,width,height,&literal,size,r,g,b,alpha,align,vertical,first,flags,limit,start,end,phase);
 BottomClip clip(l,w,h);const int drawn=original(renderer,l.x+l.width*.5f,l.y,l.width,l.height,&literal,l.size,r,g,b,alpha,1,0,0,1,9999,start,end,phase);return drawn;
}
inline bool attach(){
 image=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));if(!validate(image,[](uintptr_t p,auto& v){return ssc_names::read(p,v);}))return false;
 auto site=reinterpret_cast<unsigned char*>(image+0x7468ac);int32_t before=0;std::memcpy(&before,site+1,4);if(site[0]!=0xe8||uintptr_t(site+5+before)!=image+0x4823b0)return false;
 unsigned char* bridge=nullptr;for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((image+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};std::memcpy(bridge,jump,6);auto target=reinterpret_cast<uintptr_t>(wrapped);std::memcpy(bridge+6,&target,8);DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,14);
 if(!VirtualProtect(site,5,PAGE_EXECUTE_READWRITE,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}
 original=reinterpret_cast<ssc_names::Wrapped>(image+0x4823b0);int32_t relative=int32_t(bridge-(site+5));std::memcpy(site+1,&relative,4);FlushInstructionCache(GetCurrentProcess(),site,5);DWORD ignored;VirtualProtect(site,5,old,&ignored);return true;
}
}
