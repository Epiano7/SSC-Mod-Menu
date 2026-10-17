#include "../runtime/freecam.h"
#include "freecam_session_fixture.h"
#include <cassert>
#include <filesystem>
#include <iostream>
int main(int argc,char** argv){
 using namespace ssc_freecam;State s;assert(!s.start(1));
 s.observe(1,2,100,200,1000,1000,1000);assert(s.start(1000));s.tick(1100,1,0,false);assert(std::abs(s.x-112)<.001f&&s.y==200);
 s.observe(1,2,800,900,1000,1000,1100);assert(s.x==112&&s.y==200); // native player movement cannot drag freecam
 s.tick(1200,1,1,false);assert(std::abs(s.x-120.48528f)<.01f);float x=s.x;s.tick(1300,1,0,true);assert(s.x==x);
 s.observe(1,2,800,900,1000,1000,1300);s.tick(1400,-100,0,false);assert(std::isfinite(s.x));
 s.stop();s.observe(1,2,350,400,1000,1000,1500);assert(s.x==350&&!s.active);assert(s.start(1500));
 s.observe(3,2,200,200,1000,1000,1600);assert(!s.active);assert(s.start(1600));s.observe(3,4,200,200,1000,1000,1700);assert(!s.active);
 assert(s.start(1700));s.tick(1600,1,0,false);assert(!s.active);s.observe(3,4,200,200,1000,1000,1800);assert(s.start(1800));s.tick(2500,1,0,false);assert(!s.active&&!s.ready);
 s.observe(1,2,NAN,0,1000,1000,2600);assert(!s.ready);s.observe(1,2,0,0,0,1000,2700);assert(!s.ready);
 s.observe(1,2,999,999,1000,1000,2800);assert(s.start(2800));s.tick(2900,1,1,false);assert(s.x==1000&&s.y==1000);
 state={};state.observe(1,2,0,0,1000,1000,1000);assert(state.start(1000));assert(key(WM_KEYDOWN,'W',false)&&keys['W']);stop();assert(key(WM_KEYDOWN,'W',false));assert(key(WM_KEYUP,'W',false)&&!keys['W']);assert(!key(WM_KEYDOWN,'W',false));
 assert(state.start(1000));key(WM_KEYDOWN,'D',false);lose_focus();assert(!state.active&&!keys['D']&&!ssc_creator::freecam_hide);assert(!key(WM_KEYDOWN,'W',true));

 {FreecamSessionFixture f(true,1000);auto a=f.actor,w=f.world;assert(request_start(1000)&&f.actor==a&&f.world==w);stop();}
 {FreecamSessionFixture f(false,1000);assert(!request_start(1000)&&!state.active&&f.requests==0&&notice_until==4500);f.set(f.actor,0x13a4,0.f);assert(dead()&&request_start(1000));f.set(f.actor,0x13a4,100.f);assert(!permitted());stop();f.set(f.actor,0x8a0,3);assert(request_start(1000));}
 {FreecamSessionFixture f(false,1000);f.set(f.world,0x6a38,uint8_t(1));f.set(f.world,0x2d2,uint8_t(1));f.set(f.world,0x160,2);assert(!singleplayer()&&!request_start(1000));}
 {FreecamSessionFixture f(true,1000);assert(request_start(1000));camera.seeded=true;camera.eye={1,2,3};suspend_focus();tick(10000,false);assert(state.active&&camera.seeded&&camera.eye.x==1);resume_focus();stop();}
 {FreecamSessionFixture f(true,1000);native_time_scale=+[](uintptr_t,unsigned char){return .75f;};photo_enabled=true;assert(!toggle_photo(1000));assert(request_start(1000));auto world=state.world;assert(time_scale(world,0)==.75f);assert(toggle_photo(1001)&&time_scale(world,0)==0);assert(time_scale(world+1,0)==.75f);f.set(f.world,0x6a39,uint8_t(1));assert(time_scale(world,0)==0);f.set(f.world,0x160,2);assert(time_scale(world,0)==.75f);assert(!toggle_photo(1002));stop();assert(!photo_frozen&&time_scale(world,0)==.75f);photo_enabled=false;native_time_scale=nullptr;}
 // Solo campaign can own a network transport. Empty multiplayer and co-op cannot.
 {FreecamSessionFixture f(true,1000);f.set(f.world,0x6a39,uint8_t(1));assert(singleplayer()&&request_start(1000));f.set(f.world,0x160,2);assert(!singleplayer()&&!permitted());stop();f.set(f.world,0x160,1);f.set(f.world,0x2d2,uint8_t(0));assert(!singleplayer()&&!request_start(1000));f.set(f.world,0x2d2,uint8_t(1));f.set(f.world,0x120,uint8_t(1));assert(!singleplayer());f.set(f.world,0x120,uint8_t(0));f.set(f.world,0x6a38,uint8_t(0));assert(!singleplayer());f.set(f.world,0x6a39,uint8_t(0));assert(!singleplayer());}
 // Live-reproduced solo Survival: host=1, transport=1, dedicated=0,
 // campaign=0, humans=1, mode=6, survival=1 (2026-10-11).
 {FreecamSessionFixture f(false,1000);f.set(f.world,0x6a38,uint8_t(1));f.set(f.world,0x485,uint8_t(1));f.set(f.world,0x128,6);assert(singleplayer()&&request_start(1000));f.set(f.world,0x160,2);assert(!singleplayer()&&!permitted());stop();f.set(f.world,0x160,1);f.set(f.world,0x6a38,uint8_t(0));assert(!singleplayer());}
 // Sub-millisecond movement remains smooth even if the coarse clock repeats.
 {State smooth;smooth.observe(1,2,100,100,1000,1000,1000);assert(smooth.start(1000));for(int i=0;i<10;++i)smooth.tick(1000,1,0,false,.001f);assert(std::abs(smooth.x-101.2f)<.001f);}
 {state.active=false;auto p=listening_position(1,2,3);assert(p.x==1&&p.z==3);state.active=true;fly=false;state.x=100;state.y=200;p=listening_position(1,2,3);assert(p.x==100&&p.y==200&&p.z==3);fly=true;camera.seeded=true;camera.eye={400,500,600};p=listening_position(1,2,3);assert(p.x==400&&p.z==600);stop();}
 // Render-scoped shadow suppression preserves the user's native setting.
 {std::vector<unsigned char> memory(0xfaf95d+1);auto previous_base=base;base=reinterpret_cast<uintptr_t>(memory.data());memory[0xfaf95d]=1;
  state.active=true;hide_shadows=true;native_scene=+[]{assert(*reinterpret_cast<unsigned char*>(base+0xfaf95d)==0);state.active=false;};scene();assert(memory[0xfaf95d]==1);
  native_scene=+[]{assert(*reinterpret_cast<unsigned char*>(base+0xfaf95d)==1);};scene();assert(memory[0xfaf95d]==1);
  state.active=true;hide_shadows=false;scene();assert(memory[0xfaf95d]==1);
  memory[0xfaf95d]=0;hide_shadows=true;native_scene=+[]{assert(*reinterpret_cast<unsigned char*>(base+0xfaf95d)==0);};scene();assert(memory[0xfaf95d]==0);
  stop();hide_shadows=false;native_scene=nullptr;base=previous_base;}
 // Slow movement is camera-only, with repeat-safe hold/toggle controls.
 state={};state.observe(1,2,100,200,1000,1000,1000);assert(start(1000));fly=false;slow_key=VK_F6;slow_toggle=false;
 key(WM_KEYDOWN,'D',false);key(WM_KEYDOWN,VK_F6,false);assert(slowing());tick(1100,false);assert(std::abs(state.x-101.2f)<.001f&&state.speed==120);
 key(WM_KEYUP,VK_F6,false);assert(!slowing());tick(1200,false);assert(std::abs(state.x-113.2f)<.001f);
 slow_toggle=true;key(WM_KEYDOWN,VK_F6,false);assert(slowing());key(WM_KEYDOWN,VK_F6,false);assert(slowing());key(WM_KEYUP,VK_F6,false);assert(slowing());key(WM_KEYDOWN,VK_F6,false);assert(!slowing());key(WM_KEYUP,VK_F6,false);
 key(WM_KEYDOWN,VK_F6,false);lose_focus();assert(!slow_latched&&!slowing());slow_toggle=false;
 slow_latched=true;tick(4000,false);assert(!slow_latched);
 assert(valid_slow_key(VK_F6)&&valid_slow_key('Z')&&!valid_slow_key('W')&&!valid_slow_key(VK_RSHIFT)&&!valid_slow_key(VK_CONTROL));

 Camera c;float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,-100,-200,-300,1};assert(c.seed(identity));assert(c.eye.x==100&&c.eye.y==200&&c.eye.z==300);assert(std::abs(c.matrix()[14]+300)<.001f);
 c.move(0,1,0,20);assert(c.eye.z==280);c.move(1,0,0,20);assert(c.eye.x==120);c.move(0,0,1,10);assert(c.eye.z==290);
 Camera fine=c;c.wheel(1,false,true);fine.wheel(1,true,true);assert(std::abs((290-c.eye.z)-10*(290-fine.eye.z))<.01f);
 c.reset();c.wheel(1,false,false);fine.reset();fine.wheel(1,true,false);assert(c.zoom<fine.zoom&&fine.zoom<1);c.wheel(100,false,false);assert(c.zoom==.025f);c.wheel(-100,false,false);assert(c.zoom==32);c.wheel(NAN,false,false);assert(c.zoom==32);
 // Both normal and the game's vertically inverted projection must follow
 // screen-relative mouse motion at arbitrary orientations, including upside down.
 for(float sign:{1.f,-1.f})for(int orientation=0;orientation<8;++orientation){
  c.seed(identity);c.projection(1,sign);c.look(float(orientation*173),float(orientation*419));
  Vec3 before=c.eye,forward=c.back*(-1);c.move(0,1,0,10);assert(dot(c.eye-before,forward)>9.999f);c.move(0,-1,0,10);
  before=c.eye;Vec3 right=c.right;c.move(1,0,0,10);assert(dot(c.eye-before,right)>9.999f);c.move(-1,0,0,10);
  before=c.eye;c.look(20,0);assert((std::abs(c.back.z)>.9999f||dot(c.back*(-1),right)>0)&&dot(c.eye-before,c.eye-before)==0);
  Vec3 visual_up=c.up*sign;c.look(0,-20);assert(dot(c.back*(-1),visual_up)>0);
 }
 c.seed(identity);c.projection(1,-1);c.look(0,-1256.637061f);assert(c.back.z<-.999f); // crosses vertical freely
 c.look(0,-1256.637061f);assert(c.back.z>.999f); // full 360 degrees, no clamp/snap
 for(int i=0;i<10000;++i){c.look(1,1);assert(std::abs(c.right.z)<.0001f);}
 assert(std::abs(dot(c.right,c.back))<.001f&&std::abs(dot(c.up,c.back))<.001f&&std::abs(dot(c.back,c.back)-1)<.001f);
 c.move(1,1,1,10000000);assert(std::abs(c.eye.x)<=100000&&std::abs(c.eye.z)<=100000);
 // Level a rolled native entry without moving its eye or changing its target.
 Camera rolled;rolled.eye={50,70,200};rolled.back=normalized(Vec3{0,1,1});rolled.right={1,0,0};rolled.up=cross(rolled.back,rolled.right);
 rolled.right=rotated(rolled.right,rolled.back,.2f);rolled.up=cross(rolled.back,rolled.right);auto rolled_matrix=rolled.matrix();
 assert(c.seed(rolled_matrix.data()));assert(std::abs(c.right.z)<.0001f&&dot(c.back,rolled.back)>.9999f);
 assert(dot(c.eye-rolled.eye,c.eye-rolled.eye)<.0001f);
 for(int i=0;i<1000;++i){c.look(17,-13);c.wheel(i%2?1:-1,true,true);assert(std::abs(c.right.z)<.0001f);}
 identity[0]=NAN;assert(!c.seed(identity));identity[0]=2;assert(!c.seed(identity));
 assert(argc==2);auto path=std::filesystem::path(argv[1]);HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(file!=INVALID_HANDLE_VALUE);HANDLE mapping=CreateFileMappingW(file,nullptr,PAGE_READONLY|SEC_IMAGE_NO_EXECUTE,0,0,nullptr);assert(mapping);auto view=static_cast<const unsigned char*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,0));assert(view);
 assert(matches(view,0x866c0c));assert(!matches(view,0x866c0b));std::vector<unsigned char> changed(view,view+0x866c0c);for(size_t off:{size_t(0x451820),size_t(0x1ef462),size_t(0x3b8d40),size_t(0x589c0a),size_t(0x58f248),size_t(0x84a506),size_t(0x851dc1),size_t(0x85129e),size_t(0x84ae62),size_t(0x8569a6),size_t(0x8569c0),size_t(0x866bfc)}){changed[off]^=1;assert(!matches(changed.data(),changed.size()));changed[off]^=1;}
 UnmapViewOfFile(view);CloseHandle(mapping);CloseHandle(file);
 std::cout<<"PASS: independent camera position, normalized motion, pause, bounds, reset, actor/world change, clock/gap, invalid data, held-key release, focus loss, view-relative motion, fixed-eye look, inverted projection, unrestricted rotations, native guards, offline entry without player changes, campaign co-op gating, dead/spectator gating, photo freeze offline-only, rejection/session changes and changed-code rejection\n";
}

