#pragma once
#include "local_recorder.h"
#include <set>
#include <array>
namespace ssc_record {
// Reviewed acquired-skill vector and updater for executable 8af24e05...1de6.
// Do not use draft candidates or equipped account presets as acquired skills.
struct BuildSpan {uint32_t rva,length;uint64_t hash;};
inline constexpr BuildSpan build_spans[]={
 {0x2dde00,449,0xa29812ac74144cd6ULL},
 {0x8f8510,8108,0x1a30cf3e60baf8abULL},
 {0x3de470,4174,0x315f88510155d1d4ULL},

 {0x9387f0,1666,0xf53977d329d7c501ULL},
 {0x951ef0,24972,0xedd6018ee43491f0ULL},
 {0x958080,35024,0xeaf8f28ced647351ULL},
};
template<class Read> bool validate_build_code(uintptr_t base,Read read){
 for(const auto& span:build_spans){uint64_t hash=14695981039346656037ULL;
  uint32_t i=0;std::array<unsigned char,256> bytes{};
  for(;span.length-i>=bytes.size();i+=uint32_t(bytes.size())){if(!read(base+span.rva+i,bytes))return false;for(auto b:bytes)hash=(hash^b)*1099511628211ULL;}
  for(;i<span.length;++i){unsigned char b=0;if(!read(base+span.rva+i,b))return false;hash=(hash^b)*1099511628211ULL;}
  if(hash!=span.hash)return false;
 }return true;
}
inline const char* skill_name(int id){static const char* names[]={
"Toxic Blood",
"Illicit Income",
"Manic Mechanic",
"Vampirism",
"Regeneration",
"Self Repair",
"Force Field",
"Invisibility",
"Bullet Time",
"Frenzy",
"Blade Fury",
"Shockwave",
"Radiate",
"Venom Shot",
"Fire Storm",
"Mana Shield",
"Dash Attack",
"Gunslinger",
"Boots of Travel",
"Energy Vortex",
"Laser Scope",
"Phase Drifting",
"X-Ray Vision",
"Bouncing Bullets",
"Hypnotic Army",
"Spike Trap",
"Shrapnel",
"Automatic Armor",
"Smoke Bomb",
"Blade Mail",
"Clip Magnet",
"Teleportation",
"Killer Cooldowns",
"Proximity Rounds",
"Flaming Bullets",
"Falling Inferno",
"Skill Shots",
"Mirror Image",
"Charged Shot",
"Assassinate",
"Cluster Bomb",
"Soul Drop",
"Beefcake",
"Shift Supply",
"Critical Rage",
"Executioner",
"Stimpacks",
"Combo Killer",
"Medical Supplies",
"Rambulance",
"Armed Robbery",
"Flying Dagger",
"Poison Trail",
"Revenge Rockets"
};return id>=0&&id<54?names[id]:nullptr;}
// Reader injection is used only for isolated fixtures, never for production authorization.
template<class Read> Json read_acquired_skills(uintptr_t profile,Read read){
 uintptr_t begin=0,end=0,capacity=0;
 if(!read(profile+0x1018,begin)||!read(profile+0x1020,end)||!read(profile+0x1028,capacity)||end<begin||capacity<end||
    (end-begin)%0x240||(capacity-begin)%0x240||(capacity-begin)/0x240>128||(!begin&&(end||capacity)))return nullptr;
 // Replicated stim-target IDs: profile+320, actor+3c0, decoded at 8f7750.
 uintptr_t stim_begin=0,stim_end=0,stim_cap=0;std::set<int> stimmed;
 bool stim_valid=read(profile+0x320,stim_begin)&&read(profile+0x328,stim_end)&&read(profile+0x330,stim_cap)&&
  stim_end>=stim_begin&&stim_cap>=stim_end&&(stim_end-stim_begin)%4==0&&(stim_cap-stim_begin)%4==0&&
  (stim_cap-stim_begin)/4<=128&&(stim_begin||(!stim_end&&!stim_cap));
 if(stim_valid)for(auto at=stim_begin;at<stim_end;at+=4){int id=-1;if(!read(at,id)||!skill_name(id)||!stimmed.insert(id).second){stim_valid=false;break;}}
 Json skills=Json::array();std::set<int> ids;
 for(auto at=begin;at<end;at+=0x240){
  int id=-1,base_level=-1,bonus=-1,extra=-1,effective=-1;unsigned char excluded=0;
  if(!read(at,id)||!skill_name(id)||!ids.insert(id).second||!read(at+0x1dc,excluded)||excluded>1||
     !read(at+0x1ec,base_level)||!read(at+0x1f0,bonus)||!read(at+0x1f4,extra)||!read(at+0x1f8,effective)||
     base_level<0||base_level>100||bonus<0||bonus>100||extra<0||extra>100||effective<0||effective>300)return nullptr;
  if(excluded)continue;
  if(effective!=base_level+bonus+extra)return nullptr;
  skills.push_back({{"id",id},{"name",skill_name(id)},{"effective_level",effective},
   {"level_components",{{"native_1ec",base_level},{"native_1f0",bonus},{"native_1f4",extra}}},
   {"stimmed",stim_valid?Json(stimmed.count(id)!=0):Json(nullptr)},{"stim_count",nullptr}});
 }
 uintptr_t after_begin=0,after_end=0;
 if(!read(profile+0x1018,after_begin)||!read(profile+0x1020,after_end)||after_begin!=begin||after_end!=end)return nullptr;
 uintptr_t after_stim_begin=0,after_stim_end=0;
 if(!read(profile+0x320,after_stim_begin)||!read(profile+0x328,after_stim_end)||after_stim_begin!=stim_begin||after_stim_end!=stim_end)
  for(auto& skill:skills)skill["stimmed"]=nullptr;
 return skills;
}
}
