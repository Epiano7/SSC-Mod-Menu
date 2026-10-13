#include "../runtime/auto_messages_source.h"
#include <cassert>
#include <iostream>
using namespace ssc_auto;
int main(){
 enabled=true;Rule r;r.enabled=true;r.event=5;r.conditions={{0,0,5}};r.hold_seconds=3;r.delay_seconds=2;r.cancel_on_death=true;
 Observation o;o.valid=o.active=o.eligible=true;o.session=1;o.actor=2;o.round=1;o.last_round=3;o.death_total=0;o.values=examples();o.values["level"]="5";
 Engine e;int sent=0;auto send=[&](const std::string&){++sent;return true;};rules={r};
 e.observe(o,1000);e.observe(o,2000);o.values["level"]="4";e.observe(o,3000);o.values["level"]="5";
 for(uint64_t t=4000;t<=6000;t+=1000){e.observe(o,t);e.dispatch(t,send);}assert(sent==0);
 e.observe(o,7000);assert(e.pending.size()==1);e.dispatch(8999,send);assert(!sent);e.observe(o,8000);e.observe(o,9000);e.dispatch(9000,send);assert(sent==1);
 e.observe(o,10000);assert(e.pending.empty());
 // Delay cancellation on a death, even if a respawn occurs between samples.
 r.hold_seconds=0;r.delay_seconds=5;rules={r};e.reset();e.observe(o,1000);assert(e.pending.size()==1);o.death_total=1;e.observe(o,1100);assert(e.pending.empty());e.dispatch(6000,send);assert(sent==1);
 o.death_total=0;e.reset();e.observe(o,1000);o.eligible=false;e.observe(o,1100);assert(e.pending.empty());o.eligible=true;
 // Invalid samples, actor/account context changes, round changes and disable cancel.
 for(int kind=0;kind<5;++kind){e.reset();o.valid=true;o.actor=2;o.round=1;enabled=true;rules={r};e.observe(o,1000);assert(e.pending.size()==1);if(kind==0)o.valid=false;if(kind==1)o.actor=3;if(kind==2)o.round=2;if(kind==3)enabled=false;if(kind==4)rules[0].enabled=false;e.observe(o,1100);e.dispatch(1100,send);if(kind==1||kind==2){assert(e.pending.size()==1&&e.pending[0].due==6100);}else assert(e.pending.empty());}
 enabled=true;o.valid=true;o.round=1;o.actor=2;rules={r};
 // Rule edits discard the old delayed send; a new qualifying continuous rule starts afresh.
 e.reset();e.observe(o,1000);rules[0].message="Edited";e.observe(o,1100);e.dispatch(6000,send);assert(sent==1); // already fired this round
 // Timing and death preference survive export/import. Old 4/8-field rules get no timing.
 auto encoded=code(r);Rule decoded;assert(decode(encoded,decoded)&&decoded.delay_seconds==5&&decoded.cancel_on_death&&!decoded.enabled);
 assert(!decode(encoded+"A",decoded));auto packed=pack(r);packed.erase(packed.begin()+8,packed.end());auto old=unpack(packed);assert(!old.hold_seconds&&!old.delay_seconds&&!old.cancel_on_death);
 for(int field:{8,9}){auto bad=pack(r);bad[field]=61;try{unpack(bad);assert(false);}catch(...){}bad[field]=-1;try{unpack(bad);assert(false);}catch(...){}}
 // No head-of-line blocking from a delayed rule; preserve global spacing.
 Rule immediate=r;immediate.delay_seconds=0;immediate.message="Immediate";rules={r,immediate};e.reset();e.observe(o,1000);e.last_send=0;e.dispatch(1000,[&](const std::string& text){assert(text=="Immediate");return true;});assert(e.pending.size()==1&&e.pending[0].text==r.message);
 // A clock rollback or observation gap cannot satisfy a continuous hold.
 r.hold_seconds=3;r.delay_seconds=0;rules={r};e.reset();e.observe(o,1000);e.observe(o,2000);e.observe(o,500);e.observe(o,1500);assert(e.pending.empty());e.observe(o,9000);assert(e.pending.empty());
 // Missing conditions reset rather than invent a passing value.
 e.reset();e.observe(o,1000);e.observe(o,2000);o.values.erase("level");e.observe(o,3000);o.values["level"]="5";e.observe(o,4000);e.observe(o,5000);e.observe(o,6000);assert(e.pending.empty());e.observe(o,7000);assert(e.pending.size()==1);
 // A lethal health crossing keeps legacy behavior unless cancellation is selected.
 r={};r.enabled=true;r.event=4;r.threshold=30;rules={r};e.reset();o.values["healthRemaining"]="60";o.eligible=true;e.observe(o,1000);o.values["healthRemaining"]="0";o.eligible=false;e.observe(o,1100);assert(e.pending.size()==1);
 rules[0].cancel_on_death=true;e.reset();o.values["healthRemaining"]="60";o.eligible=true;e.observe(o,2000);o.values["healthRemaining"]="0";o.eligible=false;e.observe(o,2100);assert(e.pending.empty());
 std::cout<<"PASS: hold flicker/missing data, delay, death/respawn, context changes, rule edits, disable, bounded persistence, queue fairness, rollback/gap and lethal-trigger compatibility\n";
}
