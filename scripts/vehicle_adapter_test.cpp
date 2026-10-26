#include "../runtime/vehicle_adapter.h"
#include <cassert>
#include <vector>
#include <fstream>
#include <iostream>
#include <limits>
using namespace ssc_vehicle;
static unsigned calls=0;static unsigned char observed[4];
static void native(uintptr_t car){++calls;std::memcpy(observed,reinterpret_cast<void*>(car+0x14f),4);}
template<class T> void put(uintptr_t p,size_t off,T v){std::memcpy(reinterpret_cast<void*>(p+off),&v,sizeof v);}
int main(int argc,char**argv){
 std::vector<unsigned char> im(0x1000000),wm(0xc100),am(0x34b0),vm(0x958),sm(0x68);
 image=reinterpret_cast<uintptr_t>(im.data());auto w=reinterpret_cast<uintptr_t>(wm.data()),a=reinterpret_cast<uintptr_t>(am.data()),v=reinterpret_cast<uintptr_t>(vm.data()),seat=reinterpret_cast<uintptr_t>(sm.data());
 put(image,0xdea5d0,w);put(w,0x6a38,(unsigned char)1);put(w,0x6a39,(unsigned char)1);put(w,0x160,1);put(w,0x128,6);put(w,0x485,(unsigned char)1);put(w,0xa650,a);put(w,0xa658,a+0x34b0);put(w,0x6b98,0);put(a,0x2ad8,v);put(a,0x81,(unsigned char)1);put(a,0x13a4,100.f);put(v,0x368,seat);put(v,0x370,seat+0x68);put(seat,0x60,a);
 original=native;enabled=true;blocked=false;
 auto run=[&](float angle){put(v,0x90,angle);const unsigned char input[]={1,0,0,0};std::memcpy(vm.data()+0x14f,input,4);mapper.reset();control(v);};
 run(270);assert(observed[0]==1);run(0);assert(observed[2]==1);run(90);assert(observed[1]==1);run(180);assert(observed[3]==1);
 auto unchanged=[&]{run(90);assert(observed[0]==1&&observed[1]==0&&observed[2]==0&&observed[3]==0&&mapper.sector==-1);};
 enabled=false;unchanged();enabled=true;blocked=true;unchanged();blocked=false;
 put(w,0x6a38,(unsigned char)0);unchanged();put(w,0x6a38,(unsigned char)1);
 put(w,0x160,2);unchanged();put(w,0x160,1);
 put(w,0x120,(unsigned char)1);unchanged();put(w,0x120,(unsigned char)0);
 put(w,0x485,(unsigned char)0);unchanged();put(w,0x485,(unsigned char)1);
 put(seat,0x60,uintptr_t(0));unchanged();put(seat,0x60,a);
 put(a,0x7dc,1);unchanged();put(a,0x7dc,0);
 put(a,0x13a4,0.f);unchanged();put(a,0x13a4,100.f);
 put(a,0x8a0,3);unchanged();put(a,0x8a0,0);
 put(w,0x6b98,128);unchanged();put(w,0x6b98,0);
 put(v,0x370,seat);unchanged();put(v,0x370,seat+0x68);
 put(image,0xfb1cc0,10.f);unchanged();put(image,0xfb1cc0,0.f);
 run(std::numeric_limits<float>::quiet_NaN());assert(observed[0]==1&&mapper.sector==-1);
 unsigned char all[]={1,1,1,1};std::memcpy(vm.data()+0x14f,all,4);put(v,0x90,90.f);control(v);assert(std::memcmp(all,observed,4)==0);
 vm[0x14e]=7;vm[0x153]=9;run(90);assert(vm[0x14e]==7&&vm[0x153]==9);assert(calls==20);
 // Updates of other cars must not erase our boundary hysteresis.
 mapper.reset();mapper.buttons({true,false,false,false},44,a,v);control(v+16);assert(mapper.vehicle==v);auto boundary=mapper.buttons({true,false,false,false},48,a,v);assert(boundary.forward);
 // User's requested example: nose points left (native yaw 180):
 // A=forward, D=reverse, W=right steering, S=left steering.
 put(v,0x90,180.f);
 const unsigned char inputs[4][4]={{0,0,1,0},{0,0,0,1},{1,0,0,0},{0,1,0,0}};
 const unsigned char outputs[4][4]={{1,0,0,0},{0,1,0,0},{0,0,0,1},{0,0,1,0}};
 for(int i=0;i<4;++i){mapper.reset();std::memcpy(vm.data()+0x14f,inputs[i],4);control(v);assert(std::memcmp(observed,outputs[i],4)==0);}
 assert(!matches(im.data(),im.size()));
 if(argc>1){std::ifstream f(argv[1],std::ios::binary);assert(f);std::vector<char> data((std::istreambuf_iterator<char>(f)),{});auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(data.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(data.data()+dos->e_lfanew);std::vector<unsigned char> mapped(nt->OptionalHeader.SizeOfImage);auto sections=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)std::memcpy(mapped.data()+sections[i].VirtualAddress,data.data()+sections[i].PointerToRawData,sections[i].SizeOfRawData);assert(matches(mapped.data(),mapped.size()));for(size_t off:{size_t(0x9e1200),size_t(0x9e1420),size_t(0x9d8d00)}){mapped[off]^=1;assert(!matches(mapped.data(),mapped.size()));mapped[off]^=1;}assert(!matches(mapped.data(),0x9e1425));}
 std::cout<<"PASS: native control adapter, four headings, local driver, solo-only gate, menus/freecam gate, seat/account bounds, native passthrough, conflicting keys, invalid state and executable guards\n";
}

