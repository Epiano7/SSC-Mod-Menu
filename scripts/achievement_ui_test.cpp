#include "../runtime/overlay.cpp"
#include <cassert>
#include <iostream>
static void save_bitmap(const std::filesystem::path& p,int w,int h,const void* data){
 BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+w*h*4;
 BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=w;info.biHeight=-h;info.biPlanes=1;info.biBitCount=32;
 std::ofstream out(p,std::ios::binary);out.write(reinterpret_cast<char*>(&file),sizeof(file));out.write(reinterpret_cast<char*>(&info),sizeof(info));out.write(static_cast<const char*>(data),w*h*4);assert(out);
}
static void check_notice(const ssc_achievements::NoticeRaster& raster,ssc_achievements::NoticeSize size){
 using namespace ssc_achievements;
 const auto& layout=raster.layout;
 assert(layout.body.width==size.width*notice_raster_scale&&layout.body.height==size.height*notice_raster_scale);
 assert(layout.title_width>0&&layout.texture.width>=layout.title_width&&layout.texture.height>layout.body.height);
 assert(raster.pixels.size()==size_t(layout.texture.width)*layout.texture.height*4);
 assert(raster.pixels[3]==245);
 bool glyph=false,transparent=false;
 for(size_t i=size_t(layout.texture.width)*layout.body.height*4+3;i<raster.pixels.size();i+=4){
  glyph|=raster.pixels[i]>200;transparent|=raster.pixels[i]==0;
 }
 assert(glyph&&transparent);
}
static void save_notice_view(const char* name,const ssc_achievements::NoticeRaster& raster,uint64_t age){
 using namespace ssc_achievements;
 const auto& l=raster.layout;
 std::vector<unsigned char> frame(size_t(l.body.width)*l.body.height*4);
 for(int row=0;row<l.body.height;++row)
  std::copy_n(raster.pixels.data()+size_t(row)*l.texture.width*4,size_t(l.body.width)*4,
              frame.data()+size_t(row)*l.body.width*4);
 const int shown=std::min(l.title_width,int(l.title.right-l.title.left));
 const int scroll=int(std::lround(title_offset(l,age)));
 for(int row=0;row<l.title.bottom-l.title.top;++row){
  for(int column=0;column<shown;++column){
   const auto* src=raster.pixels.data()+(size_t(l.body.height+row)*l.texture.width+scroll+column)*4;
   auto* dst=frame.data()+(size_t(l.title.top+row)*l.body.width+l.title.left+column)*4;
   for(int c=0;c<3;++c)dst[c]=static_cast<unsigned char>((src[c]*src[3]+dst[c]*(255-src[3]))/255);
  }
 }
 save_bitmap(state_dir/name,l.body.width,l.body.height,frame.data());
}
static void shot(const char* name){paint_panel();assert(panel_w==1120&&panel_h==720);save_bitmap(state_dir/(std::string(name)+".bmp"),raster_w,raster_h,pixels);}
int main(int argc,char** argv){
 assert(argc==2);state_dir=argv[1];std::filesystem::create_directories(state_dir);native_supported=presence_supported=true;
 const auto game=std::filesystem::path("C:/Program Files (x86)/Steam/steamapps/common/SkillshotCity");
 assert(local_font.load(game/"data/translations/black128_60.glf"));assert(heavy_font.load(game/"data/translations/thick128_60.glf"));
 using namespace ssc_achievements;
 ready=true;status=L"Tracking achievements";engine.ledger.items[index("level_17")]={17,1000};engine.ledger.items[index("level_18")].value=16;
 show_manager(40);shot("all");assert(controls.size()>10);assert(quick_module_count==15);
 for(const bool show_bot:{false,true}){
  controls.clear();
  paint_manager_navigation(show_bot);
  assert(controls.size()==size_t(show_bot?11:10));
  for(size_t n=0;n<controls.size();++n){
   const auto& row=controls[n];
   assert(row.y+row.h<manager_footer_top);
   if(n)assert(controls[n-1].y+controls[n-1].h<row.y);
  }
  assert(controls.back().id==(show_bot?510:520));
 }
 paint_panel();
 opened=true;visibility=1.f;
 const LPARAM sort_button=MAKELPARAM(int(panel_x+930*draw_scale),int(panel_y+220*draw_scale));
 auto click_sort=[&](UINT down){
  assert(window_proc(nullptr,down,MK_LBUTTON,sort_button)==0);
  assert(window_proc(nullptr,WM_LBUTTONUP,0,sort_button)==0);
  paint_panel();
 };
 assert(achievement_rows().front()==index("melee_match_10"));
 click_sort(WM_LBUTTONDOWN);
 assert(achievement_sort==AchievementSort::fewest_unlocks&&achievement_rows().front()==index("melee_match_10"));
 click_sort(WM_LBUTTONDBLCLK);
 assert(achievement_sort==AchievementSort::most_unlocks&&achievement_rows().front()==index("melee_match_10"));
 click_sort(WM_LBUTTONDOWN);
 assert(achievement_sort==AchievementSort::rarity_high&&catalog[achievement_rows().front()].rarity==Rarity::mythic);
 click_sort(WM_LBUTTONDBLCLK);
 assert(achievement_sort==AchievementSort::rarity_low&&catalog[achievement_rows().front()].rarity==Rarity::common);
 click_sort(WM_LBUTTONDOWN);
 assert(achievement_sort==AchievementSort::alphabetical&&achievement_rows().front()==index("melee_match_10"));
 opened=false;
 auto before=engine.ledger.unlocked();activate(1983);assert(engine.has_notice()&&engine.ledger.unlocked()==before);
 const NoticeSize logical{notice_width,notice_height};
 const auto preview=raster_notice(0,true,logical);
 check_notice(preview,logical);
 const auto& long_title=preview.layout;
 const int overflow=long_title.title_width-int(long_title.title.right-long_title.title.left);
 assert(overflow>0&&title_offset(long_title,0)==0&&title_offset(long_title,1000)==0);
 assert(title_offset(long_title,1001)>0&&title_offset(long_title,1100)>title_offset(long_title,1001));
 assert(title_offset(long_title,5000)==float(overflow)&&title_offset(long_title,5750)==float(overflow));
 save_notice_view("toast-preview-before-scroll.bmp",preview,1000);
 save_notice_view("toast-preview-after-scroll.bmp",preview,5000);
 const auto toast=raster_notice(index("chaos_sweep"),false);
 check_notice(toast,logical);
 const auto color=rarity_color(Rarity::mythic);
 assert(toast.pixels[0]==GetBValue(color)&&toast.pixels[1]==GetGValue(color)&&toast.pixels[2]==GetRValue(color));
 assert(preview.pixels[0]==255&&preview.pixels[1]==208&&preview.pixels[2]==60);
 assert(title_offset(toast.layout,5700)==0);
 save_notice_view("toast-unlocked.bmp",toast,5700);
 const auto scaled=notice_size(2560,1440);
 assert(scaled.width==427&&scaled.height==91);
 const auto high_res=raster_notice(index("draft_round"),false,scaled);
 check_notice(high_res,scaled);
 save_notice_view("toast-1440p.bmp",high_res,1000);
 const auto small=notice_size(264,720);
 assert(small.width==240&&small.height==51);
 check_notice(raster_notice(0,false,small),small);
 const auto high=notice_size(3840,2160);
 assert(high.width==480&&high.height==102);
 check_notice(raster_notice(0,false,high),high);
 const auto pose=[&](uint64_t age){return notice_top(scaled.height,age,26,1440);};
 assert(pose(0)==-scaled.height&&pose(250)==26&&pose(notice_lifetime_ms-250)==26);
 assert(pose(100)>pose(0)&&pose(100)<pose(250));
 assert(pose(notice_lifetime_ms-100)<pose(notice_lifetime_ms-250)&&pose(notice_lifetime_ms-100)>-scaled.height);
 assert(pose(notice_lifetime_ms)==-scaled.height&&pose(notice_lifetime_ms+100)==-scaled.height);
 assert(notice_top(scaled.height,0,1300,1440)==1440&&notice_top(scaled.height,250,1300,1440)==1300);
 assert(notice_top(scaled.height,100,1300,1440)>1300&&notice_top(scaled.height,notice_lifetime_ms,1300,1440)==1440);
 activate(1988);assert(ssc_hud::editing&&ssc_hud::selected==ssc_hud::achievement_popup);
 auto& popup=ssc_hud::items[ssc_hud::achievement_popup];
 constexpr int editor_width=2560,editor_height=1440;
 ssc_hud::finish_frame(editor_width,editor_height,GetTickCount64());
 const auto default_size=notice_size(editor_width,editor_height,ssc_hud::popup_default_scale);
 assert(default_size.width==321&&default_size.height==68);
 assert(popup.x==ssc_hud::popup_default_x&&popup.y==ssc_hud::popup_default_y&&popup.scale==ssc_hud::popup_default_scale);
 // An older saved box must not override Reset Item's new defaults.
 std::ofstream(state_dir/"menu.ini")<<"hud_box_achievement_popup=.008,.021,.166895,.0628092\n"
                                     <<"hud_achievement_popup=.31,.36,1\n";
 load_settings();
 assert(popup.x==.31f&&popup.y==.36f&&popup.scale==1.f);
 assert(popup.home_x==ssc_hud::popup_default_x&&popup.home_y==ssc_hud::popup_default_y);
 update_popup_bounds(editor_width,editor_height);
 const LPARAM popup_center=MAKELPARAM(int(popup.x*editor_width+12),int(popup.y*editor_height+12));
 assert(window_proc(nullptr,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),popup_center)==0);
 assert(std::abs(popup.scale-1.1f)<.001f);
 update_popup_bounds(editor_width,editor_height);
 assert(std::abs(popup.scale-1.1f)<.001f);
 auto current_size=notice_size(editor_width,editor_height,popup.scale);
 assert(current_size.width>scaled.width&&current_size.height>scaled.height);
 assert(std::abs(popup.w*popup.scale*editor_width-current_size.width)<.01f);
 assert(std::abs(popup.h*popup.scale*editor_height-current_size.height)<.01f);
 const int mouse_x=int((popup.x+popup.w*popup.scale)*editor_width)-3;
 const int mouse_y=int((popup.y+popup.h*popup.scale)*editor_height)-3;
 const auto start=MAKELPARAM(mouse_x,mouse_y);
 const auto end=MAKELPARAM(mouse_x+int(popup.w*editor_width*.5f),mouse_y+int(popup.h*editor_height*.5f));
 assert(window_proc(nullptr,WM_LBUTTONDOWN,MK_LBUTTON,start)==0&&hud_resize);
 assert(window_proc(nullptr,WM_MOUSEMOVE,MK_LBUTTON,end)==0);
 assert(window_proc(nullptr,WM_LBUTTONUP,0,end)==0);
 assert(std::abs(popup.scale-1.6f)<.02f&&hud_drag==-1);
 const auto dragged_scale=popup.scale;
 popup.scale=1;
 load_settings();
 assert(std::abs(popup.scale-dragged_scale)<.001f);
 live_hud_wheel(-20*WHEEL_DELTA,ssc_hud::achievement_popup);
 assert(popup.scale==.5f&&notice_size(editor_width,editor_height,popup.scale).width<scaled.width);
 live_hud_wheel(30*WHEEL_DELTA,ssc_hud::achievement_popup);
 assert(popup.scale==ssc_hud::popup_max_scale);
 const auto largest=notice_size(3840,2160,popup.scale);
 assert(largest.width==960&&largest.height==204);
 check_notice(raster_notice(0,true,largest),largest);
 update_popup_bounds(264,120);
 assert(popup.scale==ssc_hud::popup_max_scale);
 assert((popup.x+popup.w*popup.scale)*264<=264.01f&&(popup.y+popup.h*popup.scale)*120<=120.01f);
 update_popup_bounds(editor_width,editor_height);
 activate(148);
 assert(popup.scale==ssc_hud::popup_default_scale);
 assert(std::abs(popup.x-ssc_hud::popup_default_x)<1.f/editor_width);
 assert(std::abs(popup.y-ssc_hud::popup_default_y)<1.f/editor_height);
 assert(notice_top(default_size.height,0,popup.y*editor_height,editor_height)==editor_height);
 ssc_hud::reset();
 assert(popup.x==ssc_hud::popup_default_x&&popup.y==ssc_hud::popup_default_y&&popup.scale==ssc_hud::popup_default_scale);
 close_menu(true);show_manager(40);
 activate(1985);assert(achievement_filter==1);auto rows=achievement_rows();assert(rows.size()==20);for(auto i:rows)assert(!engine.ledger.items[i].unlocked_at);shot("locked");
 activate(1987);assert(achievement_filter==2&&achievement_rows().size()==1);shot("unlocked");
 activate(1984);activate(1990);assert(settings_page==41);shot("detail");
 assert(parent_page(41)==40&&parent_page(40)==-1&&parent_page(39)==38&&parent_page(31)==4);
 activate(1981);assert(settings_page==40);activate(1982);assert(!engine.ledger.notifications);activate(1982);assert(engine.ledger.notifications);
 engine.capability(Kind::jumps,true);
 engine.ledger.items[index("jumps_100")]={100,1000};
 engine.ledger.items[index("jumps_300")]={300,1001};
 engine.ledger.items[index("jumps_1000")].value=456;
 Json rates={{"schema",1},{"catalog",catalog_wire_version},{"population","synced_accounts"},{"participants",8u},
   {"achievements",{{"jumps_300",{{"unlocked",1u}}},{"level_17",{{"unlocked",6u}}}}}};
 assert(completion_rates.replace(rates));
 achievement_filter=0;achievement_sort=AchievementSort::alphabetical;
 show_manager(40);opened=true;visibility=1.f;paint_panel();
 click_sort(WM_LBUTTONDOWN);
 assert(achievement_rows().front()==index("jumps_100"));
 assert(achievement_rate(index("jumps_300"))==L"12.50% of Mod Menu users have unlocked");
 assert(achievement_detail_rate(index("jumps_300"))==L"12.50% of Mod Menu users have unlocked (1/8)");
 const auto rate_revision=completion_rates.revision;
 auto doubled=rates;doubled["participants"]=16u;doubled["achievements"]["jumps_300"]["unlocked"]=2u;
 doubled["achievements"]["level_17"]["unlocked"]=12u;
 assert(completion_rates.replace(doubled)&&completion_rates.revision==rate_revision+1);
 assert(achievement_detail_rate(index("jumps_300"))==L"12.50% of Mod Menu users have unlocked (2/16)");
 doubled["achievements"]["jumps_300"]["unlocked"]=17u;
 assert(!completion_rates.replace(doubled)&&completion_rates.unlocked(index("jumps_300"))==2);
 const auto previous_save=engine.ledger.json();
 Ledger restored_for_sync;assert(Ledger::parse(previous_save,restored_for_sync));
 const auto upload=community::payload(restored_for_sync);
 assert(upload["achievements"]["level_17"]["unlocked_at"]==1000);
 assert(upload.size()==3&&upload["catalog"]==catalog_wire_version&&upload["achievements"]["jumps_1000"]["progress"]==456);
 activate(1998);assert(settings_page==43&&parent_page(43)==40);shot("community-sync");
 {
  const auto preference_state=state_dir/"sync-preference";
  std::filesystem::create_directories(preference_state);
  constexpr uint64_t account=76561198000000001ULL;
  const auto run=[&](community::Action action){
   community::Request request;
   request.action=action;
   return community::perform(preference_state,request,account);
  };
  const auto defaults=run(community::Action::load);
  assert(defaults.enabled&&defaults.token.empty());
  assert(run(community::Action::pause).persisted);
  assert(!run(community::Action::load).enabled);
  assert(run(community::Action::enable).persisted);
  assert(run(community::Action::load).enabled);

  community::Model sharing;
  CompletionRates stats;
  community::Reply reply;
  reply.action=community::Action::load;
  sharing.accept(reply,100,stats);
  assert(sharing.loaded&&!sharing.enabled&&sharing.token.empty());
  reply.action=community::Action::enable;
  reply.enabled=true;
  sharing.accept(reply,101,stats);
  assert(sharing.enabled&&sharing.token.empty()&&sharing.next==101);
  reply.action=community::Action::publish;
  reply.status=503;
  sharing.accept(reply,102,stats);
  assert(sharing.enabled&&sharing.next==30102);
  reply.action=community::Action::pause;
  reply.enabled=false;
  sharing.accept(reply,103,stats);
  assert(!sharing.enabled&&sharing.loaded);
  sharing.reset();
  assert(!sharing.enabled&&!sharing.loaded);
  reply.action=community::Action::load;
  reply.enabled=true;
  reply.token=std::string(64,'a');
  sharing.accept(reply,104,stats);
  assert(sharing.enabled&&!sharing.token.empty());
 }
 show_manager(40);paint_panel();
 shot("tiers-and-rates");
 achievement_selected=index("jumps_100");show_manager(41);shot("detail-with-counts");
 show_manager(40);paint_panel();
 click_sort(WM_LBUTTONDBLCLK);
 assert(achievement_sort==AchievementSort::most_unlocks&&achievement_rows().front()==index("level_17"));
 shot("most-unlocks");
 auto rounded=rates;
 rounded["participants"]=1000000u;
 rounded["achievements"]["jumps_300"]["unlocked"]=12u;
 rounded["achievements"]["level_17"]["unlocked"]=13u;
 assert(completion_rates.replace(rounded));
 assert(completion_rates.percent(index("jumps_300"))==completion_rates.percent(index("level_17")));
 assert(achievement_rows().front()==index("level_17"));
 achievement_sort=AchievementSort::fewest_unlocks;
 assert(achievement_rows().front()==index("jumps_100"));
 click_sort(WM_LBUTTONDOWN);
 click_sort(WM_LBUTTONDOWN);
 assert(achievement_sort==AchievementSort::rarity_high&&catalog[achievement_rows().front()].rarity==Rarity::mythic);
 opened=false;
 assert(std::none_of(rows.begin(),rows.end(),[](auto i){return catalog[i].tier>1;}));
 popup.x=.61f;popup.y=.82f;
 save();popup.x=0;popup.y=0;load_settings();
 assert(std::abs(popup.x-.61f)<.001f&&std::abs(popup.y-.82f)<.001f);
 achievement_wheel_remainder=0;
 assert(achievement_wheel_step(WHEEL_DELTA)==0&&achievement_wheel_step(WHEEL_DELTA)==1);
 assert(achievement_wheel_step(-WHEEL_DELTA/2)==0&&achievement_wheel_step(-WHEEL_DELTA*3/2)==-1);
 assert(achievement_wheel_step(WHEEL_DELTA)==0&&achievement_wheel_step(-WHEEL_DELTA)==0);
 audio::load(state_dir);
 const auto original_unlock=game/"data"/"sounds"/"itemUnlockedCampaign.wav";
 audio::prepare_default(game/"data"/"sounds"/audio::default_name,state_dir);
 assert(!audio::custom&&audio::default_sound==state_dir/"achievement-campaign-default.wav");
 const auto original_samples=ssc_audio::decode(ssc_audio::read(original_unlock));
 const auto quiet_samples=ssc_audio::decode(ssc_audio::read(audio::default_sound));
 assert(quiet_samples.size()==original_samples.size());
 assert(std::abs(ssc_audio::rms(quiet_samples)/ssc_audio::rms(original_samples)-0.65)<0.001);
 assert(std::all_of(quiet_samples.begin(),quiet_samples.end(),[](double x){return std::abs(x)<0.128;}));
 activate(1994);assert(settings_page==42);shot("unlock-sound");
 assert(audio::import_file(game/"data"/"sounds"/"flaskRareSkillUnlocked.wav")&&audio::custom);
 auto sound_before=ssc_audio::read(audio::custom_sound).raw;
 assert(!audio::import_file(state_dir/"missing.wav")&&audio::custom);
 audio::load(state_dir);assert(audio::custom&&ssc_audio::read(audio::custom_sound).raw==sound_before);
 activate(1997);audio::load(state_dir);assert(!audio::custom);
 show_manager(-1);shot("module-list");assert(std::any_of(controls.begin(),controls.end(),[](const auto& c){return c.id==1981;}));
 show_manager(17);shot("quick-menu-options");assert(option_limits[17]==8);
 std::cout<<"PASS alphabetical sort, slower wheel, exact unlock counts, sound import and reset, repeated sort clicks with and without server rates, achievement navigation, filters, 15 module bindings, preview isolation, popup resize and persistence, default placement, 2x raster and marquee timing, notifications, unlocked Bot sidebar bounds and menu bitmap fixtures\n";
}
