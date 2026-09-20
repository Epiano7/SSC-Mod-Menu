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
int sound_wheel_remainder=0;
bool sound_search_editing=false;std::wstring sound_query;size_t sound_page=0;std::vector<size_t> sound_results;
void filter_sounds(){sound_results.clear();auto query=sound_query;std::transform(query.begin(),query.end(),query.begin(),towlower);for(size_t i=0;i<ssc_sound::entries.size();++i){auto label=ssc_sound::entries[i].label;std::transform(label.begin(),label.end(),label.begin(),towlower);if(label.find(query)!=std::wstring::npos)sound_results.push_back(i);}sound_page=0;dirty=true;}
int settings_page=0,ui_scale=100;
int lab_drag=-1;
bool scale_dragging=false;int scale_preview=100;float scale_drag_left=0,scale_drag_width=1;
bool manager=false,welcome_seen=false,color_editing=false;std::wstring color_buffer;
// Advance this ID when publishing a new set of release notes.
constexpr int release_notes_id=20260919;
bool show_update_notes=true;int seen_release_notes=0;
bool update_notes_due(int phase){return presence_supported&&phase==1&&welcome_seen&&show_update_notes&&seen_release_notes!=release_notes_id&&!opened;}
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
    std::ofstream out(state_dir/L"runtime.log",std::ios::app);SYSTEMTIME now;GetSystemTime(&now);char stamp[48];std::snprintf(stamp,sizeof(stamp),"[%04u-%02u-%02u %02u:%02u:%02u UTC] ",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond);out<<stamp<<message<<"\n";
}
void cancel_edit(){rpc_editing=false;rpc_error=false;dirty=true;}
void save() {
    ssc_names::cosmetics=cosmetic_requested&&ssc_compat::supports(1);
    ssc_rpc::submit(rpc_requested&&presence_supported,rpc_id,rpc_timer,rpc_preview);
    if(state_dir.empty())return;
    auto path=state_dir/L"menu.ini",temp=state_dir/L"menu.ini.tmp";
    std::ofstream out(temp);
    out<<"show_update_notes="<<show_update_notes<<"\nseen_release_notes="<<seen_release_notes<<"\n";
    out<<"cooldown_pulse="<<ssc_cooldown::settings.enabled<<"\ncooldown_duration_ms="<<ssc_cooldown::settings.duration_ms<<"\n";
    out<<"hud_hover_fade="<<ssc_hud::hover_fade<<"\n";out<<"hud_enabled="<<ssc_hud::enabled<<"\n";for(const auto& item:ssc_hud::items){out<<"hud_box_"<<item.key<<"="<<item.home_x<<","<<item.home_y<<","<<item.w<<","<<item.h<<"\n";out<<"hud_"<<item.key<<"="<<item.x<<","<<item.y<<","<<item.scale<<"\n";}
    out<<"cosmetic_rainbow="<<ssc_names::rainbow<<"\ncosmetic_rgb="<<ssc_names::solid_rgb<<"\n";out<<"welcome_seen="<<welcome_seen<<"\n";out<<"schema=2\nclock="<<clock_enabled
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
        if(key=="hud_hover_fade"){ssc_hud::hover_fade=value!="0";continue;}
        if(key=="hud_enabled"){ssc_hud::enabled=value=="1";continue;}
        if(key.rfind("hud_box_",0)==0){for(int i=0;i<int(ssc_hud::items.size());++i)if(key==std::string("hud_box_")+ssc_hud::items[i].key){float x,y,w,h;char extra;if(std::sscanf(value.c_str(),"%f,%f,%f,%f%c",&x,&y,&w,&h,&extra)==4&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(w)&&std::isfinite(h)&&x>=-.5f&&y>=-.5f&&w>0&&h>0){ssc_hud::apply_bounds(i,{x,y,x+w,y+h,2});ssc_hud::measured[i]=false;}}continue;}
        if(key.rfind("hud_",0)==0){for(auto& item:ssc_hud::items)if(key==std::string("hud_")+item.key){float x,y,z;char extra;if(std::sscanf(value.c_str(),"%f,%f,%f%c",&x,&y,&z,&extra)==3){item.x=x;item.y=y;item.scale=z;ssc_hud::constrain(item);}}continue;}
        if(key=="rpc_id"){if(value.empty()||ssc_rpc::valid_id(value))rpc_id=value.empty()?ssc_rpc::bundled_application_id:value;continue;}
        try {size_t used=0;number=std::stoi(value,&used);if(used!=value.size())continue;}catch(...){continue;}
        if(key=="show_update_notes")show_update_notes=number!=0;
        else if(key=="seen_release_notes")seen_release_notes=number;
        else if(key=="cooldown_pulse")ssc_cooldown::settings.enabled=number!=0;
        else if(key=="cooldown_duration_ms"&&(number==1000||number==1500||number==2500))ssc_cooldown::settings.duration_ms=unsigned(number);
        else if(key=="cosmetic_rainbow")ssc_names::rainbow=number!=0;
        else if(key=="cosmetic_rgb"&&number>=0&&number<=0xffffff)ssc_names::solid_rgb=unsigned(number);
        else if(key=="welcome_seen")welcome_seen=number==1;
        else if(key=="schema")version2=number==2;
        else if(key=="clock")clock_enabled=number==1;
        else if(key=="sound_requested")sound_requested=number==1;
        else if(key=="cosmetic_requested")cosmetic_requested=number==1;
        else if(key=="rpc_requested")rpc_requested=number==1;
        else if(key=="rpc_rating")rpc_rating=number!=0;
        else if(key=="rpc_timer")rpc_timer=number!=0;
        else if(key=="match_sound_level")match_sound_level=number==1;
        else if(key=="animations")animations=number!=0;
        else if(key=="ui_scale"&&number>=50&&number<=200)ui_scale=number;
    }
    if(!version2||!developer_tools)clock_enabled=false;
}
void finish_lab_slider(){if(lab_drag>=0){lab_drag=-1;ssc_lab::recompute();dirty=true;}}
void move_lab_slider(int x){float fraction=std::clamp(((x-panel_x)/draw_scale-580)/248.f,0.f,1.f);int lo=lab_drag==1?0:1,hi=lab_drag==0?1000:lab_drag==1?200:int(ssc_lab::target.max_health);ssc_lab::editing=lab_drag;ssc_lab::edit_buffer=std::to_wstring(int(std::lround(lo+(hi-lo)*fraction)));ssc_lab::commit_edit(false);dirty=true;}
void finish_scale(){if(scale_dragging){ui_scale=scale_preview;scale_dragging=false;save();}}
void move_scale(int x){scale_preview=std::clamp(int(std::lround(50+150*(x-scale_drag_left)/std::max(1.f,scale_drag_width))),50,200);dirty=true;}
float ease(float t) {return 1.f-(1.f-t)*(1.f-t)*(1.f-t);}
void advance_animation(float seconds) {
    float goal=opened?1.f:0.f;
    if(!animations){visibility=goal;modal_visibility=manager&&opened?1.f:0.f;}
    else {float delta=std::max(0.f,std::min(seconds,.05f))/(opened?.18f:.14f);visibility=std::max(0.f,std::min(1.f,visibility+(opened?delta:-delta)));float modal_goal=manager&&opened?1.f:0.f;modal_visibility=modal_goal>modal_visibility?std::min(modal_goal,modal_visibility+delta):std::max(modal_goal,modal_visibility-delta);}
}
void refresh_dynamic_panel(ULONGLONG now,bool presence_tick=false) {
    if(!opened||!manager||ssc_hud::editing)return;
    if(presence_tick&&settings_page==5)dirty=true;
    static ULONGLONG rainbow_at=0;
    if(settings_page==4&&ssc_names::rainbow&&now-rainbow_at>=50){rainbow_at=now;dirty=true;}
}
void close_menu(bool immediate=false) {finish_scale();finish_lab_slider();if(manager&&settings_page==8){welcome_seen=true;seen_release_notes=release_notes_id;save();}if(manager&&settings_page==14){seen_release_notes=release_notes_id;save();}if(ssc_hud::editing){ssc_hud::editing=false;save();}finish_hud_drag();opened=false;dirty=true;pressed=0;keyboard_focus=0;cancel_edit();if(immediate){visibility=0;modal_visibility=0;}ReleaseCapture();log("Menu closed");}
void show_manager(int page=-1) {
    finish_scale();finish_lab_slider();if(page!=11){ssc_lab::picker=-1;ssc_lab::editing=-1;}
    // Navigation inside the full window is not a new opening transition.
    const bool already_visible=manager&&opened&&visibility>0.f;
    finish_hud_drag();ReleaseCapture();cancel_edit();manager=true;settings_page=page;
    if(!already_visible){modal_visibility=0;visibility=animations?0.f:1.f;}
    dirty=true;hovered=pressed=keyboard_focus=0;controls.clear();
}
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
bool module_ready(int id){
    if(!ssc_compat::initialized)return true;
    return id==80?ssc_sound::attached:id==81?ssc_names::attached:id==83?presence_supported:(id==86||id==193)?ssc_hud::attached:true;
}
void activate(int id) {
    if(!module_ready(id))return;
    if(id!=120&&id!=121)rpc_editing=false;
    if(id!=104)sound_search_editing=false;
    if(id==172){ssc_diagnostics::open_logs(state_dir);}
    else if(id==160){if(!presence_supported||rpc_preview.phase==1)ssc_update::check(state_dir);dirty=true;}
    else if(id==161){if(!presence_supported||rpc_preview.phase==1)ssc_update::apply();dirty=true;}
    else if(id==180||id==181){ssc_names::rainbow=id==180;color_editing=false;save();}
    else if(id>=182&&id<=187){const unsigned colors[]={0xffffff,0x55ccff,0xff66cc,0x66ee99,0xffbb44,0xff6655};ssc_names::solid_rgb=colors[id-182];ssc_names::rainbow=false;save();}
    else if(id==188){wchar_t value[8];swprintf(value,8,L"%06X",ssc_names::solid_rgb);color_buffer=value;color_editing=true;dirty=true;}
    else if(id==189){if(color_buffer.size()==6){ssc_names::solid_rgb=std::stoul(color_buffer,nullptr,16);ssc_names::rainbow=false;color_editing=false;save();}}
    else if(id==170){opened=true;show_manager(8);}
    else if(id==171){close_menu();}
    else if(id==280){show_update_notes=!show_update_notes;save();}
    else if(id==281){opened=true;show_manager(14);}
    else if(id==282){close_menu();}
    else if(id==105){ssc_sound::preview_original();dirty=true;}
    else if(id==1)close_menu();
    else if(id==3)show_manager();
    else if(id==4){ssc_hud::enabled=false;sound_requested=false;cosmetic_requested=false;rpc_requested=false;clock_enabled=false;save();}
    else if(id==5)show_quick();
    else if(id>=10&&id<=12){show_manager(id==10?-1:id-10);}
    else if(id==40){animations=!animations;visibility=opened?1.f:0.f;modal_visibility=opened&&manager?1.f:0.f;save();}
    else if(id==41&&developer_tools){clock_enabled=!clock_enabled;save();}
    else if(id>=50&&id<=52){ui_scale=id==50?85:id==51?100:115;save();}
    else if(id==54){ui_scale=100;save();}
    else if(id==80){sound_requested=!sound_requested;save();}
    else if(id==81){cosmetic_requested=!cosmetic_requested;save();}
    else if(id==82){match_sound_level=!match_sound_level;save();}
    else if(id==86){ssc_hud::enabled=!ssc_hud::enabled;save();}
    else if(id==94){if(module_ready(86))start_live_hud();else show_manager(6);}
    else if(id==146){if(module_ready(86))start_live_hud();}
    else if(id==147)close_menu(true);
    else if(id==148){auto& a=ssc_hud::items[ssc_hud::selected];a.x=a.home_x;a.y=a.home_y;a.scale=1;save();}
    else if(id==143){show_manager(10);}
    else if(id==190){ssc_hud::reset();save();show_manager(6);}
    else if(id==191){show_manager(6);}
    else if(id==193){ssc_cooldown::settings.enabled=!ssc_cooldown::settings.enabled;ssc_cooldown::tracker.clear();save();}
    else if(id>=197&&id<=199){ssc_cooldown::settings.duration_ms=id==197?1000:id==198?1500:2500;save();}
    else if(id==192){ssc_hud::hover_fade=!ssc_hud::hover_fade;save();}
    else if(id==13){show_manager(9);}
    else if(id==15){show_manager(12);}
    else if(id==270){if(ssc_record::state().enabled)ssc_record::stop();else ssc_record::start(state_dir/L"recordings");dirty=true;}
    else if(id==271){ShellExecuteW(nullptr,L"open",(state_dir/L"recordings").c_str(),nullptr,nullptr,SW_SHOWNORMAL);}
    else if(id==260){if(ssc_lab::duel_playing)ssc_lab::duel_playing=false;else ssc_lab::start_duel();dirty=true;}
    else if(id==276){ShellExecuteW(nullptr,L"open",L"https://discord.gg/skillshotcity",nullptr,nullptr,SW_SHOWNORMAL);}
    else if(id==261){++ssc_lab::seed;ssc_lab::recompute();dirty=true;}
    else if(id==262||id==263){ssc_lab::miss_enabled[id-262]=!ssc_lab::miss_enabled[id-262];ssc_lab::recompute();dirty=true;}
    else if(id==264||id==265){auto& value=ssc_lab::miss_percent[id-264];value=value>=100?0:value+10;ssc_lab::recompute();dirty=true;}
    else if(id==16){show_manager(13);}
    else if(id==272){ssc_lab::comparison_count=std::min(4,ssc_lab::comparison_count+1);dirty=true;}
    else if(id==273){ssc_lab::comparison_count=std::max(2,ssc_lab::comparison_count-1);dirty=true;}
    else if(id==274||id==275){ssc_lab::open_picker(id-272);dirty=true;}
    else if(id==14){ssc_lab::refresh();show_manager(11);}
    else if(id>=200&&id<=203){ssc_lab::select(id>=202,id%2?1:-1);ssc_lab::recompute();dirty=true;}
    else if(id==204){ssc_lab::refresh();dirty=true;}
    else if(id==205){ssc_lab::guards=!ssc_lab::guards;ssc_lab::recompute();dirty=true;}
    else if(id==206||id==207){ssc_lab::open_picker(id-206);dirty=true;}
    else if(id==208){ssc_lab::sort_mode=(ssc_lab::sort_mode+1)%3;ssc_lab::picker_page=0;dirty=true;}
    else if(id>=230&&id<=233){ssc_lab::editing=-1;ssc_lab::tab=id-230;controls.clear();dirty=true;}
    else if(id>=250&&id<=254){if(ssc_lab::commit_edit())ssc_lab::begin_edit(id-250);dirty=true;}
    else if(id==255){ssc_lab::allow_reload=!ssc_lab::allow_reload;ssc_lab::recompute();dirty=true;}
    else if(id==256){ssc_lab::target={};ssc_lab::damage_percent=ssc_lab::reload_percent=100;ssc_lab::allow_reload=true;ssc_lab::guards=false;ssc_lab::editing=-1;ssc_lab::recompute();dirty=true;}
    else if(id>=240&&id<=245){ssc_lab::duel_shown=false;ssc_lab::duel_playing=false;int col=(id-240)%2;auto& cursor=ssc_lab::shot_cursor[col];auto count=ssc_lab::trials[col].shots.size();if(id<242)cursor=std::min(cursor+1,count);else if(id<244)cursor=0;else cursor=count;dirty=true;}
    else if(id==209){ssc_lab::picker=-1;dirty=true;controls.clear();}
    else if(id==210){if(ssc_lab::picker_page>=6)ssc_lab::picker_page-=6;dirty=true;}
    else if(id==211){if(ssc_lab::picker_page+6<ssc_lab::filtered().size())ssc_lab::picker_page+=6;dirty=true;}
    else if(id>=220&&id<226){auto results=ssc_lab::filtered();auto row=ssc_lab::picker_page+id-220;if(row<results.size()){ssc_lab::column(ssc_lab::picker)=results[row];ssc_lab::picker=-1;ssc_lab::recompute();controls.clear();dirty=true;}}
    else if(id==144||id==145){ssc_hud::selected=(ssc_hud::selected+(id==144?int(ssc_hud::items.size())-1:1))%int(ssc_hud::items.size());dirty=true;}
    else if(id==83){rpc_requested=!rpc_requested;save();}
    else if(id==85){rpc_rating=!rpc_rating;save();}
    else if(id==84){rpc_timer=!rpc_timer;save();}
    else if(id==120){rpc_editing=true;rpc_select_all=true;rpc_error=false;rpc_buffer=std::wstring(rpc_id.begin(),rpc_id.end());dirty=true;}
    else if(id==121){std::string value(rpc_buffer.begin(),rpc_buffer.end());if(value.empty()||ssc_rpc::valid_id(value)){rpc_id=value.empty()?ssc_rpc::bundled_application_id:value;rpc_editing=false;rpc_error=false;save();}else {rpc_error=true;dirty=true;}}
    else if(id>=91&&id<=93){int page=id==91?3:id==92?4:5;show_manager(page);}
    else if(id==100){if(sound_page>=5)sound_page-=5;dirty=true;}
    else if(id==101){if(sound_page+5<sound_results.size())sound_page+=5;dirty=true;}
    else if(id==102){ssc_sound::import_selected(game_window,match_sound_level);if(IsWindow(game_window)&&GetForegroundWindow()==game_window){opened=true;manager=true;settings_page=3;visibility=1.f;modal_visibility=1.f;}dirty=true;}
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
    if(opened&&manager&&settings_page==3&&message==WM_MOUSEWHEEL){POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(window,&p);float x=(p.x-panel_x)/draw_scale,y=(p.y-panel_y)/draw_scale;if(x>=266&&x<=1086&&y>=187&&y<=452){sound_wheel_remainder+=GET_WHEEL_DELTA_WPARAM(wp);int steps=sound_wheel_remainder/WHEEL_DELTA;sound_wheel_remainder%=WHEEL_DELTA;int last=std::max(0,int(sound_results.size())-5);sound_page=size_t(std::clamp(int(sound_page)-steps,0,last));dirty=true;return 0;}}
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
    if(message==WM_KILLFOCUS||(message==WM_ACTIVATEAPP&&!wp)){
        // Keep the current panel and unfinished text edits across Alt+Tab,
        // but release gestures whose mouse/key-up may go to another app.
        finish_scale();finish_lab_slider();finish_hud_drag();ReleaseCapture();pressed=hovered=0;
        suppress_escape_up=false;dirty=true;
        return CallWindowProcW(previous_proc,window,message,wp,lp);
    }
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
        if(opened&&manager&&settings_page==11&&ssc_lab::tab==2&&ssc_lab::picker<0){
            if(message==WM_LBUTTONDOWN){int id=hit(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));if(id>=250&&id<=252){keyboard_focus=id;lab_drag=id-250;move_lab_slider(GET_X_LPARAM(lp));SetCapture(window);return 0;}}
            if(message==WM_MOUSEMOVE&&lab_drag>=0){move_lab_slider(GET_X_LPARAM(lp));return 0;}
            if(message==WM_LBUTTONUP&&lab_drag>=0){move_lab_slider(GET_X_LPARAM(lp));finish_lab_slider();ReleaseCapture();return 0;}
            if(message==WM_CAPTURECHANGED)finish_lab_slider();
            if(message==WM_KEYDOWN&&keyboard_focus>=250&&keyboard_focus<=252&&(wp==VK_LEFT||wp==VK_RIGHT)){
                int field=keyboard_focus-250;int value=field==0?int(ssc_lab::target.max_health):field==1?int(std::lround(100*ssc_lab::target.shield/ssc_lab::target.max_health)):int(ssc_lab::target.health);
                int lo=field==1?0:1,hi=field==1?200:field==0?1000:int(ssc_lab::target.max_health);
                ssc_lab::editing=field;ssc_lab::edit_buffer=std::to_wstring(std::clamp(value+(wp==VK_RIGHT?1:-1),lo,hi));ssc_lab::commit_edit();dirty=true;return 0;
            }
        }
        if(opened&&manager&&settings_page==11&&ssc_lab::picker>=0){
            if(message==WM_KEYDOWN&&(wp==VK_ESCAPE||wp==VK_RETURN)){ssc_lab::picker=-1;controls.clear();dirty=true;suppress_escape_up=wp==VK_ESCAPE;return 0;}
            if(message==WM_KEYDOWN&&wp=='A'&&(GetKeyState(VK_CONTROL)&0x8000)){ssc_lab::query.clear();ssc_lab::picker_page=0;dirty=true;return 0;}
            if(message==WM_CHAR){wchar_t c=wchar_t(wp);if(c==8){if(!ssc_lab::query.empty())ssc_lab::query.pop_back();}else if(c>=32&&ssc_lab::query.size()<48)ssc_lab::query+=c;ssc_lab::picker_page=0;dirty=true;return 0;}
            if(message==WM_MOUSEWHEEL){activate(GET_WHEEL_DELTA_WPARAM(wp)>0?210:211);return 0;}
        }
        if(opened&&manager&&settings_page==11&&ssc_lab::editing>=0){
            if(message==WM_KEYDOWN&&wp==VK_ESCAPE){ssc_lab::editing=-1;dirty=true;suppress_escape_up=true;return 0;}
            if(message==WM_KEYDOWN&&(wp==VK_RETURN||wp==VK_TAB)){ssc_lab::commit_edit();dirty=true;return 0;}
            if(message==WM_KEYDOWN&&wp=='A'&&(GetKeyState(VK_CONTROL)&0x8000)){ssc_lab::edit_buffer.clear();dirty=true;return 0;}
            if(message==WM_CHAR){wchar_t c=wchar_t(wp);if(c==8){if(!ssc_lab::edit_buffer.empty())ssc_lab::edit_buffer.pop_back();}else if(c>=L'0'&&c<=L'9'&&ssc_lab::edit_buffer.size()<7)ssc_lab::edit_buffer+=c;ssc_lab::edit_error=false;dirty=true;return 0;}
        }
        if(opened&&manager&&settings_page==1){
            if(message==WM_LBUTTONDOWN&&hit(GET_X_LPARAM(lp),GET_Y_LPARAM(lp))==53){scale_dragging=true;scale_drag_left=panel_x+282*draw_scale;scale_drag_width=650*draw_scale;scale_preview=ui_scale;move_scale(GET_X_LPARAM(lp));SetCapture(window);return 0;}
            if(message==WM_MOUSEMOVE&&scale_dragging){move_scale(GET_X_LPARAM(lp));return 0;}
            if(message==WM_LBUTTONUP&&scale_dragging){move_scale(GET_X_LPARAM(lp));finish_scale();finish_lab_slider();ReleaseCapture();return 0;}
            if(message==WM_CAPTURECHANGED){finish_scale();finish_lab_slider();}
            if(message==WM_KEYDOWN&&keyboard_focus==53&&(wp==VK_LEFT||wp==VK_RIGHT||wp==VK_HOME||wp==VK_END)){ui_scale=wp==VK_HOME?50:wp==VK_END?200:std::clamp(ui_scale+(wp==VK_RIGHT?1:-1),50,200);save();return 0;}
        }
        if(opened&&color_editing&&manager&&settings_page==4){
            if(message==WM_KEYDOWN&&wp==VK_ESCAPE){color_editing=false;dirty=true;return 0;}
            if(message==WM_KEYDOWN&&wp==VK_RETURN){activate(189);return 0;}
            if(message==WM_KEYDOWN&&wp=='A'&&(GetKeyState(VK_CONTROL)&0x8000)){color_buffer.clear();dirty=true;return 0;}
            if(message==WM_CHAR){wchar_t c=towupper(wchar_t(wp));if(c==8){if(!color_buffer.empty())color_buffer.pop_back();}else if(((c>=L'0'&&c<=L'9')||(c>=L'A'&&c<=L'F'))&&color_buffer.size()<6)color_buffer+=c;dirty=true;return 0;}
        }
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
        if(message==WM_KEYDOWN&&wp==VK_ESCAPE){finish_hud_drag();ReleaseCapture();suppress_escape_up=true;if(manager&&opened&&(settings_page==8||settings_page==14)){close_menu();}else if(manager&&opened&&settings_page>=0){settings_page=-1;dirty=true;controls.clear();}else if(manager&&opened)show_quick();else close_menu();return 0;}
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
    unsigned revision=0;int w=0,h=0,start=0,end=-1;float cap_height=64,cap_top=24;std::vector<Glyph> glyphs;std::vector<unsigned char> atlas;
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
        ++revision;w=head[1];h=head[2];start=head[3];end=head[4];glyphs=std::move(gs);atlas=std::move(data);
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
// Cache the expensive 4x4 atlas integration; color/rainbow blending stays live.
// Bounded and process-local: no rendered text or masks are persisted.
struct TextGlyphMask {int x,y,w,h;float cursor,width;size_t index;std::vector<float> alpha;};
struct TextMask {std::vector<TextGlyphMask> glyphs;size_t bytes=0;};
struct TextCacheEntry {const LocalFont* font;unsigned revision;int x,y,size,rw,rh;float scale;std::wstring value;TextMask mask;unsigned long long used;};
std::vector<TextCacheEntry> text_cache;
size_t text_cache_bytes=0;
unsigned long long text_cache_clock=0;
constexpr size_t text_cache_limit=8*1024*1024,text_cache_entries=128;
TextMask make_text_mask(const LocalFont& f,int x,int y,const wchar_t* value,int size) {
    TextMask result;float cursor=x*raster_scale,scale=size*raster_scale/f.cap_height;
    auto coverage=[&](float fx,float fy) {
        fx=std::max(0.f,std::min(float(f.w-1),fx));fy=std::max(0.f,std::min(float(f.h-1),fy));
        int ax=std::min(f.w-2,int(fx)),ay=std::min(f.h-2,int(fy));float tx=fx-ax,ty=fy-ay;
        auto a=[&](int xx,int yy){return f.atlas[(yy*f.w+xx)*2+1]/255.f;};
        return (a(ax,ay)*(1-tx)+a(ax+1,ay)*tx)*(1-ty)+(a(ax,ay+1)*(1-tx)+a(ax+1,ay+1)*tx)*ty;
    };
    for(const wchar_t* s=value;*s;++s) {
        if(*s==L' '){cursor+=size*.4f*raster_scale;continue;}
        int c=int(*s);if(c<f.start||c>f.end)c='?';if(c<f.start||c>f.end)continue;
        const auto& g=f.glyphs[c-f.start];float gw=std::max(0.f,g.width)*f.w*scale,gh=g.height*f.h*scale;
        if(gw<=0)continue;
        float top=y*raster_scale-f.cap_top*scale,sx=g.u0*f.w,sy=g.v0*f.h;
        int left=std::max(0,int(std::floor(cursor))),upper=std::max(0,int(std::floor(top)));
        int right=std::min(raster_w,int(std::ceil(cursor+gw))),bottom=std::min(raster_h,int(std::ceil(top+gh)));
        if(right>left&&bottom>upper){
            TextGlyphMask mask{left,upper,right-left,bottom-upper,cursor,gw,size_t(s-value),{}};
            mask.alpha.reserve(size_t(mask.w)*mask.h);
            for(int dy=upper;dy<bottom;++dy)for(int dx=left;dx<right;++dx){
                float a=0;
                for(int oy=0;oy<4;++oy)for(int ox=0;ox<4;++ox){float px=dx+(ox+.5f)/4,py=dy+(oy+.5f)/4;
                    if(px>=cursor&&px<cursor+gw&&py>=top&&py<top+gh)a+=coverage(sx+(px-cursor)/scale-.5f,sy+(py-top)/scale-.5f)/16.f;}
                mask.alpha.push_back(a);
            }
            result.bytes+=mask.alpha.size()*sizeof(float)+sizeof(TextGlyphMask);
            result.glyphs.push_back(std::move(mask));
        }
        cursor+=gw;
    }
    return result;
}
const TextMask& cached_text_mask(const LocalFont& f,int x,int y,const wchar_t* value,int size,TextMask& scratch){
    const auto stamp=++text_cache_clock;
    for(auto& e:text_cache)if(e.font==&f&&e.revision==f.revision&&e.x==x&&e.y==y&&e.size==size&&e.rw==raster_w&&e.rh==raster_h&&e.scale==raster_scale&&e.value==value){e.used=stamp;return e.mask;}
    auto mask=make_text_mask(f,x,y,value,size);
    const size_t bytes=mask.bytes+(wcslen(value)+1)*sizeof(wchar_t)+sizeof(TextCacheEntry);
    if(bytes>text_cache_limit){scratch=std::move(mask);return scratch;}
    while(!text_cache.empty()&&(text_cache.size()>=text_cache_entries||text_cache_bytes+bytes>text_cache_limit)){
        auto oldest=std::min_element(text_cache.begin(),text_cache.end(),[](const auto& a,const auto& b){return a.used<b.used;});
        text_cache_bytes-=oldest->mask.bytes+(oldest->value.size()+1)*sizeof(wchar_t)+sizeof(TextCacheEntry);text_cache.erase(oldest);
    }
    text_cache.push_back({&f,f.revision,x,y,size,raster_w,raster_h,raster_scale,value,std::move(mask),stamp});text_cache_bytes+=bytes;
    return text_cache.back().mask;
}
void text(int x,int y,const wchar_t* value,COLORREF color=ink,bool title=false,int custom_size=0,float rainbow_phase=-1) {
    int size=custom_size?custom_size:title?22:15;auto& f=face(title||custom_size!=0);
    if(f.atlas.empty()){SelectObject(canvas,title?title_font:font);SetTextColor(canvas,color);TextOutW(canvas,x,y,value,lstrlenW(value));return;}
    GdiFlush();TextMask scratch;const auto& mask=cached_text_mask(f,x,y,value,size,scratch);auto dest=static_cast<unsigned char*>(pixels);
    const unsigned char rgb[]={GetBValue(color),GetGValue(color),GetRValue(color)};
    const float n=float(lstrlenW(value));
    for(const auto& g:mask.glyphs){
        std::array<float,3> left{},right{};if(rainbow_phase>=0){left=rainbow_rgb(float(g.index)/n+rainbow_phase,.2f);right=rainbow_rgb((float(g.index)+.75f)/n+rainbow_phase,.1f);}
        size_t i=0;
        for(int dy=g.y;dy<g.y+g.h;++dy)for(int dx=g.x;dx<g.x+g.w;++dx){
            const float a=g.alpha[i++];auto pixel=dest+(dy*raster_w+dx)*4;
            for(int k=0;k<3;++k){float channel=rgb[k];if(rainbow_phase>=0){float t=unit((dx+.5f-g.cursor)/g.width);channel=255*(left[2-k]*(1-t)+right[2-k]*t);}pixel[k]=static_cast<unsigned char>(pixel[k]*(1-a)+channel*a);}
            pixel[3]=255;
        }
    }
}

