#include "../runtime/overlay.cpp"
#include <cassert>
void picture(const std::filesystem::path& path){
 BITMAPFILEHEADER file{};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+raster_w*raster_h*4;
 BITMAPINFOHEADER info{};info.biSize=sizeof(info);info.biWidth=raster_w;info.biHeight=-raster_h;info.biPlanes=1;info.biBitCount=32;
 std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&file),sizeof(file));out.write(reinterpret_cast<char*>(&info),sizeof(info));out.write(static_cast<char*>(pixels),raster_w*raster_h*4);assert(out);
}
int main(int argc,char** argv){
 assert(argc==2||argc==3);auto folder=std::filesystem::path(argv[1]);std::filesystem::create_directories(folder);state_dir=folder;
 native_supported=presence_supported=true;opened=manager=true;animations=false;visibility=modal_visibility=1;settings_page=20;
 auto report=std::make_shared<ssc_stats::Report>();ssc_stats::Round round;round.number=2;round.player_class="Caster";round.complete=round.ended=true;round.end_level=9;round.started_utc_ms=1791228600000;round.run_id=std::string(32,'a');
 for(int t=60000;t<=240000;t+=1000){ssc_stats::Point p;p.time=t-60000;p.round_time=t;p.countdown=180000-t;p.values={double(1+t/30000),double(t/100),double((t/2000)%70*100)};round.points.push_back(p);}report->rounds.push_back(round);round.number=3;round.points.resize(121);report->rounds.push_back(round);
 std::atomic_store(&ssc_stats::reader().result,std::shared_ptr<const ssc_stats::Report>(report));
 for(int scale:{75,100,150})for(int page:{20,21}){
  ui_scale=scale;settings_page=page;layout_panel(1920,1080);paint_panel();
  for(const auto& c:controls)if(c.id>=600&&c.id<=611)assert(c.x>=0&&c.y>=0&&c.x+c.w<=panel_w&&c.y+c.h<=panel_h-50);
  assert(panel_w==(page==21?1440:1120)&&panel_h==(page==21?900:720));
 }
 ui_scale=100;settings_page=20;layout_panel(1920,1080);paint_panel();picture(folder/"summary.bmp");
 activate(602);assert(dropdown==602&&statistics_class==0);paint_panel();picture(folder/"classes.bmp");select_dropdown(4);assert(dropdown==-1&&statistics_class==4);
 activate(608);assert(settings_page==21);layout_panel(1920,1080);activate(607);statistics_mouse_x=700;statistics_mouse_y=450;paint_panel();picture(folder/"graph.bmp");
 assert(statistics_whole(16247)==L"16247"&&statistics_clock(65000)==L"1:05"&&statistics_clock(-65000)==L"-1:05");
 activate(611);select_dropdown(2);assert(statistics_intervals==8);paint_panel();
 activate(610);assert(dropdown==610&&dropdown_items.size()==1);select_dropdown(0);
 activate(609);assert(settings_page==20);paint_panel();
 statistics_round=5;activate(601);assert(statistics_round==0&&statistics_select_latest);
 auto deadline=GetTickCount64()+5000;while(ssc_stats::reader().busy&&GetTickCount64()<deadline)Sleep(10);assert(!ssc_stats::reader().busy);
 refresh_dynamic_panel(GetTickCount64());assert(statistics_round==0&&!statistics_select_latest);
 statistics_round=3;activate(600);assert(statistics_round==0&&statistics_select_latest);
 deadline=GetTickCount64()+5000;while(ssc_stats::reader().busy&&GetTickCount64()<deadline)Sleep(10);assert(!ssc_stats::reader().busy);
 refresh_dynamic_panel(GetTickCount64());assert(!statistics_select_latest);
 activate(270);assert(recording_requested&&ssc_record::state().enabled);ssc_record::stop();recording_requested=false;load_settings();assert(recording_requested&&ssc_record::state().enabled);
 // A write failure stops the worker without changing the user's saved choice.
 ssc_record::state().enabled=false;ssc_record::state().status=2;save();recording_requested=false;load_settings();assert(recording_requested&&ssc_record::state().enabled);
 activate(270);assert(!recording_requested&&!ssc_record::state().enabled);load_settings();assert(!recording_requested&&!ssc_record::state().enabled);
 // Shared cosmetics remains discoverable independently of private/beta access.
 assert(!shared_receive&&!shared_publish);activate(560);assert(settings_page==22);paint_panel();picture(folder/"shared-cosmetics.bmp");
 activate(561);activate(562);assert(shared_receive&&shared_publish);shared_receive=shared_publish=false;load_settings();assert(shared_receive&&shared_publish);
 activate(561);activate(562);activate(565);assert(settings_page==4&&!shared_receive&&!shared_publish);
 private_label="Stream colors";save();private_label="Bot highlight";load_settings();assert(private_label=="Stream colors");
 assert(!valid_private_label("\nInjected")&&!valid_private_label("a=b")&&!valid_private_label(std::string(19,'a'))&&!valid_private_label("   "));
 assert(!ssc_names::private_visible());activate(566);assert(!private_label_editing);private_label_buffer="Unauthorized";activate(567);assert(private_label=="Stream colors");
 settings_page=13;layout_panel(1920,1080);activate(570);assert(credits_popup);
 for(int page=0;page<3;++page){credits_page=page;paint_panel();picture(folder/("credits-"+std::to_string(page)+".bmp"));for(const auto& c:controls)assert(c.id>=571&&c.id<=573);}
 activate(571);assert(!credits_popup);paint_panel();
 {
  ssc_shared::Style shared;shared.mode=ssc_shared::Mode::gradient;shared.count=3;shared.colors={0xff0000,0xff00,0xff};
  float r=1,g=1,b=1,phase=0,out[3]{};const auto old_color=ssc_names::color_scope,old_identity=ssc_names::identity_scope;
  {ssc_names::ColorScope outer(false);ssc_names::shared_tint(outer,shared,r,g,b,phase);
   {ssc_names::ColorScope nested(false);nested.inherit_remote();ssc_names::palette(0,out,85,0,0,0);assert(out[0]<.001f&&out[1]>.999f&&out[2]<.001f);}
   assert(ssc_names::identity_scope==2);
  }
  assert(ssc_names::color_scope==old_color&&ssc_names::identity_scope==old_identity);
 }
 if(argc==3){auto actual=std::make_shared<const ssc_stats::Report>(ssc_stats::read_folder(argv[2]));std::atomic_store(&ssc_stats::reader().result,actual);statistics_round=statistics_class=statistics_metric=0;settings_page=21;statistics_mouse_x=statistics_mouse_y=-1;layout_panel(1920,1080);paint_panel();picture(folder/"actual.bmp");}
}
