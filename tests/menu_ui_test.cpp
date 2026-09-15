#include "../runtime/overlay.cpp"
#include <cassert>
static void snapshot(const std::filesystem::path& path) {
    BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+raster_w*raster_h*4;
    BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=raster_w;info.biHeight=-raster_h;info.biPlanes=1;info.biBitCount=32;
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&file),sizeof(file));out.write(reinterpret_cast<char*>(&info),sizeof(info));out.write(static_cast<char*>(pixels),raster_w*raster_h*4);assert(out);
}
int main(int argc,char** argv) {
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
    show_quick();visibility=1;paint_panel();snapshot(state_dir/"quick.bmp");
    activate(4);assert(!clock_enabled);
    {std::ifstream in(state_dir/"menu.ini");std::string saved((std::istreambuf_iterator<char>(in)),{});assert(saved.find("unused_preference")==std::string::npos);}
    // Verify the preview against the supported game's palette leaf function without starting it.
    if(argc>3){
        HMODULE module=LoadLibraryExA(argv[3],nullptr,DONT_RESOLVE_DLL_REFERENCES);assert(module);
        using Palette=float*(__cdecl*)(uintptr_t,float*,float,float,unsigned char,unsigned char);
        auto native=reinterpret_cast<Palette>(reinterpret_cast<uintptr_t>(module)+0x151f50);
        float max_error=0;
        for(int i=0;i<4096;++i)for(float lift:{.1f,.2f}){float phase=float(i)/4096,out[3]{};native(0,out,phase*255, lift,0,1);auto preview=rainbow_rgb(phase,lift);
            for(int k=0;k<3;++k)max_error=std::max(max_error,std::abs(out[k]-preview[k]));}
        std::printf("Native palette maximum channel error: %.9f\n",max_error);assert(max_error<.000002f);FreeLibrary(module);
    }
    for(int i=0;i<128;++i){auto a=rainbow_rgb(float(i)/128,.2f),b=rainbow_rgb(1+float(i)/128,.2f);for(int k=0;k<3;++k)assert(std::abs(a[k]-b[k])<.000002f);}
    activate(80);activate(81);assert(sound_requested&&cosmetic_requested);activate(4);assert(!sound_requested&&!cosmetic_requested);
    animations=true;opened=true;show_manager();
    for(int i=0;i<8;++i){advance_animation(.025f);assert(std::abs(visibility-modal_visibility)<.00001f);}
    assert(visibility==1&&modal_visibility==1);advance_animation(.025f);assert(modal_visibility==1);
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
    activate(86);assert(ssc_hud::enabled);activate(4);assert(!ssc_hud::enabled);activate(143);assert(item.x==item.home_x&&item.scale==1);
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
    std::puts("PASS: animation reversal/reduced motion, settings round-trip/migration, fit/hit tests, transition click guard, disable/reset and all panel renders");
}
