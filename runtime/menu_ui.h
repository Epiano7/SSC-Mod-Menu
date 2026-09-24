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
unsigned quick_mask=63;
int quick_count(){int n=0;for(int i=0;i<6;++i)n+=(quick_mask>>i)&1;return n;}
int quick_height(){return 186+64*quick_count();}
bool auto_rule_editor=false,auto_guide=false;int auto_list_page=0,auto_guide_page=0;
int dropdown=-1,dropdown_choice=0,chat_field=0;std::vector<std::wstring> dropdown_items;
bool chat_editing=false,chat_select_all=false;std::string chat_buffer;
ssc_edit::Cursor edit_cursor;
struct EditVisual {std::wstring value;int id=0,x=0,y=0,w=0,size=15;size_t first=0;};
std::vector<EditVisual> edit_visuals;
int lab_drag=-1;
bool scale_dragging=false;int scale_preview=100;float scale_drag_left=0,scale_drag_width=1;
bool manager=false,welcome_seen=false,color_editing=false;std::wstring color_buffer;
bool auto_guide_visible(){return manager&&settings_page==16&&auto_guide&&!ssc_hud::editing;}

// Advance this ID when publishing a new set of release notes.
constexpr int release_notes_id=20260924;
bool show_update_notes=true;int seen_release_notes=0;
bool update_notes_due(int phase){return presence_supported&&phase==1&&welcome_seen&&show_update_notes&&seen_release_notes!=release_notes_id&&!opened;}
int canvas_w=1120;constexpr int canvas_h=720;
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
struct WheelHit {int id;POINT points[4];};
std::vector<WheelHit> wheel_hits;
bool inside_wheel(int id,float x,float y){for(const auto& shape:wheel_hits)if(shape.id==id){bool positive=false,negative=false;for(int i=0;i<4;++i){auto a=shape.points[i],b=shape.points[(i+1)%4];float cross=(b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);positive|=cross>0;negative|=cross<0;}return !(positive&&negative);}return true;}
std::vector<Control> controls;

