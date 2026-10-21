#pragma once
#include "creator_controls.h"
#include "freecam_state.h"
#include "freecam_camera.h"
#include "freecam_capture.h"
#include "freecam_ground.h"
namespace ssc_freecam {
inline State state;
inline bool hide_shadows=false;
inline bool hide_all_gui=false;
inline bool clean_view(){return state.active&&(hide_all_gui||capture::session.active());}
inline bool attached=false,hide_hud=false,fly=false,focus_suspended=false;
inline Camera camera;
inline bool fps=false,ground_supported=false;
inline float eye_height=24.f,capture_fov=75.f;
// Zero retains the native/expanded far distance. Optional shorter distances
// are explicit scenery clipping, not a change to simulation or network state.
inline int draw_distance=0;
inline float fly_speed=1200;
inline float& selected_speed(){return fly?fly_speed:state.speed;}
inline float speed_limit(){return fly?12000.f:600.f;}
inline float speed_step(){return fly?10.f:1.f;}
inline float frame_seconds(){static LARGE_INTEGER previous{},frequency{};LARGE_INTEGER current{};if(!frequency.QuadPart)QueryPerformanceFrequency(&frequency);if(!QueryPerformanceCounter(&current)||!frequency.QuadPart)return 0;double dt=previous.QuadPart?double(current.QuadPart-previous.QuadPart)/frequency.QuadPart:0;previous=current;return dt>=0&&dt<=.5?float(dt):0.f;}
inline Vec3 pan_origin{};inline float pan_x=0,pan_y=0;inline std::array<float,16> saved_projection{};
inline HWND mouse_window=nullptr;inline POINT mouse_center{},saved_cursor{};inline RECT saved_clip{};inline HCURSOR saved_shape=nullptr;
inline bool raw_mouse=false,had_raw_mouse=false,absolute_seen=false;inline LONG absolute_x=0,absolute_y=0;inline RAWINPUTDEVICE saved_raw_mouse{};
inline bool bounds_saved=false;inline float saved_bounds[4]{};
inline bool keys[256]{};
inline int slow_key=VK_F6,photo_key=VK_F8,capture_key=VK_F9;
inline bool photo_enabled=false,photo_frozen=false,hide_diagnostics=false;
using TimeScale=float(*)(uintptr_t,unsigned char);inline TimeScale native_time_scale=nullptr;
inline bool slow_toggle=false,slow_latched=false;
inline bool valid_slow_key(int key){return (key>=VK_F1&&key<=VK_F12)||(key>='A'&&key<='Z'&&key!='W'&&key!='A'&&key!='S'&&key!='D'&&key!='Q'&&key!='E')||(key>='0'&&key<='9');}
inline bool slowing(){return state.active&&(slow_toggle?slow_latched:keys[slow_key]);}
inline uintptr_t base=0;
inline uintptr_t observed_context=0,pending_actor=0,pending_world=0;
inline uint64_t pending_since=0;
inline const wchar_t* entry_status=L"";
inline uint64_t notice_until=0;
inline bool reject(uint64_t now,const wchar_t* message){entry_status=message;notice_until=now+3500;return false;}
using SpectatorRequest=void(*)(uintptr_t);
inline SpectatorRequest native_spectator_request=nullptr;
using Center=float*(*)(uintptr_t,float*);
using Hud=void(*)(uintptr_t);
inline Center original_center=nullptr;
inline Hud original_hud=nullptr;
template<class T> bool read(uintptr_t p,T& value){MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&(PAGE_NOACCESS|PAGE_GUARD))||p<reinterpret_cast<uintptr_t>(m.BaseAddress)||sizeof(T)>m.RegionSize-(p-reinterpret_cast<uintptr_t>(m.BaseAddress)))return false;std::memcpy(&value,reinterpret_cast<void*>(p),sizeof(T));return true;}
inline std::optional<capture::Surface> floor_at(float x,float y){uintptr_t map=0;if(!ground_supported||!read(base+0xde95d8,map))return std::nullopt;return capture::terrain(map,x,y,[](uintptr_t address,auto& value){return read(address,value);});}
inline void limit_floor(){auto surface=floor_at(camera.eye.x,camera.eye.y);if(surface)camera.eye.z=std::max(camera.eye.z,surface->z+eye_height);}
inline void wheel(float steps,bool fine){if(capture::session.active())return;if(fps){capture_fov=std::clamp(capture_fov-steps*(fine?1.f:5.f),20.f,100.f);}else camera.wheel(steps,fine,fly);}
inline void mouse_look(float dx,float dy){if(!capture::session.active())camera.look(dx,dy);}
inline bool spectator(){int mode=-1;return read(state.actor+0x8a0,mode)&&mode==3;}
// +6a39 means a transport exists, not that the session has other players.
// Native pause logic also uses local authority (+6a38), dedicated-server
// state (+120), solo/campaign (+2d2) and human count (+160). The count is
// incremented only for actor kind 0 at 2dc508, excluding bots (kind 1).
// Campaign alone is insufficient: co-op and remote clients stay restricted.
inline bool singleplayer(){
 unsigned char host=0,transport=1,dedicated=1,solo=0,survival=0;int humans=0,mode=-1;
 if(!state.world||!read(state.world+0x6a38,host)||!read(state.world+0x6a39,transport)||!read(state.world+0x120,dedicated)||!read(state.world+0x2d2,solo)||!read(state.world+0x160,humans)||!read(state.world+0x485,survival)||!read(state.world+0x128,mode))return false;
 return host==1&&dedicated==0&&humans==1&&(transport==0||solo==1||(mode==6&&survival==1));
}
inline bool dead(){float health=1;return state.actor&&read(state.actor+0x13a4,health)&&std::isfinite(health)&&health<=0;}
inline bool permitted(){return singleplayer()||spectator()||dead();}
inline bool toggle_photo(uint64_t now){
 if(!state.active||!photo_enabled)return false;
 if(!singleplayer())return reject(now,L"Photo freeze is available only in offline single-player.");
 photo_frozen=!photo_frozen;return true;
}
inline float time_scale(uintptr_t world,unsigned char update){
 float native=native_time_scale(world,update);uintptr_t current=0;
 return state.active&&((photo_enabled&&photo_frozen)||capture::session.active())&&world==state.world&&read(base+0xde95d0,current)&&current==world&&singleplayer()?0.f:native;
}
inline bool request_start(uint64_t now){
 if(fps&&!ground_supported)return reject(now,L"FPS terrain camera unavailable for this build.");
 if(fps)fly=true;
 if(!attached||!state.ready||now<state.seen||now-state.seen>500)return reject(now,L"Freecam unavailable: no active session.");
 uintptr_t actor=0,world=0;int local=-1,slot=-2,kind=-1;unsigned char active=0;
 if(!read(observed_context,actor)||actor!=state.actor||!read(base+0xde95d0,world)||world!=state.world||!read(world+0x6b98,local)||!read(actor+0x78,slot)||slot!=local||slot<0||!read(actor+0x7dc,kind)||kind!=0||!read(actor+0x81,active)||!active)return reject(now,L"Freecam unavailable: local player not ready.");
 if(!permitted())return reject(now,L"Freecam requires death or spectator mode in multiplayer.");
 camera.reset();slow_latched=false;focus_suspended=false;entry_status=L"";notice_until=0;return state.start(now);
}
inline bool register_mouse(HWND window){
 UINT count=0;if(GetRegisteredRawInputDevices(nullptr,&count,sizeof(RAWINPUTDEVICE))==UINT(-1)||count>256)return false;
 std::vector<RAWINPUTDEVICE> devices(count);if(count&&GetRegisteredRawInputDevices(devices.data(),&count,sizeof(RAWINPUTDEVICE))==UINT(-1))return false;
 absolute_seen=false;had_raw_mouse=false;for(const auto& d:devices)if(d.usUsagePage==1&&d.usUsage==2){saved_raw_mouse=d;had_raw_mouse=true;}
 RAWINPUTDEVICE d{1,2,0,window};raw_mouse=RegisterRawInputDevices(&d,1,sizeof d)!=FALSE;return raw_mouse;
}
inline bool restore_mouse_registration(){if(!raw_mouse)return true;RAWINPUTDEVICE d=had_raw_mouse?saved_raw_mouse:RAWINPUTDEVICE{1,2,RIDEV_REMOVE,nullptr};if(!RegisterRawInputDevices(&d,1,sizeof d))return false;raw_mouse=false;return true;}
inline void release_mouse(){if(mouse_window){restore_mouse_registration();ClipCursor(&saved_clip);if(GetForegroundWindow()==mouse_window){SetCursorPos(saved_cursor.x,saved_cursor.y);SetCursor(saved_shape);}mouse_window=nullptr;}}
inline void restore_bounds(){if(bounds_saved&&base){std::memcpy(reinterpret_cast<void*>(base+0xfb0c80),saved_bounds,sizeof saved_bounds);bounds_saved=false;}}
inline void stop(){capture::session.cancel();photo_frozen=false;focus_suspended=false;slow_latched=false;state.stop();pending_since=0;pending_actor=pending_world=0;entry_status=L"";camera.reset();release_mouse();restore_bounds();ssc_creator::freecam_hide=false;}
inline bool start(uint64_t now){camera.reset();return state.start(now);}
inline bool select_mode(int mode){
 bool target_fps=mode==2,target_fly=mode!=0;
 if(target_fps&&!ground_supported)return reject(GetTickCount64(),L"FPS terrain camera unavailable for this build.");
 if(capture::session.active())return false;
 if(state.active&&camera.seeded){
  if(target_fps)limit_floor();
  else if(!target_fly){pan_origin=camera.eye;pan_x=state.x;pan_y=state.y;camera.zoom=1;}
 }
 fps=target_fps;fly=target_fly;return true;
}

