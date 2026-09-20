#include "../runtime/overlay.cpp"
#include <cassert>
static void snapshot(const std::filesystem::path& path) {
    BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+raster_w*raster_h*4;
    BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=raster_w;info.biHeight=-raster_h;info.biPlanes=1;info.biBitCount=32;
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&file),sizeof(file));out.write(reinterpret_cast<char*>(&info),sizeof(info));out.write(static_cast<char*>(pixels),raster_w*raster_h*4);assert(out);
}
int main(int argc,char** argv) {
    native_supported=true;presence_supported=true;
    assert(argc>=2);state_dir=std::filesystem::path(argv[1]);std::filesystem::create_directories(state_dir);
    {std::ofstream out(state_dir/"menu.ini");out<<"clock=1\n";}
    load_settings();assert(!clock_enabled);
    {std::ofstream out(state_dir/"menu.ini");out<<"schema=2\nclock=1\nunused_preference=1\n";}
    load_settings();assert(!clock_enabled);
    opened=true;for(int i=0;i<4;++i)advance_animation(.05f);assert(visibility==1.f);
    advance_animation(-1);assert(visibility==1.f);
    close_menu();advance_animation(.05f);assert(visibility>0&&visibility<1);
    opened=true;advance_animation(.05f);assert(visibility>0);
    animations=false;close_menu();advance_animation(0);assert(visibility==0);
    opened=true;advance_animation(0);assert(visibility==1);
    activate(41);assert(!clock_enabled);activate(52);assert(ui_scale==115);
    clock_enabled=false;ui_scale=100;load_settings();
    assert(!clock_enabled&&ui_scale==115&&!animations);
    {std::ofstream out(state_dir/"invalid.glf",std::ios::binary);out<<"not a font";}
    LocalFont invalid;assert(!invalid.load(state_dir/"invalid.glf")&&invalid.atlas.empty());
    {std::ofstream out(state_dir/"oversized.glf",std::ios::binary);uint32_t h[]={0,0xffffffff,1024,0,128,0};out.write(reinterpret_cast<char*>(h),sizeof(h));}
    assert(!invalid.load(state_dir/"oversized.glf"));
    if(argc>2){assert(local_font.load(std::filesystem::path(argv[2])/"data/translations/black128_60.glf"));assert(heavy_font.load(std::filesystem::path(argv[2])/"data/translations/thick128_60.glf"));}
    prepare_canvas();assert(canvas&&pixels);manager=true;settings_page=4;paint_panel();layout_panel(1280,720);
    assert(panel_x>=0&&panel_y>=0&&panel_x+panel_w*draw_scale<=1280&&panel_y+panel_h*draw_scale<=720);
    assert(hit(panel_x+int(1030*draw_scale),panel_y+int(123*draw_scale))==81);
    visibility=.4f;assert(hit(panel_x+int(1030*draw_scale),panel_y+int(123*draw_scale))==0);visibility=1;
    layout_panel(800,600);assert(panel_x>=0&&panel_y>=0&&panel_x+panel_w*draw_scale<=800&&panel_y+panel_h*draw_scale<=600);
    snapshot(state_dir/"settings.bmp");settings_page=1;paint_panel();snapshot(state_dir/"interface.bmp");settings_page=2;paint_panel();snapshot(state_dir/"about.bmp");assert(std::none_of(controls.begin(),controls.end(),[](const Control& c){return c.id==41;}));
    activate(170);visibility=1;paint_panel();assert(controls.size()==1&&controls[0].id==171);snapshot(state_dir/"welcome.bmp");activate(171);assert(welcome_seen&&!opened);welcome_seen=false;load_settings();assert(welcome_seen);opened=true;show_quick();visibility=1;paint_panel();snapshot(state_dir/"quick.bmp");
    activate(4);assert(!clock_enabled);
    opened=true;manager=true;visibility=modal_visibility=1;activate(14);assert(settings_page==11&&visibility==1&&modal_visibility==1);
    paint_panel();snapshot(state_dir/"weapon-lab-unavailable.bmp");
    ssc_lab::weapons={{"Assault Rifle",600,30,20,2,1,0,0,0},{"Burst Rifle",900,24,25,2.5f,1,3,.2f,0}};ssc_lab::left=0;ssc_lab::right=1;ssc_lab::status=L"Base weapons / current game data";
    layout_panel(1920,1080);paint_panel();snapshot(state_dir/"weapon-lab.bmp");
    assert(std::count_if(controls.begin(),controls.end(),[](const Control& c){return c.id>=204&&c.id<=207;})==3);
    activate(206);assert(ssc_lab::picker==0);ssc_lab::query=L"burst";paint_panel();snapshot(state_dir/"weapon-picker.bmp");activate(220);assert(ssc_lab::left==1&&ssc_lab::picker==-1);ssc_lab::left=0;
    activate(200);assert(ssc_lab::left==1);activate(203);assert(ssc_lab::right==0);activate(205);assert(ssc_lab::guards);ssc_lab::guards=false;
    ssc_lab::left=ssc_lab::right=0;ssc_lab::shield_rules={1,1,true};ssc_lab::recompute();
    activate(240);assert(ssc_lab::shot_cursor[0]==1);activate(244);assert(ssc_lab::shot_cursor[0]==ssc_lab::trials[0].shots.size());activate(242);assert(ssc_lab::shot_cursor[0]==0);
    activate(232);activate(250);for(wchar_t c:std::wstring(L"1000"))window_proc(nullptr,WM_CHAR,c,0);window_proc(nullptr,WM_KEYDOWN,VK_RETURN,0);assert(ssc_lab::target.max_health==1000&&ssc_lab::target.health==1000);
    activate(252);ssc_lab::edit_buffer=L"1001";assert(!ssc_lab::commit_edit()&&ssc_lab::edit_error);window_proc(nullptr,WM_KEYDOWN,VK_ESCAPE,0);assert(ssc_lab::target.health==1000);
    activate(251);ssc_lab::edit_buffer=L"50";assert(ssc_lab::commit_edit());
    paint_panel();snapshot(state_dir/"weapon-target.bmp");activate(230);paint_panel();snapshot(state_dir/"weapon-calculator.bmp");activate(240);paint_panel();snapshot(state_dir/"weapon-shot.bmp");activate(231);paint_panel();snapshot(state_dir/"weapon-stats.bmp");activate(272);activate(272);assert(ssc_lab::comparison_count==4);paint_panel();snapshot(state_dir/"weapon-four-stats.bmp");activate(273);activate(273);activate(256);activate(230);
    for(int scale:{85,100,115}){ui_scale=scale;layout_panel(1280,720);paint_panel();snapshot(state_dir/("weapon-lab-"+std::to_string(scale)+".bmp"));}
    ssc_lab::left=ssc_lab::right=0;ssc_lab::recompute();activate(260);assert(ssc_lab::duel_shown&&ssc_lab::duel_playing);paint_panel();snapshot(state_dir/"weapon-duel.bmp");ssc_lab::advance_duel(9999);assert(!ssc_lab::duel_playing&&ssc_lab::duel_result.winner==2);activate(262);assert(ssc_lab::miss_enabled[0]&&!ssc_lab::duel_shown);activate(262);
    activate(232);keyboard_focus=250;auto hp_before=ssc_lab::target.max_health;window_proc(nullptr,WM_KEYDOWN,VK_LEFT,0);assert(ssc_lab::target.max_health==hp_before-1);window_proc(nullptr,WM_KEYDOWN,VK_RIGHT,0);assert(ssc_lab::target.max_health==hp_before);
    activate(16);paint_panel();snapshot(state_dir/"credits.bmp");show_manager(11);
    activate(232);lab_drag=0;move_lab_slider(panel_x+int(580*draw_scale));finish_lab_slider();assert(ssc_lab::target.max_health==1);lab_drag=0;move_lab_slider(panel_x+int(828*draw_scale));finish_lab_slider();assert(ssc_lab::target.max_health>=999);lab_drag=1;move_lab_slider(panel_x+int(704*draw_scale));finish_lab_slider();assert(ssc_lab::target.shield>980);paint_panel();snapshot(state_dir/"weapon-sliders.bmp");activate(256);activate(230);
    activate(15);paint_panel();snapshot(state_dir/"local-recording.bmp");assert(!ssc_record::state().enabled);activate(14);
    ui_scale=100;
    settings_page=1;layout_panel(1920,1080);scale_dragging=true;scale_drag_left=100;scale_drag_width=600;move_scale(100);assert(scale_preview==50);move_scale(700);assert(scale_preview==200);move_scale(300);assert(scale_preview==100);move_scale(900);assert(scale_preview==200);finish_scale();assert(ui_scale==200);ui_scale=100;load_settings();assert(ui_scale==200);activate(54);assert(ui_scale==100);
    for(int scale:{50,73,100,147,200}){ui_scale=scale;layout_panel(2560,1440);paint_panel();snapshot(state_dir/("scale-slider-"+std::to_string(scale)+".bmp"));assert(panel_x>=0&&panel_y>=0&&panel_x+panel_w*draw_scale<=2560&&panel_y+panel_h*draw_scale<=1440);}
    activate(54);
    {std::ifstream in(state_dir/"menu.ini");std::string saved((std::istreambuf_iterator<char>(in)),{});assert(saved.find("unused_preference")==std::string::npos);}
    manager=true;settings_page=2;rpc_preview.phase=2;paint_panel();
    assert(std::none_of(controls.begin(),controls.end(),[](const Control& c){return (c.id==160||c.id==161)&&c.enabled;}));
    rpc_preview.phase=1;ssc_update::version="0.2.0";paint_panel();
    assert(std::any_of(controls.begin(),controls.end(),[](const Control& c){return c.id==161&&c.enabled;}));
    settings_page=7;paint_panel();snapshot(state_dir/"update-prompt.bmp");
    native_supported=false;presence_supported=false;manager=false;paint_panel();snapshot(state_dir/"compatibility.bmp");
    assert(std::none_of(controls.begin(),controls.end(),[](const Control& c){return c.id>=80&&c.id<=94;}));
    assert(std::any_of(controls.begin(),controls.end(),[](const Control& c){return c.id==160&&c.enabled;}));
    native_supported=true;presence_supported=true;ssc_update::version.clear();rpc_preview.phase=0;
    // An unavailable module preserves its preference and cannot enter HUD edit mode.
    ssc_compat::initialized=true;ssc_compat::available=0;ssc_hud::attached=false;
    cosmetic_requested=true;activate(81);assert(cosmetic_requested);
    activate(94);assert(manager&&settings_page==6&&!ssc_hud::editing);
    manager=false;paint_panel();assert(std::none_of(controls.begin(),controls.end(),[](const Control& c){return (c.id==81||c.id==86)&&c.enabled;}));
    ssc_compat::initialized=false;cosmetic_requested=false;manager=true;
    // Verify the preview against the supported game's palette leaf function without starting it.
    if(argc>3){
        HMODULE module=LoadLibraryExA(argv[3],nullptr,DONT_RESOLVE_DLL_REFERENCES);assert(module);
        using Palette=float*(__cdecl*)(uintptr_t,float*,float,float,unsigned char,unsigned char);
        auto native=reinterpret_cast<Palette>(reinterpret_cast<uintptr_t>(module)+0x1523a0);
        float max_error=0;
        for(int i=0;i<4096;++i)for(float lift:{.1f,.2f}){float phase=float(i)/4096,out[3]{};native(0,out,phase*255, lift,0,1);auto preview=rainbow_rgb(phase,lift);
            for(int k=0;k<3;++k)max_error=std::max(max_error,std::abs(out[k]-preview[k]));}
        std::printf("Native palette maximum channel error: %.9f\n",max_error);assert(max_error<.000002f);FreeLibrary(module);
    }
    for(int i=0;i<128;++i){auto a=rainbow_rgb(float(i)/128,.2f),b=rainbow_rgb(1+float(i)/128,.2f);for(int k=0;k<3;++k)assert(std::abs(a[k]-b[k])<.000002f);}
    activate(80);activate(81);assert(sound_requested&&cosmetic_requested);activate(4);assert(!sound_requested&&!cosmetic_requested);
    animations=true;opened=true;manager=false;show_manager();
    for(int i=0;i<8;++i){advance_animation(.025f);assert(std::abs(visibility-modal_visibility)<.00001f);}
    assert(visibility==1&&modal_visibility==1);advance_animation(.025f);assert(modal_visibility==1);
    // Full-window navigation must preserve size and dimming, including Interface,
    // Miscellaneous and nested reset/welcome pages, with animations on or off.
    for(bool motion:{false,true}){
        animations=motion;opened=manager=true;visibility=modal_visibility=1;
        layout_panel(1920,1080);const auto scale=draw_scale;const int x=panel_x,y=panel_y;
        for(int id:{11,13,12,10,91,92,93,143,191,170,11}){
            activate(id);advance_animation(.016f);layout_panel(1920,1080);
            assert(visibility==1&&modal_visibility==1&&draw_scale==scale&&panel_x==x&&panel_y==y);
        }
    }
    // Changing the preference in an already-open window never replays an opening.
    visibility=.4f;modal_visibility=.4f;activate(40);
    assert(visibility==1&&modal_visibility==1);
    animations=true;
    // Background RPC ticks and solid previews must not repaint static pages.
    for(int page:{-1,1,2,3,6,8,9,10}){
        settings_page=page;dirty=false;
        for(ULONGLONG t=1000;t<=11000;t+=50)refresh_dynamic_panel(t,t%1000==0);
        assert(!dirty);
    }
    settings_page=5;dirty=false;refresh_dynamic_panel(12000,true);assert(dirty);
    settings_page=4;ssc_names::rainbow=false;dirty=false;refresh_dynamic_panel(13000,true);assert(!dirty);
    ssc_names::rainbow=true;refresh_dynamic_panel(14000);assert(dirty);
    dirty=false;refresh_dynamic_panel(14001);assert(!dirty);
    refresh_dynamic_panel(14050);assert(dirty);
    close_menu();for(int i=0;i<8;++i){advance_animation(.025f);assert(std::abs(visibility-modal_visibility)<.00001f);}
    opened=true;visibility=1;manager=true;settings_page=-1;paint_panel();snapshot(state_dir/"modules.bmp");
    assert(std::count_if(controls.begin(),controls.end(),[](const Control& c){return c.id>=91&&c.id<=94;})==4);
    settings_page=3;paint_panel();snapshot(state_dir/"sound.bmp");settings_page=4;paint_panel();snapshot(state_dir/"cosmetics.bmp");
    rpc_requested=false;activate(120);rpc_buffer=L"123456789012345678";activate(121);assert(rpc_id=="123456789012345678"&&!rpc_editing);
    rpc_id.clear();load_settings();assert(rpc_id=="123456789012345678");
    activate(120);rpc_buffer=L"not an ID";activate(121);assert(rpc_error&&rpc_editing);cancel_edit();
    rpc_id.clear();activate(83);assert(rpc_requested);activate(4);assert(!rpc_requested);
    activate(84);activate(85);assert(!rpc_timer&&!rpc_rating);rpc_timer=rpc_rating=true;load_settings();assert(!rpc_timer&&!rpc_rating);
    show_manager(5);rpc_preview=ssc_rpc::from_steam("#Status_Server","BR Solos Round 2 - Level 7 / Marksman");paint_panel();snapshot(state_dir/"presence.bmp");
    assert(std::none_of(controls.begin(),controls.end(),[](const Control& c){return c.id==120||c.id==121;}));
    for(auto& c:controls)assert(c.x>=0&&c.y>=0&&c.x+c.w<=panel_w&&c.y+c.h<=panel_h);
    animations=false;opened=true;visibility=1;manager=true;settings_page=-1;
    for(int scale:{85,100,115}){ui_scale=scale;layout_panel(1920,1080);paint_panel();assert(raster_w==int(std::lround(canvas_w*scale/100.f)));assert(raster_h==int(std::lround(canvas_h*scale/100.f)));snapshot(state_dir/("modules-"+std::to_string(scale)+".bmp"));assert(hit(panel_x+int(1030*draw_scale),panel_y+int(200*draw_scale))==91);}
    ui_scale=100;layout_panel(1920,1080);settings_page=6;paint_panel();snapshot(state_dir/"hud-editor.bmp");
    ssc_hud::reset();auto& item=ssc_hud::items[0];assert(hud_hit(hud_view_x+(item.x+.01f)*hud_view_w,hud_view_y+(item.y+.01f)*hud_view_h)==0);
    hud_drag=0;hud_start_x=hud_start_y=0;hud_item_x=item.x;hud_item_y=item.y;hud_item_scale=1;hud_resize=false;move_hud(-hud_view_w*.5f,hud_view_h*.3f);assert(std::abs(item.x-.26f)<.00001f&&std::abs(item.y-.32f)<.00001f);
    finish_hud_drag();ssc_hud::reset();load_settings();assert(std::abs(item.x-.26f)<.00001f&&std::abs(item.y-.32f)<.00001f);
    hud_drag=0;hud_item_scale=1;hud_resize=true;move_hud(item.w*hud_view_w*.5f,item.h*hud_view_h*.5f);assert(std::abs(item.scale-1.5f)<.00001f);finish_hud_drag();
    activate(86);assert(ssc_hud::enabled);activate(4);assert(!ssc_hud::enabled);activate(143);assert(settings_page==10);activate(191);assert(settings_page==6);activate(143);activate(190);assert(item.x==item.home_x&&item.scale==1);activate(192);assert(!ssc_hud::hover_fade);ssc_hud::hover_fade=true;load_settings();assert(!ssc_hud::hover_fade);activate(192);
    ssc_cooldown::settings={};activate(193);assert(!ssc_cooldown::settings.enabled);
    activate(199);ssc_cooldown::settings={};load_settings();
    assert(!ssc_cooldown::settings.enabled&&ssc_cooldown::settings.opacity==1&&ssc_cooldown::settings.duration_ms==2500);
    activate(193);activate(198);
    assert(ssc_cooldown::settings.enabled&&ssc_cooldown::settings.opacity==1.f&&ssc_cooldown::settings.duration_ms==1500);
    show_manager(9);paint_panel();snapshot(state_dir/"miscellaneous.bmp");
    for(const auto& c:controls)assert(c.x>=0&&c.y>=0&&c.x+c.w<=panel_w&&c.y+c.h<=panel_h);
    manager=true;settings_page=6;opened=true;hud_drag=0;item.x=.2f;
    window_proc(nullptr,WM_KEYDOWN,VK_ESCAPE,1);assert(hud_drag==-1&&settings_page==-1);ssc_hud::reset();load_settings();assert(std::abs(item.x-.2f)<.00001f);
    close_menu(true);assert(visibility==0);
    ssc_hud::view_w=1280;ssc_hud::view_h=720;ssc_hud::reset();
    WNDCLASSW test_class{};test_class.lpfnWndProc=DefWindowProcW;test_class.hInstance=GetModuleHandleW(nullptr);test_class.lpszClassName=L"SSCLiveHudInputTest";assert(RegisterClassW(&test_class));
    HWND test_window=CreateWindowW(test_class.lpszClassName,L"HUD input test",WS_OVERLAPPED,140,90,1300,760,nullptr,nullptr,test_class.hInstance,nullptr);assert(test_window);
    activate(94);assert(ssc_hud::editing&&opened&&modal_visibility==0);
    for(int i=0;i<int(ssc_hud::items.size());++i){ssc_hud::selected=i;auto& a=ssc_hud::items[i];float scale=a.scale;POINT point{620,360};ClientToScreen(test_window,&point);
        window_proc(test_window,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),MAKELPARAM(point.x,point.y));assert(a.scale>scale);
    }
    // Wheel input also works for a selected thin/overlapping item without a hover hit.
    ssc_hud::selected=3;ssc_hud::items[3].scale=2;live_hud_wheel(120,-1);assert(ssc_hud::items[3].scale>2);
    ssc_hud::items[3].scale=6;live_hud_wheel(120,-1);assert(ssc_hud::items[3].scale==6);
    layout_panel(1280,720);paint_live_hud_toolbar();snapshot(state_dir/"hud-live-toolbar.bmp");assert(panel_h==68);
    hud_drag=7;ssc_hud::freeze_bounds=true;hud_resize=false;hud_start_x=hud_start_y=0;hud_item_x=ssc_hud::items[7].x;hud_item_y=ssc_hud::items[7].y;
    live_hud_move(-10,-10);assert(ssc_hud::items[7].x==0&&ssc_hud::items[7].y==0);
    window_proc(test_window,WM_KEYDOWN,VK_ESCAPE,1);assert(!ssc_hud::editing&&!opened&&hud_drag==-1&&!ssc_hud::freeze_bounds);
    DestroyWindow(test_window);UnregisterClassW(test_class.lpszClassName,test_class.hInstance);
    // Realistic comparison/playback snapshots; hidden renderer only.
    opened=true;manager=true;visibility=modal_visibility=1;ui_scale=100;show_manager(11);layout_panel(1920,1080);
    ssc_lab::weapons={{"USP Tactical",360,12,11,1,1},{"Deagle",200,7,30,1,1},{"Minigun",720,60,10.5f,1.5f,1},{"Barrett M98B",60,5,100,1.5f,1}};
    ssc_lab::status=L"Base weapons / current game data";int categories[]={2,2,6,7},tiers[]={0,1,2,1};for(int i=0;i<4;++i){ssc_lab::weapons[i].category=categories[i];ssc_lab::weapons[i].tier=tiers[i];ssc_lab::column(i)=i;}
    ssc_lab::weapons[2].projectiles=3;ssc_lab::target={300,150,300};ssc_lab::guards=false;ssc_lab::shield_rules={1,1,true};ssc_lab::recompute();ssc_lab::comparison_count=4;ssc_lab::tab=1;paint_panel();snapshot(state_dir/"comparison-four-realistic.bmp");
    activate(274);assert(ssc_lab::picker==2);ssc_lab::query=L"T3 LMG";paint_panel();snapshot(state_dir/"picker-tier-search.bmp");activate(220);assert(ssc_lab::column(2)==2&&ssc_lab::picker==-1);
    ssc_lab::tab=0;ssc_lab::start_duel();paint_panel();snapshot(state_dir/"duel-start.bmp");ssc_lab::advance_duel(1.9);paint_panel();snapshot(state_dir/"duel-mid.bmp");ssc_lab::advance_duel(100);assert(!ssc_lab::duel_playing);paint_panel();snapshot(state_dir/"duel-end.bmp");
    activate(16);paint_panel();snapshot(state_dir/"credits-final.bmp");assert(std::any_of(controls.begin(),controls.end(),[](const Control& c){return c.id==276&&c.enabled;}));
    show_manager(11);activate(233);paint_panel();snapshot(state_dir/"weapon-info.bmp");assert(ssc_lab::tab==3);activate(230);ssc_lab::miss_enabled[0]=ssc_lab::miss_enabled[1]=false;paint_panel();assert(std::none_of(controls.begin(),controls.end(),[](const Control& c){return c.id==261;}));activate(262);paint_panel();assert(std::any_of(controls.begin(),controls.end(),[](const Control& c){return c.id==261;}));
    activate(15);paint_panel();snapshot(state_dir/"recording-simple.bmp");
    // Release notes are menu-only, once per notes revision, with a persistent opt-out.
    presence_supported=true;welcome_seen=true;opened=false;seen_release_notes=0;show_update_notes=true;
    assert(!update_notes_due(0)&&!update_notes_due(2)&&update_notes_due(1));
    activate(281);assert(!update_notes_due(1));paint_panel();snapshot(state_dir/"update-notes.bmp");
    activate(282);assert(!opened&&seen_release_notes==release_notes_id&&!update_notes_due(1));
    seen_release_notes=0;load_settings();assert(seen_release_notes==release_notes_id);
    activate(280);seen_release_notes=0;assert(!update_notes_due(1));show_update_notes=true;load_settings();assert(!show_update_notes);
    activate(281);assert(opened&&settings_page==14);window_proc(nullptr,WM_KEYDOWN,VK_ESCAPE,1);assert(!opened&&seen_release_notes==release_notes_id);
    activate(280);welcome_seen=false;seen_release_notes=0;opened=true;show_manager(8);activate(171);assert(welcome_seen&&seen_release_notes==release_notes_id&&!update_notes_due(1));
    show_manager(1);opened=true;paint_panel();snapshot(state_dir/"interface-update-notes.bmp");
    std::puts("PASS: animation reversal/reduced motion, settings round-trip/migration, fit/hit tests, transition click guard, disable/reset and all panel renders");
}
