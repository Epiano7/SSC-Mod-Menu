// Included inside the runtime's private namespace. No game code addresses or bundled assets.
constexpr bool developer_tools=false;
using Swap=BOOL(WINAPI*)(HDC);
Swap original_swap;
WNDPROC previous_proc;
HWND game_window;
bool opened=false,clock_enabled=false,dirty=true,save_failed=false,suppress_escape_up=false;
bool animations=true,sound_requested=false,cosmetic_requested=false,match_sound_level=true;
bool rpc_requested=false,rpc_timer=true,rpc_rating=true,rpc_editing=false,rpc_select_all=false,rpc_error=false;
std::string rpc_id=ssc_rpc::bundled_application_id;std::wstring rpc_buffer;ssc_rpc::Snapshot rpc_preview;
bool sound_search_editing=false;std::wstring sound_query;size_t sound_page=0;std::vector<size_t> sound_results;
void filter_sounds(){sound_results.clear();auto query=sound_query;std::transform(query.begin(),query.end(),query.begin(),towlower);for(size_t i=0;i<ssc_sound::entries.size();++i){auto label=ssc_sound::entries[i].label;std::transform(label.begin(),label.end(),label.begin(),towlower);if(label.find(query)!=std::wstring::npos)sound_results.push_back(i);}sound_page=0;dirty=true;}
int settings_page=0,ui_scale=100;
bool manager=false;
constexpr int canvas_w=1120,canvas_h=720;
float raster_scale=1.f;int raster_w=canvas_w,raster_h=canvas_h;
int panel_w=540,panel_h=384,panel_x=24,panel_y=40;
float draw_scale=1.f,visibility=0.f,modal_visibility=0.f;
ULONGLONG last_frame=0;
int hovered=0,pressed=0,keyboard_focus=0;
GLuint texture;
HGLRC owner_context;
HDC canvas;
HBITMAP bitmap;
HFONT font,title_font;
void* pixels;
std::filesystem::path state_dir;
const COLORREF ink=RGB(236,242,255),muted=RGB(153,175,206),cyan=RGB(76,193,255);
struct Control {int id,x,y,w,h;bool enabled;};
std::vector<Control> controls;

