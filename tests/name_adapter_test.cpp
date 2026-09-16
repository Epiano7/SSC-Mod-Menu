#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <iterator>
#include <vector>
#include <cassert>
#include <iostream>
#include "../runtime/cosmetic_adapter.h"
static float rr,gg,bb,aa,pp;
static float capture(void* renderer,float x,float y,void* name,float size,float r,float g,float b,float alpha,int align,float width,unsigned char depth,unsigned char shadow,float phase){
    assert(renderer==(void*)1&&x==2&&y==3&&name==(void*)4&&size==5&&align==10&&width==11&&depth==12&&shadow==13);
    rr=r;gg=g;bb=b;aa=alpha;pp=phase;return 42.f;
}
extern "C" float test_score_call(void*,float,float,void*,float,float,float,float,float,int,float,unsigned char,unsigned char,float,uintptr_t);
template<class T> void put(uintptr_t p,T v){std::memcpy((void*)p,&v,sizeof(v));}
void small_string(uintptr_t p,const char* s){size_t n=std::strlen(s);assert(n<16);std::memcpy((void*)p,s,n+1);put(p+16,n);put(p+24,size_t(15));}
static unsigned char native_result=0;
static unsigned char consume(void*,int,void* s){small_string((uintptr_t)s,"consumed");return native_result;}
int main(){
    // Bulk string reads must validate every region and never read a guard page.
    SYSTEM_INFO sys{};GetSystemInfo(&sys);size_t page=sys.dwPageSize;
    auto memory=(uintptr_t)VirtualAlloc(nullptr,page*2,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);assert(memory);
    std::array<unsigned char,32> native{};std::string decoded;
    const char* long_name="LongAccountAcrossPageBoundary";size_t n=std::strlen(long_name),cap=64;auto chars=memory+page-8;
    std::memcpy((void*)chars,long_name,n);std::memcpy(native.data(),&chars,8);std::memcpy(native.data()+16,&n,8);std::memcpy(native.data()+24,&cap,8);
    DWORD protection=0;assert(VirtualProtect((void*)(memory+page),page,PAGE_READONLY,&protection));
    assert(ssc_names::account_string((uintptr_t)native.data(),decoded)&&decoded=="longaccountacrosspageboundary");
    assert(VirtualProtect((void*)(memory+page),page,PAGE_NOACCESS,&protection));
    assert(!ssc_names::account_string((uintptr_t)native.data(),decoded));
    assert(VirtualProtect((void*)(memory+page),page,PAGE_READWRITE|PAGE_GUARD,&protection));
    assert(!ssc_names::account_string((uintptr_t)native.data(),decoded));
    assert(!ssc_names::read_bytes(UINTPTR_MAX-4,native.data(),32));
    VirtualFree((void*)memory,0,MEM_RELEASE);
    std::vector<unsigned char> image(0x1000000),world(0xc000),actors(5*0x3460),row(0xd0);
    auto base=(uintptr_t)image.data(),w=(uintptr_t)world.data(),a=(uintptr_t)actors.data(),r=(uintptr_t)row.data();
    ssc_names::image_base=base;ssc_names::native_draw=capture;
    put(base+0xddb5d0,w);put(w+0xa5b8,a);put(w+0xa5c0,a+actors.size());put(w+0xbf48,a);
    put(base+0xe0f380,0);put(base+0xe12de8,a);small_string(base+0xdf5030,"OwnerAccount");small_string(a+0x6d8,"owneraccount");
    put(base+0xf984b4,2);put(w+0x190,0);put(w+0x28c,2);
    for(int i=0;i<5;++i){put(a+i*0x3460+0x78,i);put<unsigned char>(a+i*0x3460+0x81,1);put(a+i*0x3460+0x7c4,i==0?0:1);}
    put(a+0x3460+0x7c4,0);assert(ssc_names::identify(a).local);assert(!ssc_names::identify(a+0x3460).local);
    put(w+0xbf48,a+0x3460);assert(ssc_names::identify(a).local); // Spectating another human must not change own identity.
    small_string(a+0x3460+0x780,"OwnerAccount");assert(!ssc_names::identify(a+0x3460).local);
    small_string(a+0x6d8,"OtherAccount");assert(!ssc_names::identify(a).local);small_string(a+0x6d8,"OwnerAccount");put(a+0x3460+0x7c4,1);
    auto invoke=[&](){return test_score_call((void*)1,2,3,(void*)4,5,6,7,8,.5f,10,11,12,13,14,r);};
    put(r+0x2c,2);assert(invoke()==42&&rr==6&&gg==7&&bb==8&&aa==.5f&&pp==14);
    put(r+0x2c,0);ssc_names::cosmetics=true;put(base+0xfa1e04,2.5f);put(base+0xfa2860,.5f);invoke();assert(pp==.25f&&rr==6);
    ssc_names::rainbow=false;ssc_names::solid_rgb=0x336699;invoke();assert(pp==-1&&std::abs(rr-.2f)<.0001f&&std::abs(gg-.4f)<.0001f&&std::abs(bb-.6f)<.0001f&&aa==.5f);ssc_names::rainbow=true;
    put(a+0x3460+0x7c4,0);invoke();assert(pp==.25f);
    put(base+0xe0f380,1);invoke();assert(pp==14);put(base+0xe0f380,0);
    ssc_names::native_predicate=consume;std::array<unsigned char,32> owned{};
    small_string((uintptr_t)owned.data(),"OWNERACCOUNT");
    assert(!ssc_names::unambiguous_event_account((uintptr_t)owned.data()));
    small_string(a+0x3460+0x780,"OtherPlayer");assert(ssc_names::unambiguous_event_account((uintptr_t)owned.data()));
    small_string((uintptr_t)owned.data(),"OWNERACCOUNT");assert(ssc_names::predicate(nullptr,3,owned.data())==1);assert(std::strcmp((char*)owned.data(),"consumed")==0);
    small_string((uintptr_t)owned.data(),"OtherAccount");assert(ssc_names::predicate(nullptr,3,owned.data())==0);
    native_result=1;assert(ssc_names::predicate(nullptr,3,owned.data())==1);native_result=0;
    ssc_names::cosmetics=false;small_string((uintptr_t)owned.data(),"OwnerAccount");assert(ssc_names::predicate(nullptr,3,owned.data())==0);
    float phase=.75f;put(base+0xfa1e04,3.4e38f);put(base+0xfa2860,3.4e38f);assert(!ssc_names::native_phase(phase)&&phase==.75f);
    std::cout<<"PASS: name bridge, multiplayer own-slot/account identity, spectating, display-name collision, owned-string consumption, native effect preservation and disable\n";
}
