#include "../runtime/quick_chat.h"
#include <cassert>
#include <iostream>
static std::vector<std::string> written;
static void* fake_assign(uintptr_t,const void* ptr,size_t start,size_t count){assert(start==0&&count==size_t(-1));auto& s=*static_cast<const ssc_chat::NativeString*>(ptr);written.emplace_back(reinterpret_cast<const char*>(s.pointer),s.size);return nullptr;}
template<class T> void put(unsigned char* p,size_t at,T value){std::memcpy(p+at,&value,sizeof(value));}
static float seen_width=0;
static uintptr_t label_capture(uintptr_t a,float x,float y,float width,float height,uintptr_t text,float scale,float r,float g,float b,float alpha,int align,int vertical,uintptr_t start,uintptr_t clip,uintptr_t limit,float p17,float p18,uintptr_t p19){assert(a==1&&x==2&&y==3&&height==5&&text==6&&scale==7&&r==8&&g==9&&b==10&&alpha==11&&align==12&&vertical==13&&start==14&&clip==15&&limit==16&&p17==17&&p18==18&&p19==19);seen_width=width;return 12345;}
int main(int argc,char** argv){using namespace ssc_chat;assert(argc==2);auto dir=std::filesystem::path(argv[1]);std::filesystem::create_directories(dir);
 original_label=label_capture;enabled=false;assert(draw_label(1,2,3,99999,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19)==12345&&seen_width==99999);enabled=true;assert(draw_label(1,2,3,99999,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19)==12345&&seen_width==220);enabled=false;
 // Match native eight-slot geometry and phase filtering, including happy face at the right.
 assert((visible_order(false)==std::array<int,8>{0,1,2,3,4,5,6,7}));
 assert((visible_order(true)==std::array<int,8>{0,1,2,3,4,5,6,8}));
 assert(preview_angle(4)==0);assert(std::cos(preview_angle(0))==-1);
 selected=6;move_visible(1,true);assert(selected==8&&order[8]==6&&order[7]==7);reset();selected=0;
 assert(valid_text("GG!")&&!valid_text("Hi\nthere")&&!valid_text(std::string(101,'a'))&&!valid_text("XL_AUTOCHAT_GOODGAME"));
 std::array<unsigned char,0x2680> actor{};std::array<unsigned char,0x48*3> entries{};
 for(int i=0;i<3;++i){auto p=entries.data()+i*0x48;std::string value="stock"+std::to_string(i);std::memcpy(p,value.data(),value.size());put(p,0x10,size_t(value.size()));put(p,0x18,size_t(15));put(p,0x24,i);put(p,0x28,int(0x200+i));put(p,0x38,float(i*120));}
 put(actor.data(),0x2670,reinterpret_cast<uintptr_t>(entries.data()));put(actor.data(),0x2678,reinterpret_cast<uintptr_t>(entries.data()+entries.size()));assign=fake_assign;
 assert(!apply(reinterpret_cast<uintptr_t>(actor.data()))&&written.empty());enabled=true;slots[1]={"Custom text longer than fifteen",8};selected=1;move(-1);assert(order[0]==1&&selected==0);
 assert(apply(reinterpret_cast<uintptr_t>(actor.data())));assert((written==std::vector<std::string>{slots[1].text,"stock0","stock2"}));int icon;std::memcpy(&icon,entries.data()+0x28,4);assert(icon==0x208);float angle;std::memcpy(&angle,entries.data()+0x38,4);assert(angle==0); // Geometry remains in slot positions.
 auto_text="Thanks for playing!";auto_timing=2;assert(save(dir));reset();enabled=false;auto_text="GG";load(dir);assert(enabled&&order[0]==1&&slots[1].icon==8&&auto_text=="Thanks for playing!"&&auto_timing==2);
 {std::ofstream out(dir/"quick-chat.json");out<<"{\"schema\":1,\"order\":[0,0,0,0,0,0,0,0,0]}";}load(dir);assert(order[0]==1);auto_text="I fired {shots_fired} bullets";assert(preview().find(L"not available")!=std::wstring::npos);

 // Preserve native greeting tokens (and thus the game's randomized variants), even after reordering/icon changes.
 reset();std::string greeting="XL1000_AUTOCHAT_HI01";auto first=entries.data();put(first,0,reinterpret_cast<uintptr_t>(greeting.data()));put(first,0x10,greeting.size());put(first,0x18,greeting.size());put(first,0x24,6);slots[6].icon=2;selected=6;move(-1);
 written.clear();assert(apply(reinterpret_cast<uintptr_t>(actor.data())));assert(written.back()==greeting);
 slots[6].text="My exact greeting";written.clear();assert(apply(reinterpret_cast<uintptr_t>(actor.data())));assert(written.back()=="My exact greeting");
 written.clear();put(actor.data(),0x2678,reinterpret_cast<uintptr_t>(entries.data()+1));assert(!apply(reinterpret_cast<uintptr_t>(actor.data()))&&written.empty());
 // Reproduce real startup: the two wheel hooks share one execute/read page.
 auto test_image=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));assert(test_image);
 const uint32_t calls[]={0x80a32f,0x3c5145,0x3c5873,0x3c59c4},targets[]={0x805680,0x805680,0x482b60,0x47c900};
 for(int i=0;i<4;++i){test_image[calls[i]]=0xe8;int32_t rel=int32_t(targets[i]-calls[i]-5);std::memcpy(test_image+calls[i]+1,&rel,4);}
 DWORD protection=0;assert(VirtualProtect(test_image,0x900000,PAGE_EXECUTE_READ,&protection));assert(attach(reinterpret_cast<uintptr_t>(test_image)));
 uintptr_t bridge_address=0;
 for(int i=0;i<4;++i){MEMORY_BASIC_INFORMATION memory{};assert(VirtualQuery(test_image+calls[i],&memory,sizeof(memory)));assert(memory.Protect==PAGE_EXECUTE_READ);int32_t rel=0;std::memcpy(&rel,test_image+calls[i]+1,4);auto destination=reinterpret_cast<uintptr_t>(test_image)+calls[i]+5+rel;if(!i)bridge_address=destination;assert(destination==bridge_address+(i>=2?(i-1)*32:0));}
 assert(VirtualFree(reinterpret_cast<void*>(bridge_address),0,MEM_RELEASE));assert(VirtualFree(test_image,0,MEM_RELEASE));
 std::cout<<"PASS: wheel content/order/icon mapping, unchanged geometry, subset stock entries, disabled passthrough, native-string bridge layout, settings validation and local-only automatic message preview\n";
}
