#include "../runtime/achievement_core.h"
#include "../runtime/achievement_jumps.h"
#include <cassert>
#include <fstream>
#include <iostream>

namespace jumps=ssc_achievements::jumps;
using ssc_achievements::Engine;
using ssc_achievements::Kind;
constexpr size_t stride=0x34b8,image_size=0x1000000;
static unsigned native_calls=0;
template<class T>void write(uintptr_t address,T value){std::memcpy(reinterpret_cast<void*>(address),&value,sizeof value);}
static void queue(uintptr_t vector,const int* cue){assert(vector&&cue&&*cue==2);++native_calls;}

// Call the installed relay with the register layout of the native call site.
extern "C" void test_jump(uintptr_t relay,uintptr_t vector,const int* cue,uintptr_t actor,int pad);
asm(".text\n.globl test_jump\n.def test_jump; .scl 2; .type 32; .endef\n"
    ".seh_proc test_jump\ntest_jump:\npushq %rdi\n.seh_pushreg %rdi\npushq %r15\n.seh_pushreg %r15\n"
    "subq $0x28,%rsp\n.seh_stackalloc 0x28\n.seh_endprologue\n"
    "movq %rcx,%r11\nmovq %rdx,%rcx\nmovq %r8,%rdx\nmovq %r9,%rdi\nmovl 0x60(%rsp),%r15d\n"
    "call *%r11\naddq $0x28,%rsp\npopq %r15\npopq %rdi\nret\n.seh_endproc\n");

int main(int argc,char** argv){
 assert(argc==2);
 auto* fixture=static_cast<unsigned char*>(VirtualAlloc(nullptr,image_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 assert(fixture);
 std::ifstream input(argv[1],std::ios::binary);
 assert(input.read(reinterpret_cast<char*>(fixture),image_size));
 const auto base=reinterpret_cast<uintptr_t>(fixture),site=base+jumps::completion_site;
 const auto code=fixture+jumps::completion_site;
 const auto changed=jumps::guards[0].rva;
 int32_t original=0;
 std::memcpy(&original,code+1,4);
 fixture[changed]^=1;
 assert(!jumps::attach(base));
 int32_t relative=0;
 std::memcpy(&relative,code+1,4);
 assert(relative==original&&site+5+relative==base+jumps::queue_rva);
 fixture[changed]^=1;
 assert(jumps::attach(base));
 std::memcpy(&relative,code+1,4);
 const auto relay=site+5+relative;
 assert(relay!=base+jumps::queue_rva);
 jumps::original_queue=queue;

 std::vector<unsigned char> world_data(0x10000),actors(stride*2);
 const auto world=reinterpret_cast<uintptr_t>(world_data.data()),actor=reinterpret_cast<uintptr_t>(actors.data());
 write(base+0xfb7734,2);write(base+0xfb7748,0.0);write(base+0xdef5d0,world);write(base+0xe27138,actor);write(base+0xe23678,0);
 write(world+0x128,6);write(world+0x6a39,uint8_t(1));write(world+0x160,2);write(world+0x3e0,2);write(world+0x46c,3);write(world+0x3c0,-1.f);write(world+0xa658,actor);write(world+0xa660,actor+actors.size());
 write(actor+0x78,0);write(actor+0x81,uint8_t(1));write(actor+0x3375,uint8_t(1));write(actor+0x54,10.f);
 auto fire=[&](float frame,int pad=-1,uintptr_t other=0){
  write(world+0x434,frame);
  const int cue=2;
  const unsigned before=native_calls;
  const uintptr_t local=other?other:actor;
  test_jump(relay,local+8,&cue,local,pad);
  assert(native_calls==before+1);
 };
 assert(jumps::take_progress(true)==0);
 fire(8.f);fire(8.f);fire(7.995f);
 assert(jumps::take_progress(true)==1);
 fire(8.f);fire(8.3f);fire(9.f);
 assert(jumps::take_progress(true)==2);

 fire(10.f,0);fire(11.f,42);fire(12.f,-1,actor+stride);
 write(actor+0x3375,uint8_t(0));fire(13.f);write(actor+0x3375,uint8_t(1));
 write(actor+0x54,-5.f);fire(14.f);write(actor+0x54,10.f);
 write(actor+0x2ae0,uintptr_t(1));fire(15.f);write(actor+0x2ae0,uintptr_t(0));
 write(actor,uint8_t(1));fire(16.f);write(actor,uint8_t(0));
 write(world+0x3e0,1);fire(17.f);write(world+0x3e0,2);
 write(world+0x6ae8,1.f);fire(18.f);write(world+0x6ae8,0.f);
 write(world+0x6b98,1);fire(19.f);write(world+0x6b98,0);
 write(world+0x160,1);
 write(world+0x2d2,uint8_t(1));fire(19.1f);write(world+0x2d2,uint8_t(0));
 write(world+0x121,uint8_t(1));fire(19.2f);write(world+0x121,uint8_t(0));
 assert(jumps::take_progress(true)==0);
 fire(19.3f);assert(jumps::take_progress(true)==1);

 jumps::take_progress(false);
 fire(20.f);
 assert(jumps::take_progress(true)==0);
 fire(20.f);assert(jumps::take_progress(true)==1);
 write(world+0x3e0,3);fire(1.f);assert(jumps::take_progress(true)==1);
 fire(2.f);jumps::take_progress(false,true);
 assert(jumps::take_progress(true)==0);
 fire(2.f);assert(jumps::take_progress(true)==1);

 Engine engine;
 engine.capability(Kind::jumps,true);
 engine.activity(Kind::jumps,9999,1795000000);
 fire(3.f);fire(3.f);engine.activity(Kind::jumps,jumps::take_progress(true),1795000000);
 auto get=[&](const char* id){return engine.ledger.items[ssc_achievements::index(id)];};
 assert(get("jumps_10000").unlocked_at&&get("jumps_50000").value==10000);
 ssc_achievements::Ledger restored;
 assert(ssc_achievements::Ledger::parse(engine.ledger.json(),restored));engine.replace(restored);
 engine.activity(Kind::jumps,40000,1795000000);
 assert(get("jumps_50000").unlocked_at&&get("jumps_100000").value==50000);
 engine.activity(Kind::jumps,50000,1795000000);
 assert(get("jumps_100000").unlocked_at);
 VirtualFree(fixture,0,MEM_RELEASE);
 std::cout<<"PASS exact guarded launch relay, native cue preservation, duplicate and replay rejection, jump-pad exclusion, local active actor checks, account resets and all three milestone thresholds\n";
}
