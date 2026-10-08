#include "../runtime/shared_cosmetics.h"
#include <cassert>
#include <iostream>
using namespace ssc_shared;
struct Identity {
 inline static std::atomic<uint64_t> id{76561198000000001ULL};
 bool attach(){return true;}uint64_t read()const{return id.load();}
};
struct Net {
 inline static std::atomic<int> requests{0};inline static std::atomic<bool> blocking{false},stopped{false};
 ssc_beta::Reply request(const wchar_t*,const std::string&){++requests;while(blocking&&!stopped)Sleep(10);
  return {200,"{\"ok\":true,\"profiles\":[{\"account_key\":\"other\",\"style\":{\"colors\":[5623039],\"mode\":\"solid\"},\"ttl_seconds\":30}]}"};}
 void stop(){stopped=true;}void cancel(){stop();}
};
int main(){
 BasicService<Identity,Net> service;service.configure(std::filesystem::temp_directory_path());Style appearance,received;
 service.update("owner",{"other","other"},appearance,true,false);
 for(int i=0;i<100&&!service.get("other",received);++i)Sleep(10);
 assert(service.get("other",received)&&received.colors[0]==5623039);
 service.update("owner",{"other"},appearance,false,false);assert(!service.get("other",received));
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
 Net::requests=0;Net::stopped=false;Net::blocking=true;BasicService<Identity,Net> blocked;blocked.configure(std::filesystem::temp_directory_path());
 blocked.update("owner",{"other"},appearance,true,false);for(int i=0;i<100&&Net::requests==0;++i)Sleep(10);assert(Net::requests==1);
 const auto start=GetTickCount64();blocked.stop();assert(GetTickCount64()-start<1000);assert(!blocked.get("other",received));
 std::cout<<"Shared cosmetics worker: async lookup, opt-out, account switch and cancellation passed\n";
}