void button(int id,int x,int y,int w,int h,const wchar_t* label,bool selected=false,bool enabled=true,bool centered=false) {
    controls.push_back({id,x,y,w,h,enabled});bool focus=enabled&&(hovered==id||keyboard_focus==id);
    COLORREF base=selected?RGB(27,106,196):RGB(33,57,88);
    if(focus)base=RGB(46,121,193);
    if(enabled&&pressed==id)base=RGB(26,81,133);
    if(!enabled)base=RGB(22,35,51);
    polygon(x,y,w,h,base);rectangle(x+8,y+1,w-16,1,focus?cyan:RGB(63,91,132));
    if(focus)rectangle(x+8,y+h-2,w-16,2,cyan);
    int size=15;while(size>11&&text_width(label,size)>w-28)--size;
    text(centered?x+(w-text_width(label,size))/2:x+14,y+(h-size)/2,label,enabled?ink:muted,false,size);
}
void toggle(int id,int x,int y,bool enabled) {if(!module_ready(id)){button(id,x,y,94,36,L"PAUSED",false,false);return;}button(id,x,y,94,36,enabled?L"ON":L"OFF",enabled);rectangle(x+70,y+10,10,16,enabled?RGB(99,239,173):muted);}
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
    text(26,24,L"SSC MOD MENU",ink,true);text(26,58,manager?L"MODULE SETTINGS":L"QUICK MENU",muted);
    if(!(manager&&(settings_page==8||settings_page==14)))button(1,panel_w-60,23,36,36,L"X");
    if(!native_supported){
        text(26,116,L"GAME UPDATE DETECTED",cyan);
        text(26,156,L"Modules are paused until a compatible mod update.",muted,false,14);
        text(26,209,ssc_update::message.c_str(),ink,false,14);
        button(160,26,270,250,42,L"CHECK FOR UPDATES",false,!ssc_update::process);
        button(161,294,270,310,42,L"UPDATE AND RESTART",true,ssc_update::available());
    } else if(manager&&settings_page==8){
        text(50,125,L"WELCOME TO SSC MOD MENU",ink,true);
        text(50,200,L"Press RIGHT SHIFT to open the mod menu.");
        text(50,244,L"Toggle modules there, or open Settings to customize them.",muted);
        text(50,328,L"The mod is installed in your Steam game folder.");
        text(50,372,L"Launch Skillshot City through Steam as usual.",muted);
        text(50,416,L"You do not need to run the installer again.",muted);
        text(50,512,L"Reopen this guide from Interface > Welcome guide.",muted);
        button(171,410,610,300,48,L"GOT IT",true,true,true);
    } else if(manager&&settings_page==14){
        text(50,111,L"WHAT'S NEW",ink,true);
        auto bullet=[&](int y,const wchar_t* label){rectangle(54,y+7,5,5,cyan);text(72,y,label,ink,false,15);};
        text(50,153,L"WEAPON LAB",cyan);
        bullet(185,L"Search by weapon name, tier or type");
        bullet(209,L"Compare up to four weapons, with the best stats highlighted");
        bullet(233,L"Adjust health, shield and accuracy; watch two players fight with reloads");
        bullet(257,L"Read the Info tab for help using the calculator");
        text(72,283,L"Estimates exclude skills and some special weapon effects",muted,false,14);
        text(50,321,L"LOCAL ROUND RECORDING",cyan);
        bullet(353,L"Save BR snapshots of class, level, health and weapon inventory on your PC");
        bullet(377,L"Off each launch. Nothing is uploaded");
        text(72,403,L"For a future module concept; you are welcome to use it and share your files",muted,false,14);
        text(50,441,L"INTERFACE & HUD",cyan);
        bullet(473,L"UI scale slider: 50%-200%");
        bullet(497,L"Fixed the animation when switching to Misc");
        bullet(521,L"Skill-ready pulse briefly restores health HUD opacity (thanks, nam!)");
        bullet(545,L"Simpler controls, credits and a Skillshot City Discord link");
        text(50,585,L"Manage this popup in Interface > Show update notes",muted,false,14);
        button(282,410,624,300,48,L"GOT IT",true,true,true);
    } else if(!manager) {
        const wchar_t* names[]={L"SOUND REPLACER",L"COSMETICS",L"DISCORD PRESENCE",L"HUD EDITOR"};
        const bool enabled[]={sound_requested,cosmetic_requested,rpc_requested,ssc_hud::enabled};const int ids[]={80,81,83,86};
        for(int i=0;i<4;++i){int y=100+i*64;rectangle(20,y,600,56,RGB(16,37,62));text(32,y+20,names[i]);toggle(ids[i],350,y+10,enabled[i]);button(91+i,456,y+10,146,36,L"SETTINGS");}
        button(3,20,374,366,42,L"ALL MODULES",true,true,true);button(4,402,374,218,42,L"DISABLE ALL");
        if(save_failed)text(24,425,L"Could not save settings",RGB(247,191,83));
    } else {
        button(10,24,108,192,44,L"MODULES",settings_page==-1||(settings_page>=3&&settings_page<=6));
        button(11,24,166,192,44,L"INTERFACE",settings_page==1);
        button(13,24,224,192,44,L"MISC",settings_page==9);
        button(12,24,282,192,44,L"ABOUT",settings_page==2);
        button(14,24,340,192,44,L"WEAPON LAB",settings_page==11);button(15,24,398,192,44,L"RECORDING",settings_page==12);button(16,24,456,192,44,L"CREDITS",settings_page==13);
        rectangle(238,108,1,546,RGB(38,65,101));

        if(settings_page==-1) {
            text(266,111,L"MODULES",ink,true);
            const wchar_t* names[]={L"Sound replacer",L"Cosmetics",L"Discord presence",L"HUD editor"};
            const bool enabled[]={sound_requested,cosmetic_requested,rpc_requested,ssc_hud::enabled};const int ids[]={80,81,83,86};
            for(int i=0;i<4;++i){int y=174+i*80;rectangle(266,y,820,66,RGB(16,37,62));text(284,y+22,names[i],ink,true);toggle(ids[i],822,y+15,enabled[i]);button(91+i,932,y+15,138,36,L"SETTINGS");}
        } else if(settings_page==1) {
            text(266,111,L"INTERFACE",ink,true);
            rectangle(266,174,820,66,RGB(16,37,62));text(282,198,L"MENU ANIMATIONS");toggle(40,978,189,animations);
            text(266,326,L"UI SCALE");auto scale_text=std::to_wstring(scale_dragging?scale_preview:ui_scale)+L"%";text(850,326,scale_text.c_str(),cyan);
            controls.push_back({53,266,365,682,44,true});rectangle(282,384,650,6,muted);int knob=282+int(650*((scale_dragging?scale_preview:ui_scale)-50)/150.f);rectangle(282,384,knob-282,6,cyan);rectangle(knob-6,374,12,26,keyboard_focus==53?ink:cyan);
            text(266,414,L"50%",muted,false,12);text(890,414,L"200%",muted,false,12);button(54,966,368,120,40,L"RESET");
            text(266,454,L"RIGHT SHIFT",cyan);text(450,454,L"Open / close quick menu");text(266,494,L"ESCAPE",cyan);text(450,494,L"Back to quick menu, then close");
            text(266,550,L"TAB / ENTER",cyan);text(450,550,L"Focus controls / activate");
            button(170,266,604,300,40,L"WELCOME GUIDE");button(281,584,604,250,40,L"WHAT'S NEW");
            text(282,269,L"SHOW UPDATE NOTES");toggle(280,978,258,show_update_notes);
        } else if(settings_page==3) {
            text(266,111,L"SOUND REPLACER",ink,true);toggle(80,986,106,sound_requested);
            text(266,152,L"WAV / MP3 imports. Restart game to apply changes.",muted);
            if(sound_results.empty()&&sound_query.empty())filter_sounds();
            std::wstring query=L"SEARCH: "+sound_query+(sound_search_editing?L"_":L"");button(104,266,187,540,36,query.c_str(),sound_search_editing);
            button(100,822,187,120,36,L"PREVIOUS");button(101,958,187,128,36,L"NEXT");
            for(size_t i=0;i<5&&sound_page+i<sound_results.size();++i){size_t index=sound_results[sound_page+i];auto label=ssc_sound::entries[index].label+(ssc_sound::entries[index].imported?L" [CUSTOM]":L"");button(110+int(i),266,236+int(i)*42,820,36,label.c_str(),index==ssc_sound::selection);}

            if(ssc_sound::selection<ssc_sound::entries.size()){
                auto& e=ssc_sound::entries[ssc_sound::selection];std::wstring info=e.label+L" / "+std::to_wstring(e.rate)+L" Hz / "+std::to_wstring(e.channels)+L" ch";
                text(266,490,info.c_str(),cyan,false,14);
            }
            button(102,266,518,242,40,L"IMPORT AUDIO");button(103,526,518,242,40,L"RESTORE ORIGINAL");
            text(792,518,L"MATCH LEVEL",muted,false,12);toggle(82,982,520,match_sound_level);
            text(266,578,ssc_sound::status.c_str(),ink,false,13);
            button(105,266,613,280,38,L"PREVIEW ORIGINAL",false,ssc_sound::selection<ssc_sound::entries.size());
        } else if(settings_page==4) {
            text(266,111,L"COSMETICS",ink,true);toggle(81,986,106,cosmetic_requested);
            if(!ssc_names::attached)text(266,160,L"Unavailable on this game version",RGB(247,191,83));
            button(180,266,198,250,42,L"RAINBOW",ssc_names::rainbow);button(181,534,198,250,42,L"SOLID COLOR",!ssc_names::rainbow);
            const wchar_t* labels[]={L"WHITE",L"CYAN",L"PINK",L"GREEN",L"GOLD",L"CORAL"};
            for(int i=0;i<6;++i)button(182+i,266+i*136,266,126,36,labels[i]);
            wchar_t hex[16];swprintf(hex,16,L"#%06X",ssc_names::solid_rgb);auto label=color_editing?L"#"+color_buffer+L"_":std::wstring(hex);
            button(188,266,328,240,40,label.c_str(),color_editing);button(189,520,328,140,40,L"APPLY",false,color_editing&&color_buffer.size()==6);
            text(266,409,L"PREVIEW",muted);
            float phase=0;ssc_names::native_phase(phase);
            auto rgb=ssc_names::solid_rgb;auto color=RGB((rgb>>16)&255,(rgb>>8)&255,rgb&255);
            text(282,455,L"Example player",color,true,0,ssc_names::rainbow?phase:-1);

        } else if(settings_page==5) {
            text(266,111,L"DISCORD PRESENCE",ink,true);toggle(83,986,106,rpc_requested);
            auto connection=presence_supported?ssc_rpc::status():std::wstring(L"Unavailable on this game version");text(266,168,connection.c_str(),cyan);
            rectangle(266,212,820,130,RGB(16,37,62));text(282,226,L"ACTIVITY PREVIEW",muted,false,13);
            auto wide=[](const std::string& value){int n=MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),nullptr,0);std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),out.data(),n);return out;};
            auto details=wide(rpc_preview.details),state=wide(rpc_preview.state);
            text(282,266,details.c_str(),ink,false,14);text(282,306,state.c_str(),muted,false,14);
            text(266,388,L"SHOW ELAPSED TIME",ink);toggle(84,986,378,rpc_timer);
            text(266,444,L"SHOW SSC RATING WHEN RANKED",ink);toggle(85,986,434,rpc_rating);

        } else if(settings_page==7){
            text(266,111,L"UPDATE AVAILABLE",ink,true);
            text(266,180,ssc_update::message.c_str(),cyan);
            text(266,238,L"Download, install and restart Skillshot City.",muted);
            button(161,266,308,340,48,L"UPDATE AND RESTART",true,ssc_update::available()&&(rpc_preview.phase==1||!presence_supported));
            button(1,630,308,180,48,L"LATER");
        } else if(settings_page==9){
            text(266,111,L"MISC",ink,true);
            text(282,198,L"HUD HOVER FADE");toggle(192,978,189,ssc_hud::hover_fade);
            text(282,246,L"Fade HUD elements near your cursor.",muted);
            text(282,310,L"READY SKILL PULSE");toggle(193,978,301,ssc_cooldown::settings.enabled);
            text(282,354,L"Restore the health / skill HUD when a cooldown ends.",muted);
            text(282,518,L"DURATION");
            button(197,282,554,210,38,L"1 SECOND",ssc_cooldown::settings.duration_ms==1000);button(198,508,554,210,38,L"1.5 SECONDS",ssc_cooldown::settings.duration_ms==1500);button(199,734,554,210,38,L"2.5 SECONDS",ssc_cooldown::settings.duration_ms==2500);
        } else if(settings_page==13){
            text(266,111,L"CREDITS",ink,true);
            text(282,204,L"bencelot",cyan,true);text(282,248,L"Skillshot City developer.",muted,false,14);
            text(282,326,L"Epiano7",cyan,true);text(282,370,L"SSC Mod Menu developer",muted,false,14);
            text(282,448,L"SSC Discord community",cyan,true);
            text(282,492,L"To everyone in the SSC Discord who provided feedback",muted,false,14);
            text(282,523,L"during early module development, thank you!",muted,false,14);
            button(276,282,572,360,38,L"discord.gg/skillshotcity");
        } else if(settings_page==12){
            text(266,111,L"LOCAL RECORDING",ink,true);
            text(266,200,L"RECORDING");toggle(270,520,190,ssc_record::state().enabled);
            button(271,646,190,420,44,L"OPEN RECORDINGS FOLDER");
            const wchar_t* statuses[]={L"Stopped",L"Recording locally",L"Unable to write recording",L"Storage limit reached",L"Starting",L"Stopping",L"Waiting for an active BR round"};text(266,267,statuses[std::min(6u,ssc_record::state().status.load())],cyan);
            text(266,318,L"Saves round data locally while you play BR.",muted,false,14);
            text(266,356,L"Skips menus and lobbies. Off each launch.",muted,false,14);
        } else if(settings_page==11){
            text(266,111,L"WEAPON LAB",ink,true);
            if(ssc_lab::weapons.empty())text(266,157,ssc_lab::status.c_str(),muted,false,14);
            button(204,914,105,172,38,L"REFRESH");
            button(230,266,190,194,38,L"CALCULATOR",ssc_lab::tab==0);
            button(231,470,190,204,38,L"WEAPON STATS",ssc_lab::tab==1);
            button(232,684,190,210,38,L"TARGET SETUP",ssc_lab::tab==2);
            button(233,904,190,182,38,L"INFO",ssc_lab::tab==3);
            if(ssc_lab::tab==3){
                text(282,265,L"Select a weapon for Player A and Player B.",ink,false,15);
                text(282,305,L"Each player fires at the other; bars belong to that player.",muted,false,13);
                text(282,356,L"PLAY DUEL runs shots and reloads until someone wins.",ink,false,14);
                text(282,396,L"Target Setup applies the same starting health and shield to both.",muted,false,13);
                text(282,447,L"MISS adds a chance for each projectile to miss.",ink,false,14);
                text(282,487,L"REROLL MISSES tries a different pattern of hits and misses.",muted,false,13);
                text(282,538,L"Weapon Stats compares up to four weapons; blue marks the best values.",muted,false,12);
                text(282,588,L"Estimates exclude travel, recovery and automatic perk effects.",muted,false,12);
            }
            if(!ssc_lab::weapons.empty()&&ssc_lab::tab!=3){
                auto number=[](double v,const wchar_t* unit=L""){if(!std::isfinite(v))return std::wstring(L"Not modeled");wchar_t b[80];swprintf(b,80,std::abs(v-std::round(v))<.0001?L"%.0f%ls":L"%.2f%ls",v,unit);return std::wstring(b);};
                if(ssc_lab::tab==2){
                    const wchar_t* labels[]={L"MAX HEALTH",L"CURRENT HEALTH",L"SHIELD",L"DAMAGE %",L"RELOAD TIME %"};
                    const int fields[]={0,2,1,3,4};
                    const int values[]={int(ssc_lab::target.max_health),int(ssc_lab::target.health),int(ssc_lab::target.shield),ssc_lab::damage_percent,ssc_lab::reload_percent};
                    for(int row=0;row<5;++row){int field=fields[row];text(282,255+row*49,labels[row],muted,false,13);auto value=ssc_lab::editing==field?ssc_lab::edit_buffer+L"_":std::to_wstring(values[row]);
                        if(field<=2){int lo=field==1?0:1,hi=field==0?1000:field==1?200:int(ssc_lab::target.max_health);int v=field==1?int(std::lround(100*ssc_lab::target.shield/ssc_lab::target.max_health)):values[row];
                            if(field==1)value=std::to_wstring(int(std::lround(ssc_lab::target.shield)))+L" ("+std::to_wstring(v)+L"%)";
                            text(580,242+row*49,value.c_str(),cyan,false,12);controls.push_back({250+field,580,243+row*49,260,44,true});rectangle(580,277+row*49,248,4,muted);int knob=580+int(248.f*(v-lo)/std::max(1,hi-lo));rectangle(knob-4,269+row*49,8,20,cyan);
                        }else button(250+field,580,243+row*49,260,38,value.c_str(),ssc_lab::editing==field);}
                    button(205,858,243,212,38,ssc_lab::guards?L"GUARDS":L"PLAYERS");
                    button(255,858,295,212,70,ssc_lab::allow_reload?L"RELOAD: ON":L"RELOAD: OFF",ssc_lab::allow_reload);
                    button(256,858,390,212,38,L"RESET TARGET");
                    text(282,511,ssc_lab::edit_error?L"Enter a value within range; current HP cannot exceed max HP.":L"Drag sliders or use Left / Right for one-step changes.",ssc_lab::edit_error?RGB(255,190,80):muted,false,12);
                    text(282,543,L"Health: 1-1,000. Shield: 0-200% of max health.",muted,false,12);
                    text(282,575,L"Modifiers are manual totals, not automatic perk or class effects.",muted,false,12);
                    text(282,608,L"No regeneration, travel time, armor, evasion or death-prevention perks.",muted,false,11);
                }else if(ssc_lab::tab==1){
                    const wchar_t* labels[]={L"DAMAGE / SHOT",L"FIRE RATE",L"MAGAZINE",L"RELOAD TIME",L"GAME DPS",L"DPS INCL. RELOADS"};
                    button(272,266,244,190,34,L"ADD WEAPON",false,ssc_lab::comparison_count<4);
                    button(273,266,284,190,30,L"REMOVE LAST",false,ssc_lab::comparison_count>2);
                    for(int row=0;row<6;++row)text(266,345+row*36,labels[row],muted,false,11);
                    double values[4][6]{};
                    for(int col=0;col<ssc_lab::comparison_count;++col){const auto& w=ssc_lab::weapons[ssc_lab::column(col)];auto m=ssc_lab::calculate(w,ssc_lab::guards);
                        double v[]={w.hit*(ssc_lab::guards?w.guard:1.f),double(w.rpm),double(w.magazine),w.reload,m.displayed_dps,m.sustained};std::copy(v,v+6,values[col]);}
                    int width=612/ssc_lab::comparison_count;
                    for(int col=0;col<ssc_lab::comparison_count;++col){int x=474+col*width;const auto& w=ssc_lab::weapons[ssc_lab::column(col)];
                        auto name=ssc_lab::wide_name(w.name);while(name.size()>1&&text_width((name+L"...").c_str(),11)>width-30)name.pop_back();if(name!=ssc_lab::wide_name(w.name))name+=L"...";
                        button(col<2?206+col:272+col,x,250,width-10,72,L"");text(x+14,263,name.c_str(),ink,false,11);auto suffix=ssc_lab::metadata(w);if(!suffix.empty())text(x+14,294,(L"("+suffix+L")").c_str(),cyan,false,10);
                        for(int row=0;row<6;++row){bool better=false,worse=false;double v=values[col][row];
                            for(int other=0;other<ssc_lab::comparison_count;++other)if(other!=col&&std::isfinite(v)&&std::isfinite(values[other][row])){double diff=(v-values[other][row])*(row==3?-1:1);better|=diff>1e-6;worse|=diff< -1e-6;}
                            bool best=better&&!worse;if(best)rectangle(x,337+row*36,width-10,31,RGB(24,76,116));
                            text(x+8,345+row*36,number(v,row==1?L" RPM":row==3?L" s":L"").c_str(),best?cyan:ink,false,12);}
                    }
                    text(266,581,L"Game DPS: firing damage rate. DPS incl. reloads: average over firing + reloads.",muted,false,11);
                    text(266,612,L"Blue marks best comparable values. Damage assumes every projectile hits.",muted,false,11);
                }else{
                    for(int col=0;col<2;++col){int x=266+col*418;const auto& w=ssc_lab::weapons[col?ssc_lab::right:ssc_lab::left];auto name=ssc_lab::label(w);button(206+col,x,247,402,44,name.c_str());
                        text(x+10,295,col?L"PLAYER B":L"PLAYER A",cyan,false,11);
                        auto& trial=ssc_lab::trials[col];size_t cursor=std::min(ssc_lab::shot_cursor[col],trial.shots.size());
                        float hp=ssc_lab::target.health,shield=ssc_lab::target.shield;double time=0;unsigned reloads=0;
                        if(cursor){auto& shot=trial.shots[cursor-1];time=shot.time;reloads=shot.reloads;}
                        auto& incoming=ssc_lab::trials[1-col];auto hits=std::min(ssc_lab::shot_cursor[1-col],incoming.shots.size());if(hits){hp=incoming.shots[hits-1].health;shield=incoming.shots[hits-1].shield;}
                        if(ssc_lab::duel_shown){hp=ssc_lab::duel_result.players[col].health;shield=ssc_lab::duel_result.players[col].shield;time=ssc_lab::duel_result.time;reloads=ssc_lab::duel_result.reloads[col];cursor=ssc_lab::duel_result.shots[col];}
                        text(x+10,318,(hp<=0?L"DEAD":L"HP "+number(hp)+L" / "+number(ssc_lab::target.max_health)).c_str(),hp<=0?RGB(255,80,80):ink,false,12);
                        auto visual=ssc_lab::duel_shown?ssc_lab::visual_target(ssc_lab::trials[1-col],ssc_lab::target,ssc_lab::duel_elapsed,ssc_lab::duel_visual_time):ssc_lab::Target{hp,shield,ssc_lab::target.max_health};
                        // Compact overhead-style bars, rather than the bottom HUD strip.
                        rectangle(x+10,337,380,9,RGB(12,26,39));rectangle(x+10,337,int(380*std::clamp(visual.health/ssc_lab::target.max_health,0.f,1.f)),9,RGB(0,237,105));
                        text(x+10,361,(L"SHIELD "+number(shield)+L" ("+std::to_wstring(int(std::lround(100*shield/ssc_lab::target.max_health)))+L"%)").c_str(),cyan,false,12);
                        rectangle(x+10,388,380,7,RGB(12,26,39));rectangle(x+10,388,int(380*std::clamp(visual.shield/std::max(1.f,ssc_lab::target.shield),0.f,1.f)),7,RGB(46,163,250));
                        if(auto reason=ssc_lab::exclusion(w))text(x+10,422,reason,RGB(255,190,80),false,11);
                        else{auto outcome=trial.killed?std::to_wstring(trial.shots.size())+L" shots  /  "+number(trial.shots.back().time,L" s"):trial.capped?L"Exceeds 10,000-shot limit":L"Not lethal within one magazine";
                            if(ssc_lab::duel_shown){bool reloading=cursor>0&&cursor<trial.shots.size()&&trial.shots[cursor].reloads>trial.shots[cursor-1].reloads;
                                text(x+10,417,hp<=0?L"DEAD":ssc_lab::duel_playing?(reloading?L"RELOADING":L"FIRING"):ssc_lab::duel_elapsed<ssc_lab::duel_complete.time?L"DUEL STOPPED":L"DUEL FINISHED",cyan,false,13);
                            }else text(x+10,417,outcome.c_str(),ink,false,13);
                            text(x+10,452,(L"Shot "+std::to_wstring(cursor)+L"   "+number(time,L" s")+L"   Reloads "+std::to_wstring(reloads)).c_str(),muted,false,11);
                            if(ssc_lab::duel_shown&&ssc_lab::duel_playing&&cursor>0&&cursor<trial.shots.size()&&trial.shots[cursor].reloads>trial.shots[cursor-1].reloads){
                                double start=trial.shots[cursor-1].time,end=trial.shots[cursor].time;float progress=float(std::clamp((time-start)/std::max(.001,end-start),0.,1.));
                                rectangle(x+10,486,380,5,RGB(12,26,39));rectangle(x+10,486,int(380*progress),5,cyan);
                            }
                            if(cursor&&!ssc_lab::duel_shown){auto& shot=trial.shots[cursor-1];text(x+10,483,(L"Last hit: "+number(shot.hp_damage)+L" HP / "+number(shot.shield_damage)+L" shield").c_str(),muted,false,11);}
                        }
                        button(240+col,x+10,521,176,36,col?L"FIRE AT A":L"FIRE AT B",false,!ssc_lab::duel_shown&&cursor<trial.shots.size());button(242+col,x+202,521,188,36,L"RESET SHOTS",false,cursor>0);
                        button(244+col,x+10,568,180,36,L"SHOW RESULT",false,!ssc_lab::duel_shown&&!trial.shots.empty());button(262+col,x+202,568,100,36,L"MISS",ssc_lab::miss_enabled[col]);button(264+col,x+310,568,80,36,(std::to_wstring(ssc_lab::miss_percent[col])+L"%").c_str());
                    }
                    button(260,266,614,236,34,ssc_lab::duel_playing?L"STOP DUEL":L"PLAY DUEL",false,!ssc_lab::trials[0].shots.empty()&&!ssc_lab::trials[1].shots.empty());if(ssc_lab::miss_enabled[0]||ssc_lab::miss_enabled[1])button(261,514,614,220,34,L"REROLL MISSES");text(748,625,ssc_lab::duel_playing?L"DUEL IN PROGRESS":ssc_lab::duel_shown&&ssc_lab::duel_elapsed<ssc_lab::duel_complete.time?L"DUEL STOPPED":ssc_lab::duel_shown?(ssc_lab::duel_result.winner==2?L"EST. DRAW":ssc_lab::duel_result.winner==0?L"PLAYER A WINS":ssc_lab::duel_result.winner==1?L"PLAYER B WINS":L"NO WINNER / LIMIT"):L"",cyan,false,11);
                }
                if(ssc_lab::picker>=0){
                    controls.erase(std::remove_if(controls.begin(),controls.end(),[](const Control& c){return c.id>=200&&c.id<=275;}),controls.end());
                    rectangle(266,188,820,463,RGB(16,37,62));text(282,202,(L"SEARCH: "+ssc_lab::query+L"_").c_str(),cyan,false,14);
                    const wchar_t* sorts[]={L"NAME A-Z",L"FIRE RATE",L"GAME DPS"};button(208,730,194,220,36,sorts[ssc_lab::sort_mode]);button(209,966,194,104,36,L"CLOSE");
                    auto matches=ssc_lab::filtered();
                    for(size_t r=0;r<6&&ssc_lab::picker_page+r<matches.size();++r){auto idx=matches[ssc_lab::picker_page+r];auto label=ssc_lab::label(ssc_lab::weapons[idx]);button(220+int(r),282,247+int(r)*52,788,44,label.c_str(),idx==ssc_lab::column(ssc_lab::picker));}
                    if(matches.empty())text(282,265,L"No matching weapons",muted);
                    button(210,282,580,180,38,L"PREVIOUS",false,ssc_lab::picker_page>=6);button(211,478,580,180,38,L"NEXT",false,ssc_lab::picker_page+6<matches.size());
                }
            }
        } else if(settings_page==10){
            text(266,111,L"RESET HUD LAYOUT?",ink,true);
            text(266,194,L"Restore every HUD element's position and size?",muted);
            button(190,266,280,300,44,L"RESET HUD LAYOUT",true);
            button(191,586,280,180,44,L"CANCEL");
        } else if(settings_page==6){
            text(266,111,L"HUD EDITOR",ink,true);if(!ssc_hud::attached)text(266,160,L"Unavailable on this game version",RGB(247,191,83));toggle(86,986,106,ssc_hud::enabled);
            button(146,266,188,300,48,L"EDIT HUD",false,ssc_hud::attached);button(143,266,254,300,40,L"RESET LAYOUT");
        } else {
            text(266,111,L"ABOUT SSC MOD MENU",ink,true);
            text(266,157,L"0.1.3",cyan);
            text(266,203,L"Optional client-side features for Skillshot City.",muted);
            rectangle(266,255,820,118,RGB(16,37,62));
            text(282,273,L"GAME COMPATIBILITY");
            text(282,314,L"Modules checked independently at startup",muted);
            text(282,343,L"Changed dependencies pause only the affected modules.",muted);
            text(266,414,L"UPDATES");
            text(266,455,ssc_update::message.c_str(),muted,false,14);
            button(160,266,492,260,40,L"CHECK FOR UPDATES",false,!ssc_update::process&&(rpc_preview.phase==1||!presence_supported));
            button(161,542,492,340,40,L"UPDATE AND RESTART",true,ssc_update::available()&&(rpc_preview.phase==1||!presence_supported));

            button(172,266,545,260,34,L"OPEN LOGS");
            text(266,605,L"Unofficial mod client. Not affiliated with the game developer.",muted,false,14);

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






