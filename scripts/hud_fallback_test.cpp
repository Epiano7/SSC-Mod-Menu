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
 reset();game_base=1;enabled=false;editing=true;
 finish_frame(1920,1080,1000);
 assert(!visible(2,1000)); // No native group seen: no phantom menu targets.
 Scope scope{};Hook hook{0,0,2,0};begin(scope,hook);end(scope);
 assert(visible(2,1000)&&fallback_bounds(2,1000));
 assert(!visible(1,1000)&&!visible(0,1000)&&!visible(-1,1000));
 assert(!visible(2,3000)); // No native calls: expires normally.
 last_drawn[2]=4000;assert(!visible(2,3999));
 editing=false;assert(!visible(2,4000));
 editing=true;Bounds b{.1f,.2f,.3f,.4f,4};apply_bounds(2,b);last_seen[2]=4000;
 assert(visible(2,4000)&&!fallback_bounds(2,4000));
 last_drawn[2]=6500;assert(visible(2,6500)&&fallback_bounds(2,6500)); // Cached measured box is still usable.
 items[2].x=.4f;items[2].y=.5f;items[2].scale=1.5f;
 auto m=transform(items[2]);assert(std::abs(m[0]*(2*items[2].home_x-1)+m[12]-(2*items[2].x-1))<.00001f);
 finish_frame(1280,720,6600);assert(!measured[2]&&fallback_bounds(2,6600));
 editing=false;enabled=false;finish_frame(1280,720,6700);assert(!capture_frame&&!visible(2,6700));
 std::cout<<"PASS unmeasured native group selection, no phantom targets, expiry, cached bounds, exact bounds priority, resize, transform, disabled behavior\n";
}
