#include "../runtime/recorder_source.h"
#include "../runtime/auto_messages_source.h"
#include <cassert>
#include <iostream>
template<class T> void put(std::vector<unsigned char>& v,size_t at,T value){std::memcpy(v.data()+at,&value,sizeof value);}
int main(int argc,char** argv){
 using namespace ssc_record;
 auto read=[](uintptr_t p,auto& v){return ssc_names::read(p,v);};
 std::vector<unsigned char> profile(0x1200),skills(2*0x240),stims(4);
 auto p=reinterpret_cast<uintptr_t>(profile.data()),s=reinterpret_cast<uintptr_t>(skills.data()),t=reinterpret_cast<uintptr_t>(stims.data());
 put(profile,0x1018,s);put(profile,0x1020,s+skills.size());put(profile,0x1028,s+skills.size());
 put(profile,0x320,t);put(profile,0x328,t+4);put(profile,0x330,t+4);put(stims,0,5);
 for(size_t off:{size_t(0),size_t(0x240)}){put(skills,off,off?10:5);put(skills,off+0x1ec,3);put(skills,off+0x1f0,1);put(skills,off+0x1f4,2);put(skills,off+0x1f8,6);}
 auto got=read_acquired_skills(p,read);assert(got.size()==2&&got[0]["name"]=="Self Repair"&&got[1]["name"]=="Blade Fury");
 assert(got[0]["effective_level"]==6&&got[0]["stimmed"]==true&&got[1]["stimmed"]==false&&got[0]["stim_count"].is_null());
 put(skills,0x1f8,7);assert(read_acquired_skills(p,read).is_null());put(skills,0x1f8,6);
 put(skills,0x240,5);assert(read_acquired_skills(p,read).is_null());put(skills,0x240,54);assert(read_acquired_skills(p,read).is_null());put(skills,0x240,10);
 put(skills,0x1ec,-1);assert(read_acquired_skills(p,read).is_null());put(skills,0x1ec,3);
 put(profile,0x1020,s+1);assert(read_acquired_skills(p,read).is_null());put(profile,0x1020,s+skills.size());
 put(profile,0x1028,s+129*0x240);assert(read_acquired_skills(p,read).is_null());put(profile,0x1028,s+skills.size());
 put(stims,0,999);assert(read_acquired_skills(p,read)[0]["stimmed"].is_null());put(stims,0,5);
 int calls=0;auto changing=[&](uintptr_t addr,auto& v){bool ok=read(addr,v);if(addr==p+0x1018&&++calls==2)put(profile,0x1020,s);return ok;};
 assert(read_acquired_skills(p,changing).is_null());put(profile,0x1020,s+skills.size());
 put(profile,0x1018,uintptr_t(0));put(profile,0x1020,uintptr_t(0));put(profile,0x1028,uintptr_t(0));assert(read_acquired_skills(p,read).empty());
 auto a=Json{{"class_id",1},{"health",100},{"skills",got}},b=a;b["health"]=40;assert(build_identity(a)==build_identity(b));b["skills"][0]["effective_level"]=7;assert(build_identity(a)!=build_identity(b));
 assert(argc==2);auto path=std::filesystem::path(argv[1]);HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(file!=INVALID_HANDLE_VALUE);
 HANDLE mapping=CreateFileMappingW(file,nullptr,PAGE_READONLY|SEC_IMAGE_NO_EXECUTE,0,0,nullptr);assert(mapping);auto view=MapViewOfFile(mapping,FILE_MAP_READ,0,0,0);assert(view);auto base=reinterpret_cast<uintptr_t>(view);
 assert(ssc_auto::recording_clock_supported(base));assert(ssc_auto::recording_hud_countdown(120.2f,0)==121);assert(ssc_auto::recording_hud_countdown(-1,125.9f)==-125);assert(validate_build_code(base,read));auto corrupted=[&](uintptr_t addr,auto& v){bool ok=read(addr,v);if(addr==base+build_spans[0].rva)reinterpret_cast<unsigned char*>(&v)[0]^=1;return ok;};assert(!validate_build_code(base,corrupted));
 // Exercise the production actor->embedded-profile path, not only the decoder.
 auto fixture=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x1000000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(fixture);
 for(const auto& span:build_spans)std::memcpy(fixture+span.rva,reinterpret_cast<void*>(base+span.rva),span.length);
 std::vector<unsigned char> world(0xa680),actor(0x3478);
 put(profile,0x1018,s);put(profile,0x1020,s+skills.size());put(profile,0x1028,s+skills.size());
 std::memcpy(actor.data()+0xa0,profile.data(),profile.size());
 auto actor_ptr=reinterpret_cast<uintptr_t>(actor.data()),world_ptr=reinterpret_cast<uintptr_t>(world.data());
 std::memcpy(fixture+0xde55d0,&world_ptr,8);std::memcpy(fixture+0xe1d0b8,&actor_ptr,8);
 put(world,0xa618,actor_ptr);put(world,0xa620,actor_ptr+actor.size());
 put(actor,0x78,0);put(actor,0x81,(unsigned char)1);put(actor,0x350,3);put(actor,0x870,10);
 auto integrated=sample_local_build(reinterpret_cast<uintptr_t>(fixture));
 assert(integrated["skills_status"]=="observed"&&integrated["skills"].size()==2&&integrated["skills"][0]["stimmed"]==true);
 put(actor,0x78,1);assert(sample_local_build(reinterpret_cast<uintptr_t>(fixture))["skills"].is_null());
 VirtualFree(fixture,0,MEM_RELEASE);
 UnmapViewOfFile(view);CloseHandle(mapping);CloseHandle(file);
 std::cout<<"PASS: skill IDs/levels, replicated stim state, invalid/changed vectors, duplicate IDs, missing state, change detection, current executable guards and changed-code rejection\n";
}