float unit(float value){return std::clamp(value,0.f,1.f);}
void log(const char* message) {
    if(state_dir.empty())return;
    std::ofstream out(state_dir/L"runtime.log",std::ios::app);out<<message<<"\n";
}
void cancel_edit(){rpc_editing=false;rpc_error=false;dirty=true;}
void save() {
    ssc_names::cosmetics=cosmetic_requested;
    ssc_rpc::submit(rpc_requested,rpc_id,rpc_timer,rpc_preview);
    if(state_dir.empty())return;
    auto path=state_dir/L"menu.ini",temp=state_dir/L"menu.ini.tmp";
    std::ofstream out(temp);
    out<<"hud_enabled="<<ssc_hud::enabled<<"\n";for(const auto& item:ssc_hud::items){out<<"hud_box_"<<item.key<<"="<<item.home_x<<","<<item.home_y<<","<<item.w<<","<<item.h<<"\n";out<<"hud_"<<item.key<<"="<<item.x<<","<<item.y<<","<<item.scale<<"\n";}
    out<<"schema=2\nclock="<<clock_enabled
       <<"\nsound_requested="<<sound_requested<<"\ncosmetic_requested="<<cosmetic_requested<<"\nmatch_sound_level="<<match_sound_level
       <<"\nrpc_requested="<<rpc_requested<<"\nrpc_id="<<rpc_id<<"\nrpc_timer="<<rpc_timer<<"\nrpc_rating="<<rpc_rating
       <<"\nanimations="<<animations<<"\nui_scale="<<ui_scale<<"\n";out.close();
    save_failed=!out||!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);dirty=true;
}
constexpr float hud_view_x=266,hud_view_y=188,hud_view_w=820,hud_view_h=461;
int hud_drag=-1;bool hud_resize=false;float hud_start_x=0,hud_start_y=0,hud_item_x=0,hud_item_y=0,hud_item_scale=1;
void finish_hud_drag(){ssc_hud::freeze_bounds=false;if(hud_drag>=0){hud_drag=-1;save();}}
int hud_hit(float x,float y){
    for(int i=int(ssc_hud::items.size())-1;i>=0;--i){auto& item=ssc_hud::items[i];float bx=hud_view_x+item.x*hud_view_w,by=hud_view_y+item.y*hud_view_h;
        if(x>=bx-6&&x<=bx+item.w*item.scale*hud_view_w+6&&y>=by-6&&y<=by+item.h*item.scale*hud_view_h+6)return i;}
    return -1;
}
void move_hud(float x,float y){
    if(hud_drag<0)return;
    auto& item=ssc_hud::items[hud_drag];float dx=(x-hud_start_x)/hud_view_w,dy=(y-hud_start_y)/hud_view_h;
    if(hud_resize)item.scale=hud_item_scale+std::max(dx/item.w,dy/item.h);else{item.x=hud_item_x+dx;item.y=hud_item_y+dy;}
    ssc_hud::constrain(item);dirty=true;
}
void load_settings() {
    std::ifstream in(state_dir/L"menu.ini");std::string line;bool version2=false;
    // Defaults are inert. Only v2 carries the test clock forward.
    while(std::getline(in,line)) {
        auto at=line.find('=');if(at==std::string::npos)continue;
        std::string key=line.substr(0,at),value=line.substr(at+1);int number=0;
        if(key=="hud_enabled"){ssc_hud::enabled=value=="1";continue;}
        if(key.rfind("hud_box_",0)==0){for(int i=0;i<int(ssc_hud::items.size());++i)if(key==std::string("hud_box_")+ssc_hud::items[i].key){float x,y,w,h;char extra;if(std::sscanf(value.c_str(),"%f,%f,%f,%f%c",&x,&y,&w,&h,&extra)==4&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(w)&&std::isfinite(h)&&x>=-.5f&&y>=-.5f&&w>0&&h>0){ssc_hud::apply_bounds(i,{x,y,x+w,y+h,2});ssc_hud::measured[i]=false;}}continue;}
        if(key.rfind("hud_",0)==0){for(auto& item:ssc_hud::items)if(key==std::string("hud_")+item.key){float x,y,z;char extra;if(std::sscanf(value.c_str(),"%f,%f,%f%c",&x,&y,&z,&extra)==3){item.x=x;item.y=y;item.scale=z;ssc_hud::constrain(item);}}continue;}
        if(key=="rpc_id"){if(value.empty()||ssc_rpc::valid_id(value))rpc_id=value.empty()?ssc_rpc::bundled_application_id:value;continue;}
        try {size_t used=0;number=std::stoi(value,&used);if(used!=value.size())continue;}catch(...){continue;}
        if(key=="schema")version2=number==2;
        else if(key=="clock")clock_enabled=number==1;
        else if(key=="sound_requested")sound_requested=number==1;
        else if(key=="cosmetic_requested")cosmetic_requested=number==1;
        else if(key=="rpc_requested")rpc_requested=number==1;
        else if(key=="rpc_rating")rpc_rating=number!=0;
        else if(key=="rpc_timer")rpc_timer=number!=0;
        else if(key=="match_sound_level")match_sound_level=number==1;
        else if(key=="animations")animations=number!=0;
        else if(key=="ui_scale"&&(number==85||number==100||number==115))ui_scale=number;
    }
    if(!version2||!developer_tools)clock_enabled=false;
}
float ease(float t) {return 1.f-(1.f-t)*(1.f-t)*(1.f-t);}
void advance_animation(float seconds) {
    float goal=opened?1.f:0.f;
    if(!animations){visibility=goal;modal_visibility=manager&&opened?1.f:0.f;}
    else {float delta=std::max(0.f,std::min(seconds,.05f))/(opened?.18f:.14f);visibility=std::max(0.f,std::min(1.f,visibility+(opened?delta:-delta)));float modal_goal=manager&&opened?1.f:0.f;modal_visibility=modal_goal>modal_visibility?std::min(modal_goal,modal_visibility+delta):std::max(modal_goal,modal_visibility-delta);}
}
void close_menu(bool immediate=false) {if(ssc_hud::editing){ssc_hud::editing=false;save();}finish_hud_drag();opened=false;dirty=true;pressed=0;keyboard_focus=0;cancel_edit();if(immediate){visibility=0;modal_visibility=0;}ReleaseCapture();log("Menu closed");}
void show_manager(int page=-1) {finish_hud_drag();ReleaseCapture();cancel_edit();manager=true;modal_visibility=0;settings_page=page;visibility=animations?0.f:1.f;dirty=true;hovered=pressed=keyboard_focus=0;controls.clear();}
void show_quick() {finish_hud_drag();ReleaseCapture();cancel_edit();manager=false;visibility=animations?0.f:1.f;dirty=true;hovered=pressed=keyboard_focus=0;controls.clear();}
void start_live_hud(){finish_hud_drag();ReleaseCapture();cancel_edit();ssc_hud::enabled=true;ssc_hud::editing=true;ssc_hud::capture_frame=true;opened=true;manager=false;visibility=1;modal_visibility=0;controls.clear();pressed=hovered=keyboard_focus=0;dirty=true;save();}
int live_hud_hit(float x,float y){
 auto inside=[&](int i){if(!ssc_hud::visible(i,GetTickCount64()))return false;const auto& a=ssc_hud::items[i];float px=6.f/std::max(1,ssc_hud::view_w),py=6.f/std::max(1,ssc_hud::view_h);return x>=a.x-px&&x<=a.x+a.w*a.scale+px&&y>=a.y-py&&y<=a.y+a.h*a.scale+py;};
 if(inside(ssc_hud::selected))return ssc_hud::selected;
 int best=-1;float area=100;
 for(int i=0;i<int(ssc_hud::items.size());++i)if(inside(i)){const auto& a=ssc_hud::items[i];float size=a.w*a.h*a.scale*a.scale;if(size<area){best=i;area=size;}}
 return best;
}
void live_hud_move(float x,float y){if(hud_drag<0)return;auto& a=ssc_hud::items[hud_drag];float dx=x-hud_start_x,dy=y-hud_start_y;if(hud_resize)a.scale=hud_item_scale+std::max(dx/a.w,dy/a.h);else{a.x=hud_item_x+dx;a.y=hud_item_y+dy;}ssc_hud::constrain(a);dirty=true;}
void live_hud_wheel(int delta,int hovered_item){if(hud_drag>=0){finish_hud_drag();ReleaseCapture();}if(hovered_item>=0)ssc_hud::selected=hovered_item;auto& a=ssc_hud::items[ssc_hud::selected];a.scale*=std::pow(1.1f,float(delta)/WHEEL_DELTA);ssc_hud::constrain(a);save();}
void activate(int id) {
    if(id!=120&&id!=121)rpc_editing=false;
    if(id!=104)sound_search_editing=false;
    if(id==1)close_menu();
    else if(id==3)show_manager();
    else if(id==4){ssc_hud::enabled=false;sound_requested=false;cosmetic_requested=false;rpc_requested=false;clock_enabled=false;save();}
    else if(id==5)show_quick();
    else if(id>=10&&id<=12){cancel_edit();settings_page=id==10?-1:id-10;dirty=true;hovered=pressed=keyboard_focus=0;controls.clear();}
    else if(id==40){animations=!animations;save();}
    else if(id==41&&developer_tools){clock_enabled=!clock_enabled;save();}
    else if(id>=50&&id<=52){ui_scale=id==50?85:id==51?100:115;save();}
    else if(id==80){sound_requested=!sound_requested;save();}
    else if(id==81){cosmetic_requested=!cosmetic_requested;save();}
    else if(id==82){match_sound_level=!match_sound_level;save();}
    else if(id==86){ssc_hud::enabled=!ssc_hud::enabled;save();}
    else if(id==94)start_live_hud();
    else if(id==146)start_live_hud();
    else if(id==147)close_menu(true);
    else if(id==148){auto& a=ssc_hud::items[ssc_hud::selected];a.x=a.home_x;a.y=a.home_y;a.scale=1;save();}
    else if(id==143){ssc_hud::reset();save();}
    else if(id==144||id==145){ssc_hud::selected=(ssc_hud::selected+(id==144?int(ssc_hud::items.size())-1:1))%int(ssc_hud::items.size());dirty=true;}
    else if(id==83){rpc_requested=!rpc_requested;save();}
    else if(id==85){rpc_rating=!rpc_rating;save();}
    else if(id==84){rpc_timer=!rpc_timer;save();}
    else if(id==120){rpc_editing=true;rpc_select_all=true;rpc_error=false;rpc_buffer=std::wstring(rpc_id.begin(),rpc_id.end());dirty=true;}
    else if(id==121){std::string value(rpc_buffer.begin(),rpc_buffer.end());if(value.empty()||ssc_rpc::valid_id(value)){rpc_id=value.empty()?ssc_rpc::bundled_application_id:value;rpc_editing=false;rpc_error=false;save();}else {rpc_error=true;dirty=true;}}
    else if(id>=91&&id<=93){int page=id==91?3:id==92?4:5;if(!manager)show_manager(page);else {cancel_edit();settings_page=page;dirty=true;controls.clear();}}
    else if(id==100){if(sound_page>=5)sound_page-=5;dirty=true;}
    else if(id==101){if(sound_page+5<sound_results.size())sound_page+=5;dirty=true;}
    else if(id==102){ssc_sound::import_selected(game_window,match_sound_level);if(IsWindow(game_window)&&GetForegroundWindow()==game_window){opened=true;manager=true;settings_page=3;visibility=animations?0.f:1.f;}dirty=true;}
    else if(id==103){ssc_sound::remove_selected();dirty=true;}
    else if(id==104){sound_search_editing=true;dirty=true;}
    else if(id>=110&&id<115){size_t index=sound_page+id-110;if(index<sound_results.size())ssc_sound::selection=sound_results[index];dirty=true;}

}
int hit(int x,int y) {
    if(!opened||visibility<.85f)return 0;
    float lx=(x-panel_x)/draw_scale,ly=(y-panel_y)/draw_scale;
    for(auto& c:controls)if(c.enabled&&lx>=c.x&&lx<c.x+c.w&&ly>=c.y&&ly<c.y+c.h)return c.id;
    return 0;
}
LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    const bool menu_key=wp==VK_RSHIFT||(wp==VK_SHIFT&&((lp>>16)&255)==0x36);
    if(message==WM_KEYDOWN&&menu_key) {
        if(!(lp&(1LL<<30))) {
            if(opened)close_menu(ssc_hud::editing);
            else {
                opened=true;manager=false;dirty=true;hovered=pressed=keyboard_focus=0;controls.clear();
                for(int key=8;key<256;++key)if(GetKeyState(key)&0x8000)
                    CallWindowProcW(previous_proc,window,WM_KEYUP,key,(1LL<<31)|(1LL<<30)|1);
                CallWindowProcW(previous_proc,window,WM_LBUTTONUP,0,0);CallWindowProcW(previous_proc,window,WM_RBUTTONUP,0,0);
                ReleaseCapture();SetCursor(LoadCursor(nullptr,IDC_ARROW));log("Quick menu opened");
            }
        }return 0;
    }
    if(message==WM_KEYUP&&menu_key)return 0;
    if(message==WM_KEYUP&&wp==VK_ESCAPE&&suppress_escape_up){suppress_escape_up=false;return 0;}
    if(message==WM_KILLFOCUS||(message==WM_ACTIVATEAPP&&!wp))close_menu(true);
    if(ssc_hud::editing){
        if(message==WM_SETCURSOR){SetCursor(LoadCursor(nullptr,IDC_ARROW));return TRUE;}
        if(message==WM_KEYDOWN&&wp==VK_ESCAPE){suppress_escape_up=true;close_menu(true);return 0;}
        if(message==WM_KEYDOWN&&wp==VK_TAB&&!(lp&(1LL<<30))){activate((GetKeyState(VK_SHIFT)&0x8000)?144:145);return 0;}
        if(message==WM_KEYDOWN&&wp==VK_HOME){activate(148);return 0;}
        POINT cursor{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        if(message==WM_MOUSEWHEEL)ScreenToClient(window,&cursor);
        float x=float(cursor.x)/std::max(1,ssc_hud::view_w),y=float(cursor.y)/std::max(1,ssc_hud::view_h);
        if(message==WM_LBUTTONDOWN){pressed=hit(cursor.x,cursor.y);if(pressed)return 0;int i=live_hud_hit(x,y);if(i>=0){ssc_hud::selected=i;hud_drag=i;ssc_hud::freeze_bounds=true;auto& a=ssc_hud::items[i];hud_start_x=x;hud_start_y=y;hud_item_x=a.x;hud_item_y=a.y;hud_item_scale=a.scale;hud_resize=cursor.x>=(a.x+a.w*a.scale)*ssc_hud::view_w-12&&cursor.y>=(a.y+a.h*a.scale)*ssc_hud::view_h-12;SetCapture(window);dirty=true;}return 0;}
        if(message==WM_MOUSEMOVE){if(hud_drag>=0)live_hud_move(x,y);return 0;}
        if(message==WM_LBUTTONUP){if(hud_drag>=0){live_hud_move(x,y);finish_hud_drag();ReleaseCapture();}else{int id=hit(cursor.x,cursor.y),down=pressed;pressed=0;if(id&&id==down)activate(id);}return 0;}
        if(message==WM_MOUSEWHEEL){live_hud_wheel(GET_WHEEL_DELTA_WPARAM(wp),live_hud_hit(x,y));return 0;}
        if(message==WM_CAPTURECHANGED){finish_hud_drag();return 0;}
        if((message>=WM_KEYFIRST&&message<=WM_KEYLAST)||(message>=WM_MOUSEFIRST&&message<=WM_MOUSELAST))return 0;
        if(message==WM_INPUT)return DefWindowProcW(window,message,wp,lp);
    }
    if(opened||visibility>0.f) {
        if(opened&&rpc_editing){
            if(message==WM_KEYDOWN&&wp==VK_ESCAPE){rpc_editing=false;rpc_error=false;dirty=true;suppress_escape_up=true;return 0;}
            if(message==WM_KEYDOWN&&wp==VK_RETURN){activate(121);return 0;}
            if(message==WM_KEYDOWN&&wp=='A'&&(GetKeyState(VK_CONTROL)&0x8000)){rpc_select_all=true;return 0;}
            if(message==WM_KEYDOWN&&wp=='V'&&(GetKeyState(VK_CONTROL)&0x8000)){
                if(OpenClipboard(window)){HANDLE h=GetClipboardData(CF_UNICODETEXT);if(h){auto p=static_cast<const wchar_t*>(GlobalLock(h));if(p){size_t cap=GlobalSize(h)/sizeof(wchar_t),n=0;while(n<cap&&n<21&&p[n])++n;if(n<=20&&n<cap&&p[n]==0){rpc_buffer.assign(p,n);rpc_select_all=false;}GlobalUnlock(h);}}CloseClipboard();}dirty=true;return 0;
            }
            if(message==WM_CHAR){wchar_t c=static_cast<wchar_t>(wp);if(c==8){if(rpc_select_all)rpc_buffer.clear();else if(!rpc_buffer.empty())rpc_buffer.pop_back();rpc_select_all=false;}else if(c>=L'0'&&c<=L'9'){if(rpc_select_all)rpc_buffer.clear();rpc_select_all=false;if(rpc_buffer.size()<20)rpc_buffer+=c;}rpc_error=false;dirty=true;return 0;}
        }
        if(sound_search_editing&&opened&&manager&&settings_page==3){
            if(message==WM_KEYDOWN&&(wp==VK_ESCAPE||wp==VK_RETURN)){sound_search_editing=false;dirty=true;return 0;}
            if(message==WM_KEYDOWN&&wp=='A'&&(GetKeyState(VK_CONTROL)&0x8000)){sound_query.clear();filter_sounds();return 0;}
            if(message==WM_CHAR){wchar_t c=static_cast<wchar_t>(wp);if(c==8){if(!sound_query.empty())sound_query.pop_back();}else if(c>=32&&c<127&&sound_query.size()<48)sound_query.push_back(c);filter_sounds();return 0;}
        }
        if(message==WM_SETCURSOR){SetCursor(LoadCursor(nullptr,IDC_ARROW));return TRUE;}
        if(message==WM_KEYDOWN&&wp==VK_ESCAPE){finish_hud_drag();ReleaseCapture();suppress_escape_up=true;if(manager&&opened&&settings_page>=0){settings_page=-1;dirty=true;controls.clear();}else if(manager&&opened)show_quick();else close_menu();return 0;}
        if(opened&&message==WM_KEYDOWN&&wp==VK_TAB&&!(lp&(1LL<<30))) {
            std::vector<int> ids;for(auto& c:controls)if(c.enabled)ids.push_back(c.id);
            auto it=std::find(ids.begin(),ids.end(),keyboard_focus);int n=int(ids.size());
            if(n){int pos=it==ids.end()?-1:int(it-ids.begin());pos=(pos+((GetKeyState(VK_SHIFT)&0x8000)?n-1:1)+n)%n;keyboard_focus=ids[pos];dirty=true;}return 0;
        }
        if(opened&&message==WM_KEYDOWN&&(wp==VK_RETURN||wp==VK_SPACE)&&!(lp&(1LL<<30))){activate(keyboard_focus);return 0;}
        if(manager&&settings_page==6&&visibility>=.85f){
            float mx=(GET_X_LPARAM(lp)-panel_x)/draw_scale,my=(GET_Y_LPARAM(lp)-panel_y)/draw_scale;
            if(message==WM_LBUTTONDOWN){int i=hud_hit(mx,my);if(i>=0){ssc_hud::selected=i;hud_drag=i;auto& item=ssc_hud::items[i];hud_start_x=mx;hud_start_y=my;hud_item_x=item.x;hud_item_y=item.y;hud_item_scale=item.scale;
                hud_resize=mx>=hud_view_x+(item.x+item.w*item.scale)*hud_view_w-12&&my>=hud_view_y+(item.y+item.h*item.scale)*hud_view_h-12;SetCapture(window);dirty=true;return 0;}}
            if(message==WM_MOUSEMOVE&&hud_drag>=0){move_hud(mx,my);return 0;}
            if(message==WM_LBUTTONUP&&hud_drag>=0){move_hud(mx,my);finish_hud_drag();ReleaseCapture();return 0;}
            if(message==WM_MOUSEWHEEL){if(hud_drag>=0)return 0;POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(window,&p);int i=hud_hit((p.x-panel_x)/draw_scale,(p.y-panel_y)/draw_scale);if(i>=0){ssc_hud::selected=i;auto& item=ssc_hud::items[i];item.scale*=std::pow(1.1f,GET_WHEEL_DELTA_WPARAM(wp)/120.f);ssc_hud::constrain(item);save();}return 0;}
        }
        if(message==WM_CAPTURECHANGED)finish_hud_drag();
        if(message==WM_MOUSEMOVE){int id=hit(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));if(hovered!=id){hovered=id;keyboard_focus=0;dirty=true;}}
        if(message==WM_LBUTTONDOWN){pressed=hit(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));dirty=true;}
        if(message==WM_LBUTTONUP){int id=hit(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));int down=pressed;pressed=0;if(id&&id==down)activate(id);dirty=true;}
        if((message>=WM_KEYFIRST&&message<=WM_KEYLAST)||(message>=WM_MOUSEFIRST&&message<=WM_MOUSELAST))return 0;
        if(message==WM_INPUT)return DefWindowProcW(window,message,wp,lp);
    }
    if(message==WM_NCDESTROY){opened=false;visibility=0;game_window=nullptr;}
    return CallWindowProcW(previous_proc,window,message,wp,lp);
}

