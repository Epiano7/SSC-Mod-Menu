#include "../runtime/combat_stats.h"
#include <cassert>
#include <iostream>
using namespace ssc_combat;
template<class T> void put(uintptr_t p,T v){std::memcpy(reinterpret_cast<void*>(p),&v,sizeof(v));}
extern "C" uintptr_t combat_test_call(uintptr_t,const float*,uintptr_t,uintptr_t,uintptr_t);
static uintptr_t expected_actor=0,expected_weapon=0;static int sends=0,hits=0;
static int spawn_original(uintptr_t a,unsigned char b,uintptr_t w,unsigned char d,uintptr_t e,uintptr_t f,float g,float h,unsigned char i,unsigned char j,unsigned char k){assert(a==expected_actor&&w==expected_weapon&&b==2&&d==4&&e==5&&f==6&&g==7.5f&&h==8.5f&&i==9&&j==10&&k==11);++sends;return 8;}
static uintptr_t append_original(uintptr_t v,const float* p){assert(v==123&&p[0]==4&&p[1]==12&&p[2]==1);++hits;return 0x12345678;}
int main(){
 auto base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x1000000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(base);std::vector<unsigned char> world(0xb000),actors(0x3460*2),weapon(0x748);auto w=reinterpret_cast<uintptr_t>(world.data()),a=reinterpret_cast<uintptr_t>(actors.data()),wp=reinterpret_cast<uintptr_t>(weapon.data());expected_actor=a;expected_weapon=wp;image_base=base;original_spawn=spawn_original;original_append=append_original;
 put(base+0xfa335c,2);put(base+0xddb5d0,w);put(w+0x128,6);put(w+0x3e0,2);put(w+0x46c,3);put(w+0xa5b8,a);put(w+0xa5c0,a+actors.size());put(base+0xe12de8,a);put(base+0xe0f380,0);put(a+0x81,uint8_t(1));put(a+0x1288,wp);put(a+0x1290,wp+weapon.size());put(a+0x3460+0x7c4,0);auto c=context(base);assert(c.valid&&c.active&&c.actor==a);c.active=false;c.preparing=true;counter.observe(c);enabled=true;
 assert(spawn(a,2,wp,4,5,6,7.5f,8.5f,9,10,11)==8&&sends==1&&counter.totals.shots==1&&counter.totals.projectiles==8);
 float values[]={4,12,1};assert(combat_test_call(123,values,a,a+0x3460,wp)==0x12345678&&hits==1&&counter.totals.player_hits==1&&counter.totals.impact==12);
 combat_test_call(123,values,a+0x3460,a,wp);assert(hits==2&&counter.totals.hits==1);combat_test_call(123,values,a,a,wp);assert(hits==3&&counter.totals.hits==1);
 enabled=false;combat_test_call(123,values,a,a+0x3460,wp);assert(hits==4&&counter.totals.hits==1);assert(spawn(a,2,wp,4,5,6,7.5f,8.5f,9,10,11)==8&&sends==2&&counter.totals.shots==1);
 VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);std::cout<<"PASS: all 11 spawn arguments and return, native hit bridge registers, original delegates, ownership and disabled gating\n";
}
