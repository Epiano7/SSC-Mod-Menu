#include "../runtime/overlay.cpp"
#include <cassert>
#include <iostream>
static unsigned char native_false(void*,int,void*){return 0;}
static unsigned char native_true(void*,int,void*){return 1;}
static void small(uintptr_t p,const char* text){const size_t n=std::strlen(text);std::memset(reinterpret_cast<void*>(p),0,32);std::memcpy(reinterpret_cast<void*>(p),text,n);std::memcpy(reinterpret_cast<void*>(p+16),&n,8);size_t cap=15;std::memcpy(reinterpret_cast<void*>(p+24),&cap,8);}
static void snapshot(const std::filesystem::path& path){
 BITMAPFILEHEADER f{};f.bfType=0x4d42;f.bfOffBits=sizeof(f)+sizeof(BITMAPINFOHEADER);f.bfSize=f.bfOffBits+raster_w*raster_h*4;
 BITMAPINFOHEADER h{};h.biSize=sizeof(h);h.biWidth=raster_w;h.biHeight=-raster_h;h.biPlanes=1;h.biBitCount=32;
 std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&f),sizeof(f));out.write(reinterpret_cast<char*>(&h),sizeof(h));out.write(static_cast<char*>(pixels),raster_w*raster_h*4);assert(out);
}
int main(){
 state_dir=std::filesystem::current_path()/"work/update-26025030-build/ui";std::filesystem::create_directories(state_dir);
 native_supported=true;presence_supported=true;show_cosmetics_notice();assert(!manager&&cosmetics_popup);
 auto game=std::filesystem::path("C:/Program Files (x86)/Steam/steamapps/common/SkillshotCity");
 assert(local_font.load(game/"data/translations/black128_60.glf"));assert(heavy_font.load(game/"data/translations/thick128_60.glf"));
 using namespace ssc_cosmetic_access;
 for(int i=0;i<4;++i){current={};current.dlc_known=current.pass_known=i!=0;if(i>=2)current.dlcs={L"Starter Pack",L"Deluxe Edition"};current.active_pass=i==3;paint_panel();assert(panel_w==960&&panel_h==600);for(const auto& c:controls)assert(c.id==1);snapshot(state_dir/("tier-"+std::to_string(i)+".bmp"));}
 account_key="owner";close_menu();assert(cosmetics_notice_account=="owner");cosmetics_notice_account.clear();load_settings();assert(cosmetics_notice_account=="owner");
 // Popup navigation must not steal the Freecam page or other controls.
 show_manager(25);assert(!cosmetics_popup&&manager);paint_panel();assert(panel_w==1120);
 show_cosmetics_notice();dirty=false;refresh_dynamic_panel(10000);assert(dirty);
 close_menu();
 // Actual observer must record original false even if cosmetic predicate returns true.
 std::vector<unsigned char> image(0x1000000);ssc_names::image_base=reinterpret_cast<uintptr_t>(image.data());small(ssc_names::image_base+0xe041e8,"owner");std::array<unsigned char,32> name{};small(reinterpret_cast<uintptr_t>(name.data()),"owner");
 ssc_names::native_predicate=native_false;ssc_names::cosmetics=true;ssc_names::attached=false;
 assert(ssc_names::predicate(nullptr,-1,name.data())==1);assert(!ssc_names::native_pass_value&&ssc_names::native_pass_account=="owner");
 ssc_names::native_predicate=native_true;assert(ssc_names::native_pass_probe(nullptr,-1,name.data())==1&&ssc_names::native_pass_value);
 small(reinterpret_cast<uintptr_t>(name.data()),"other");ssc_names::native_predicate=native_false;ssc_names::native_pass_probe(nullptr,-1,name.data());assert(ssc_names::native_pass_value);
 ssc_names::image_base=0;
 std::cout<<"PASS: four popup states, acknowledgement persistence, original native pass result and other-account isolation\n";
}
