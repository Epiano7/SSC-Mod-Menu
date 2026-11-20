#pragma once
#include <cmath>
namespace ssc_challenge {
struct SilentRerollRules {int price=0,base_cap=0,class_bonus=0,used=0,free_until=0,valid_offers=0,available_skills=0;float perk_bonus=0,cash=0;bool perk=false,class_level_two=false,extra_rule=false,unlimited=false,reduced=false,mode_allows=false,invalid_offer=false,blocked=false;};
struct SilentRerollPlan {bool valid=false,available=false;int cost=0,cap=0;};
inline SilentRerollPlan silent_reroll_plan(const SilentRerollRules& r){
 if(r.price<0||r.price>1000000||r.base_cap<0||r.base_cap>1000||r.class_bonus<0||r.class_bonus>1000||r.used<0||r.used>1024||r.free_until<0||r.free_until>1000||!std::isfinite(r.perk_bonus)||r.perk_bonus<0||r.perk_bonus>1000||!std::isfinite(r.cash)||r.cash<0||r.valid_offers<0||r.valid_offers>3||r.available_skills<0||r.available_skills>54)return {};
 int cap=r.base_cap;if(r.perk)cap=int(float(cap)+r.perk_bonus);if(r.class_level_two)cap+=r.class_bonus;if(r.extra_rule)++cap;if(r.unlimited)cap=99;if(r.reduced)--cap;
 const int cost=r.unlimited||r.used<(r.extra_rule?r.free_until:0)?0:r.price;
 return {true,!r.blocked&&r.valid_offers==2&&(r.mode_allows||r.available_skills>2)&&(r.used<cap||r.invalid_offer)&&r.cash>=float(cost),cost,cap};
}
}
