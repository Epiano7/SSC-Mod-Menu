#include "../runtime/draft_training.h"
#include <cassert>
#include <iostream>
template<class F>void wait_for(F f){for(int i=0;i<200&&!f();++i)Sleep(10);assert(f());}
int main(){using namespace ssc_training;auto root=std::filesystem::path("work")/("training-delivery-test-"+std::to_string(GetTickCount64()));
 auto* service=new Service(+[](const std::string&,DWORD* http){*http=400;return -1;});
 service->configure(root,true,false);Round r;r.begin(std::string(32,'a'),1,0,false);service->submit(r.finish(1,true));
 auto pending=root/"draft-training-pending"/(std::string(32,'a')+".json");auto rejected=pending;rejected.replace_extension(".rejected");
 wait_for([&]{return std::filesystem::exists(pending);});assert(!std::filesystem::exists(rejected));
 service->configure(root,true,true);wait_for([&]{return std::filesystem::exists(rejected)&&service->state==4;});assert(!std::filesystem::exists(pending));
 std::ifstream f(root/"draft-training-pending/upload-status.json");Json j;f>>j;assert(j["action"]=="rejected_retained"&&j["http_status"]==400);
 service->configure(root,true,false);wait_for([&]{return service->state==5;});assert(std::filesystem::exists(rejected));
 service->configure(root,false,false);wait_for([&]{return service->state==0;});assert(!std::filesystem::exists(rejected));
 std::cout<<"PASS: authorization pause retains data, rejection quarantines, receipt records HTTP, explicit opt-out clears\n";
}
