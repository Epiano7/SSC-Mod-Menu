#include "../runtime/freecam_capture_geometry.h"
#include "../runtime/freecam_ground.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <iostream>
int main(){
 using namespace ssc_freecam;using namespace ssc_freecam::capture;
 {
 Camera flight;flight.seeded=true;flight.eye={0,0,100};flight.right={1,0,0};flight.up={0,0,1};flight.back={0,-1,0};
 auto flat=[](float,float)->std::optional<Surface>{return Surface{0,{0,0,1}};};
 floor_flight(flight,0,1,0,120,24,flat);assert(std::abs(flight.eye.y-120)<.001f&&flight.eye.z==100);
 floor_flight(flight,0,0,-1,120,24,flat);assert(flight.eye.z==24);
 floor_flight(flight,1,0,0,10,24,[](float x,float)->std::optional<Surface>{return Surface{x*3,{-.95f,0,.1f}};});assert(flight.eye.x==10&&flight.eye.z==54);
 floor_flight(flight,0,1,-1,10,24,[](float,float)->std::optional<Surface>{return std::nullopt;});assert(flight.eye.y>120&&flight.eye.z==54);
 floor_flight(flight,0,0,1,10,24,flat);assert(flight.eye.z==64&&flight.back.y==-1&&flight.up.z==1);
 }
 GroundPolicy policy{10,2,3,.7f};Camera c;c.seeded=true;c.eye={0,0,10};c.right={1,0,0};c.back={0,-1,0};c.up={0,0,1};
 auto plane=[](GroundQuery)->std::optional<Surface>{return Surface{0,{0,0,1}};};
 assert(ground_move(c,0,1,2,policy,plane)&&c.eye.y==2&&c.eye.z==10);
 assert(ground_move(c,1,1,2,policy,plane));assert(std::abs(c.eye.x-std::sqrt(2.f))<.001f);
 auto before=c.eye;
 assert(!ground_move(c,1,0,1,policy,[](GroundQuery)->std::optional<Surface>{return std::nullopt;}));assert(dot(c.eye-before,c.eye-before)==0);
 assert(!ground_move(c,0,0,0,policy,[](GroundQuery)->std::optional<Surface>{return Surface{3,{0,0,1}};}));
 assert(!ground_move(c,0,0,0,policy,[](GroundQuery)->std::optional<Surface>{return Surface{-4,{0,0,1}};}));
 assert(!ground_move(c,0,0,0,policy,[](GroundQuery)->std::optional<Surface>{return Surface{0,{1,0,0}};}));
 assert(!ground_move(c,0,0,0,policy,[](GroundQuery)->std::optional<Surface>{return Surface{NAN,{0,0,1}};}));
 assert(!ground_move(c,0,0,0,GroundPolicy{},plane));assert(!ground_move(c,NAN,0,1,policy,plane));
 assert(ground_move(c,0,0,0,policy,[](GroundQuery)->std::optional<Surface>{return Surface{1,{0,.6f,.8f}};}));assert(c.eye.z==11&&c.up.z==1);
 // Looking vertically never turns forward motion into flight.
 c.back={0,0,1};assert(ground_move(c,0,1,1,policy,plane)&&c.eye.z==10);
 for(unsigned i=0;i<6;++i){auto f=face_camera({1,2,3},i);assert(f&&f->eye.x==1&&dot(f->right,f->up)==0&&dot(f->back,f->back)==1);auto b=cross(f->right,f->up);assert(dot(b-f->back,b-f->back)==0);auto m=f->matrix();assert(std::isfinite(m[14]));}
 assert(!face_camera({NAN,0,0},0)&&!face_camera({},6));
 auto p=perspective(90,1,1,1000);assert(p&&std::abs((*p)[0]-1)<.00001f&&(*p)[11]==-1);
 assert(!perspective(180,1,1,1000)&&!perspective(90,0,1,1000)&&!perspective(90,1,0,1000)&&!perspective(90,1,2,1));
 {
  std::vector<unsigned char> memory(0x83000);uintptr_t map=0x10000,cells=map+0x82700,corners=map+0x82800;
  auto put=[&](uintptr_t offset,auto value){std::memcpy(memory.data()+offset,&value,sizeof value);};
  auto read=[&](uintptr_t address,auto& value){if(address<map||address-map+sizeof value>memory.size())return false;std::memcpy(&value,memory.data()+address-map,sizeof value);return true;};
  put(0x98,1);put(0x9c,1);put(0x8261c,uint32_t(1));put(0x825b8,cells);put(0x826b8,corners);put(0x82706,int16_t(10));
  auto flat=terrain(map,10,10,read);assert(flat&&flat->z==10&&flat->normal.z==1);
  assert(!terrain(map,-1,0,read)&&!terrain(map,32,0,read)&&!terrain(map,NAN,0,read));
  put(0x82700,uint8_t(1));put(0x82800,0.f);put(0x82804,16.f);put(0x82808,0.f);put(0x8280c,16.f);
  auto slope=terrain(map,16,16,read);assert(slope&&std::abs(slope->z-8)<.001&&slope->normal.x<0&&slope->normal.z>.8f);
  put(0x8278a,int16_t(20));assert(!terrain(map,16,16,read)); // raised structure: no fake roof support
  put(0x8278a,int16_t(0));put(0x82800,NAN);assert(!terrain(map,16,16,read));
 }
 std::cout<<"Freecam capture geometry tests passed\n";
}