float unit(float value){return std::clamp(value,0.f,1.f);}
void log(const char* message) {
    if(state_dir.empty())return;
    std::ofstream out(state_dir/L"runtime.log",std::ios::app);SYSTEMTIME now;GetSystemTime(&now);char stamp[48];std::snprintf(stamp,sizeof(stamp),"[%04u-%02u-%02u %02u:%02u:%02u UTC] ",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond);out<<stamp<<message<<"\n";
}
void activate(int id);
void commit_text(){if(chat_editing)activate(321);}
void cancel_edit(){commit_text();dropdown=-1;chat_editing=false;sound_search_editing=false;color_editing=false;rpc_editing=false;rpc_error=false;dirty=true;}
void save() {
    ssc_names::cosmetics=cosmetic_requested&&ssc_compat::supports(1);
    ssc_rpc::submit(rpc_requested&&presence_supported,rpc_id,rpc_timer,rpc_preview);
    if(state_dir.empty())return;
    bool chat_saved=ssc_chat::save(state_dir);
    bool auto_saved=ssc_auto::save(state_dir);
    auto path=state_dir/L"menu.ini",temp=state_dir/L"menu.ini.tmp";
    std::ofstream out(temp);
    out<<"quick_modules="<<quick_mask<<"\n";
    out<<"show_update_notes="<<show_update_notes<<"\nseen_release_notes="<<seen_release_notes<<"\n";
    out<<"cooldown_pulse="<<ssc_cooldown::settings.enabled<<"\ncooldown_duration_ms="<<ssc_cooldown::settings.duration_ms<<"\n";
    out<<"hud_hover_fade="<<ssc_hud::hover_fade<<"\n";out<<"hud_enabled="<<ssc_hud::enabled<<"\n";for(const auto& item:ssc_hud::items){out<<"hud_box_"<<item.key<<"="<<item.home_x<<","<<item.home_y<<","<<item.w<<","<<item.h<<"\n";out<<"hud_"<<item.key<<"="<<item.x<<","<<item.y<<","<<item.scale<<"\n";}
    out<<"cosmetic_rainbow="<<ssc_names::rainbow<<"\ncosmetic_rgb="<<ssc_names::solid_rgb<<"\n";out<<"welcome_seen="<<welcome_seen<<"\n";out<<"schema=2\nclock="<<clock_enabled
       <<"\nsound_requested="<<sound_requested<<"\ncosmetic_requested="<<cosmetic_requested<<"\nmatch_sound_level="<<match_sound_level
       <<"\nrpc_requested="<<rpc_requested<<"\nrpc_id="<<rpc_id<<"\nrpc_timer="<<rpc_timer<<"\nrpc_rating="<<rpc_rating
       <<"\nanimations="<<animations<<"\nui_scale="<<ui_scale<<"\n";out.close();
    save_failed=!auto_saved||!chat_saved||!out||!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);dirty=true;
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
    ssc_chat::load(state_dir);ssc_auto::load(state_dir);
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
        if(key=="quick_modules"&&number>=0&&number<64)quick_mask=unsigned(number);
        else if(key=="show_update_notes")show_update_notes=number!=0;
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
    if(presence_tick&&(settings_page==5||settings_page==16))dirty=true;
    static ULONGLONG caret_at=0;if((chat_editing||sound_search_editing||color_editing||rpc_editing||(settings_page==11&&(ssc_lab::picker>=0||ssc_lab::editing>=0)))&&now/500!=caret_at){caret_at=now/500;dirty=true;}
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
bool wheel_round_ended=false;
void open_dropdown(int id,std::vector<std::wstring> labels,int choice){dropdown=dropdown==id?-1:id;dropdown_items=std::move(labels);dropdown_choice=choice;dirty=true;}
void select_dropdown(int value){auto& r=ssc_auto::current();int id=dropdown;dropdown=-1;if(id==488){const int levels[]={0,50,75,100,125,150,200,250,300};if(value>=0&&value<9)ssc_sound::set_volume(levels[value]);}else if(id==462){wheel_round_ended=value==1;auto visible=ssc_chat::visible_order(wheel_round_ended);if(std::find(visible.begin(),visible.end(),ssc_chat::selected)==visible.end())ssc_chat::selected=visible.back();chat_editing=false;}else if(id==324){auto& slot=ssc_chat::slots[ssc_chat::order[ssc_chat::selected]];slot.icon=value;slot.custom=false;}else if(id==326)r.event=ssc_auto::choice_event(value);else if(id==344)ssc_auto::variable=value;else if(id==405)r.any=value==1;else if(id==406)r.repeats=value+1;else if(id==208){ssc_lab::sort_mode=value;ssc_lab::picker_page=0;}else if(id>=410&&id<413)r.conditions[id-410].stat=value;else if(id>=420&&id<423)r.conditions[id-420].comparison=value;save();}
void activate(int id) {
    if(!module_ready(id))return;
    int active_field=chat_field==0?320:chat_field==1?400:chat_field==2?401:450+chat_field-3;
    if(chat_editing&&id!=321&&id!=active_field&&id!=345){commit_text();if(chat_editing)return;}
    if(id!=120&&id!=121)rpc_editing=false;
    if(id!=104)sound_search_editing=false;
    if(id>=1000&&id<1100&&dropdown>=0){select_dropdown(id-1000);return;}
    if(id==490){edit_cursor.begin(ssc_lab::query.size());dirty=true;}
    else if(id==487){ssc_sound::toggle_variants();dirty=true;}
    else if(id==488){if(ssc_sound::selection<ssc_sound::entries.size()){auto v=ssc_sound::clip_volume(ssc_sound::entries[ssc_sound::selection]);open_dropdown(id,{L"0%",L"50%",L"75%",L"100%",L"125%",L"150%",L"200%",L"250%",L"300%"},v==0?0:v==50?1:v==75?2:v==100?3:v==125?4:v==150?5:v==200?6:v==250?7:8);}}
    else if(id==489){ssc_sound::preview_custom(game_window);dirty=true;}
    else if(id==480){show_manager(17);}
    else if(id>=481&&id<=486){quick_mask^=1u<<(id-481);save();}
    else if(id==300&&ssc_chat::attached){ssc_chat::enabled=!ssc_chat::enabled;save();}
    else if(id==301){show_manager(15);}
    else if(id==302){auto_rule_editor=false;auto_list_page=0;ssc_auto::import_pending=ssc_auto::delete_pending=false;show_manager(16);}
    else if(id>=310&&id<=318){ssc_chat::selected=id-310;chat_editing=false;dirty=true;}
    else if(id==320){if(chat_editing&&chat_field==0){dirty=true;return;}chat_field=0;chat_buffer=settings_page==15?ssc_chat::slots[ssc_chat::order[ssc_chat::selected]].text:ssc_auto::current().message;chat_editing=true;chat_select_all=false;edit_cursor.begin(chat_buffer.size());dirty=true;}
    else if(id==321){bool ok=false;if(settings_page==15){ok=ssc_chat::valid_text(chat_buffer);if(ok)ssc_chat::slots[ssc_chat::order[ssc_chat::selected]].text=chat_buffer;}else{auto candidate=ssc_auto::current();if(chat_field==0)candidate.message=chat_buffer;else if(chat_field==1)candidate.name=chat_buffer;else{try{size_t used=0;int value=std::stoi(chat_buffer,&used);if(used!=chat_buffer.size())throw 0;if(chat_field==2)candidate.threshold=value;else candidate.conditions.at(chat_field-3).value=value;}catch(...){ssc_auto::status=L"Enter a valid number";dirty=true;return;}}ok=ssc_auto::valid(candidate);if(ok)ssc_auto::current()=candidate;}if(ok){chat_editing=false;ssc_auto::status.clear();save();}else{ssc_auto::status=L"Check the name, message or number";dirty=true;}}

    else if(id==322){ssc_chat::move_visible(-1,wheel_round_ended);chat_editing=false;save();}
    else if(id==323){ssc_chat::move_visible(1,wheel_round_ended);chat_editing=false;save();}
    else if(id==324){open_dropdown(id,std::vector<std::wstring>(ssc_chat::names,ssc_chat::names+9),ssc_chat::slots[ssc_chat::order[ssc_chat::selected]].icon);}
    else if(id==462){open_dropdown(id,{L"During round",L"Round ended"},wheel_round_ended?1:0);}
    else if(id==325){ssc_chat::reset();chat_editing=false;save();}
    else if(id==328){int slot=ssc_chat::order[ssc_chat::selected];if(ssc_wheel_images::choose(game_window,state_dir,slot)){ssc_chat::slots[slot].custom=true;save();}dirty=true;}
    else if(id==329){ssc_chat::slots[ssc_chat::order[ssc_chat::selected]].custom=false;save();}
    else if(id==326){std::vector<std::wstring> labels;for(int event:ssc_auto::selectable_events)labels.emplace_back(ssc_auto::events[event]);open_dropdown(id,labels,ssc_auto::event_choice(ssc_auto::current().event));}
    else if(id==327){ssc_auto::current().message="GG!";chat_editing=false;save();}
    else if(id==330){ssc_auto::enabled=!ssc_auto::enabled;ssc_auto::engine.reset();save();}
    else if(id==331){auto& r=ssc_auto::current();r.enabled=!r.enabled;save();}
    else if((id==332||id==333)&&!ssc_auto::rules.empty()){ssc_auto::selected=std::clamp(ssc_auto::selected+(id==332?-1:1),0,int(ssc_auto::rules.size())-1);chat_editing=false;ssc_auto::delete_pending=ssc_auto::import_pending=false;dirty=true;}
    else if(id==334&&ssc_auto::rules.size()<12){ssc_auto::rules.push_back({});ssc_auto::selected=int(ssc_auto::rules.size())-1;auto_rule_editor=true;ssc_auto::delete_pending=false;chat_editing=false;save();}
    else if(id==335&&!ssc_auto::rules.empty()){if(ssc_auto::delete_pending){ssc_auto::rules.erase(ssc_auto::rules.begin()+ssc_auto::selected);ssc_auto::current();auto_rule_editor=false;auto_list_page=std::min(auto_list_page,std::max(0,(int(ssc_auto::rules.size())-1)/6));ssc_auto::delete_pending=false;ssc_auto::engine.reset();chat_editing=false;save();}else {ssc_auto::delete_pending=true;dirty=true;}}
    else if(id==336){ssc_auto::status=ssc_auto::copy_clipboard(game_window,ssc_auto::code(ssc_auto::current()))?L"Rule code copied":L"Could not copy rule";dirty=true;}
    else if(id==337){ssc_auto::import_pending=ssc_auto::decode(ssc_auto::read_clipboard(game_window),ssc_auto::imported);ssc_auto::status=ssc_auto::import_pending?L"Review this rule before importing":L"Clipboard does not contain a supported rule code";chat_editing=false;dirty=true;}
    else if(id==338&&ssc_auto::import_pending&&ssc_auto::rules.size()<12){ssc_auto::rules.push_back(ssc_auto::imported);ssc_auto::selected=int(ssc_auto::rules.size())-1;ssc_auto::import_pending=false;auto_rule_editor=false;auto_list_page=ssc_auto::selected/6;ssc_auto::status=L"Imported with rule disabled";save();}
    else if(id==339){ssc_auto::import_pending=ssc_auto::delete_pending=false;dirty=true;}
    else if(id==346){auto_guide=!auto_guide;dirty=true;}
    else if(id==460){auto_guide_page=std::max(0,auto_guide_page-1);dirty=true;}
    else if(id==461){auto_guide_page=std::min((ssc_auto::variable_count-1)/5,auto_guide_page+1);dirty=true;}
    else if(id==347){auto_guide=false;dirty=true;}
    else if(id==348){if(chat_editing)activate(321);if(chat_editing)return;auto_rule_editor=false;ssc_auto::delete_pending=false;auto_list_page=ssc_auto::selected/6;dirty=true;}
    else if(id==349||id==350){auto_list_page=std::clamp(auto_list_page+(id==349?-1:1),0,std::max(0,(int(ssc_auto::rules.size())-1)/6));dirty=true;}
    else if(id>=360&&id<372){int index=id-360;if(index<int(ssc_auto::rules.size())){ssc_auto::rules[index].enabled=!ssc_auto::rules[index].enabled;save();}}
    else if(id>=380&&id<392){int index=id-380;if(index<int(ssc_auto::rules.size())){ssc_auto::selected=index;auto& rule=ssc_auto::current();if(rule.min_level){rule.conditions.push_back({0,0,rule.min_level});rule.min_level=0;}auto_rule_editor=true;ssc_auto::delete_pending=false;chat_editing=false;dirty=true;}}
    else if(id==400||id==401||(id>=450&&id<453)){int next_field=id==400?1:id==401?2:3+id-450;if(chat_editing&&chat_field==next_field){dirty=true;return;}chat_field=next_field;auto& r=ssc_auto::current();chat_buffer=chat_field==1?r.name:std::to_string(chat_field==2?r.threshold:r.conditions[chat_field-3].value);chat_editing=true;chat_select_all=false;edit_cursor.begin(chat_buffer.size());dirty=true;}
    else if(id==405){open_dropdown(id,{L"All conditions (AND)",L"Any condition (OR)"},ssc_auto::current().any?1:0);}
    else if(id==406){open_dropdown(id,{L"1 time",L"2 times",L"3 times",L"4 times",L"5 times"},ssc_auto::current().repeats-1);}
    else if(id==407&&ssc_auto::current().conditions.size()<3){ssc_auto::current().conditions.push_back({});save();}
    else if(id>=410&&id<413){open_dropdown(id,std::vector<std::wstring>(ssc_auto::condition_labels,ssc_auto::condition_labels+ssc_auto::condition_count),ssc_auto::current().conditions[id-410].stat);}
    else if(id>=420&&id<423){open_dropdown(id,{L"At least",L"At most",L"Equal to"},ssc_auto::current().conditions[id-420].comparison);}
    else if(id>=430&&id<433){auto& v=ssc_auto::current().conditions;v.erase(v.begin()+id-430);chat_editing=false;save();}
    else if(id==340||id==341){auto& value=ssc_auto::current().threshold;value=std::clamp(value+(id==340?-1:1)*((GetKeyState(VK_SHIFT)&0x8000)?10:1),1,1000);save();}
    else if(id==342||id==343){auto& value=ssc_auto::current().min_level;value=std::clamp(value+(id==342?-1:1),0,1000);save();}
    else if(id==344){open_dropdown(id,std::vector<std::wstring>(ssc_auto::variable_labels,ssc_auto::variable_labels+ssc_auto::variable_count),ssc_auto::variable);}
    else if(id==345){std::string token="{"+std::string(ssc_auto::variables[ssc_auto::variable])+"}";if(!chat_editing||chat_field!=0){chat_field=0;chat_buffer=ssc_auto::current().message;chat_editing=true;edit_cursor.begin(chat_buffer.size());}edit_cursor.insert(chat_buffer,token,100);chat_select_all=false;dirty=true;}

    else if(id==172){ssc_diagnostics::open_logs(state_dir);}
    else if(id==160){if(!presence_supported||rpc_preview.phase==1)ssc_update::check(state_dir);dirty=true;}
    else if(id==161){if(!presence_supported||rpc_preview.phase==1)ssc_update::apply();dirty=true;}
    else if(id==180||id==181){ssc_names::rainbow=id==180;color_editing=false;save();}
    else if(id>=182&&id<=187){const unsigned colors[]={0xffffff,0x55ccff,0xff66cc,0x66ee99,0xffbb44,0xff6655};ssc_names::solid_rgb=colors[id-182];ssc_names::rainbow=false;save();}
    else if(id==188){if(color_editing){dirty=true;return;}wchar_t value[8];swprintf(value,8,L"%06X",ssc_names::solid_rgb);color_buffer=value;color_editing=true;edit_cursor.begin(color_buffer.size());dirty=true;}
    else if(id==189){if(color_buffer.size()==6){ssc_names::solid_rgb=std::stoul(color_buffer,nullptr,16);ssc_names::rainbow=false;color_editing=false;save();}}
    else if(id==170){opened=true;show_manager(8);}
    else if(id==171){close_menu();}
    else if(id==280){show_update_notes=!show_update_notes;save();}
    else if(id==281){opened=true;show_manager(14);}
    else if(id==282){close_menu();}
    else if(id==105){ssc_sound::preview_original();dirty=true;}
    else if(id==1)close_menu();
    else if(id==3)show_manager();
    else if(id==4){ssc_auto::enabled=false;ssc_auto::engine.reset();ssc_chat::enabled=false;ssc_hud::enabled=false;sound_requested=false;cosmetic_requested=false;rpc_requested=false;clock_enabled=false;save();}
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
    else if(id==274||id==275){ssc_lab::open_picker(id-272);edit_cursor.begin(ssc_lab::query.size());dirty=true;}
    else if(id==14){ssc_lab::refresh();show_manager(11);}
    else if(id>=200&&id<=203){ssc_lab::select(id>=202,id%2?1:-1);ssc_lab::recompute();dirty=true;}
    else if(id==204){ssc_lab::refresh();dirty=true;}
    else if(id==205){ssc_lab::guards=!ssc_lab::guards;ssc_lab::recompute();dirty=true;}
    else if(id==206||id==207){ssc_lab::open_picker(id-206);edit_cursor.begin(ssc_lab::query.size());dirty=true;}
    else if(id==208){open_dropdown(id,{L"Name A-Z",L"Fire rate",L"Game DPS"},ssc_lab::sort_mode);}
    else if(id>=230&&id<=233){ssc_lab::editing=-1;ssc_lab::tab=id-230;controls.clear();dirty=true;}
    else if(id>=250&&id<=254){if(ssc_lab::commit_edit()){ssc_lab::begin_edit(id-250);edit_cursor.begin(ssc_lab::edit_buffer.size());}dirty=true;}
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
    else if(id==120){rpc_editing=true;rpc_select_all=false;rpc_error=false;rpc_buffer=std::wstring(rpc_id.begin(),rpc_id.end());edit_cursor.begin(rpc_buffer.size());dirty=true;}
    else if(id==121){std::string value(rpc_buffer.begin(),rpc_buffer.end());if(value.empty()||ssc_rpc::valid_id(value)){rpc_id=value.empty()?ssc_rpc::bundled_application_id:value;rpc_editing=false;rpc_error=false;save();}else {rpc_error=true;dirty=true;}}
    else if(id>=91&&id<=93){int page=id==91?3:id==92?4:5;show_manager(page);}
    else if(id==100){if(sound_page>=5)sound_page-=5;dirty=true;}
    else if(id==101){if(sound_page+5<sound_results.size())sound_page+=5;dirty=true;}
    else if(id==102){ssc_sound::import_selected(game_window,match_sound_level);if(IsWindow(game_window)&&GetForegroundWindow()==game_window){opened=true;manager=true;settings_page=3;visibility=1.f;modal_visibility=1.f;}dirty=true;}
    else if(id==103){ssc_sound::remove_selected();dirty=true;}
    else if(id==104){sound_search_editing=true;edit_cursor.begin(sound_query.size());dirty=true;}
    else if(id>=110&&id<115){size_t index=sound_page+id-110;if(index<sound_results.size())ssc_sound::selection=sound_results[index];dirty=true;}

}
int hit(int x,int y) {
    if(!opened||visibility<.85f)return 0;
    float lx=(x-panel_x)/draw_scale,ly=(y-panel_y)/draw_scale;
    for(auto it=controls.rbegin();it!=controls.rend();++it){auto& c=*it;if(dropdown>=0&&c.id<1000&&c.id!=dropdown)continue;if(c.enabled&&lx>=c.x&&lx<c.x+c.w&&ly>=c.y&&ly<c.y+c.h&&inside_wheel(c.id,lx,ly))return c.id;}
    return 0;
}
void place_caret(int id,int screen_x);
template<class S,class Accept> bool edit_input(S& value,UINT message,WPARAM wp,size_t limit,Accept accept,HWND window){
 bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
 if(message==WM_KEYDOWN){
  if(wp==VK_LEFT||wp==VK_RIGHT||wp==VK_HOME||wp==VK_END){edit_cursor.move(value,int(wp),shift,ctrl);dirty=true;return true;}
  if(wp==VK_DELETE){edit_cursor.del(value);dirty=true;return true;}
  if(ctrl&&wp=='A'){edit_cursor.anchor=0;edit_cursor.pos=value.size();dirty=true;return true;}
  if(ctrl&&(wp=='C'||wp=='X')){edit_cursor.clamp(value);if(edit_cursor.selected()){auto part=value.substr(std::min(edit_cursor.pos,edit_cursor.anchor),std::max(edit_cursor.pos,edit_cursor.anchor)-std::min(edit_cursor.pos,edit_cursor.anchor));if(ssc_auto::copy_clipboard(window,std::string(part.begin(),part.end()))&&wp=='X')edit_cursor.erase(value);}dirty=true;return true;}
  if(ctrl&&wp=='V'){auto incoming=ssc_auto::read_clipboard(window);if(incoming.empty())return true;S text;for(auto c:incoming){if(!accept(unsigned(c)))return true;text+=typename S::value_type(c);}edit_cursor.insert(value,text,limit);dirty=true;return true;}
 }
 if(message==WM_CHAR){unsigned c=unsigned(wp);if(c==8)edit_cursor.backspace(value);else if(!ctrl&&accept(c))edit_cursor.insert(value,S(1,typename S::value_type(c)),limit);dirty=true;return true;}
 return false;
}
LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    if(opened&&dropdown>=0){if(message==WM_MOUSEWHEEL){dropdown_choice=std::clamp(dropdown_choice-GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA,0,int(dropdown_items.size())-1);dirty=true;return 0;}if(message==WM_KEYDOWN){if(wp==VK_ESCAPE){dropdown=-1;dirty=true;suppress_escape_up=true;return 0;}if(wp==VK_UP||wp==VK_DOWN||wp==VK_TAB){dropdown_choice=std::clamp(dropdown_choice+(wp==VK_UP?-1:1),0,int(dropdown_items.size())-1);dirty=true;return 0;}if(wp==VK_RETURN){select_dropdown(dropdown_choice);return 0;}}if(message==WM_LBUTTONDOWN&&hit(GET_X_LPARAM(lp),GET_Y_LPARAM(lp))==0){dropdown=-1;pressed=0;dirty=true;return 0;}}

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
        if(opened&&manager&&(settings_page==15||settings_page==16)&&chat_editing){
            if(message==WM_KEYDOWN&&wp==VK_ESCAPE){chat_editing=false;dirty=true;suppress_escape_up=true;return 0;}
            if(message==WM_KEYDOWN&&wp==VK_RETURN){activate(321);return 0;}
            if(edit_input(chat_buffer,message,wp,chat_field==1&&settings_page==16?32:100,[](unsigned c){return c>=32&&c<=126;},window))return 0;
        }
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
            if(edit_input(ssc_lab::query,message,wp,48,[](unsigned c){return c>=32&&c<127;},window)){ssc_lab::picker_page=0;return 0;}
            if(message==WM_MOUSEWHEEL){activate(GET_WHEEL_DELTA_WPARAM(wp)>0?210:211);return 0;}
        }
        if(opened&&manager&&settings_page==11&&ssc_lab::editing>=0){
            if(message==WM_KEYDOWN&&wp==VK_ESCAPE){ssc_lab::editing=-1;dirty=true;suppress_escape_up=true;return 0;}
            if(message==WM_KEYDOWN&&(wp==VK_RETURN||wp==VK_TAB)){ssc_lab::commit_edit();dirty=true;return 0;}
            if(edit_input(ssc_lab::edit_buffer,message,wp,7,[](unsigned c){return c>='0'&&c<='9';},window)){ssc_lab::edit_error=false;return 0;}
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
            if(edit_input(color_buffer,message,wp,6,[](unsigned c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');},window))return 0;
        }
        if(opened&&rpc_editing){
            if(message==WM_KEYDOWN&&wp==VK_ESCAPE){rpc_editing=false;rpc_error=false;dirty=true;suppress_escape_up=true;return 0;}
            if(message==WM_KEYDOWN&&wp==VK_RETURN){activate(121);return 0;}
            if(edit_input(rpc_buffer,message,wp,20,[](unsigned c){return c>='0'&&c<='9';},window)){rpc_error=false;return 0;}
        }
        if(sound_search_editing&&opened&&manager&&settings_page==3){
            if(message==WM_KEYDOWN&&(wp==VK_ESCAPE||wp==VK_RETURN)){sound_search_editing=false;dirty=true;return 0;}
            if(edit_input(sound_query,message,wp,48,[](unsigned c){return c>=32&&c<127;},window)){filter_sounds();return 0;}
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
        if(message==WM_LBUTTONUP){int id=hit(GET_X_LPARAM(lp),GET_Y_LPARAM(lp));int down=pressed;pressed=0;if(id&&id==down){activate(id);place_caret(id,GET_X_LPARAM(lp));}else if(!id){commit_text();}dirty=true;}
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
    canvas_w=auto_guide_visible()?1440:1120;
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
void editbox(int id,int x,int y,int w,int h,const std::wstring& value,bool active,const wchar_t* placeholder=L""){
 button(id,x,y,w,h,L"",active);constexpr int size=15;size_t first=0;
 if(active){edit_cursor.clamp(value);first=std::min(edit_cursor.view,edit_cursor.pos);while(first<edit_cursor.pos&&text_width(value.substr(first,edit_cursor.pos-first).c_str(),size)>w-32)++first;edit_cursor.view=first;}
 size_t last=first;while(last<value.size()&&text_width(value.substr(first,last-first+1).c_str(),size)<=w-32)++last;
 if(active&&edit_cursor.selected()){size_t lo=std::max(first,std::min(edit_cursor.pos,edit_cursor.anchor)),hi=std::min(last,std::max(edit_cursor.pos,edit_cursor.anchor));if(hi>lo){int left=int(text_width(value.substr(first,lo-first).c_str(),size)),right=int(text_width(value.substr(first,hi-first).c_str(),size));rectangle(x+14+left,y+6,right-left,h-12,RGB(30,108,178));}}
 auto shown=value.substr(first,last-first);text(x+14,y+(h-size)/2,value.empty()&&!active?placeholder:shown.c_str(),value.empty()&&!active?muted:ink,false,size);
 if(active&&(GetTickCount64()/500)%2==0){int offset=int(text_width(value.substr(first,edit_cursor.pos-first).c_str(),size));rectangle(x+14+offset,y+7,2,h-14,cyan);}
 edit_visuals.push_back({value,id,x,y,w,size,first});
}
void place_caret(int id,int screen_x){auto it=std::find_if(edit_visuals.begin(),edit_visuals.end(),[&](const EditVisual& v){return v.id==id;});if(it==edit_visuals.end())return;float x=(screen_x-panel_x)/draw_scale-it->x-14;size_t at=it->first;while(at<it->value.size()){float left=text_width(it->value.substr(it->first,at-it->first).c_str(),it->size),right=text_width(it->value.substr(it->first,at-it->first+1).c_str(),it->size);if(x<(left+right)/2)break;++at;}edit_cursor.pos=edit_cursor.anchor=at;dirty=true;}
std::vector<std::wstring> wrap_label(const std::wstring& value,int width,int size){std::vector<std::wstring> lines;size_t start=0;while(start<value.size()){size_t end=start,space=std::wstring::npos;while(end<value.size()&&text_width(value.substr(start,end-start+1).c_str(),size)<=width){if(value[end]==L' ')space=end;++end;}if(end==start)++end;if(end<value.size()&&space!=std::wstring::npos&&space>start)end=space;lines.push_back(value.substr(start,end-start));start=end;while(start<value.size()&&value[start]==L' ')++start;}return lines;}
void toggle(int id,int x,int y,bool enabled) {if(!module_ready(id)){button(id,x,y,94,36,L"PAUSED",false,false);return;}button(id,x,y,94,36,enabled?L"ON":L"OFF",enabled);rectangle(x+70,y+10,10,16,enabled?RGB(99,239,173):muted);}
void finish_canvas() {
    GdiFlush();auto b=static_cast<unsigned char*>(pixels);
    int pw=int(std::lround(panel_w*raster_scale)),ph=int(std::lround(panel_h*raster_scale));
    for(int y=0;y<ph;++y)for(int x=0;x<pw;++x)b[(y*raster_w+x)*4+3]=255;
    int corner=int(std::lround(10*raster_scale));
    for(int y=0;y<corner;++y)for(int x=0;x<corner-y;++x){b[(y*raster_w+x)*4+3]=0;b[(y*raster_w+pw-1-x)*4+3]=0;b[((ph-1-y)*raster_w+x)*4+3]=0;b[((ph-1-y)*raster_w+pw-1-x)*4+3]=0;}

}

void preview_pixels(int x,int y,int size,const unsigned char* source,int width,int height,int sx=0,int sy=0,int sw=0,int sh=0){
 if(!source||width<=0||height<=0){return;}
 if(!sw){sw=width;}if(!sh){sh=height;}GdiFlush();auto dest=static_cast<unsigned char*>(pixels);
 int left=int(x*raster_scale),top=int(y*raster_scale),extent=std::max(1,int(size*raster_scale));for(int dy=0;dy<extent;++dy)for(int dx=0;dx<extent;++dx){int px=left+dx,py=top+dy;if(px<0||py<0||px>=raster_w||py>=raster_h)continue;int tx=sx+dx*sw/extent,ty=sy+dy*sh/extent;if(tx<0||ty<0||tx>=width||ty>=height)continue;auto src=source+(ty*width+tx)*4;auto dst=dest+(py*raster_w+px)*4;float a=src[3]/255.f;for(int k=0;k<3;++k)dst[k]=static_cast<unsigned char>(dst[k]*(1-a)+src[2-k]*a);dst[3]=255;}
}
void preview_icon(int slot,int x,int y,int size){
 const auto& config=ssc_chat::slots[slot];if(config.custom&&!ssc_wheel_images::images[slot].empty()){preview_pixels(x,y,size,ssc_wheel_images::images[slot].data(),64,64);return;}
 static GLuint cached=0;static int width=0,height=0,cols=0,rows=0,tile=0;static std::vector<unsigned char> atlas;
 if(wglGetCurrentContext()&&ssc_chat::attached){auto record=ssc_chat::stock_atlas();GLuint id=0;int w=0,h=0,c=0,r=0,t=0;using ssc_names::read;if(record&&read(record+0x20,id)&&read(record+0x24,w)&&read(record+0x28,h)&&read(record+0x2c,c)&&read(record+0x30,r)&&read(record+0x34,t)&&w>0&&h>0&&w<=4096&&h<=4096&&c>0&&r>0&&t>0&&c*t<=w&&r*t<=h&&glIsTexture(id)&&id!=cached){GLint bound=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);atlas.resize(size_t(w)*h*4);glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_PACK_ALIGNMENT,1);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);using Bind=void(APIENTRY*)(GLenum,GLuint);auto bind=reinterpret_cast<Bind>(wglGetProcAddress("glBindBuffer"));GLint pack=0;if(bind){glGetIntegerv(0x88ed,&pack);bind(0x88eb,0);}glBindTexture(GL_TEXTURE_2D,id);glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,atlas.data());glBindTexture(GL_TEXTURE_2D,bound);if(bind)bind(0x88eb,pack);glPopClientAttrib();cached=id;width=w;height=h;cols=c;rows=r;tile=t;}}
 int index=0x200+config.icon;if(!atlas.empty()&&index<cols*rows){preview_pixels(x,y,size,atlas.data(),width,height,index%cols*tile,index/cols*tile,tile,tile);return;}
 const wchar_t* symbols[]={L">>",L"!",L"YES",L"NO",L"WP",L"THX",L"HI",L"GLHF",L"GG"};text(x,y+size/3,symbols[config.icon],cyan,false,12);
}
void wheel_preview(){
 const float cx=479,cy=375,radius=158;auto visible=ssc_chat::visible_order(wheel_round_ended);
 for(int position=0;position<8;++position){int i=visible[position];double angle=ssc_chat::preview_angle(position);float x=cx+float(std::cos(angle))*radius,y=cy+float(std::sin(angle))*radius;
  POINT points[4]={{LONG(cx+std::cos(angle-.30)*93),LONG(cy+std::sin(angle-.30)*93)},{LONG(cx+std::cos(angle-.30)*192),LONG(cy+std::sin(angle-.30)*192)},{LONG(cx+std::cos(angle+.30)*192),LONG(cy+std::sin(angle+.30)*192)},{LONG(cx+std::cos(angle+.30)*93),LONG(cy+std::sin(angle+.30)*93)}};
  auto brush=CreateSolidBrush(i==ssc_chat::selected?RGB(27,106,196):RGB(24,47,75));auto old=SelectObject(canvas,brush);auto pen=SelectObject(canvas,GetStockObject(NULL_PEN));Polygon(canvas,points,4);SelectObject(canvas,pen);SelectObject(canvas,old);DeleteObject(brush);
  WheelHit shape{};shape.id=310+i;int left=points[0].x,right=left,top=points[0].y,bottom=top;for(int j=0;j<4;++j){shape.points[j]=points[j];left=std::min(left,int(points[j].x));right=std::max(right,int(points[j].x));top=std::min(top,int(points[j].y));bottom=std::max(bottom,int(points[j].y));}wheel_hits.push_back(shape);controls.push_back({310+i,left,top,right-left+1,bottom-top+1,true});preview_icon(ssc_chat::order[i],int(x)-22,int(y)-22,44);
 }
 auto& selected=ssc_chat::slots[ssc_chat::order[ssc_chat::selected]];auto value=chat_editing?chat_buffer:selected.text;auto label=value.empty()?std::wstring(ssc_chat::names[ssc_chat::order[ssc_chat::selected]]):ssc_chat::widen(value);auto lines=wrap_label(label,166,13);int size=lines.size()>6?11:13;if(size==11)lines=wrap_label(label,166,size);int top=int(cy)-int(lines.size())*(size+3)/2;for(size_t row=0;row<lines.size();++row)text(int(cx)-text_width(lines[row].c_str(),size)/2,top+int(row)*(size+3),lines[row].c_str(),ink,false,size);button(462,329,574,300,32,wheel_round_ended?L"ROUND ENDED":L"DURING ROUND");
}
void paint_panel() {
    prepare_canvas();if(!pixels)return;controls.clear();wheel_hits.clear();
    edit_visuals.clear();
    panel_w=manager?1120:640;panel_h=manager?720:quick_height();
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
        text(50,153,L"CUSTOM QUICK CHAT",cyan);
        bullet(185,L"Customize wheel messages, order and icons with a live preview");
        bullet(209,L"Import your own images; long messages wrap inside the wheel");
        text(50,257,L"AUTO MESSAGES",cyan);
        bullet(289,L"Create rules for round events and stat thresholds");
        bullet(313,L"Use {variables} or [variables], conditions and shareable rule codes");
        bullet(337,L"Includes a removable Auto-GG rule and a detached variable guide");
        text(50,385,L"SOUND & INTERFACE",cyan);
        bullet(417,L"Custom sound previews and per-clip volume from 0%-300%");
        bullet(441,L"Apply a Killed Human replacement to all three sound variants");
        bullet(465,L"Choose which modules appear in the quick menu under Interface");
        bullet(489,L"Text cursor, selection and automatic saving when leaving text fields");
        bullet(513,L"Fixed wheel click areas and improved round tracking");
        text(50,585,L"Manage this popup in Interface > Show update notes",muted,false,14);
        button(282,410,624,300,48,L"GOT IT",true,true,true);
    } else if(!manager) {
        const wchar_t* names[]={L"SOUND REPLACER",L"COSMETICS",L"DISCORD PRESENCE",L"HUD EDITOR",L"CUSTOM QUICK CHAT",L"AUTO MESSAGES"};
        const bool enabled[]={sound_requested,cosmetic_requested,rpc_requested,ssc_hud::enabled,ssc_chat::enabled,ssc_auto::enabled};const int ids[]={80,81,83,86,300,330},settings[]={91,92,93,94,301,302};
        int row=0;for(int i=0;i<6;++i)if(quick_mask&(1u<<i)){int y=100+row++*64;rectangle(20,y,600,56,RGB(16,37,62));text(32,y+20,names[i],ink,false,15);toggle(ids[i],350,y+10,enabled[i]);button(settings[i],456,y+10,146,36,L"SETTINGS");}
        int bottom=118+row*64;button(3,20,bottom,366,42,L"ALL MODULES",true,true,true);button(4,402,bottom,218,42,L"DISABLE ALL");
        if(save_failed)text(24,bottom+51,L"Could not save settings",RGB(247,191,83));
    } else {
        button(10,24,108,192,44,L"MODULES",settings_page==-1||(settings_page>=3&&settings_page<=6));
        button(11,24,166,192,44,L"INTERFACE",settings_page==1);
        button(13,24,224,192,44,L"MISC",settings_page==9);
        button(12,24,282,192,44,L"ABOUT",settings_page==2);
        button(14,24,340,192,44,L"WEAPON LAB",settings_page==11);button(15,24,398,192,44,L"RECORDING",settings_page==12);button(16,24,456,192,44,L"CREDITS",settings_page==13);
        rectangle(238,108,1,546,RGB(38,65,101));

        if(settings_page==17){
            text(266,111,L"QUICK MENU",ink,true);
            const wchar_t* names[]={L"Sound replacer",L"Cosmetics",L"Discord presence",L"HUD editor",L"Custom quick chat",L"Auto messages"};
            for(int i=0;i<6;++i){int y=188+i*70;text(282,y+9,names[i]);toggle(481+i,978,y,(quick_mask&(1u<<i))!=0);}
        } else if(settings_page==-1) {
            text(266,111,L"MODULES",ink,true);
            const wchar_t* names[]={L"Sound replacer",L"Cosmetics",L"Discord presence",L"HUD editor"};
            const bool enabled[]={sound_requested,cosmetic_requested,rpc_requested,ssc_hud::enabled};const int ids[]={80,81,83,86};
            for(int i=0;i<4;++i){int y=174+i*80;rectangle(266,y,820,66,RGB(16,37,62));text(284,y+22,names[i],ink,true);toggle(ids[i],822,y+15,enabled[i]);button(91+i,932,y+15,138,36,L"SETTINGS");}
            rectangle(266,494,820,66,RGB(16,37,62));text(284,516,L"Custom quick chat",ink,true);toggle(300,822,509,ssc_chat::enabled);button(301,932,509,138,36,L"SETTINGS");
            rectangle(266,574,820,66,RGB(16,37,62));text(284,596,L"Auto messages",ink,true);toggle(330,822,589,ssc_auto::enabled);button(302,932,589,138,36,L"SETTINGS");
        } else if(settings_page==15){
            text(266,111,L"CUSTOM QUICK CHAT",ink,true);toggle(300,986,106,ssc_chat::enabled);
            wheel_preview();auto& slot=ssc_chat::slots[ssc_chat::order[ssc_chat::selected]];
            text(720,180,L"MESSAGE",muted,false,13);auto value=chat_editing?chat_buffer:slot.text;
            editbox(320,720,207,366,40,ssc_chat::widen(value),chat_editing,L"Use original message");button(321,720,259,160,36,L"SAVE",true,chat_editing);
            text(720,317,L"ICON",muted,false,13);button(324,720,342,366,38,slot.custom?L"Custom image":ssc_chat::names[slot.icon]);button(328,720,392,220,38,L"IMPORT IMAGE");button(329,952,392,134,38,L"RESET",false,slot.custom);
            button(322,720,452,175,38,L"MOVE UP",false,ssc_chat::selected>0);button(323,911,452,175,38,L"MOVE DOWN",false,ssc_chat::selected<8);
            button(325,720,513,366,38,L"RESET WHEEL");if(!ssc_wheel_images::status.empty())text(720,568,ssc_wheel_images::status.substr(0,40).c_str(),cyan,false,12);
            text(286,613,L"Click a slot to edit it",muted,false,13);
        } else if(settings_page==16){
            text(266,111,L"AUTO MESSAGES",ink,true);
            auto& r=ssc_auto::current();
            if(ssc_auto::import_pending){
                const auto& incoming=ssc_auto::imported;text(266,185,ssc_chat::widen(incoming.name).c_str(),cyan,true);auto event=std::wstring(ssc_auto::events[incoming.event]);if(incoming.event>=3)event+=L" "+std::to_wstring(incoming.threshold);text(266,234,event.c_str());
                text(266,275,(L"Send "+std::to_wstring(incoming.repeats)+(incoming.repeats==1?L" time":L" times")).c_str());
                if(incoming.min_level)text(660,275,(L"Minimum level: "+std::to_wstring(incoming.min_level)).c_str(),muted,false,14);
                if(!incoming.conditions.empty()){text(266,311,incoming.any?L"Any condition (OR)":L"All conditions (AND)",muted,false,14);const auto& stats=ssc_auto::condition_labels;const wchar_t* op[]={L"at least",L"at most",L"equal to"};for(int i=0;i<int(incoming.conditions.size());++i){auto& c=incoming.conditions[i];auto line=std::wstring(stats[c.stat])+L" "+op[c.comparison]+L" "+std::to_wstring(c.value);text(266,340+i*27,line.c_str(),ink,false,14);}}
                text(266,438,ssc_chat::widen(incoming.message.substr(0,65)).c_str(),ink,false,14);if(incoming.message.size()>65)text(266,465,ssc_chat::widen(incoming.message.substr(65)).c_str(),ink,false,14);
                text(266,514,L"The imported rule starts disabled",muted,false,14);button(338,266,561,300,40,L"IMPORT RULE",true,ssc_auto::rules.size()<12);button(339,590,561,180,40,L"CANCEL");

            }else if(!auto_rule_editor){
                button(334,266,163,220,40,L"NEW RULE",true,ssc_auto::rules.size()<12);button(337,502,163,200,40,L"IMPORT CODE");button(346,718,163,250,40,L"VARIABLE GUIDE",auto_guide);
                auto_list_page=std::clamp(auto_list_page,0,std::max(0,(int(ssc_auto::rules.size())-1)/6));
                if(ssc_auto::rules.empty()){text(286,278,L"No rules yet",cyan,true);text(286,323,L"Choose New Rule to set when a message is sent",muted,false,15);}
                for(int row=0;row<6;++row){int index=auto_list_page*6+row;if(index>=int(ssc_auto::rules.size()))break;const auto& item=ssc_auto::rules[index];int y=218+row*63;
                    rectangle(266,y,820,55,RGB(16,37,62));auto label=ssc_chat::widen(item.name);
                    text(280,y+7,label.c_str(),ink,false,15);auto message=ssc_chat::widen(item.message.substr(0,52));if(item.message.size()>52)message+=L"...";text(280,y+31,message.c_str(),muted,false,12);
                    toggle(360+index,822,y+9,item.enabled);button(380+index,938,y+9,132,36,L"EDIT");
                }
                if(!ssc_auto::status.empty())text(266,598,ssc_auto::status.substr(0,76).c_str(),cyan,false,11);
                auto count=std::count_if(ssc_auto::rules.begin(),ssc_auto::rules.end(),[](const ssc_auto::Rule& item){return item.enabled;});
                text(266,619,(std::to_wstring(count)+L" enabled / "+std::to_wstring(ssc_auto::rules.size())+L" rules").c_str(),muted,false,13);
                if(ssc_auto::rules.size()>6){button(349,850,607,44,36,L"<",false,auto_list_page>0);text(917,618,(std::to_wstring(auto_list_page+1)+L" / "+std::to_wstring((ssc_auto::rules.size()+5)/6)).c_str(),ink,false,13);button(350,1026,607,44,36,L">",false,(auto_list_page+1)*6<int(ssc_auto::rules.size()));}
            }else{
                button(348,266,157,205,34,L"< ALL RULES");button(335,822,157,156,34,ssc_auto::delete_pending?L"CONFIRM":L"DELETE");toggle(331,986,155,r.enabled);
                text(266,209,L"NAME",muted,false,13);editbox(400,366,201,720,34,ssc_chat::widen(chat_editing&&chat_field==1?chat_buffer:r.name),chat_editing&&chat_field==1);
                text(266,252,L"WHEN",muted,false,13);button(326,366,244,510,34,ssc_auto::events[r.event]);if(r.event>=3)editbox(401,894,244,192,34,ssc_chat::widen(chat_editing&&chat_field==2?chat_buffer:std::to_string(r.threshold)),chat_editing&&chat_field==2);
                text(266,295,L"ONLY IF",muted,false,13);if(!r.conditions.empty())button(405,366,287,330,32,r.any?L"ANY CONDITION (OR)":L"ALL CONDITIONS (AND)");else text(366,296,L"No conditions",muted,false,14);button(407,826,287,260,32,L"ADD CONDITION",false,r.conditions.size()<3);
                const auto& stats=ssc_auto::condition_labels;const wchar_t* comparisons[]={L"At least",L"At most",L"Equal to"};
                for(int i=0;i<int(r.conditions.size());++i){auto& c=r.conditions[i];int y=326+i*36;button(410+i,366,y,245,30,stats[c.stat]);button(420+i,621,y,205,30,comparisons[c.comparison]);editbox(450+i,836,y,196,30,ssc_chat::widen(chat_editing&&chat_field==3+i?chat_buffer:std::to_string(c.value)),chat_editing&&chat_field==3+i);button(430+i,1044,y,42,30,L"X");}
                text(266,447,L"MESSAGE",muted,false,13);auto value=chat_editing&&chat_field==0?chat_buffer:r.message;editbox(320,366,437,720,36,ssc_chat::widen(value),chat_editing&&chat_field==0);
                text(266,490,L"SEND",muted,false,13);button(406,366,479,160,34,(std::to_wstring(r.repeats)+(r.repeats==1?L" time":L" times")).c_str());button(344,546,479,295,34,ssc_auto::variable_labels[ssc_auto::variable]);button(345,853,479,233,34,L"INSERT VARIABLE");
                std::string preview,error;ssc_auto::format(value,ssc_auto::examples(),preview,error);text(266,528,L"EXAMPLE",cyan,false,12);text(366,528,ssc_chat::widen((error.empty()?preview:error).substr(0,76)).c_str(),ink,false,12);
                button(321,266,562,150,36,L"SAVE",true,chat_editing);button(336,432,562,302,36,L"COPY RULE CODE");button(346,750,562,336,36,L"VARIABLE GUIDE",auto_guide);
                if(!ssc_auto::status.empty())text(266,616,ssc_auto::status.substr(0,88).c_str(),cyan,false,12);

            }
        } else if(settings_page==1) {
            text(266,111,L"INTERFACE",ink,true);button(480,800,106,286,38,L"QUICK MENU MODULES");
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
            editbox(104,266,187,540,36,sound_query,sound_search_editing,L"Search sounds");
            button(100,822,187,120,36,L"PREVIOUS");button(101,958,187,128,36,L"NEXT");
            for(size_t i=0;i<5&&sound_page+i<sound_results.size();++i){size_t index=sound_results[sound_page+i];auto label=ssc_sound::entries[index].label+(ssc_sound::has_replacement(ssc_sound::entries[index])?L" [CUSTOM]":L"");button(110+int(i),266,236+int(i)*34,820,30,label.c_str(),index==ssc_sound::selection);}

            if(ssc_sound::selection<ssc_sound::entries.size()){
                auto& e=ssc_sound::entries[ssc_sound::selection];std::wstring info=e.label+L" / "+std::to_wstring(e.rate)+L" Hz / "+std::to_wstring(e.channels)+L" ch";
                text(266,424,info.c_str(),cyan,false,14);
            }
            button(102,266,454,242,36,L"IMPORT AUDIO");button(103,526,454,242,36,L"RESTORE ORIGINAL");
            text(792,464,L"MATCH LEVEL",muted,false,12);toggle(82,982,454,match_sound_level);
            text(266,640,ssc_sound::status.c_str(),ink,false,11);
            if(ssc_sound::selection<ssc_sound::entries.size()){auto& e=ssc_sound::entries[ssc_sound::selection];text(266,514,L"USE FOR ALL VARIANTS",muted,false,13);button(487,600,500,150,36,ssc_sound::all_variants(e)?L"ON":L"OFF",ssc_sound::all_variants(e),ssc_sound::has_replacement(e)&&!ssc_sound::family(e).empty());text(792,514,L"VOLUME",muted,false,13);button(488,914,500,172,36,(std::to_wstring(ssc_sound::clip_volume(e))+L"%").c_str(),false,ssc_sound::has_replacement(e));}
            button(489,566,586,280,38,L"PREVIEW CUSTOM",false,ssc_sound::selection<ssc_sound::entries.size()&&ssc_sound::has_replacement(ssc_sound::entries[ssc_sound::selection]));
            button(105,266,586,280,38,L"PREVIEW ORIGINAL",false,ssc_sound::selection<ssc_sound::entries.size());
        } else if(settings_page==4) {
            text(266,111,L"COSMETICS",ink,true);toggle(81,986,106,cosmetic_requested);
            if(!ssc_names::attached)text(266,160,L"Unavailable on this game version",RGB(247,191,83));
            button(180,266,198,250,42,L"RAINBOW",ssc_names::rainbow);button(181,534,198,250,42,L"SOLID COLOR",!ssc_names::rainbow);
            const wchar_t* labels[]={L"WHITE",L"CYAN",L"PINK",L"GREEN",L"GOLD",L"CORAL"};
            for(int i=0;i<6;++i)button(182+i,266+i*136,266,126,36,labels[i]);
            wchar_t hex[16];swprintf(hex,16,L"#%06X",ssc_names::solid_rgb);auto label=color_editing?L"#"+color_buffer+L"_":std::wstring(hex);
            editbox(188,266,328,240,40,color_editing?color_buffer:std::wstring(hex),color_editing);button(189,520,328,140,40,L"APPLY",false,color_editing&&color_buffer.size()==6);
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
                        }else editbox(250+field,580,243+row*49,260,38,ssc_lab::editing==field?ssc_lab::edit_buffer:std::to_wstring(values[row]),ssc_lab::editing==field);}
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
                    rectangle(266,188,820,463,RGB(16,37,62));editbox(490,282,194,430,36,ssc_lab::query,true,L"Search weapons");
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
            text(266,157,L"0.1.4",cyan);
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
    if(dropdown>=0){auto owner=std::find_if(controls.begin(),controls.end(),[](const Control& c){return c.id==dropdown;});if(owner!=controls.end()){int x=owner->x,w=std::max(owner->w,210),rows=std::min(8,int(dropdown_items.size())),first=std::clamp(dropdown_choice-rows/2,0,std::max(0,int(dropdown_items.size())-rows)),height=rows*32,y=owner->y+owner->h+3;if(y+height>654)y=std::max(90,owner->y-height-3);w=std::min(w,1100-x);rectangle(x-2,y-2,w+4,height+4,cyan);for(int row=0;row<rows;++row){int i=first+row;button(1000+i,x,y+row*32,w,32,dropdown_items[i].c_str(),i==dropdown_choice);}}}

    finish_canvas();
    if(auto_guide_visible()){
        GdiFlush();auto data=static_cast<unsigned char*>(pixels);
        panel_w=1440;
        rectangle(1132,100,284,514,RGB(8,21,41));rectangle(1132,100,284,46,RGB(18,41,72));rectangle(1132,146,284,2,cyan);
        text(1146,116,L"VARIABLE GUIDE",ink,false,17);button(347,1374,106,30,30,L"X",false,true,true);
        text(1146,167,L"Use {name} or [name]",muted,false,13);
        for(int row=0;row<5;++row){int i=auto_guide_page*5+row;if(i>=ssc_auto::variable_count)break;int y=205+row*68;text(1146,y,ssc_auto::variable_labels[i],ink,false,14);auto token=L"{"+ssc_chat::widen(ssc_auto::variables[i])+L"}";text(1146,y+21,token.c_str(),cyan,false,13);text(1146,y+42,ssc_auto::variable_details[i],muted,false,11);}
        button(460,1146,560,62,30,L"<",false,auto_guide_page>0);button(461,1338,560,62,30,L">",false,auto_guide_page<(ssc_auto::variable_count-1)/5);text(1225,569,(std::to_wstring(auto_guide_page+1)+L" / "+std::to_wstring((ssc_auto::variable_count+4)/5)).c_str(),muted,false,13);
        GdiFlush();int left=int(std::lround(1132*raster_scale)),right=int(std::lround(1416*raster_scale)),top=int(std::lround(100*raster_scale)),bottom=int(std::lround(614*raster_scale));
        for(int y=top;y<bottom;++y)for(int x=left;x<right;++x)data[(size_t(y)*raster_w+x)*4+3]=255;
    }
}
void paint_live_hud_toolbar(){
 prepare_canvas();if(!pixels)return;std::memset(pixels,0,raster_w*raster_h*4);controls.clear();edit_visuals.clear();rectangle(0,0,panel_w,panel_h,RGB(12,30,50));
 wchar_t label[128];const auto& a=ssc_hud::items[ssc_hud::selected];swprintf(label,128,L"%ls  %d%%",a.name,int(std::lround(a.scale*100)));text(12,10,label,cyan,false,16);
 text(12,42,L"Drag / wheel resize / Tab select / Esc done",muted,false,12);
 button(144,386,8,34,28,L"<");button(145,428,8,34,28,L">");button(148,474,8,168,28,L"RESET ITEM");button(147,654,8,94,28,L"DONE");finish_canvas();
}
void layout_panel(int width,int height) {
    if(ssc_hud::editing){panel_w=760;panel_h=68;float base=std::min(1.f,std::max(.1f,float(width-24)/panel_w));if(std::abs(raster_scale-base)>.00001f){raster_scale=base;dirty=true;}draw_scale=base;panel_x=int((width-panel_w*base)*.5f);panel_y=ssc_hud::items[ssc_hud::selected].y<.13f?int(height-panel_h*base-12):12;return;}

    panel_w=auto_guide_visible()?1440:manager?1120:640;panel_h=manager?720:quick_height();
    float fit=std::min(float(width-40)/(auto_guide_visible()?1760:panel_w),float(height-40)/panel_h);
    float base=std::min(float(ui_scale)/100.f,std::max(.1f,fit));float e=ease(visibility);
    if(std::abs(raster_scale-base)>.00001f){raster_scale=base;dirty=true;}
    draw_scale=base*(manager?(.88f+.12f*e):(.96f+.04f*e));
    panel_x=manager?int((width-(auto_guide_visible()?1120:panel_w)*draw_scale)*.5f):int(width-panel_w*draw_scale-24+(1-e)*24);
    panel_y=manager?int((height-panel_h*draw_scale)*.5f+(1-e)*18):int(40+(1-e)*12);
}






