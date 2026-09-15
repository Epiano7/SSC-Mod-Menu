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
    assert(window_proc(nullptr,WM_KEYDOWN,VK_ESCAPE,1)==0 && !opened);
    assert(window_proc(nullptr,WM_KEYUP,VK_ESCAPE,3LL<<30)==0);
    assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,left)==73 && !opened);
    std::puts("PASS: Right Shift open/close, scan-code distinction, repeat suppression, key-up consumption, Escape, Left Shift and Insert preserved");
}