inline void capture_mouse(HWND window,bool menu){
 if(!state.active||!fly||menu||!window||GetForegroundWindow()!=window){release_mouse();return;}
 RECT r{};if(!GetClientRect(window,&r)||r.right<=0||r.bottom<=0){stop();return;}
 POINT origin{0,0};ClientToScreen(window,&origin);RECT clip{origin.x,origin.y,origin.x+r.right,origin.y+r.bottom};
 if(!mouse_window){if(!GetCursorPos(&saved_cursor)||!GetClipCursor(&saved_clip)||!register_mouse(window)){stop();return;}saved_shape=GetCursor();mouse_window=window;mouse_center={r.right/2,r.bottom/2};SetCursorPos(origin.x+mouse_center.x,origin.y+mouse_center.y);}
 mouse_center={r.right/2,r.bottom/2};ClipCursor(&clip);SetCursor(nullptr);
}
inline void raw_motion(LPARAM lp){
 if(!raw_mouse||!mouse_window||capture::session.active())return;
 RAWINPUT input{};UINT size=sizeof input;UINT got=GetRawInputData(reinterpret_cast<HRAWINPUT>(lp),RID_INPUT,&input,&size,sizeof(RAWINPUTHEADER));
 if(got==UINT(-1)||got<sizeof(RAWINPUTHEADER)+sizeof(RAWMOUSE)||input.header.dwType!=RIM_TYPEMOUSE)return;
 if(input.data.mouse.usFlags&MOUSE_MOVE_ABSOLUTE){
  LONG x=input.data.mouse.lLastX,y=input.data.mouse.lLastY;
  if(absolute_seen){bool desktop=(input.data.mouse.usFlags&MOUSE_VIRTUAL_DESKTOP)!=0;float dx=float(x-absolute_x)*GetSystemMetrics(desktop?SM_CXVIRTUALSCREEN:SM_CXSCREEN)/65535.f,dy=float(y-absolute_y)*GetSystemMetrics(desktop?SM_CYVIRTUALSCREEN:SM_CYSCREEN)/65535.f;mouse_look(dx,dy);}
  absolute_x=x;absolute_y=y;absolute_seen=true;
 }else {absolute_seen=false;mouse_look(float(input.data.mouse.lLastX),float(input.data.mouse.lLastY));}
}
// Called at the final translation of each world render pass, before native
// inverse-view caches are calculated. Never affects menu/portrait transforms.
inline void APIENTRY view(float x,float y,float z){
 glTranslatef(x,y,z);if(!state.active)return;
 GLfloat m[16]{},projection[16]{};glGetFloatv(GL_MODELVIEW_MATRIX,m);glGetFloatv(GL_PROJECTION_MATRIX,projection);camera.projection(projection[0],projection[5]);
 if(!camera.seeded){if(!camera.seed(m)){stop();return;}if(fps)limit_floor();pan_origin=camera.eye;pan_x=state.x;pan_y=state.y;std::copy(projection,projection+16,saved_projection.begin());}
 // A followed player can jump or change the native zoom. Both view and
 // projection belong to the free camera until exit, never to that player.
 std::copy(saved_projection.begin(),saved_projection.end(),projection);
 if(!fly){camera.eye=pan_origin+Vec3{state.x-pan_x,state.y-pan_y,0};float distance=std::abs(pan_origin.z);camera.eye=camera.eye+camera.back*(distance*(camera.zoom-1));}
 // Keep the working 3D projection and its axis signs; lens zoom only scales it.
 if(fps){float lens=std::tan(75.f*.00872664626f)/std::tan(capture_fov*.00872664626f);projection[0]*=lens;projection[5]*=lens;}

 Camera rendered_camera=camera;capture::session.view(rendered_camera,projection);
 auto matrix=rendered_camera.matrix();glLoadMatrixf(matrix.data());
 glMatrixMode(GL_PROJECTION);glLoadMatrixf(projection);glMatrixMode(GL_MODELVIEW);
 // Preserve native near-plane precision: the old .5-to-1,000,000 range
 // introduced depth fighting even at normal zoom.
 if(projection[11]==-1.f&&projection[15]==0.f){float near_plane=projection[14]/(projection[10]-1),far_plane=projection[14]/(projection[10]+1);
  float wanted=fly?std::max(far_plane,std::hypot(state.max_x,state.max_y)+std::sqrt(dot(camera.eye,camera.eye))):far_plane*std::max(1.f,camera.zoom);
  if(fly&&draw_distance>0)wanted=std::max(near_plane+1.f,float(draw_distance));
  if(std::isfinite(wanted)&&near_plane>0&&wanted>near_plane&&wanted!=far_plane){projection[10]=-(wanted+near_plane)/(wanted-near_plane);projection[14]=-2*wanted*near_plane/(wanted-near_plane);glMatrixMode(GL_PROJECTION);glLoadMatrixf(projection);glMatrixMode(GL_MODELVIEW);}
 }
 if(base&&(fly||camera.zoom>1.f)){if(!bounds_saved){std::memcpy(saved_bounds,reinterpret_cast<void*>(base+0xfb0c80),sizeof saved_bounds);bounds_saved=true;}
  float bounds[4];if(fly){bounds[0]=-256;bounds[1]=-256;bounds[2]=state.max_x+512;bounds[3]=state.max_y+512;rendered_camera.visible_bounds(projection,state.max_x,state.max_y,bounds);}
  else {bounds[0]=state.x+(saved_bounds[0]-state.x)*camera.zoom;bounds[1]=state.y+(saved_bounds[1]-state.y)*camera.zoom;bounds[2]=saved_bounds[2]*camera.zoom;bounds[3]=saved_bounds[3]*camera.zoom;}
  std::memcpy(reinterpret_cast<void*>(base+0xfb0c80),bounds,sizeof bounds);
 }
}
inline float* center(uintptr_t context,float* output){
 observed_context=context;
 restore_bounds();auto result=original_center(context,output);uintptr_t actor=0,world=0,map=0;int width=0,height=0;
 bool valid=read(context,actor)&&read(base+0xde95d0,world)&&read(base+0xde95d8,map)&&read(map+0x98,width)&&read(map+0x9c,height)&&width>0&&height>0&&width<=10000&&height<=10000;
 if(focus_suspended&&actor==state.actor&&world==state.world)state.seen=GetTickCount64();
 state.observe(valid?actor:0,valid?world:0,output[0],output[1],float(width)*32,float(height)*32,GetTickCount64());
 if(state.active&&!permitted())stop();
 if(!state.active){photo_frozen=false;slow_latched=false;camera.reset();release_mouse();}
 ssc_creator::freecam_hide=state.active;
 if(state.active){output[0]=fly&&camera.seeded?camera.eye.x:state.x;output[1]=fly&&camera.seeded?camera.eye.y:state.y;}return result;
}
inline void hud(uintptr_t context){restore_bounds();if(clean_view()||(state.active&&hide_hud)){ssc_creator::WriteMask mask;original_hud(context);}else original_hud(context);}
inline void poll_entry(uint64_t now){
 if(pending_since){
  if(state.actor!=pending_actor||state.world!=pending_world||!state.ready||now<state.seen||now-state.seen>500||now<pending_since||now-pending_since>5000){pending_since=0;entry_status=L"Spectator switch was not confirmed. Try the game menu.";}
  else if(spectator()){pending_since=0;camera.reset();if(state.start(now))entry_status=L"";}
 }
}
inline void tick(uint64_t now,bool menu,HWND window=nullptr,float precise_dt=-1){
 if(capture::session.active()){if(menu||focus_suspended||!state.active||!singleplayer()||state.world!=capture::session.world||state.actor!=capture::session.actor||now-capture::session.started>10000)capture::session.cancel();else {state.last=now;return;}}
 if(focus_suspended){state.last=now;release_mouse();return;}
 poll_entry(now);
 float speed=state.speed;float scale=slowing()&&!menu?.1f:1.f;state.speed=speed*scale;
 auto previous=state.last;state.tick(now,fly?0.f:float(keys['D']||keys[VK_RIGHT])-float(keys['A']||keys[VK_LEFT]),fly?0.f:float(keys['S']||keys[VK_DOWN])-float(keys['W']||keys[VK_UP]),menu,precise_dt);state.speed=speed;
 if(state.active&&fly&&!menu){float dt=precise_dt>=0?precise_dt:float(now-previous)/1000;float side=float(keys['D']||keys[VK_RIGHT])-float(keys['A']||keys[VK_LEFT]),forward=float(keys['W']||keys[VK_UP])-float(keys['S']||keys[VK_DOWN]);
 float rise=float(keys[VK_SPACE])-float(keys[VK_SHIFT]||keys[VK_LSHIFT]||keys[VK_RSHIFT]);
 if(fps)capture::floor_flight(camera,side,forward,rise,fly_speed*scale*dt,eye_height,[](float x,float y){return floor_at(x,y);});
 else camera.move(side,forward,rise,fly_speed*scale*dt);}

 if(!state.active){photo_frozen=false;slow_latched=false;camera.reset();release_mouse();restore_bounds();}capture_mouse(window,menu);
 ssc_creator::freecam_hide=state.active;
}
inline void suspend_focus(){capture::session.cancel();focus_suspended=true;release_mouse();slow_latched=false;std::fill(std::begin(keys),std::end(keys),false);}
inline void resume_focus(){focus_suspended=false;state.last=GetTickCount64();if(state.ready)state.seen=state.last;}
inline void lose_focus(){stop();std::fill(std::begin(keys),std::end(keys),false);}
inline bool key(UINT message,WPARAM key,bool menu){
 if(key>=256)return false;
 bool down=message==WM_KEYDOWN||message==WM_SYSKEYDOWN,up=message==WM_KEYUP||message==WM_SYSKEYUP;
 if(!down&&!up)return false;
 if(up){bool was=keys[key];keys[key]=false;return !menu&&(state.active||was);}
 if(menu){keys[key]=false;return false;}
 if(state.active){if(int(key)==capture_key&&!keys[key]){if(!fly||!singleplayer()||!camera.seeded)reject(GetTickCount64(),L"Location capture requires 3D Freecam in single-player.");else if(!capture::session.begin(camera,state.actor,state.world,GetTickCount64()))reject(GetTickCount64(),L"Capture already in progress. Please wait.");else {capture::session.package.manifest["ground_verified"]=false;capture::session.package.manifest["terrain_floor_limit_enabled"]=fps;capture::session.package.manifest["eye_height_native_units"]=fps?capture::Json(eye_height):capture::Json(nullptr);capture::session.package.manifest["map_extent_native_units"]={state.max_x,state.max_y};}}if(int(key)==photo_key&&photo_enabled&&!keys[key])toggle_photo(GetTickCount64());if(int(key)==slow_key&&slow_toggle&&!keys[key])slow_latched=!slow_latched;keys[key]=true;return true;}
 return keys[key]; // Ignore held-key repeats after exiting until its release.
}
// Replace only the paired billboard rotations, retaining native translation,
// scale, visibility decisions and matrix push/pop. Pan remains entirely native.
inline bool billboard_active(){return state.active&&fly&&camera.seeded;}
inline void APIENTRY billboard_yaw(float angle,float x,float y,float z){if(!billboard_active())glRotatef(angle,x,y,z);}
inline void APIENTRY billboard_pitch(float angle,float x,float y,float z){
 // Native callers push a matrix around each label assembly. Collapse only
 // that assembly, including its bars/icons, until the caller's matching pop.
 // No world geometry, gameplay state or persistent GL write mask is changed.
 if(clean_view()){glScalef(0,0,0);return;}
 if(!billboard_active()){glRotatef(angle,x,y,z);return;}
 // Parent objects can add their own tilt. Remove the complete current
 // rotation, not just the camera's rotation, keeping the anchor and scale.
 float m[16];glGetFloatv(GL_MODELVIEW_MATRIX,m);
 for(int col=0;col<3;++col){float scale=std::sqrt(m[col*4]*m[col*4]+m[col*4+1]*m[col*4+1]+m[col*4+2]*m[col*4+2]);for(int row=0;row<3;++row)m[col*4+row]=col==row?scale:0;}
 glLoadMatrixf(m);
}
using Listener3f=void(*)(int,float,float,float);inline Listener3f native_listener=nullptr;
inline Vec3 listening_position(float x,float y,float z){
 if(!state.active)return{x,y,z};
 if(fly&&camera.seeded)return camera.eye;
 return{state.x,state.y,z};
}
inline void listener(int property,float x,float y,float z){
 if(property==0x1004){auto point=listening_position(x,y,z);x=point.x;y=point.y;z=point.z;
  // The sound distance checks share these listener coordinates with OpenAL.
  float position[]={x,y,z};std::memcpy(reinterpret_cast<void*>(base+0xe06508),position,sizeof position);
 }
 native_listener(property,x,y,z);
}
using Scene=void(*)();inline Scene native_scene=nullptr;
inline void scene(){
 // This is the native render-only Shadows switch, scoped to one world draw.
 // Restore it even when Freecam stops during that draw; never save game config.
 auto flag=reinterpret_cast<unsigned char*>(base+0xfb09dd);
 const unsigned char previous=*flag;
 const bool hidden=state.active&&hide_shadows;
 if(hidden)*flag=0;
 native_scene();
 if(hidden)*flag=previous;
}
using DebugDraw=void(*)(uintptr_t,uintptr_t,uintptr_t,int);inline DebugDraw native_debug=nullptr;
inline void debug_draw(uintptr_t a,uintptr_t b,uintptr_t c,int d){if(clean_view()||(state.active&&hide_diagnostics)){ssc_creator::WriteMask mask;native_debug(a,b,c,d);}else native_debug(a,b,c,d);}
inline bool matches(const unsigned char* b,size_t size){
 if(size<0x86744c)return false;
 const unsigned char caller[]={0x0,0xf,0x28,0xc2,0xf2,0xf,0x59,0xd8,0xf2,0xf,0x11,0x1d,0xbc,0xf5,0x74,0x0,0xe8,0x6f,0x79,0x98,0xff,0x40,0x32,0xff,0xf2,0xf,0x11,0x35,0xac,0xf5,0x74,0x0};
 if(std::memcmp(b+0x86742c,caller,sizeof caller)!=0)return false;
 if(ssc_creator::hash(b+0x856d60,0x1b36)!=0x72d75b84ac8203b7ULL)return false;
 if(ssc_creator::hash(b+0x1aa400,0x7e7)!=0xc90407b3d8bc10bbULL)return false;
 {int32_t d=0;std::memcpy(&d,b+0x1eef4e,4);if(b[0x1eef4c]!=0xff||b[0x1eef4d]!=0x15||int64_t(0x1eef52)+d!=0xb87b30)return false;}
 if(ssc_creator::hash(b+0x2b6c90,0x355)!=0x9354de4651c02925ULL||ssc_creator::hash(b+0x214939,0x30)!=0x25d549b56a2345ccULL)return false;
 if(ssc_creator::hash(b+0x851f00,0x49fe)!=0xd08973f2e7cfa2ebULL||ssc_creator::hash(b+0x84e5c0,0x393b)!=0x1cb740e82d6feeULL||ssc_creator::hash(b+0x84b550,0xdb4)!=0x8a0ddd82d0f83b82ULL)return false;
 for(auto function:{std::pair<uint32_t,uint64_t>{0x589e50,0x3c44142fc46550ceULL},{0x58f470,0xf25a685ed8eea870ULL},{0x84a200,0xfe90a9cc60504dddULL}}){size_t length=function.first==0x589e50?0x1e09:function.first==0x58f470?0xa3e:0x1344;if(ssc_creator::hash(b+function.first,length)!=function.second)return false;}
 for(uint32_t site:{0x589f9au,0x589fb4u,0x58f5bfu,0x58f5d8u,0x84ad1cu,0x84ad36u,0x8525f1u,0x85260bu,0x851ab5u,0x851aceu,0x84b678u,0x84b692u,0x8571d6u,0x8571f0u}){int32_t d=0;std::memcpy(&d,b+site+2,4);if(b[site]!=0xff||b[site+1]!=0x15||int64_t(site)+6+d!=0xdef870)return false;}
 if(ssc_creator::hash(b+0x4325c0,0x2e5)!=0xa796c088ebe2e5e3ULL)return false;
 if(ssc_creator::hash(b+0x1f0eb0,0x1d1)!=0x3684895c2d4bb4b0ULL)return false;
 for(uint32_t site:{0x1f011bu,0x1f03bfu,0x1f0fe1u}){int32_t d=0;std::memcpy(&d,b+site+2,4);if(b[site]!=0xff||b[site+1]!=0x15||int64_t(site)+6+d!=0xdef868)return false;}
 if(ssc_creator::hash(b+0x4518f0,0x490)!=0x3368cc511ba65a10ULL||ssc_creator::hash(b+0x1eedb0,6566)!=0xcde326686ef466f9ULL||ssc_creator::hash(b+0x3b9290,0x1f8c)!=0xc2943add927b7321ULL)return false;
 for(auto pair:{std::pair<uint32_t,uint32_t>{0x1ef551,0x4518f0},{0x1f0693,0x3b9290},{0x214940,0x2b6c90},{0x1aa343,0x1aa400}}){int32_t d=0;std::memcpy(&d,b+pair.first+1,4);if(b[pair.first]!=0xe8||int64_t(pair.first)+5+d!=pair.second)return false;}return true;
}
inline bool attach(){
 auto b=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));if(!b)return false;auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(b);if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(b+dos->e_lfanew);if(nt->Signature!=IMAGE_NT_SIGNATURE||!matches(b,nt->OptionalHeader.SizeOfImage))return false;
 base=reinterpret_cast<uintptr_t>(b);ground_supported=ssc_creator::hash(b+0x582ad0,0x229)==0x25b146266b8a0dbaULL;unsigned char* bridge=nullptr;
 native_time_scale=reinterpret_cast<TimeScale>(b+0x2b6c90);
 native_scene=reinterpret_cast<Scene>(b+0x1eedb0);
 native_debug=reinterpret_cast<DebugDraw>(b+0x1aa400);
 native_listener=*reinterpret_cast<Listener3f*>(b+0xb87b30);
 native_spectator_request=reinterpret_cast<SpectatorRequest>(b+0x4325c0);
 for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((base+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!bridge)return false;
 uintptr_t hooks[]={reinterpret_cast<uintptr_t>(center),reinterpret_cast<uintptr_t>(hud),reinterpret_cast<uintptr_t>(view),reinterpret_cast<uintptr_t>(view),reinterpret_cast<uintptr_t>(view),reinterpret_cast<uintptr_t>(billboard_yaw),reinterpret_cast<uintptr_t>(billboard_pitch),reinterpret_cast<uintptr_t>(billboard_yaw),reinterpret_cast<uintptr_t>(billboard_pitch),reinterpret_cast<uintptr_t>(billboard_yaw),reinterpret_cast<uintptr_t>(billboard_pitch),reinterpret_cast<uintptr_t>(billboard_yaw),reinterpret_cast<uintptr_t>(billboard_pitch),reinterpret_cast<uintptr_t>(billboard_yaw),reinterpret_cast<uintptr_t>(billboard_pitch),reinterpret_cast<uintptr_t>(billboard_yaw),reinterpret_cast<uintptr_t>(billboard_pitch),reinterpret_cast<uintptr_t>(time_scale),reinterpret_cast<uintptr_t>(listener),reinterpret_cast<uintptr_t>(billboard_yaw),reinterpret_cast<uintptr_t>(billboard_pitch),reinterpret_cast<uintptr_t>(scene)};const unsigned char jump[]={0xff,0x25,0,0,0,0};
 for(int i=0;i<22;++i){std::memcpy(bridge+16*i,jump,6);std::memcpy(bridge+16*i+6,&hooks[i],8);}DWORD old=0;
 if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,352);
 unsigned char* sites[]={b+0x1ef551,b+0x1f0693,b+0x1f011b,b+0x1f03bf,b+0x1f0fe1,b+0x589f9a,b+0x589fb4,b+0x58f5bf,b+0x58f5d8,b+0x84ad1c,b+0x84ad36,b+0x8525f1,b+0x85260b,b+0x851ab5,b+0x851ace,b+0x84b678,b+0x84b692,b+0x214940,b+0x1eef4c,b+0x8571d6,b+0x8571f0,b+0x86743c};DWORD protection[22]{};const size_t lengths[]={5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,5,6,6,6,5};
 for(int i=0;i<22;++i)if(!VirtualProtect(sites[i],lengths[i],PAGE_EXECUTE_READWRITE,&protection[i])){for(int j=0;j<i;++j){DWORD ignored;VirtualProtect(sites[j],lengths[j],protection[j],&ignored);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 original_center=reinterpret_cast<Center>(b+0x4518f0);original_hud=reinterpret_cast<Hud>(b+0x3b9290);
 for(int i=0;i<22;++i){int32_t d=int32_t(reinterpret_cast<uintptr_t>(bridge+16*i)-reinterpret_cast<uintptr_t>(sites[i]+5));sites[i][0]=0xe8;std::memcpy(sites[i]+1,&d,4);if(lengths[i]==6)sites[i][5]=0x90;FlushInstructionCache(GetCurrentProcess(),sites[i],lengths[i]);}
 for(int i=21;i>=0;--i){DWORD ignored;VirtualProtect(sites[i],lengths[i],protection[i],&ignored);}return true;
}
}
