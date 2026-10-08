#pragma once
#include "presence_source.h"
#include "local_recorder.h"
#include "build_recording_source.h"
namespace ssc_record {
inline bool build_code_supported(uintptr_t base){
 static uintptr_t checked=0;static bool valid=false;
 if(checked!=base){checked=base;valid=validate_build_code(base,[](uintptr_t p,auto& v){return ssc_names::read(p,v);});}
 return valid;
}

// BR uses a native pre-round counter and countdown. Unknown round layouts
// fail closed rather than collecting lobby activity.
inline bool active_round(uintptr_t base,int phase){
    if(phase!=2||!ssc_compat::supports(2)||!ssc_compat::supports(4))return false;
    using ssc_names::read;uintptr_t world=0;int mode=0,round=0,last=0;float countdown=0,ending=0;unsigned char survival=0;
    return read(base+ssc_compat::resolve(0xde45d0),world)&&world&&
        read(world+0x128,mode)&&mode==6&&read(world+0x485,survival)&&!survival&&read(world+0x3e0,round)&&round>=2&&round<=101&&
        read(world+0x46c,last)&&last>=1&&last<=100&&round-1<=last&&read(world+0x3c0,countdown)&&
        std::isfinite(countdown)&&countdown<=0&&read(world+0x6ae8,ending)&&std::isfinite(ending)&&ending<=0;
}
inline Json sample_local_build(uintptr_t base){
    Json result={{"class_id",nullptr},{"level",nullptr},{"health",nullptr},{"max_health",nullptr},{"inventory_weapons",nullptr},{"build_schema",1},{"skills",nullptr},{"skills_status","actor_unavailable"},{"stim_source","replicated_skill_ids"}};
    if(!ssc_compat::supports(4))return result;
    using ssc_names::read;uintptr_t world=0,first=0,last=0,steam_first=0;int local=-1,slot=-1,kind=-1;unsigned char active=0;
    if(!read(base+ssc_compat::resolve(0xde45d0),world)||!world||!read(world+0xa618,first)||!read(world+0xa620,last)||!first||last<first||(last-first)%0x3478||(last-first)/0x3478>2048
       ||!read(base+ssc_compat::resolve(0xe1c0a8),steam_first)||first!=steam_first||!read(base+ssc_compat::resolve(0xe18628),local)||local<0||uintptr_t(local)>=(last-first)/0x3478)return result;
    auto actor=first+uintptr_t(local)*0x3478;
    if(!read(actor+0x78,slot)||slot!=local||!read(actor+0x7dc,kind)||kind!=0||!read(actor+0x81,active)||!active)return result;
    int class_id=-1,level=-1,round=-1,mode=-1;
    if(read(actor+0x350,class_id)&&class_id>=0&&class_id<512)result["class_id"]=class_id;
    if(read(actor+0x870,level)&&level>=0&&level<=10000)result["level"]=level;
    if(read(world+0x128,mode))result["native_mode_id"]=mode;
    if(read(world+0x3e0,round)&&round>=0&&round<1000)result["native_round_index"]=round;
    result["skills_status"]="unsupported_game_layout";
    if(build_code_supported(base)){
        result["skills"]=read_acquired_skills(actor+0xa0,[](uintptr_t p,auto& v){return read(p,v);});
        result["skills_status"]=result["skills"].is_array()?"observed":"invalid_or_changing_state";
    }
    // These layouts are covered by the Weapon Lab's health and weapon-HUD checks.
    if(!ssc_compat::supports(8))return result;
    float hp=0,max_hp=0;
    if(read(actor+0x13a4,hp)&&std::isfinite(hp)&&hp>=0&&hp<=1000000)result["health"]=hp;
    if(read(actor+0x858,max_hp)&&std::isfinite(max_hp)&&max_hp>0&&max_hp<=1000000)result["max_health"]=max_hp;
    uintptr_t start=0,end=0;
    if(read(actor+0x12a0,start)&&read(actor+0x12a8,end)&&start&&end>=start&&(end-start)%0x748==0&&(end-start)/0x748<=16){
        Json inventory=Json::array();for(auto record=start;record<end;record+=0x748){auto label=ssc_rpc::native_string(record+0x48);if(label.empty())return result;inventory.push_back(label);}result["inventory_weapons"]=inventory;
    }
    return result;
}
}
