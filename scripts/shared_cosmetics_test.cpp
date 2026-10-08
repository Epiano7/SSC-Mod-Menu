#include "../runtime/shared_cosmetics_core.h"
#include <cassert>
#include <iostream>
using namespace ssc_shared;
const Time start{100000,100000};
std::string response(Json appearance={{"mode","solid"},{"colors",Json::array({0x55ccff})}}){
 return Json{{"ok",true},{"profiles",Json::array({{{"account_key","player_one"},{"style",appearance},{"ttl_seconds",30}}})}}.dump();
}
int main(){
 Cache c;Style s;c.context(123,true);auto p=c.begin({"player_one"},start);
 assert(c.accept(p,response(),{100200,100200}));assert(c.get("player_one",{100300,100300},s)&&s.colors[0]==0x55ccff);
 assert(!c.get("player",{100300,100300},s));assert(!c.get("Player_one",{100300,100300},s));
 assert(!c.accept(p,response(),{100400,100400})); // replay
 for(auto now:{Time{130000,130000},Time{99999,100100},Time{100100,99999},Time{130000,100100},Time{100100,130000}}){
  Cache isolated;isolated.context(123,true);auto request=isolated.begin({"player_one"},start);assert(isolated.accept(request,response(),start));
  assert(!isolated.get("player_one",now,s));assert(!isolated.get("player_one",start,s));
 }
 c.context(124,true);assert(!c.get("player_one",start,s));assert(!c.accept(p,response(),start));
 p=c.begin({"player_one"},start);c.context(0,true);assert(!c.accept(p,response(),start));
 c.context(124,true);p=c.begin({"player_one"},start);c.context(124,false);assert(!c.accept(p,response(),start));
 c.context(124,true);p=c.begin({"player_one"},start);auto newer=c.begin({"player_one"},start);assert(!c.accept(p,response(),start));assert(c.accept(newer,response(),start));
 p=c.begin({"player_one"},start);assert(c.accept(p,"{\"ok\":true,\"profiles\":[]}",start));assert(!c.get("player_one",start,s));
 for(auto bad:{"<html>proxy failure</html>","{\"ok\":true,\"ok\":true,\"profiles\":[]}","{\"ok\":true,\"profiles\":[],\"bot_module\":true}"}){
  p=c.begin({"player_one"},start);assert(!c.accept(p,bad,start));
 }
 for(auto field:{"account_key","ttl_seconds","style"}){
  auto j=Json::parse(response());if(std::string(field)=="account_key")j["profiles"][0][field]="unrequested";
  else if(std::string(field)=="ttl_seconds")j["profiles"][0][field]=121;
  else j["profiles"][0][field]["bot_module"]=true;
  p=c.begin({"player_one"},start);assert(!c.accept(p,j.dump(),start));
 }
 auto duplicate=Json::parse(response());duplicate["profiles"].push_back(duplicate["profiles"][0]);
 p=c.begin({"player_one"},start);assert(!c.accept(p,duplicate.dump(),start));
 p=c.begin({"player_one"},start);assert(!c.accept(p,response(),{110000,110000}));
 for(auto appearance:{Json{{"mode","rainbow"},{"colors",Json::array()}},Json{{"mode","gradient"},{"colors",Json::array({1,2,3})}}}){
  assert(encode(style(appearance))==appearance);p=c.begin({"player_one"},start);assert(c.accept(p,response(appearance),start));
 }
 bool rejected=false;try{c.begin({"player_one","player_one"},start);}catch(...){rejected=true;}assert(rejected);
 std::cout<<"Shared cosmetics cache/protocol checks passed\n";
}
