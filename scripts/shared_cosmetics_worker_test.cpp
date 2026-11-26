#include "../runtime/shared_cosmetics.h"
#include <cassert>
#include <iostream>
using namespace ssc_shared;
struct Identity {
 inline static std::atomic<uint64_t> id{76561198000000001ULL};
 inline static std::atomic<bool> hold_next{false},holding{false},release{false};
 bool attach(){return true;}
 uint64_t read()const{
  if(hold_next.exchange(false)){holding=true;while(!release)Sleep(1);}
  return id.load();
 }
};
struct Net {
 inline static std::atomic<int> requests{0};inline static std::atomic<bool> blocking{false},stopped{false};
 inline static std::mutex seen_mutex;inline static std::set<std::string> seen;
 ssc_beta::Reply request(const wchar_t* path,const std::string& body){++requests;
  while(blocking&&!stopped)Sleep(10);
  if(std::wstring(path).find(L"/status")!=std::wstring::npos)return {200,"{\"ok\":true,\"api_version\":1,\"app_id\":308600,\"service\":\"shared-cosmetics\",\"lease_seconds\":120}"};
  if(std::wstring(path).find(L"/publish")!=std::wstring::npos){auto j=Json::parse(body);return {200,Json{{"ok",true},{"account_key",j["account_key"]},{"lease_seconds",j["operation"]["action"]=="remove"?0:120}}.dump()};}
  {std::lock_guard<std::mutex> lock(seen_mutex);auto j=Json::parse(body);for(auto& a:j["accounts"])seen.insert(a.get<std::string>());}while(blocking&&!stopped)Sleep(10);
  auto request=Json::parse(body);Json profiles=Json::array();for(auto& account:request["accounts"])if(account=="other")profiles.push_back(Json{{"account_key","other"},{"style",{{"colors",Json::array({5623039})},{"mode","solid"}}},{"ttl_seconds",30}});
  return {200,Json{{"ok",true},{"profiles",profiles}}.dump()};}
 void stop(){stopped=true;}void cancel(){stop();}
};
int main(){
 ssc_privacy::reader=+[]{return ssc_privacy::Detection::off;};
 
 BasicService<Identity,Net> service;service.configure(std::filesystem::temp_directory_path());Style appearance,received;
 service.update("owner",{"other","other"},appearance,true,false);
 for(int i=0;i<100&&!service.get("other",received);++i)Sleep(10);
 assert(service.get("other",received)&&received.colors[0]==5623039);
 std::shared_ptr<const Cache::View> frame;
 for(int i=0;i<100&&!frame;++i){service.render_view(frame);if(!frame)Sleep(1);}
 assert(frame&&frame->get("other",GetTickCount64(),received)&&received.colors[0]==5623039);
 assert(!frame->get("other",GetTickCount64()+30000,received));
 Identity::hold_next=true;
 std::thread contention([&]{Style style;service.get("other",style);});
 for(int i=0;i<1000&&!Identity::holding;++i)Sleep(1);
 assert(Identity::holding);
 assert(!service.render_view(frame)&&frame);
 assert(!service.observe_render("new_chat",GetTickCount64()));
 assert(!service.update("owner",{"other"},appearance,true,false,true));
 Identity::release=true;
 contention.join();
 ssc_privacy::manual=true;
 for(int i=0;i<100&&frame;++i){service.render_view(frame);if(frame)Sleep(1);}
 assert(!frame);
 ssc_privacy::manual=false;
 service.update("owner",{"other"},appearance,false,false);assert(!service.get("other",received));
 assert(service.render_view(frame)&&!frame);
 service.update("owner",{"other"},appearance,true,false);
 for(int i=0;i<100&&!service.get("other",received);++i)Sleep(10);
 assert(service.get("other",received));Identity::id=76561198000000002ULL;assert(!service.get("other",received));
 service.stop();
 Net::requests=0;Net::stopped=false;
 BasicService<Identity,Net> chat;chat.configure(std::filesystem::temp_directory_path());
 chat.update("owner",{},appearance,true,false);assert(!chat.get("other",received));
 chat.update("owner",{},appearance,true,false);
 for(int i=0;i<100&&!chat.get("other",received);++i)Sleep(10);
 assert(chat.get("other",received));chat.stop();
 // Full roster plus a newly visible out-of-match chat sender must all be queried.
 Net::stopped=false;{std::lock_guard<std::mutex> lock(Net::seen_mutex);Net::seen.clear();}
 BasicService<Identity,Net> crowded;crowded.configure(std::filesystem::temp_directory_path());
 std::vector<std::string> roster;for(int i=0;i<32;++i)roster.push_back("actor"+std::to_string(i));
 crowded.update("owner",roster,appearance,true,false);crowded.get("z_chat",received);crowded.update("owner",roster,appearance,true,false);
 bool complete=false;for(int i=0;i<500&&!complete;++i){Sleep(10);std::lock_guard<std::mutex> lock(Net::seen_mutex);complete=Net::seen.size()==33&&Net::seen.count("z_chat");}
 assert(complete);crowded.stop();
 Net::requests=0;Net::stopped=false;Net::blocking=true;BasicService<Identity,Net> blocked;blocked.configure(std::filesystem::temp_directory_path());
 blocked.update("owner",{"other"},appearance,true,false);for(int i=0;i<100&&Net::requests==0;++i)Sleep(10);assert(Net::requests==1);
 const auto start=GetTickCount64();blocked.stop();assert(GetTickCount64()-start<1000);assert(!blocked.get("other",received));
 std::cout<<"PASS shared cosmetics worker: nonblocking render, expiry, privacy, account switch and cancellation\n";
}
