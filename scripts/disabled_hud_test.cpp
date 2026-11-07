#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cassert>
#include <iostream>
#include "../runtime/hud_editor.h"
int main(){
 using namespace ssc_hud;
 enabled=false;editing=false;active_item=2;game_base=1;
 finish_frame(1920,1080,1000);assert(!capture_frame);
 // Even a stale capture flag must not touch OpenGL or game memory when off.
 capture_frame=true;assert(!start_capture());
 capture_queued_rectangle(reinterpret_cast<const unsigned char*>(1));
 capture_cached_quad();assert(capture_depth==0&&queued_frame[2].points==0);
 editing=true;finish_frame(1920,1080,2000);assert(capture_frame);
 editing=false;finish_frame(1920,1080,3000);assert(!capture_frame);
 enabled=true;finish_frame(1920,1080,4000);assert(capture_frame);
 enabled=false;finish_frame(1920,1080,5000);assert(!capture_frame);
 std::cout<<"HUD capture: disabled skips native/GL access; editing and enabled capture remain available.\n";
}