// Bounds-checked reader for the local game's legacy GLF atlas. Pointer fields are ignored.
struct Glyph {float width,height,u0,v0,u1,v1;};
struct LocalFont {
    int w=0,h=0,start=0,end=-1;float cap_height=64,cap_top=24;std::vector<Glyph> glyphs;std::vector<unsigned char> atlas;
    bool load(const std::filesystem::path& path) {
        std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)return false;
        auto size=in.tellg();if(size<24||size>16*1024*1024)return false;in.seekg(0);
        uint32_t head[6];in.read(reinterpret_cast<char*>(head),24);
        if(head[1]<16||head[1]>2048||head[2]<16||head[2]>2048||head[3]>head[4]||head[4]>255)return false;
        size_t count=head[4]-head[3]+1,bytes=size_t(head[1])*head[2]*2;
        if(size_t(size)!=24+count*sizeof(Glyph)+bytes)return false;
        std::vector<Glyph> gs(count);std::vector<unsigned char> data(bytes);
        in.read(reinterpret_cast<char*>(gs.data()),gs.size()*sizeof(Glyph));in.read(reinterpret_cast<char*>(data.data()),data.size());if(!in)return false;
        for(auto& g:gs)if(!std::isfinite(g.width)||!std::isfinite(g.height)||!std::isfinite(g.u0)||!std::isfinite(g.v0)||!std::isfinite(g.u1)||!std::isfinite(g.v1)||g.width<-.001f||g.width>1||g.height<=0||g.height>1||g.u0<-.00001f||g.v0<-.00001f||g.u1>1.00001f||g.v1>1.00001f||g.u1+.00001f<g.u0||g.v1+.00001f<g.v0)return false;
        w=head[1];h=head[2];start=head[3];end=head[4];glyphs=std::move(gs);atlas=std::move(data);
        if(start<='H'&&end>='H') {
            auto& g=glyphs['H'-start];int top=h,bottom=-1;
            for(int y=std::max(0,int(g.v0*h));y<std::min(h,int(g.v1*h));++y)
                for(int x=std::max(0,int(g.u0*w));x<std::min(w,int(g.u1*w));++x)
                    if(atlas[(y*w+x)*2+1]>128){top=std::min(top,y);bottom=std::max(bottom,y);}
            if(bottom>=top){cap_height=float(bottom-top+1);cap_top=top-g.v0*h;}
        }
        return true;
    }
};
LocalFont local_font,heavy_font;
LocalFont& face(bool heavy) {return heavy&&!heavy_font.atlas.empty()?heavy_font:local_font;}
void rectangle(int x,int y,int w,int h,COLORREF color) {
    RECT r{x,y,x+w,y+h};HBRUSH b=CreateSolidBrush(color);FillRect(canvas,&r,b);DeleteObject(b);
}
void polygon(int x,int y,int w,int h,COLORREF color) {
    POINT p[]={{x+8,y},{x+w-8,y},{x+w,y+8},{x+w,y+h-8},{x+w-8,y+h},{x+8,y+h},{x,y+h-8},{x,y+8}};
    HBRUSH b=CreateSolidBrush(color);auto old=SelectObject(canvas,b);auto pen=SelectObject(canvas,GetStockObject(NULL_PEN));Polygon(canvas,p,8);SelectObject(canvas,old);SelectObject(canvas,pen);DeleteObject(b);
}
void prepare_canvas() {
    int rw=std::max(1,int(std::lround(canvas_w*raster_scale))),rh=std::max(1,int(std::lround(canvas_h*raster_scale)));
    if(canvas&&rw==raster_w&&rh==raster_h)return;
    if(canvas){DeleteDC(canvas);DeleteObject(bitmap);DeleteObject(font);DeleteObject(title_font);canvas=nullptr;}
    raster_w=rw;raster_h=rh;canvas=CreateCompatibleDC(nullptr);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=raster_w;info.bmiHeader.biHeight=-raster_h;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    bitmap=CreateDIBSection(canvas,&info,DIB_RGB_COLORS,&pixels,nullptr,0);SelectObject(canvas,bitmap);
    font=CreateFontW(-18,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    title_font=CreateFontW(-27,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");SetBkMode(canvas,TRANSPARENT);SetMapMode(canvas,MM_ANISOTROPIC);SetWindowExtEx(canvas,canvas_w,canvas_h,nullptr);SetViewportExtEx(canvas,raster_w,raster_h,nullptr);
    wchar_t exe[32768];if(GetModuleFileNameW(nullptr,exe,32768)) {
        auto dir=std::filesystem::path(exe).parent_path()/L"data/translations";
        if(!local_font.load(dir/L"black128_60.glf")&&local_font.atlas.empty())local_font.load(dir/L"regular128_72.glf");
        heavy_font.load(dir/L"thick128_60.glf");
    }
    log(local_font.atlas.empty()?"Menu font: system fallback":"Menu font: local game atlas");
}
float text_width(const wchar_t* value,int size) {
    auto& f=face(true);float width=0,scale=size/f.cap_height;
    for(auto s=value;*s;++s){if(*s==L' '){width+=size*.4f;continue;}int c=int(*s);
        if(c>=f.start&&c<=f.end&&!f.atlas.empty())width+=std::max(0.f,f.glyphs[c-f.start].width)*f.w*scale;
        else width+=size*.7f;
    }return width;
}
// Native rainbow palette: asymmetric channel ramps and luminance balancing.
// A glyph spans index/length to (index + .75)/length, with edge lifts .2/.1.
std::array<float,3> rainbow_rgb(float phase,float lift){
    float t=unit(std::max(0.f,(phase-std::floor(phase))*255.f-1.f)*1.003937006f/255.f);
    auto ramp=[](float d,bool broad){return d<1.0/6?1.f:std::max(0.f,float(1-(double(d)-1.0/6)*(broad?4:6)));};
    float gd=float(std::abs(1.0/3-t)),bd=float(std::abs(2.0/3-t));
    std::array<float,3> c={unit(ramp(std::min(t,1-t),false)+lift),unit(ramp(std::min(gd,1-gd),t<1.0/3||t>5.0/6)+lift),unit(ramp(std::min(bd,1-bd),t<1.0/6||t>2.0/3)+lift)};
    auto luminance=[&](){return c[0]*.35f+c[1]*.55f+c[2]*.1f;};float l=luminance();
    if(l>=.5f){float factor=.5f/l;for(auto& v:c)v=std::min(1.f,v*factor)*.5f+v*.5f;}
    else {for(auto& v:c)v=std::min(1.f,v+(.5f-l)*.5f);l=luminance();if(l<.5f)for(auto& v:c)v=std::min(1.f,v+(.5f-l)*.5f);}
    return c;
}
void text(int x,int y,const wchar_t* value,COLORREF color=ink,bool title=false,int custom_size=0,float rainbow_phase=-1) {
    int size=custom_size?custom_size:title?22:15;auto& f=face(title||custom_size!=0);
    if(f.atlas.empty()){SelectObject(canvas,title?title_font:font);SetTextColor(canvas,color);TextOutW(canvas,x,y,value,lstrlenW(value));return;}
    GdiFlush();float cursor=x*raster_scale,scale=size*raster_scale/f.cap_height;auto dest=static_cast<unsigned char*>(pixels);
    auto coverage=[&](float fx,float fy) {
        fx=std::max(0.f,std::min(float(f.w-1),fx));fy=std::max(0.f,std::min(float(f.h-1),fy));
        int ax=std::min(f.w-2,int(fx)),ay=std::min(f.h-2,int(fy));float tx=fx-ax,ty=fy-ay;
        auto a=[&](int xx,int yy){return f.atlas[(yy*f.w+xx)*2+1]/255.f;};
        return (a(ax,ay)*(1-tx)+a(ax+1,ay)*tx)*(1-ty)+(a(ax,ay+1)*(1-tx)+a(ax+1,ay+1)*tx)*ty;
    };
    for(const wchar_t* s=value;*s;++s) {
        if(*s==L' '){cursor+=size*.4f*raster_scale;continue;}
        int c=int(*s);if(c<f.start||c>f.end)c='?';if(c<f.start||c>f.end)continue;
        auto& g=f.glyphs[c-f.start];float gw=std::max(0.f,g.width)*f.w*scale,gh=g.height*f.h*scale;
        if(gw<=0)continue;
        float top=y*raster_scale-f.cap_top*scale,sx=g.u0*f.w,sy=g.v0*f.h;
        std::array<float,3> left{},right{};if(rainbow_phase>=0){float n=float(lstrlenW(value)),index=float(s-value);left=rainbow_rgb(index/n+rainbow_phase,.2f);right=rainbow_rgb((index+.75f)/n+rainbow_phase,.1f);}
        for(int dy=std::max(0,int(std::floor(top)));dy<std::min(raster_h,int(std::ceil(top+gh)));++dy)
            for(int dx=std::max(0,int(std::floor(cursor)));dx<std::min(raster_w,int(std::ceil(cursor+gw)));++dx) {
                float a=0;
                // Integrate the atlas footprint instead of taking one undersampled point.
                for(int oy=0;oy<4;++oy)for(int ox=0;ox<4;++ox){float px=dx+(ox+.5f)/4,py=dy+(oy+.5f)/4;
                    if(px>=cursor&&px<cursor+gw&&py>=top&&py<top+gh)a+=coverage(sx+(px-cursor)/scale-.5f,sy+(py-top)/scale-.5f)/16.f;}
                auto pixel=dest+(dy*raster_w+dx)*4;unsigned char rgb[]={GetBValue(color),GetGValue(color),GetRValue(color)};
                for(int k=0;k<3;++k){float channel=rgb[k];if(rainbow_phase>=0){float t=unit((dx+.5f-cursor)/gw);channel=255*(left[2-k]*(1-t)+right[2-k]*t);}pixel[k]=static_cast<unsigned char>(pixel[k]*(1-a)+channel*a);}
                pixel[3]=255;
            }
        cursor+=gw; // Preserve native advances; do not round each glyph or add tracking.
    }
}
void button(int id,int x,int y,int w,int h,const wchar_t* label,bool selected=false) {
    controls.push_back({id,x,y,w,h,true});bool focus=hovered==id||keyboard_focus==id;
    COLORREF base=selected?RGB(27,106,196):RGB(33,57,88);
    if(focus)base=RGB(46,121,193);
    if(pressed==id)base=RGB(26,81,133);
    polygon(x,y,w,h,base);rectangle(x+8,y+1,w-16,1,focus?cyan:RGB(63,91,132));
    if(focus)rectangle(x+8,y+h-2,w-16,2,cyan);
    int size=15;while(size>11&&text_width(label,size)>w-28)--size;
    text(x+14,y+(h-size)/2,label,ink,false,size);
}
void toggle(int id,int x,int y,bool enabled) {button(id,x,y,94,36,enabled?L"ON":L"OFF",enabled);rectangle(x+70,y+10,10,16,enabled?RGB(99,239,173):muted);}
void finish_canvas() {
    GdiFlush();auto b=static_cast<unsigned char*>(pixels);
    int pw=int(std::lround(panel_w*raster_scale)),ph=int(std::lround(panel_h*raster_scale));
    for(int y=0;y<ph;++y)for(int x=0;x<pw;++x)b[(y*raster_w+x)*4+3]=255;
    int corner=int(std::lround(10*raster_scale));
    for(int y=0;y<corner;++y)for(int x=0;x<corner-y;++x){b[(y*raster_w+x)*4+3]=0;b[(y*raster_w+pw-1-x)*4+3]=0;b[((ph-1-y)*raster_w+x)*4+3]=0;b[((ph-1-y)*raster_w+pw-1-x)*4+3]=0;}

}
void paint_panel() {
    prepare_canvas();if(!pixels)return;controls.clear();
    panel_w=manager?1120:640;panel_h=manager?720:442;
    std::memset(pixels,0,raster_w*raster_h*4);
    rectangle(0,0,panel_w,panel_h,RGB(8,21,41));rectangle(4,4,panel_w-8,78,RGB(18,41,72));
    rectangle(4,82,panel_w-8,2,RGB(48,135,208));
    text(26,24,L"SSC MODS",ink,true);text(26,58,manager?L"MODULE SETTINGS":L"QUICK MENU",muted);
    button(1,panel_w-60,23,36,36,L"X");
    if(!manager) {
        const wchar_t* names[]={L"SOUND REPLACER",L"COSMETICS",L"DISCORD PRESENCE",L"HUD EDITOR"};
        const bool enabled[]={sound_requested,cosmetic_requested,rpc_requested,ssc_hud::enabled};const int ids[]={80,81,83,86};
        for(int i=0;i<4;++i){int y=100+i*64;rectangle(20,y,600,56,RGB(16,37,62));text(32,y+20,names[i]);toggle(ids[i],350,y+10,enabled[i]);button(91+i,456,y+10,146,36,L"SETTINGS");}
        button(3,20,374,366,42,L"ALL MODULES >",true);button(4,402,374,218,42,L"DISABLE ALL");
        if(save_failed)text(24,425,L"Could not save settings",RGB(247,191,83));
    } else {
        button(10,24,108,192,44,L"MODULES",settings_page==-1||settings_page>=3);
        button(11,24,166,192,44,L"INTERFACE",settings_page==1);
        button(12,24,224,192,44,L"ABOUT",settings_page==2);
        rectangle(238,108,1,546,RGB(38,65,101));

        if(settings_page==-1) {
            text(266,111,L"MODULES",ink,true);
            const wchar_t* names[]={L"Sound replacer",L"Cosmetics",L"Discord presence",L"HUD editor"};
            const bool enabled[]={sound_requested,cosmetic_requested,rpc_requested,ssc_hud::enabled};const int ids[]={80,81,83,86};
            for(int i=0;i<4;++i){int y=174+i*80;rectangle(266,y,820,66,RGB(16,37,62));text(284,y+22,names[i],ink,true);toggle(ids[i],822,y+15,enabled[i]);button(91+i,932,y+15,138,36,L"SETTINGS");}
        } else if(settings_page==1) {
            text(266,111,L"INTERFACE",ink,true);
            rectangle(266,174,820,66,RGB(16,37,62));text(282,198,L"MENU ANIMATIONS");toggle(40,978,189,animations);
            text(266,326,L"UI SCALE");button(50,266,368,190,44,L"85%",ui_scale==85);button(51,470,368,190,44,L"100%",ui_scale==100);button(52,674,368,190,44,L"115%",ui_scale==115);
            text(266,454,L"RIGHT SHIFT",cyan);text(450,454,L"Open / close quick menu");text(266,494,L"ESCAPE",cyan);text(450,494,L"Back to quick menu, then close");
            text(266,550,L"TAB / ENTER",cyan);text(450,550,L"Focus controls / activate");
        } else if(settings_page==3) {
            text(266,111,L"SOUND REPLACER",ink,true);toggle(80,986,106,sound_requested);
            text(266,152,L"PCM WAV replacements / restart game to apply changes",muted);
            if(sound_results.empty()&&sound_query.empty())filter_sounds();
            std::wstring query=L"SEARCH: "+sound_query+(sound_search_editing?L"_":L"");button(104,266,187,540,36,query.c_str(),sound_search_editing);
            button(100,822,187,120,36,L"PREVIOUS");button(101,958,187,128,36,L"NEXT");
            for(size_t i=0;i<5&&sound_page+i<sound_results.size();++i){size_t index=sound_results[sound_page+i];auto label=ssc_sound::entries[index].label+(ssc_sound::entries[index].imported?L" [CUSTOM]":L"");button(110+int(i),266,236+int(i)*42,820,36,label.c_str(),index==ssc_sound::selection);}

            if(ssc_sound::selection<ssc_sound::entries.size()){
                auto& e=ssc_sound::entries[ssc_sound::selection];std::wstring info=e.label+L" / "+std::to_wstring(e.rate)+L" Hz / "+std::to_wstring(e.channels)+L" ch";
                text(266,490,info.c_str(),cyan,false,14);
            }
            button(102,266,518,242,40,L"IMPORT WAV");button(103,526,518,242,40,L"RESTORE ORIGINAL");
            text(792,518,L"MATCH LEVEL",muted,false,12);toggle(82,982,520,match_sound_level);
            text(266,578,ssc_sound::status.c_str(),ink,false,13);
            if(!ssc_sound::attached)text(266,619,L"Audio adapter unavailable",muted,false,13);
        } else if(settings_page==4) {
            text(266,111,L"COSMETICS",ink,true);toggle(81,986,106,cosmetic_requested);
            if(!ssc_names::attached)text(266,160,L"Unavailable on this game version",RGB(247,191,83));
            text(266,337,L"ANIMATED PREVIEW");
            float phase=0;ssc_names::native_phase(phase);
            text(282,409,L"Example player",ink,true,0,phase);

        } else if(settings_page==5) {
            text(266,111,L"DISCORD PRESENCE",ink,true);toggle(83,986,106,rpc_requested);
            auto connection=ssc_rpc::status();text(266,168,connection.c_str(),cyan);
            rectangle(266,212,820,130,RGB(16,37,62));text(282,226,L"ACTIVITY PREVIEW",muted,false,13);
            auto wide=[](const std::string& value){int n=MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),nullptr,0);std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),out.data(),n);return out;};
            auto details=wide(rpc_preview.details),state=wide(rpc_preview.state);
            text(282,266,details.c_str(),ink,false,14);text(282,306,state.c_str(),muted,false,14);
            text(266,388,L"SHOW ELAPSED TIME",ink);toggle(84,986,378,rpc_timer);
            text(266,444,L"SHOW SSC RATING WHEN RANKED",ink);toggle(85,986,434,rpc_rating);

        } else if(settings_page==6){
            text(266,111,L"HUD EDITOR",ink,true);toggle(86,986,106,ssc_hud::enabled);
            button(146,266,188,300,48,L"EDIT HUD");button(143,266,254,300,40,L"RESET LAYOUT");
        } else {
            text(266,111,L"ABOUT SSC MODS",ink,true);
            text(266,157,L"0.1.0-beta.1",cyan);
            text(266,203,L"Optional client-side features for Skillshot City.",muted);
            rectangle(266,255,820,118,RGB(16,37,62));
            text(282,273,L"GAME COMPATIBILITY");
            text(282,314,L"Supported game build: September 12, 2026",muted);
            text(282,343,L"Game updates may require a newer version of SSC Mods.",muted);
            text(266,414,L"UPDATES");
            text(266,455,L"In-app updates coming soon.",muted);
            text(266,496,L"Releases: github.com/Epiano7/SSC-Mod-Client",muted);
            text(266,578,L"Unofficial mod client. Not affiliated with the game developer.",muted,false,14);

        }
        rectangle(20,669,1080,1,RGB(38,65,101));if(save_failed)text(28,687,L"Could not save settings",RGB(247,191,83));
        button(5,862,677,226,34,L"<  QUICK MENU");
    }
    finish_canvas();
}
void paint_live_hud_toolbar(){
 prepare_canvas();if(!pixels)return;std::memset(pixels,0,raster_w*raster_h*4);controls.clear();rectangle(0,0,panel_w,panel_h,RGB(12,30,50));
 wchar_t label[128];const auto& a=ssc_hud::items[ssc_hud::selected];swprintf(label,128,L"%ls  %d%%",a.name,int(std::lround(a.scale*100)));text(12,10,label,cyan,false,16);
 text(12,42,L"Drag / wheel resize / Tab select / Esc done",muted,false,12);
 button(144,386,8,34,28,L"<");button(145,428,8,34,28,L">");button(148,474,8,168,28,L"RESET ITEM");button(147,654,8,94,28,L"DONE");finish_canvas();
}
void layout_panel(int width,int height) {
    if(ssc_hud::editing){panel_w=760;panel_h=68;float base=std::min(1.f,std::max(.1f,float(width-24)/panel_w));if(std::abs(raster_scale-base)>.00001f){raster_scale=base;dirty=true;}draw_scale=base;panel_x=int((width-panel_w*base)*.5f);panel_y=ssc_hud::items[ssc_hud::selected].y<.13f?int(height-panel_h*base-12):12;return;}

    panel_w=manager?1120:640;panel_h=manager?720:442;
    float fit=std::min(float(width-40)/panel_w,float(height-40)/panel_h);
    float base=std::min(float(ui_scale)/100.f,std::max(.1f,fit));float e=ease(visibility);
    if(std::abs(raster_scale-base)>.00001f){raster_scale=base;dirty=true;}
    draw_scale=base*(manager?(.88f+.12f*e):(.96f+.04f*e));
    panel_x=manager?int((width-panel_w*draw_scale)*.5f):int(width-panel_w*draw_scale-24+(1-e)*24);
    panel_y=manager?int((height-panel_h*draw_scale)*.5f+(1-e)*18):int(40+(1-e)*12);
}






