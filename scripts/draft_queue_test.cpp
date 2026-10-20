#include "../runtime/overlay.cpp"
#include <cassert>
#include <iostream>
template<class T> void put(uintptr_t p,T v){std::memcpy(reinterpret_cast<void*>(p),&v,sizeof(v));}

static void native_panel(uintptr_t,uintptr_t,float,uint64_t,char,char){}
int main(){
 std::vector<unsigned char> image(0x1000000),context(0x3200),actor(0x3500),world(0x7000),offers(36),old_offers(36);
 uintptr_t g=reinterpret_cast<uintptr_t>(image.data()),c=reinterpret_cast<uintptr_t>(context.data()),a=reinterpret_cast<uintptr_t>(actor.data()),w=reinterpret_cast<uintptr_t>(world.data()),o=reinterpret_cast<uintptr_t>(offers.data()),old=reinterpret_cast<uintptr_t>(old_offers.data());
 ssc_advisor::game=g;ssc_advisor::private_access=+[](){return true;};ssc_advisor::use_api=false;ssc_advisor::original_panel=native_panel;
 put(g+0xfb159c,32.f);put(c,a);put(g+0xde95d0,w);put(a+0x78,0);put(w+0x6b98,0);put(a+0x7dc,0);put(a+0x81,(unsigned char)1);put(a+0x350,3);put(a+0x1240,2);
 put(g+0xe04350,o);put(g+0xe04358,o+36);put(a+0x558,old);put(a+0x560,old+36);put(g+0xfb0ac0,1920);put(g+0xfb0ac4,1080);
 for(int i=0;i<3;++i){put(o+i*12,i<2?i:-1);put(old+i*12,i<2?i:-1);auto button=c+0x310+i*0x170;put(button+0x14,100+i*200);put(button+0x18,50);put(button+0x1c,180);put(button+0x20,80);put(button+0x44,(unsigned char)1);}
 auto panel=[&](){ssc_advisor::panel(c,0,0,0,0,0);};panel();assert(ssc_advisor::border_ready());assert(ssc_advisor::recommended==0||ssc_advisor::recommended==1);
 put(g+0xfb159c,31.9f);panel();assert(!ssc_advisor::border_ready());put(g+0xfb159c,32.f);
 // Select one of two queued drafts without releasing Ctrl. Native input gates
 // remain disabled and actor copy lags, while the displayed list advances.
 put(a+0x1240,1);put(o,2);put(o+12,3);for(int i=0;i<3;++i)put(c+0x310+i*0x170+0x44,(unsigned char)0);
 panel();assert(ssc_advisor::border_ready());assert(ssc_advisor::recommended==2||ssc_advisor::recommended==3);
 put(g+0xfb159c,0.f);panel();assert(!ssc_advisor::border_ready());put(g+0xfb159c,32.f);panel();assert(ssc_advisor::border_ready());
 put(c+0x3148,.3f);panel();assert(!ssc_advisor::border_ready());put(c+0x3148,0.f);panel();assert(ssc_advisor::border_ready());
 put(o+12,2);panel();assert(!ssc_advisor::border_ready());put(o+12,3);
 put(a+0x1240,0);panel();assert(!ssc_advisor::border_ready());put(a+0x1240,1);put(w+0x6b98,1);panel();assert(!ssc_advisor::border_ready());
 // Training snapshot uses the native context, independent of key/panel state.
 auto native=g+0xe03e98;put(native+0x2b0,3);put(native+0x5ec,1);put(native+0x4b8,o);put(native+0x4c0,o+36);put(native+0x4c8,o+36);put(native+0x4d0,old);put(native+0x4d8,old+36);put(native+0x4e0,old+36);
 auto reader=[](uintptr_t p,auto& v){return ssc_advisor::read(p,v);};ssc_training::NativeDraft draft;
 assert(ssc_training::read_native_draft(native,draft,reader));assert(draft.counter==1&&draft.offers[0]==2&&draft.previous[0]==0);
 put(native+0x4c0,o+37);assert(!ssc_training::read_native_draft(native,draft,reader));put(native+0x4c0,o+36);put(o,54);assert(!ssc_training::read_native_draft(native,draft,reader));put(o,2);
 assert(!ssc_training::native_supported()); // An unknown executable must fail closed.
 std::cout<<"PASS: native selection visibility and queued drafts; bounded training snapshot and unknown-build guard\n";
}
