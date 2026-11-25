#pragma once
#include "kos_storage.h"
#include "planned_native_guards.h"
#include "presence_source.h"
#include "anonymous_lobby.h"
#include <filesystem>
#include <fstream>
namespace ssc_kos {
inline bool enabled=false;
inline unsigned color_rgb=0xff4545;
inline List list;
inline std::filesystem::path directory;
inline bool saved=true;
struct NativeTarget {Target target;uintptr_t actor=0,world=0;bool historical=false;};
inline std::vector<NativeTarget> recent_public;
inline NativeTarget selected;
inline NativeTarget player(uintptr_t actor){
 NativeTarget out;if(!ssc_planned_native::kos)return out;
 uintptr_t world=0,first=0,last=0;int kind=-1,slot=-1;unsigned char active=0,anon=2;
 auto base=ssc_names::image_base;
 if(!ssc_names::read(base+ssc_compat::resolve(0xdef5d0),world)||!world||!ssc_names::read(world+0xa658,first)||!ssc_names::read(world+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048||actor<first||actor>=last||(actor-first)%0x34b8)return out;
 if(!ssc_names::read(actor+0x78,slot)||slot<0||uintptr_t(slot)!=(actor-first)/0x34b8||!ssc_names::read(actor+0x81,active)||!active||!ssc_names::read(actor+0x7dc,kind)||(kind!=0&&kind!=1)||!ssc_names::read(actor+0x209,anon)||anon>1)return out;
 // Tournament titles can force anonymity independently of the actor setting.
 if(ssc_privacy::tournament_validated.load()&&ssc_privacy::community_cup_anonymity(world,[](uintptr_t p,auto& v){return ssc_names::read(p,v);})==ssc_privacy::Detection::on)return out;
 // Reject anonymous actors BEFORE reading their account or display string.
 out.target.player.visibility=anon?Visibility::anonymous:Visibility::public_player;if(anon)return out;
 if(kind==1){int start=-1,offset=-1,count=-1,total=int((last-first)/0x34b8);uintptr_t owner=1;if(!ssc_names::bot_layout||!ssc_names::read(base+0xfb990c,start)||!ssc_names::read(world+0x190,offset)||!ssc_names::read(world+0x28c,count)||!ssc_names::bot_range(slot,kind,start,offset,count,total)||!ssc_names::read(actor+0x2e8,owner)||owner)return out;}
 if(kind==0){auto identity=ssc_names::identify(actor);out.target.player.local=identity.local;out.target.player.account=identity.account;}
 out.target.player.handle=ssc_rpc::native_string(actor+0x798);
 if(kind==1&&valid_text(out.target.player.handle,64))out.target.player.account="handle:"+canonical(out.target.player.handle);
 out.actor=actor;out.world=world;out.target.generation=world;return out;
}
inline std::vector<NativeTarget> roster(){
 std::vector<NativeTarget> out;uintptr_t world=0,first=0,last=0;
 if(!ssc_planned_native::kos||!ssc_names::read(ssc_names::image_base+ssc_compat::resolve(0xdef5d0),world)||!world||!ssc_names::read(world+0xa658,first)||!ssc_names::read(world+0xa660,last)||last<first||(last-first)%0x34b8||(last-first)/0x34b8>2048)return out;
 for(auto a=first;a<last;a+=0x34b8){auto p=player(a);if(eligible(p.target.player))out.push_back(std::move(p));}return out;
}
inline bool save(){if(directory.empty())return false;auto path=directory/L"kos.json",temp=directory/L"kos.json.tmp";std::ofstream f(temp,std::ios::binary);f<<encode(list);f.close();saved=bool(f)&&MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);return saved;}
inline void load(const std::filesystem::path& dir){directory=dir;std::error_code e;auto file=dir/L"kos.json";if(!std::filesystem::exists(file,e))return;if(std::filesystem::file_size(file,e)>65536||e){saved=false;return;}std::ifstream f(file,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(f)),{});saved=decode(bytes,list);}
inline void remember(const std::vector<NativeTarget>& public_players){
 for(const auto& p:public_players){if(!eligible(p.target.player))continue;auto key=canonical(p.target.player.account);recent_public.erase(std::remove_if(recent_public.begin(),recent_public.end(),[&](const NativeTarget& e){return canonical(e.target.player.account)==key;}),recent_public.end());auto copy=p;copy.historical=true;recent_public.insert(recent_public.begin(),std::move(copy));if(recent_public.size()>64)recent_public.pop_back();}
}
inline void tick_recent(uint64_t now){static uint64_t at=0;if(!enabled){recent_public.clear();return;}if(at&&now>=at&&now-at<1000)return;at=now;remember(roster());}
inline bool resolve_choice(const NativeTarget&,NativeTarget&);
inline std::vector<NativeTarget> choices(){auto out=roster();for(const auto& p:recent_public){NativeTarget valid;if(resolve_choice(p,valid)&&std::none_of(out.begin(),out.end(),[&](const NativeTarget& current){return canonical(current.target.player.account)==canonical(p.target.player.account);}))out.push_back(p);}return out;}
inline bool resolve_choice(const NativeTarget& choice,NativeTarget& current){
 if(!choice.historical){current=player(choice.actor);return current.world==choice.world&&target_still_public(choice.target,current.target);}
 if(!eligible(choice.target.player)||std::none_of(recent_public.begin(),recent_public.end(),[&](const NativeTarget& e){return e.world==choice.world&&e.actor==choice.actor&&target_still_public(choice.target,e.target);}))return false;
 uintptr_t world=0;if(!ssc_names::read(ssc_names::image_base+ssc_compat::resolve(0xdef5d0),world))return false;
 if(world==choice.world){current=player(choice.actor);return current.world==choice.world&&target_still_public(choice.target,current.target);}
 // Past public identity only: do not dereference stale actors or associate it
 // with any currently anonymous player. tint() still checks live anonymity.
 current=choice;return true;
}
inline bool commit(){NativeTarget current;if(!resolve_choice(selected,current))return false;bool ok=list.marked(current.target.player)?list.remove(current.target.player.account):list.add(current.target.player);return ok&&save();}
inline std::string manage_handle(std::string handle,const std::string& mode="toggle"){
 auto begin=handle.find_first_not_of(" \t");if(begin==std::string::npos)return "Enter a player handle";handle=handle.substr(begin,handle.find_last_not_of(" \t")-begin+1);if(!valid_text(handle,64))return "Use a handle of 1-64 plain characters";
 const bool exists=list.has_handle(handle);if(mode=="remove"||(mode=="toggle"&&exists)){if(!list.remove_handle(handle))return "Handle is not on your KOS list";return save()?"Removed from KOS":"Removed; could not save KOS list";}
 if(mode!="add"&&mode!="toggle")return "Use kos add HANDLE or kos remove HANDLE";
 auto public_players=choices();auto key=canonical(handle);auto found=std::find_if(public_players.begin(),public_players.end(),[&](const NativeTarget& p){return canonical(p.target.player.handle)==key;});bool ok=false;
 if(found!=public_players.end()){NativeTarget current;if(!resolve_choice(*found,current))return "Player is no longer public";ok=list.add(current.target.player);}else ok=list.add_handle(handle);
 if(!ok){return "KOS list is full or handle is invalid";}
 return save()?"Added to KOS":"Added; could not save KOS list";
}
inline bool tint(uintptr_t actor,unsigned& rgb){if(!enabled||list.entries().empty())return false;auto p=player(actor);if(!list.marked(p.target.player))return false;rgb=color_rgb;return true;}
struct Row {uintptr_t actor=0;float x=0,y=0,w=0,h=0;uint64_t at=0;};
inline std::vector<Row> rows;
inline int client_width=0,client_height=0;
inline bool (*tab_state)()=+[](){return (GetAsyncKeyState(VK_TAB)&0x8000)!=0;};
inline bool tab_held(){return tab_state();}
inline bool recent(uint64_t now,uint64_t then){return now>=then&&now-then<200;}
inline bool pointer_active(uint64_t){return false;}
inline bool screen_row(float x,float y,float size,float width,int logical_w,int logical_h,int client_w,int client_h,Row& row){
 if(logical_w<=0||logical_h<=0||client_w<=0||client_h<=0||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(size)||!std::isfinite(width)||size<=0||width<=0)return false;
 const float sx=float(client_w)/logical_w,sy=float(client_h)/logical_h;row.x=(x-3)*sx;row.y=(y-size*.25f)*sy;row.w=(width+6)*sx;row.h=size*1.5f*sy;return std::isfinite(row.x)&&std::isfinite(row.y)&&std::isfinite(row.w)&&std::isfinite(row.h)&&row.w>0&&row.h>0;
}
inline void score_row(uintptr_t,float,float,float,float){}
inline bool contains(const Row& row,float x,float y){return std::isfinite(x)&&std::isfinite(y)&&x>=row.x&&x<=row.x+row.w&&y>=row.y&&y<=row.y+row.h;}
inline bool select_at(float,float){return false;}
}
