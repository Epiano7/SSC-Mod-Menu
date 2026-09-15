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
static int calls=0;static bool inspect=false;static std::array<float,16> expected{};
static void __cdecl original(uintptr_t value){assert(value==123);++calls;if(inspect){float p[16];glGetFloatv(GL_PROJECTION_MATRIX,p);for(int i=0;i<16;++i)assert(std::abs(p[i]-expected[i])<.00001f);}}
int main(){
    ssc_hud::reset();for(auto& item:ssc_hud::items){auto m=ssc_hud::transform(item);for(int i=0;i<16;++i)assert(m[i]==(i%5==0?1.f:0.f));}
    auto& item=ssc_hud::items[0];item.x=.1f;item.y=.2f;item.scale=1.5f;auto m=ssc_hud::transform(item);
    assert(std::abs(m[0]*(2*item.home_x-1)+m[12]-(2*item.x-1))<.00001f);
    assert(std::abs(m[5]*(1-2*item.home_y)+m[13]-(1-2*item.y))<.00001f);
    item.x=100;item.y=-100;item.scale=99;ssc_hud::constrain(item);assert(item.scale==6&&item.y==0&&item.x==0);
    item.x=-100;ssc_hud::constrain(item);assert(std::abs(item.x+item.w*item.scale-1)<.00001f);
    item.x=NAN;item.y=INFINITY;item.scale=NAN;ssc_hud::constrain(item);assert(std::isfinite(item.x)&&std::isfinite(item.y)&&item.scale==1);
    ssc_hud::originals[0]=original;ssc_hud::enabled=false;ssc_hud::draw<0>(123);assert(calls==1);
    ssc_hud::reset();ssc_hud::enabled=true;ssc_hud::draw<0>(123);assert(calls==2);
    // A hidden, private GL context validates the transform and state restoration.
    WNDCLASSW cls{};cls.style=CS_OWNDC;cls.lpfnWndProc=DefWindowProcW;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"SSCHudTest";assert(RegisterClassW(&cls));
    HWND window=CreateWindowW(cls.lpszClassName,L"HUD test",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,cls.hInstance,nullptr);assert(window);HDC dc=GetDC(window);
    PIXELFORMATDESCRIPTOR pf{};pf.nSize=sizeof(pf);pf.nVersion=1;pf.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pf.iPixelType=PFD_TYPE_RGBA;pf.cColorBits=24;assert(SetPixelFormat(dc,ChoosePixelFormat(dc,&pf),&pf));
    HGLRC rc=wglCreateContext(dc);assert(rc&&wglMakeCurrent(dc,rc));glMatrixMode(GL_PROJECTION);glLoadIdentity();glMatrixMode(GL_MODELVIEW);glLoadIdentity();glTranslatef(3,4,0);
    item.x=.1f;item.y=.2f;item.scale=1.25f;expected=ssc_hud::transform(item);inspect=true;ssc_hud::draw<0>(123);assert(calls==3);
    GLint mode;float projection[16],model[16];glGetIntegerv(GL_MATRIX_MODE,&mode);assert(mode==GL_MODELVIEW);glGetFloatv(GL_PROJECTION_MATRIX,projection);for(int i=0;i<16;++i)assert(projection[i]==(i%5==0?1.f:0.f));glGetFloatv(GL_MODELVIEW_MATRIX,model);assert(model[12]==3&&model[13]==4);assert(glGetError()==GL_NO_ERROR);
    wglMakeCurrent(nullptr,nullptr);wglDeleteContext(rc);ReleaseDC(window,dc);DestroyWindow(window);UnregisterClassW(cls.lpszClassName,cls.hInstance);
    std::puts("PASS: HUD anchors, bounds, invalid settings, disabled/default passthrough and GL projection/modelview restoration");
}
