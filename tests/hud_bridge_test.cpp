#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include "../runtime/hud_editor.h"
using U=uintptr_t;
static double __cdecl native(U a,double b,float c,U d,U e,U f,U g,U h,U i,U j,U k,U l,U m,U n,U o,U p,U q,U r,U s,U t){
 assert(a==1&&b==2.25&&c==3.5f&&d==4);
 const U stack[]={e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t};for(U x=0;x<16;++x)assert(stack[x]==x+5);
 return 123.875;
}
static void matrix_is(const std::array<float,16>& expected){float actual[16];glGetFloatv(GL_PROJECTION_MATRIX,actual);for(int i=0;i<16;++i)assert(std::abs(actual[i]-expected[i])<.00001f);}
static float mouse_x=0,mouse_y=0;static int fade_calls=0;
static float __cdecl native_fade(U,float alpha,float x,float y,float w,float h,float feather,char){++fade_calls;assert(std::abs(x-100)<.01f&&std::abs(y-120)<.01f&&std::abs(w-240)<.01f&&std::abs(h-210)<.01f&&feather==90);return mouse_x>=x&&mouse_x<=x+w&&mouse_y>=y&&mouse_y<=y+h?alpha*.2f:alpha;}
int main(){
 // Exercise the real machine bridge with mixed register and all 16 stack arguments.
 auto stub=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(stub);
 const unsigned index=14;const uintptr_t entry=reinterpret_cast<uintptr_t>(ssc_hud::ssc_hud_bridge);
 const unsigned char code[]={0x41,0xba,0,0,0,0,0xff,0x25,0,0,0,0};std::memcpy(stub,code,12);std::memcpy(stub+2,&index,4);std::memcpy(stub+12,&entry,8);
 DWORD old;assert(VirtualProtect(stub,4096,PAGE_EXECUTE_READ,&old));FlushInstructionCache(GetCurrentProcess(),stub,4096);
 ssc_hud::enabled=false;ssc_hud::game_base=reinterpret_cast<U>(native)-ssc_hud::hooks[index].target;
 auto call=reinterpret_cast<decltype(&native)>(stub);assert(call(1,2.25,3.5f,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)==123.875);
 WNDCLASSW cls{};cls.style=CS_OWNDC;cls.lpfnWndProc=DefWindowProcW;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"SSCHudBridgeTest";assert(RegisterClassW(&cls));
 HWND window=CreateWindowW(cls.lpszClassName,L"HUD bridge test",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,cls.hInstance,nullptr);assert(window);HDC dc=GetDC(window);
 PIXELFORMATDESCRIPTOR pf{};pf.nSize=sizeof(pf);pf.nVersion=1;pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=24;assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&pf),&pf));
 HGLRC rc=wglCreateContext(dc);assert(rc&&wglMakeCurrent(dc,rc));glMatrixMode(GL_PROJECTION);glLoadIdentity();glMatrixMode(GL_MODELVIEW);glLoadIdentity();
 ssc_hud::reset();const auto identity=ssc_hud::transform(ssc_hud::items[0]);ssc_hud::enabled=true;
 ssc_hud::items[2].x=.2f;ssc_hud::items[2].scale=1.3f;ssc_hud::items[5].x=.4f;ssc_hud::items[4].y=.4f;
 ssc_hud::Scope map,round,timer,unrelated;
 ssc_hud::begin(map,ssc_hud::hooks[2]);matrix_is(ssc_hud::transform(ssc_hud::items[2]));
 ssc_hud::begin(round,ssc_hud::hooks[5]);matrix_is(ssc_hud::transform(ssc_hud::items[5]));
 ssc_hud::begin(timer,ssc_hud::hooks[14]);matrix_is(ssc_hud::transform(ssc_hud::items[4]));ssc_hud::end(timer);matrix_is(ssc_hud::transform(ssc_hud::items[5]));
 // The real bridge must also preserve arguments/results while its GL scope is active.
 assert(call(1,2.25,3.5f,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20)==123.875);matrix_is(ssc_hud::transform(ssc_hud::items[5]));
 ssc_hud::end(round);matrix_is(ssc_hud::transform(ssc_hud::items[2]));
 ssc_hud::begin(unrelated,ssc_hud::hooks[10]);matrix_is(identity);ssc_hud::end(unrelated);matrix_is(ssc_hud::transform(ssc_hud::items[2]));
 ssc_hud::end(map);matrix_is(identity);assert(!ssc_hud::map_projection);assert(glGetError()==GL_NO_ERROR);
 // Capture actual rendered vertices, neutralize the old transform, and rebase
 // without moving the user's existing layout. Then prove the real edge reaches x=0.
 ssc_hud::reset();ssc_hud::game_base=0;ssc_hud::view_w=1000;ssc_hud::view_h=600;
 glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,1000,600,0,-1,1);glMatrixMode(GL_MODELVIEW);
 auto& weapon=ssc_hud::items[7];weapon.x=0;weapon.y=.2f;ssc_hud::measured[7]=false;ssc_hud::measured_frame[7]={};
 ssc_hud::native_begin=glBegin;ssc_hud::native_vertex3=glVertex3f;
 ssc_hud::Scope weapon_scope;ssc_hud::begin(weapon_scope,ssc_hud::hooks[7]);
 ssc_hud::capture_begin(GL_QUADS);ssc_hud::capture_v3(900,500,0);ssc_hud::capture_v3(980,500,0);ssc_hud::capture_v3(980,570,0);ssc_hud::capture_v3(900,570,0);glEnd();ssc_hud::end(weapon_scope);
 auto b=ssc_hud::measured_frame[7];assert(b.points==4);ssc_hud::apply_bounds(7,b);
 assert(std::abs(weapon.home_x-.9f)<.00001f&&std::abs(weapon.w-.08f)<.00001f&&std::abs(weapon.x-.07f)<.00001f);
 weapon.x=0;ssc_hud::constrain(weapon);auto edge=ssc_hud::transform(weapon);assert(std::abs(edge[0]*(2*.9f-1)+edge[12]+1)<.00001f);
 // Resize and move the native hover rectangle with the visible HUD; the old
 // location no longer fades it, while the new location does.
 weapon.x=.1f;weapon.y=.2f;weapon.scale=3;ssc_hud::active_item=7;ssc_hud::game_base=reinterpret_cast<U>(native_fade)-0x443300;
 ssc_hud::fade_w=1000;ssc_hud::fade_h=600;ssc_hud::view_w=2560;ssc_hud::view_h=1440;
 mouse_x=940;mouse_y=535;assert(ssc_hud::fade_hook(0,1,900,500,80,70,30,0)==1);
 mouse_x=120;mouse_y=140;assert(ssc_hud::fade_hook(0,1,900,500,80,70,30,0)==.2f);
 ssc_hud::editing=true;assert(ssc_hud::fade_hook(0,1,900,500,80,70,30,0)==1&&fade_calls==2);ssc_hud::editing=false;
 assert(glGetError()==GL_NO_ERROR);
 wglMakeCurrent(nullptr,nullptr);wglDeleteContext(rc);ReleaseDC(window,dc);DestroyWindow(window);UnregisterClassW(cls.lpszClassName,cls.hInstance);VirtualFree(stub,0,MEM_RELEASE);
 std::puts("PASS: real HUD bridge mixed register/16 stack arguments, floating return, independent map/round/timer transforms and unrelated draw isolation");
}
