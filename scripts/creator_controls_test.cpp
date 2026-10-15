#include "../runtime/freecam.h"
#include <cassert>
#include <filesystem>
#include <iostream>
static int calls=0;
static void native_draw(uintptr_t actor,unsigned char mode){
 assert(actor==123&&mode==7);++calls;
 glColor3f(1,1,1);glBegin(GL_QUADS);glVertex2f(-1,-1);glVertex2f(1,-1);glVertex2f(1,1);glVertex2f(-1,1);glEnd();
}
int main(int argc,char** argv){
 assert(argc==2);auto path=std::filesystem::path(argv[1]);
 HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(file!=INVALID_HANDLE_VALUE);
 HANDLE mapping=CreateFileMappingW(file,nullptr,PAGE_READONLY|SEC_IMAGE_NO_EXECUTE,0,0,nullptr);assert(mapping);
 auto view=static_cast<const unsigned char*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,0));assert(view);
 assert(ssc_creator::matches(view,0x847208));assert(!ssc_creator::matches(view,0x847207));
 std::vector<unsigned char> changed(view,view+0x847208);changed[0x843570]^=1;assert(!ssc_creator::matches(changed.data(),changed.size()));changed[0x843570]^=1;changed[0x3b7e87]^=1;assert(!ssc_creator::matches(changed.data(),changed.size()));
 UnmapViewOfFile(view);CloseHandle(mapping);CloseHandle(file);
 WNDCLASSW wc{};wc.style=CS_OWNDC;wc.lpfnWndProc=DefWindowProcW;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"SSCCreatorTest";assert(RegisterClassW(&wc));
 HWND window=CreateWindowW(wc.lpszClassName,L"SSC creator test",WS_OVERLAPPEDWINDOW,0,0,64,64,nullptr,nullptr,wc.hInstance,nullptr);assert(window);

 // Raw mouse registration is process-local and must preserve the game's flags.
 RAWINPUTDEVICE previous_mouse{1,2,RIDEV_INPUTSINK,window};assert(RegisterRawInputDevices(&previous_mouse,1,sizeof previous_mouse));
 assert(ssc_freecam::register_mouse(window)&&ssc_freecam::had_raw_mouse&&ssc_freecam::saved_raw_mouse.dwFlags==RIDEV_INPUTSINK);
 assert(ssc_freecam::restore_mouse_registration());UINT device_count=8;RAWINPUTDEVICE restored[8]{};assert(GetRegisteredRawInputDevices(restored,&device_count,sizeof(RAWINPUTDEVICE))!=UINT(-1));bool found_mouse=false;for(UINT i=0;i<device_count;++i)if(restored[i].usUsagePage==1&&restored[i].usUsage==2){found_mouse=true;assert(restored[i].dwFlags==RIDEV_INPUTSINK&&restored[i].hwndTarget==window);}assert(found_mouse);
 previous_mouse.dwFlags=RIDEV_REMOVE;previous_mouse.hwndTarget=nullptr;assert(RegisterRawInputDevices(&previous_mouse,1,sizeof previous_mouse));
 assert(ssc_freecam::register_mouse(window)&&!ssc_freecam::had_raw_mouse);assert(ssc_freecam::restore_mouse_registration());
 HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR pfd{};pfd.nSize=sizeof pfd;pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=24;pfd.cDepthBits=24;pfd.cStencilBits=8;
 int format=ChoosePixelFormat(dc,&pfd);assert(format&&SetPixelFormat(dc,format,&pfd));HGLRC rc=wglCreateContext(dc);assert(rc&&wglMakeCurrent(dc,rc));
 glViewport(0,0,16,16);glDisable(GL_DEPTH_TEST);glClearColor(0,0,0,1);glClear(GL_COLOR_BUFFER_BIT);ssc_creator::original=native_draw;
 ssc_creator::hide_crosshair=true;ssc_creator::draw(123,7);unsigned char pixel[4]{};glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==0&&calls==1);
 GLboolean color[4]{},depth=0;GLint stencil=0;glGetBooleanv(GL_COLOR_WRITEMASK,color);glGetBooleanv(GL_DEPTH_WRITEMASK,&depth);glGetIntegerv(GL_STENCIL_WRITEMASK,&stencil);assert(color[0]&&color[1]&&color[2]&&color[3]&&depth);
 ssc_creator::hide_crosshair=false;ssc_creator::draw(123,7);glReadPixels(8,8,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);assert(pixel[0]==255&&calls==2);
 glColorMask(GL_TRUE,GL_FALSE,GL_TRUE,GL_FALSE);glDepthMask(GL_FALSE);glStencilMask(5);
 auto separate=reinterpret_cast<ssc_creator::WriteMask::StencilSeparate>(wglGetProcAddress("glStencilMaskSeparate"));if(separate)separate(GL_BACK,7);
 ssc_creator::hide_crosshair=true;ssc_creator::draw(123,7);
 glGetBooleanv(GL_COLOR_WRITEMASK,color);glGetBooleanv(GL_DEPTH_WRITEMASK,&depth);glGetIntegerv(GL_STENCIL_WRITEMASK,&stencil);assert(color[0]&&!color[1]&&color[2]&&!color[3]&&!depth&&stencil==5&&calls==3);assert(glGetError()==GL_NO_ERROR);
 if(separate){glGetIntegerv(0x8ca5,&stencil);assert(stencil==7);}

 // Hiding diagnostics retains the native callback and restores GL write masks.
 ssc_freecam::native_debug=+[](uintptr_t,uintptr_t,uintptr_t,int){GLboolean c[4];glGetBooleanv(GL_COLOR_WRITEMASK,c);assert(!c[0]&&!c[1]&&!c[2]&&!c[3]);};ssc_freecam::state.active=true;ssc_freecam::hide_diagnostics=true;ssc_freecam::debug_draw(0,0,0,0);glGetBooleanv(GL_COLOR_WRITEMASK,color);assert(color[0]&&!color[1]&&color[2]&&!color[3]);ssc_freecam::hide_diagnostics=false;ssc_freecam::stop();ssc_freecam::native_debug=nullptr;
 // Exercise the exact world-view hook with a real OpenGL context.
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glFrustum(-1,1,-1,1,1,1000);glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 ssc_freecam::state={};ssc_freecam::state.observe(1,2,100,200,1000,1000,1000);assert(ssc_freecam::start(1000));ssc_freecam::camera.zoom=.5f;ssc_freecam::fly=false;
 ssc_freecam::view(-100,-200,-300);float matrix[16]{};glGetFloatv(GL_MODELVIEW_MATRIX,matrix);assert(matrix[12]==-100&&matrix[13]==-200&&matrix[14]==-150);
 glGetFloatv(GL_PROJECTION_MATRIX,matrix);assert(matrix[11]==-1&&matrix[10]<0&&matrix[14]<0);
 ssc_freecam::stop();ssc_freecam::state.observe(1,2,100,200,1000,1000,1000);assert(ssc_freecam::start(1000));ssc_freecam::fly=true;glLoadIdentity();ssc_freecam::view(-100,-200,-300);assert(ssc_freecam::camera.eye.z==300);
 ssc_freecam::camera.move(0,1,0,50);glLoadIdentity();ssc_freecam::view(-400,-500,-900);glGetFloatv(GL_MODELVIEW_MATRIX,matrix);assert(std::abs(matrix[12]+100)<.001f&&std::abs(matrix[13]+200)<.001f&&std::abs(matrix[14]+250)<.001f); // independent of the actor's next native view

 // Native target impulses (jump/damage/pad/drop) and projection changes
 // must not move either camera mode after entry.
 for(bool mode:{false,true}){ssc_freecam::stop();ssc_freecam::state.observe(1,2,100,200,1000,1000,1000);assert(ssc_freecam::start(1000));ssc_freecam::fly=mode;
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glFrustum(-1,1,-1,1,5,2000);glMatrixMode(GL_MODELVIEW);glLoadIdentity();ssc_freecam::view(-100,-200,-300);float stable[16],stable_projection[16];glGetFloatv(GL_MODELVIEW_MATRIX,stable);glGetFloatv(GL_PROJECTION_MATRIX,stable_projection);
 for(float impulse:{40.f,-70.f,500.f,-1000.f}){glMatrixMode(GL_PROJECTION);glLoadIdentity();glFrustum(-2,2,-2,2,5,3000);glMatrixMode(GL_MODELVIEW);glLoadIdentity();glRotatef(3,0,0,1);ssc_freecam::view(-100+impulse,-200,-300+impulse);glGetFloatv(GL_MODELVIEW_MATRIX,matrix);for(int j=0;j<16;++j)assert(std::abs(matrix[j]-stable[j])<.001f);glGetFloatv(GL_PROJECTION_MATRIX,matrix);for(int j=0;j<16;++j)assert(std::abs(matrix[j]-stable_projection[j])<.001f);}}
 // Pan at normal zoom must be byte-for-byte native matrices and render bounds.
 ssc_freecam::stop();ssc_freecam::state.observe(1,2,100,200,1000,1000,1000);assert(ssc_freecam::start(1000));ssc_freecam::fly=false;
 std::vector<unsigned char> memory(0xfadbe0);ssc_freecam::base=reinterpret_cast<uintptr_t>(memory.data());float original_bounds[]={10,20,300,400};std::memcpy(memory.data()+0xfadbd0,original_bounds,sizeof original_bounds);
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glFrustum(-1,1,1,-1,5,2000);float original_projection[16];glGetFloatv(GL_PROJECTION_MATRIX,original_projection);glMatrixMode(GL_MODELVIEW);glLoadIdentity();ssc_freecam::view(-100,-200,-300);
 glGetFloatv(GL_PROJECTION_MATRIX,matrix);assert(std::memcmp(matrix,original_projection,sizeof matrix)==0);assert(std::memcmp(memory.data()+0xfadbd0,original_bounds,sizeof original_bounds)==0&&!ssc_freecam::bounds_saved);
 ssc_freecam::camera.zoom=2;glLoadIdentity();ssc_freecam::view(-100,-200,-300);float changed_bounds[4];std::memcpy(changed_bounds,memory.data()+0xfadbd0,sizeof changed_bounds);assert(changed_bounds[2]==600&&changed_bounds[3]==800); // scale visible area, not the entire map
 glGetFloatv(GL_PROJECTION_MATRIX,matrix);float near_plane=matrix[14]/(matrix[10]-1);assert(std::abs(near_plane-5)<.001f);assert(ssc_freecam::camera.screen_y==-1);
 ssc_freecam::stop();assert(std::memcmp(memory.data()+0xfadbd0,original_bounds,sizeof original_bounds)==0);ssc_freecam::base=0;
 // A close 3D view must reduce submitted XY area while retaining the complete
 // frustum and tile margin; the far-plane option must preserve the near plane.
 ssc_freecam::state.observe(1,2,4000,4000,10000,10000,1000);assert(ssc_freecam::start(1000));ssc_freecam::fly=true;ssc_freecam::draw_distance=1000;ssc_freecam::base=reinterpret_cast<uintptr_t>(memory.data());
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glFrustum(-1,1,1,-1,5,2000);glMatrixMode(GL_MODELVIEW);glLoadIdentity();ssc_freecam::view(-4000,-4000,-300);
 glGetFloatv(GL_PROJECTION_MATRIX,matrix);assert(std::abs(matrix[14]/(matrix[10]-1)-5)<.001f);assert(std::abs(matrix[14]/(matrix[10]+1)-1000)<.1f);
 std::memcpy(changed_bounds,memory.data()+0xfadbd0,sizeof changed_bounds);assert(changed_bounds[2]<1000&&changed_bounds[3]<1000);
 assert(changed_bounds[0]<=3800&&changed_bounds[0]+changed_bounds[2]>=4200);
 float invalid_projection[16]{};float sentinel[]={1,2,3,4};assert(!ssc_freecam::camera.visible_bounds(invalid_projection,10000,10000,sentinel)&&sentinel[0]==1&&sentinel[3]==4);
 ssc_freecam::stop();assert(std::memcmp(memory.data()+0xfadbd0,original_bounds,sizeof original_bounds)==0);ssc_freecam::base=0;ssc_freecam::draw_distance=0;
 ssc_freecam::stop();glLoadIdentity();ssc_freecam::view(-400,-500,-900);glGetFloatv(GL_MODELVIEW_MATRIX,matrix);assert(matrix[12]==-400&&matrix[14]==-900);assert(glGetError()==GL_NO_ERROR);
 // At arbitrary yaw/pitch, billboard rotations cancel the view rotation.
 // Translation (world anchor) remains untouched and native scale stays usable.
 ssc_freecam::state.active=true;ssc_freecam::fly=true;
 for(int i=0;i<80;++i){
  float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,-100,-200,-300,1};
  assert(ssc_freecam::camera.seed(identity));ssc_freecam::camera.look(i*117.f,i*233.f);
  auto v=ssc_freecam::camera.matrix();glLoadMatrixf(v.data());glTranslatef(75,25,90);glRotatef(17.f*i,1,0,0);
  float before[16];glGetFloatv(GL_MODELVIEW_MATRIX,before);
  ssc_freecam::billboard_yaw(31,0,0,1);ssc_freecam::billboard_pitch(45,1,0,0);
  glGetFloatv(GL_MODELVIEW_MATRIX,matrix);
  for(int col=0;col<3;++col)for(int row=0;row<3;++row)assert(std::abs(matrix[col*4+row]-(col==row?1.f:0.f))<.0001f);
  for(int j=12;j<16;++j)assert(matrix[j]==before[j]);
 }
 for(int native_case=0;native_case<3;++native_case){
  ssc_freecam::state.active=native_case!=0;ssc_freecam::fly=native_case!=1;ssc_freecam::camera.seeded=native_case!=2;
  glLoadIdentity();glRotatef(31,0,0,1);glRotatef(45,1,0,0);float expected[16];glGetFloatv(GL_MODELVIEW_MATRIX,expected);
  glLoadIdentity();ssc_freecam::billboard_yaw(31,0,0,1);ssc_freecam::billboard_pitch(45,1,0,0);glGetFloatv(GL_MODELVIEW_MATRIX,matrix);
  for(int j=0;j<16;++j)assert(matrix[j]==expected[j]);
 }
 ssc_freecam::stop();assert(glGetError()==GL_NO_ERROR);
 wglMakeCurrent(nullptr,nullptr);wglDeleteContext(rc);ReleaseDC(window,dc);DestroyWindow(window);UnregisterClassW(wc.lpszClassName,wc.hInstance);
 std::cout<<"PASS: current native ABI guard, changed-code rejection, native callback retained, hidden/visible pixels and GL write-mask restoration\n";
}
