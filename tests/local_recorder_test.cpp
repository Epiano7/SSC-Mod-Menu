#include "../runtime/recorder_source.h"
#include <array>
#include <cassert>
#include <iostream>
void wait_status(unsigned expected){for(int i=0;i<500&&ssc_record::state().status!=expected;++i)Sleep(10);assert(ssc_record::state().status==expected);}
template<class T> void put(unsigned char* p,size_t offset,T value){std::memcpy(p+offset,&value,sizeof(value));}
void test_source(){
 auto base=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1000000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(base);
 std::vector<unsigned char> world(0xa600),actor(0x3460),weapons(0x748);auto a=reinterpret_cast<uintptr_t>(actor.data());
 put(base,0xddb5d0,reinterpret_cast<uintptr_t>(world.data()));put(base,0xe12de8,a);put(base,0xe0f380,0);
 put(world.data(),0xa5b8,a);put(world.data(),0xa5c0,a+actor.size());put(world.data(),0x128,6);put(world.data(),0x3e0,3);
 put(world.data(),0x46c,5);auto address=reinterpret_cast<uintptr_t>(base);
 assert(ssc_record::active_round(address,2));assert(!ssc_record::active_round(address,1));assert(!ssc_record::active_round(address,3));
 put(world.data(),0x3e0,1);assert(!ssc_record::active_round(address,2));put(world.data(),0x3e0,3);
 put(world.data(),0x3c0,1.f);assert(!ssc_record::active_round(address,2));put(world.data(),0x3c0,0.f);
 put(world.data(),0x3e0,6);assert(!ssc_record::active_round(address,2));put(world.data(),0x3e0,3);
 put(world.data(),0x128,0);assert(!ssc_record::active_round(address,2));put(world.data(),0x128,6);
 put(actor.data(),0x78,0);put(actor.data(),0x81,(unsigned char)1);put(actor.data(),0x350,5);put(actor.data(),0x858,7);put(actor.data(),0x138c,75.f);put(actor.data(),0x840,100.f);
 auto w=reinterpret_cast<uintptr_t>(weapons.data());put(actor.data(),0x1288,w);put(actor.data(),0x1290,w+weapons.size());std::memcpy(weapons.data()+0x48,"Fixture",7);put(weapons.data(),0x58,size_t(7));put(weapons.data(),0x60,size_t(15));
 auto snapshot=ssc_record::sample_local_build(reinterpret_cast<uintptr_t>(base));assert(snapshot["class_id"]==5&&snapshot["health"]==75&&snapshot["inventory_weapons"][0]=="Fixture");
 put(base,0xe12de8,uintptr_t(0));snapshot=ssc_record::sample_local_build(reinterpret_cast<uintptr_t>(base));assert(snapshot["class_id"].is_null());
 VirtualFree(base,0,MEM_RELEASE);
}
int main(int argc,char**argv){test_source();assert(argc==2);using namespace ssc_record;auto folder=std::filesystem::path(argv[1]);assert(!std::filesystem::exists(folder));
 sample(2,"ignored","ignored",GetTickCount64());assert(!std::filesystem::exists(folder));
 start(folder);wait_status(6);round_state(false);sample(1,"menu","menu",GetTickCount64());assert(!std::filesystem::exists(folder));round_state(true);wait_status(1);auto now=GetTickCount64();sample(2,"BR Solos - Round 1","Class - Level 3",now);sample(2,"BR Solos - Round 1","Class - Level 3",now+1);sample(2,"BR Solos - Round 1","Class - Level 4",now+2);stop();wait_status(0);
 unsigned files=0;for(auto& entry:std::filesystem::directory_iterator(folder)){++files;std::ifstream file(entry.path());std::string line;std::vector<Json> events;while(std::getline(file,line))events.push_back(Json::parse(line));assert(events.size()==4);assert(events[0]["game_sha256"].get<std::string>().size()==64);assert(events[0]["combat_counters_available"]==false);assert(events.back()["event"]=="recording_stopped");assert(events[2]["class_level_label"]=="Class - Level 4");}
 assert(files==1);start(folder);round_state(true);wait_status(1);round_state(false);wait_status(6);assert(state().enabled);sample(1,"menu","menu",GetTickCount64());round_state(true);wait_status(1);sample(2,"BR Solos - Round 2","Class - Level 4",GetTickCount64());stop();wait_status(0);assert(std::distance(std::filesystem::directory_iterator(folder),std::filesystem::directory_iterator())==3);
 auto bad=folder/"not-a-directory";{std::ofstream f(bad);f<<"x";}start(bad);round_state(true);wait_status(2);assert(!state().enabled);
 std::cout<<"PASS: recorder off by default, background output, deduplication, valid JSON, build hash, clean stop, unique sessions and write failure\n";
}
