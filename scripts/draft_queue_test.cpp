#include "../runtime/overlay.cpp"
#include <cassert>
#include <iostream>
template<class T> void put(uintptr_t p,T v){std::memcpy(reinterpret_cast<void*>(p),&v,sizeof(v));}
static bool held=true;
static void native_panel(uintptr_t,uintptr_t,float,uint64_t,char,char){}
int main(){
 std::vector<unsigned char> image(0x1000000),context(0x3200),actor(0x3500),world(0x7000),offers(36),old_offers(36);
 uintptr_t g=reinterpret_cast<uintptr_t>(image.data()),c=reinterpret_cast<uintptr_t>(context.data()),a=reinterpret_cast<uintptr_t>(actor.data()),w=reinterpret_cast<uintptr_t>(world.data()),o=reinterpret_cast<uintptr_t>(offers.data()),old=reinterpret_cast<uintptr_t>(old_offers.data());
 ssc_advisor::game=g;ssc_advisor::private_access=+[](){return true;};ssc_advisor::selection_held=+[](){return held;};ssc_advisor::use_api=false;ssc_advisor::original_panel=native_panel;
 put(c,a);put(g+0xde85d0,w);put(a+0x78,0);put(w+0x6b98,0);put(a+0x7dc,0);put(a+0x81,(unsigned char)1);put(a+0x350,3);put(a+0x1240,2);
 put(g+0xe03350,o);put(g+0xe03358,o+36);put(a+0x558,old);put(a+0x560,old+36);put(g+0xfafa40,1920);put(g+0xfafa44,1080);
 for(int i=0;i<3;++i){put(o+i*12,i<2?i:-1);put(old+i*12,i<2?i:-1);auto button=c+0x310+i*0x170;put(button+0x14,100+i*200);put(button+0x18,50);put(button+0x1c,180);put(button+0x20,80);put(button+0x44,(unsigned char)1);}
 auto panel=[&](){ssc_advisor::panel(c,0,0,0,0,0);};panel();assert(ssc_advisor::border_ready());assert(ssc_advisor::recommended==0||ssc_advisor::recommended==1);
 // Select one of two queued drafts without releasing Ctrl. Native input gates
 // remain disabled and actor copy lags, while the displayed list advances.
 put(a+0x1240,1);put(o,2);put(o+12,3);for(int i=0;i<3;++i)put(c+0x310+i*0x170+0x44,(unsigned char)0);
 panel();assert(ssc_advisor::border_ready());assert(ssc_advisor::recommended==2||ssc_advisor::recommended==3);
 held=false;panel();assert(!ssc_advisor::border_ready());held=true;panel();assert(ssc_advisor::border_ready());
 put(c+0x3148,.3f);panel();assert(!ssc_advisor::border_ready());put(c+0x3148,0.f);panel();assert(ssc_advisor::border_ready());
 put(o+12,2);panel();assert(!ssc_advisor::border_ready());put(o+12,3);
 put(a+0x1240,0);panel();assert(!ssc_advisor::border_ready());put(a+0x1240,1);put(w+0x6b98,1);panel();assert(!ssc_advisor::border_ready());
 std::cout<<"PASS: queued drafts refresh while Ctrl held; input-disabled buttons, actor lag, key release, transition, duplicate offers, empty queue, nonlocal guards\n";
}
