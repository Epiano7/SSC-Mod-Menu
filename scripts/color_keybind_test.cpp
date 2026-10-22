#include "../runtime/overlay.cpp"
#include <cassert>
#include <iostream>
static LRESULT CALLBACK game_proc(HWND,UINT,WPARAM,LPARAM){return 73;}
static float red,green,blue,phase_seen,alpha_seen;static int scope_seen;
static float fake_draw(void*,float,float,void*,float,float r,float g,float b,float a,int,float,unsigned char,unsigned char,float phase){red=r;green=g;blue=b;alpha_seen=a;phase_seen=phase;scope_seen=ssc_names::color_scope;return 42;}
template<class T> void put(uintptr_t address,T value){std::memcpy(reinterpret_cast<void*>(address),&value,sizeof(value));}
static void small(uintptr_t p,const char* text){size_t n=std::strlen(text);assert(n<16);std::memcpy(reinterpret_cast<void*>(p),text,n+1);put(p+16,n);put(p+24,size_t(15));}
static unsigned char consume_account(void*,int,void* p){small(reinterpret_cast<uintptr_t>(p),"consumed");return 0;}
static bool shared_fixture(const std::string& key,ssc_shared::Style& s){s.mode=ssc_shared::Mode::solid;s.count=1;s.colors[0]=0x123456;return key=="remote";}
int main(){
 state_dir=std::filesystem::current_path()/"keybind-test-state";std::filesystem::create_directories(state_dir);
 previous_proc=game_proc;visibility=0;
 constexpr LPARAM right=1|(0x36LL<<16),left=1|(0x2aLL<<16);
 assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,left)==73&&!opened);
 assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,right)==0&&opened);
 assert(window_proc(nullptr,WM_KEYDOWN,VK_SHIFT,right|(1LL<<30))==0&&opened);
 manager=true;settings_page=1;activate(55);assert(menu_binding);
 window_proc(nullptr,WM_KEYDOWN,VK_F8,1);assert(menu_hotkey==VK_F8&&!menu_binding&&opened);
 window_proc(nullptr,WM_KEYUP,VK_F8,1);window_proc(nullptr,WM_KEYDOWN,VK_F8,1);assert(!opened);
 menu_hotkey=VK_RSHIFT;load_settings();assert(menu_hotkey==VK_F8);
 window_proc(nullptr,WM_KEYDOWN,VK_F8,1);assert(opened);
 manager=true;settings_page=1;activate(55);window_proc(nullptr,WM_KEYDOWN,'K',1);assert(menu_hotkey=='K');
 assert(window_proc(nullptr,WM_CHAR,'k',1)==0);window_proc(nullptr,WM_KEYUP,'K',1);
 activate(55);window_proc(nullptr,WM_SYSKEYDOWN,VK_MENU,1|(1LL<<24));assert(menu_hotkey==VK_RMENU);
 window_proc(nullptr,WM_SYSKEYUP,VK_MENU,1|(1LL<<24));activate(56);assert(menu_hotkey==VK_RSHIFT);
 activate(55);window_proc(nullptr,WM_KILLFOCUS,0,0);assert(!menu_binding);
 assert(physical_menu_key(VK_CONTROL,1|(1LL<<24))==VK_RCONTROL);
 assert(valid_menu_key(VK_ESCAPE)&&valid_menu_key(VK_OEM_1)&&!valid_menu_key(999));
 // Killfeed display labels need not match the account resolved by the predicate.
 using namespace ssc_names;native_draw=fake_draw;cosmetics=true;rainbow=false;gradient=false;solid_rgb=0x336699;
 auto draw_event=[](){return event_draw(nullptr,0,0,reinterpret_cast<void*>(4),12,1,1,1,.5f,0,0,0,0,.8f);};
 event_appearance.own=true;assert(draw_event()==42&&scope_seen==1&&red>.19f&&red<.21f);assert(!event_appearance.own);
 draw_event();assert(scope_seen==0&&red==1&&phase_seen==.8f);
 event_appearance.remote=true;event_appearance.style.mode=ssc_shared::Mode::solid;event_appearance.style.count=1;event_appearance.style.colors[0]=0x123456;
 draw_event();assert(scope_seen==3&&red>.07f&&red<.08f&&alpha_seen==.5f);
 attached=true;bot_layout=true;render_access=true;render_access_at=GetTickCount64();bot_rgb=0xffbc41;bot_alpha=128;
 event_appearance.bot=true;draw_event();assert(red==1&&green>.73f&&green<.74f&&phase_seen==-1&&alpha_seen>.25f&&alpha_seen<.26f);
 render_access=false;event_appearance.bot=true;draw_event();assert(phase_seen==.8f&&alpha_seen==.5f);
 assert(identity_scope==-1&&color_scope==0);
 // Real MSVC account-string and actor layouts: predicate sees raw name,
 // then native code consumes it before the formatted label is drawn.
 std::vector<unsigned char> image(0x1000000),world(0xb000),actors(0x34b0*2);
 image_base=reinterpret_cast<uintptr_t>(image.data());auto w=reinterpret_cast<uintptr_t>(world.data()),a=reinterpret_cast<uintptr_t>(actors.data());
 put(image_base+0xde95d0,w);put(w+0xa650,a);put(w+0xa658,a+actors.size());put(image_base+0xe21140,a);put(image_base+0xe1d688,0);
 small(image_base+0xe031e8,"owner");put(a+0x78,0);put(a+0x7dc,0);put(a+0x81,(unsigned char)1);small(a+0x6f0,"owner");small(a+0x798,"owner");
 auto bot=a+0x34b0;put(bot+0x78,1);put(bot+0x7dc,1);put(bot+0x81,(unsigned char)1);small(bot+0x798,"robot");
 put(image_base+0xfb389c,1);put(w+0x190,0);put(w+0x28c,1);
 shared_label_world=w;shared_label_index["robot"]=bot;render_access=true;render_access_at=GetTickCount64();native_predicate=consume_account;lookup_shared=shared_fixture;
 std::array<unsigned char,32> raw{};auto rawaddr=reinterpret_cast<uintptr_t>(raw.data());
 small(rawaddr,"robot");assert(event_predicate(nullptr,0,raw.data())==0);assert(event_appearance.bot);assert(std::strcmp(reinterpret_cast<char*>(raw.data()),"consumed")==0);
 draw_event();assert(phase_seen==-1&&green>.73f&&green<.74f);
 small(rawaddr,"remote");event_predicate(nullptr,0,raw.data());assert(event_appearance.remote&&!event_appearance.bot);draw_event();assert(scope_seen==3);
 small(rawaddr,"owner");event_predicate(nullptr,0,raw.data());assert(event_appearance.own);draw_event();assert(scope_seen==1);
 small(rawaddr,"unknown");event_predicate(nullptr,0,raw.data());draw_event();assert(scope_seen==0&&phase_seen==.8f);
 // Canonical UI rows work without a world, and override native rainbow or friend green.
 put(image_base+0xde95d0,uintptr_t(0));frame_styles.clear();
 auto row=[&](){return canonical_account_draw(nullptr,0,0,raw.data(),12,0,1,0,.4f,0,0,0,0,.8f);};
 small(rawaddr,"remote");assert(row()==42&&scope_seen==3&&phase_seen==0&&alpha_seen==.4f);
 small(rawaddr,"owner");row();assert(scope_seen==1&&red>.19f&&red<.21f);
 small(rawaddr,"unknown");row();assert(scope_seen==0&&red==0&&green==1&&phase_seen==.8f);
 assert(identity_scope==-1&&color_scope==0);
 // Vehicle attribution uses its actual occupant, including prefixed/renamed bot labels.
 put(image_base+0xde95d0,w);std::array<unsigned char,0x400> vehicle{};std::array<unsigned char,0x70> seats{};
 auto v=reinterpret_cast<uintptr_t>(vehicle.data()),seat=reinterpret_cast<uintptr_t>(seats.data());put(v+0x368,seat);put(seat+0x60,bot);
 auto car=[&](){return ssc_vehicle_name_draw(nullptr,0,0,raw.data(),12,1,1,1,.5f,0,0,0,0,.8f,v);};
 small(rawaddr,"not-account");render_access=true;render_access_at=GetTickCount64();car();assert(green>.73f&&green<.74f&&phase_seen==-1);
 put(seat+0x60,a);car();assert(scope_seen==1);
 put(bot+0x7dc,0);small(bot+0x6f0,"remote");put(seat+0x60,bot);car();assert(scope_seen==3);
 put(seat+0x60,uintptr_t(0));car();assert(scope_seen==0&&phase_seen==.8f);
 assert(identity_scope==-1&&color_scope==0);
 image_base=0;
 std::cout<<"PASS: menu binding capture, persistence, reset, modifiers, focus; event tint isolation and bot/remote colors\n";
}
