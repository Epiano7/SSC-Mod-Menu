#include "achievement_service.h"
#include <cassert>
#include <iostream>
using namespace ssc_achievements;
Reply wait(Service& service){for(int i=0;i<200;++i){auto out=service.poll();if(!out.empty())return out.front();Sleep(10);}throw std::runtime_error("No storage reply");}
int main(int argc,char** argv){
 assert(argc==2);auto folder=std::filesystem::path(argv[1]);std::filesystem::create_directories(folder);
 Saved saved;saved.ledger.items[0]={17,1234};saved.imported={true,1000,6000,{"test_match"}};
 saved.pending.push_back({{1795000000,1795001200,true,true,true},0,3,7});
 auto saved_json=pack(saved);Saved roundtrip;assert(unpack(saved_json,roundtrip)&&roundtrip.pending.size()==1&&roundtrip.pending[0].userid==7&&roundtrip.pending[0].attempts==3&&roundtrip.pending[0].claim.blind);
 saved_json["pending_results"][0]["finish"]=1794000000;assert(!unpack(saved_json,roundtrip));
 auto path=folder/"direct.json";assert(write_saved(path,saved));Saved loaded;assert(unpack(read_json(path),loaded)&&loaded.ledger.items[0].unlocked_at==1234&&loaded.imported.kills==6000);
 saved.ledger.items[1]={18,1240};assert(write_saved(path,saved));assert(std::filesystem::exists(folder/"direct.json.bak"));
 {std::ofstream f(path);f<<"corrupt";}assert(!write_saved(path,saved));std::ifstream f(path);std::string raw;f>>raw;assert(raw=="corrupt");f.close();
 assert(unpack(read_json(folder/"direct.json.bak"),loaded)&&loaded.ledger.items[0].unlocked_at==1234);
 Service svc(folder);assert(svc.load(111,1));auto r=wait(svc);assert(r.loaded&&!r.failed&&r.account==111);
 assert(svc.save(111,1,saved));r=wait(svc);assert(!r.failed);
 assert(svc.load(111,2));r=wait(svc);assert(r.generation==2&&r.loaded&&r.saved.ledger.items[1].unlocked_at==1240);
 assert(svc.load(222,3));r=wait(svc);assert(r.loaded&&!r.failed&&r.saved.ledger.unlocked()==0);
 {std::ofstream c(folder/"achievements/333.json");c<<"{ invalid";}
 assert(svc.load(333,4));r=wait(svc);assert(r.failed&&r.loaded);
 std::cout<<"PASS asynchronous account isolation, atomic saves, verified backups and corrupt-file preservation\n";
}
