#include "../runtime/overlay.cpp"
#include <cassert>

static LRESULT CALLBACK game_proc(HWND,UINT,WPARAM,LPARAM) { return 73; }
int main() {
    previous_proc=game_proc;
    constexpr LPARAM right=1|(0x36LL<<16),left=1|(0x2aLL<<16);
    assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,left)==73 && !opened);
    assert(window_proc(nullptr,WM_KEYDOWN,VK_INSERT,1)==73 && !opened);
    assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,right)==0 && opened);
    assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,right|(1LL<<30))==0 && opened);
    assert(window_proc(nullptr,WM_KEYUP,VK_SHIFT,right|(3LL<<30))==0 && opened);
    assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,right)==0 && !opened);
    assert(window_proc(nullptr,WM_KEYUP,VK_SHIFT,right|(3LL<<30))==0 && !opened);
    assert(window_proc(nullptr,WM_KEYDOWN,VK_RSHIFT,right)==0 && opened);
    visibility=1;
    assert(window_proc(nullptr,WM_KILLFOCUS,0,0)==73&&opened&&visibility==1);
    manager=true;settings_page=4;color_editing=true;color_buffer=L"12AB";
    pressed=189;hovered=189;
    assert(window_proc(nullptr,WM_ACTIVATEAPP,FALSE,0)==73);
    assert(opened&&manager&&settings_page==4&&color_editing&&color_buffer==L"12AB"&&pressed==0&&hovered==0);
    assert(window_proc(nullptr,WM_ACTIVATEAPP,TRUE,0)==73&&opened);
    color_editing=false;manager=true;settings_page=8;welcome_seen=false;
    assert(window_proc(nullptr,WM_KILLFOCUS,0,0)==73&&opened&&!welcome_seen&&settings_page==8);
    manager=false;ssc_hud::editing=true;hud_drag=6;ssc_hud::freeze_bounds=true;
    assert(window_proc(nullptr,WM_KILLFOCUS,0,0)==73&&opened&&ssc_hud::editing);
    assert(hud_drag==-1&&!ssc_hud::freeze_bounds);
    ssc_hud::editing=false;
    assert(window_proc(nullptr,WM_KEYDOWN,VK_ESCAPE,1)==0 && !opened);
    assert(window_proc(nullptr,WM_KEYUP,VK_ESCAPE,3LL<<30)==0);
    visibility=0; // Closing animation has completed before gameplay input resumes.
    assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,left)==73 && !opened);
    std::puts("PASS: Right Shift/Escape handling; focus loss preserves panels, drafts, welcome and HUD editor while ending gestures");
}
