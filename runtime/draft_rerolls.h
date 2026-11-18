#pragma once
#include "challenge_modes.h"
namespace ssc_challenge {
// Exhaust normal available rerolls before selecting. A bounded chain guarantees
// a skill is eventually selected even when a mode supplies unlimited free rolls.
struct Reroller {
 static constexpr unsigned chain_limit=100;
 bool pending=false;Offer before;uintptr_t actor=0,world=0;int round=0,points=0,class_id=-1;unsigned used=0;uint64_t sent=0;
 void reset(){*this={};}
 template<class Send>bool ready(const Offer& o,uint64_t now,Send send){
  if(actor!=o.actor||world!=o.world||round!=o.round||class_id!=o.class_id||o.points<points||o.generation<before.generation){reset();actor=o.actor;world=o.world;round=o.round;class_id=o.class_id;}points=o.points;
  if(pending){if(o.generation>before.generation){pending=false;}else if(o.reroll_known&&!o.reroll_available&&now>=sent&&now-sent>=1000){pending=false;return true;}else{if(now>=sent&&now-sent>15000)status=L"Waiting for reroll confirmation; Random Drafts is still on";return false;}}
  if(!random.enabled)return false;
  if(o.points<=0||o.skills.empty())return true;
  if(used>=chain_limit)return true;
  if(!o.reroll_known){status=L"Waiting for the native reroll button";return false;}
  if(!o.reroll_available)return true;
  if(sent&&now>=sent&&now-sent<350)return false;
  before=o;sent=now;if(send()){pending=true;++used;status=L"Using an available reroll";}else{status=L"Reroll unavailable; selecting from the current choices";return true;}return false;
 }
};inline Reroller reroller;
}
