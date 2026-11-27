#include "client_commit_guard.h"
#include "draft_profile_guards.h"
#include "draft_submit_guard.h"
#pragma once
#include "challenge_modes.h"
#include "challenge_guards.h"
#include "draft_rerolls.h"
#include "draft_reroll_guards.h"
#include "draft_choice_guards.h"
#include "silent_reroll.h"
#include "silent_draft_guards.h"
#include "performance_metrics.h"
namespace ssc_challenge {
inline uintptr_t game=0;inline bool selecting=false,in_session=false;
using Buy=bool(*)(uintptr_t,int,uintptr_t,int,unsigned char,unsigned char,unsigned char,int,unsigned char);
using Action=bool(*)(uintptr_t,int,unsigned char,unsigned char);
using CanAct=bool(*)(uintptr_t);inline CanAct native_can_act=nullptr;
using MutationCost=int(*)(uintptr_t,int);using MutationBlocked=bool(*)();using IsMutated=bool(*)(uintptr_t,int);
using StimRank=int(*)(uintptr_t,int);using StimRarity=int(*)(uintptr_t,int,int);
using AddProfileMutation=void(*)(uintptr_t,const int*);
using RecordStimBonus=void(*)(uintptr_t,int,int);
inline StimRank native_stim_rank=nullptr;inline StimRarity native_stim_rarity=nullptr;
inline AddProfileMutation native_add_mutation=nullptr;inline RecordStimBonus native_record_stim_bonus=nullptr;
inline MutationCost native_mutation_cost=nullptr;inline MutationBlocked native_mutation_blocked=nullptr;inline IsMutated native_is_mutated=nullptr;inline bool choices_supported=false;
using Reroll=bool(*)(uintptr_t,unsigned char,unsigned char);inline Reroll native_reroll=nullptr;inline bool reroll_supported=false;
struct RerollPanel {uintptr_t actor=0,world=0;int points=0,generation=-1;uint64_t at=0;bool available=false;};inline RerollPanel reroll_panel;
template<class Read>bool validate_rerolls(uintptr_t base,Read read){for(const auto& guard:reroll_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<guard.size;++i){unsigned char b=0;if(!read(base+guard.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=guard.hash)return false;}return true;}
inline void observe_reroll_panel(uintptr_t context);
struct RerollRequest {bool pending=false;uintptr_t actor=0,world=0;int points=0,generation=-1;uint64_t at=0;};inline RerollRequest reroll_request;
inline bool request_reroll(uintptr_t,unsigned char,unsigned char);
extern "C" inline __attribute__((used)) bool ssc_draft_reroll_click(uintptr_t context,unsigned char clicked) noexcept;
using HasSkill=bool(*)(uintptr_t,int);using WorldRule=bool(*)(uintptr_t,int,unsigned char);using WorldModifier=bool(*)(uintptr_t,int);using SkillCount=int(*)(uintptr_t);using CanLearn=bool(*)(uintptr_t,int,uintptr_t,unsigned char,unsigned char);
inline HasSkill native_has_skill=nullptr;inline WorldRule native_world_rule=nullptr;inline WorldModifier native_world_modifier=nullptr;inline SkillCount native_skill_count=nullptr;inline CanLearn native_can_learn=nullptr;
inline bool (*native_reroll_blocked)(uintptr_t)=nullptr;inline Reroll native_generate=nullptr;
inline uintptr_t panel_context=0;
inline SilentRerollPlan reroll_plan(uintptr_t actor,uintptr_t world,uintptr_t context=0){
 using ssc_names::read;SilentRerollRules r;int level=-1,mode=-1,pending=0;unsigned char unlimited=2;uintptr_t first=0,last=0,cap=0;const auto profile=game+0xe09e98;
 if(!choices_supported||!actor||!world||!native_has_skill||!native_world_rule||!native_world_modifier||!native_skill_count||!native_can_learn||!native_reroll_blocked||!read(game+0xfbae68,r.price)||!read(game+0xfbae74,r.base_cap)||!read(game+0xfbbc74,r.class_bonus)||!read(game+0xfbac08,r.perk_bonus)||!read(game+0xfba5b8,r.free_until)||!read(profile+0x5ec,r.used)||!read(actor+0x358,level)||level<0||level>1000||!read(world+0x30f,unlimited)||unlimited>1||!read(game+0xe095ac,mode)||!read(actor+0x8c0,r.cash)||!read(actor+0x12f8,pending)||pending<-1)return {};
 if(!read(profile+0x4b8,first)||!read(profile+0x4c0,last)||!read(profile+0x4c8,cap)||!first||last<first||cap<last||(last-first)%12||(last-first)/12>3)return {};
 r.unlimited=unlimited!=0;r.mode_allows=mode>0;r.class_level_two=level>=2;r.perk=native_has_skill(actor+0xa0,16);r.extra_rule=native_world_rule(world,19,0);r.reduced=native_world_modifier(world,14);r.available_skills=native_skill_count(profile);r.blocked=pending>=0||native_reroll_blocked(actor+0xa0);
 for(auto p=first;p<last;p+=12){int id=-2;if(!read(p,id)||id<-1||id>=54)return {};if(id>=0){++r.valid_offers;if(!native_can_learn(profile,id,0,0,1))r.invalid_offer=true;}}
 if(context){uintptr_t current=0;float cooldown=0;if(!read(context,current)||current!=actor||!read(context+0x3148,cooldown)||!std::isfinite(cooldown)||cooldown<0||cooldown>60)return {};r.blocked=r.blocked||cooldown>0;}
 return silent_reroll_plan(r);
}
using SubmitProfile=void(*)(uintptr_t);inline SubmitProfile native_submit=nullptr;
inline Buy native_buy=nullptr;inline Action native_action=nullptr;
inline bool buy_hook(uintptr_t p,int id,uintptr_t third,int amount,unsigned char a,unsigned char b,unsigned char c,int extra,unsigned char d){if(random.enabled&&in_session&&!selecting)return false;return native_buy(p,id,third,amount,a,b,c,extra,d);}
template<class Read>bool validate_actions(uintptr_t base,Read read){for(const auto& guard:action_guards){uint64_t hash=14695981039346656037ULL;for(uint32_t i=0;i<guard.size;++i){unsigned char b=0;if(!read(base+guard.rva+i,b))return false;hash=(hash^b)*1099511628211ULL;}if(hash!=guard.hash)return false;}return true;}
template<class Read>bool validate_choices(uintptr_t base,Read read){for(const auto& guard:choice_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<guard.size;++i){unsigned char b=0;if(!read(base+guard.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=guard.hash)return false;}return true;}
template<class T>bool write_local(uintptr_t p,T value){static_assert(sizeof(T)==1||sizeof(T)==4);MEMORY_BASIC_INFORMATION m{};if(!p||!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&PAGE_GUARD)||!((m.Protect&0xff)==PAGE_READWRITE||(m.Protect&0xff)==PAGE_WRITECOPY))return false;auto start=reinterpret_cast<uintptr_t>(m.BaseAddress);if(p<start||p-start>m.RegionSize||sizeof(T)>m.RegionSize-(p-start))return false;std::memcpy(reinterpret_cast<void*>(p),&value,sizeof value);return true;}
inline bool snapshot(Offer& out);
struct ViewScope {uintptr_t actor=0;unsigned char actor_before=0,global_before=0;bool active=false;};inline ViewScope auto_view;
inline bool begin_draft_view(uintptr_t){return false;}
inline void end_draft_view(){}

inline bool action_index(uintptr_t actor,int& index){uintptr_t a=0,b=0;int selected=-2;if(!native_action||!ssc_names::read(actor+0xdc0,a)||!ssc_names::read(actor+0xdc8,b)||!a||b<a||(b-a)%0x14||(b-a)/0x14>256||!ssc_names::read(actor+0xdd8,selected)||selected<-1||(selected>=0&&uintptr_t(selected)>=(b-a)/0x14))return false;for(uintptr_t p=a;p<b;p+=0x14){int id=-1;if(!ssc_names::read(p,id))return false;if(id==6){index=int((p-a)/0x14);return true;}}return false;}
// Despite the legacy helper name, this is the normal deferred CLIENT request
// used by the native skill button when game+0xfba9cc is set.
inline bool offline_action(uintptr_t actor,int action){
 if(!actor||action!=6){return false;}auto p=actor+0x1300;MEMORY_BASIC_INFORMATION m{};
 if(!VirtualQuery(reinterpret_cast<void*>(p),&m,sizeof m)||m.State!=MEM_COMMIT||(m.Protect&PAGE_GUARD)||!((m.Protect&0xff)==PAGE_READWRITE||(m.Protect&0xff)==PAGE_WRITECOPY))return false;
 auto begin=reinterpret_cast<uintptr_t>(m.BaseAddress);if(p<begin||p-begin>m.RegionSize||sizeof(action)>m.RegionSize-(p-begin))return false;
 std::memcpy(reinterpret_cast<void*>(p),&action,sizeof action);return true;
}
inline bool (*commit_local)(uintptr_t,int)=offline_action;
inline bool deferred_skill_mode(bool& deferred){unsigned char mode=2;if(!ssc_names::read(game+0xfbcb5c,mode)||mode>1)return false;deferred=mode==1;return true;}
inline bool skill_purchase_ready(uintptr_t actor){bool deferred=false;if(!deferred_skill_mode(deferred))return false;if(!deferred)return true;int pending=-2;return ssc_names::read(actor+0x1300,pending)&&pending==-1;}
inline bool skill_commit_ready(uintptr_t actor){int index=-1,old=-2;if(!native_can_act||!action_index(actor,index)||!ssc_names::read(actor+0xdd8,old)||!write_local(actor+0xdd8,index))return false;const bool ready=native_can_act(actor)&&native_action(actor,6,1,1);write_local(actor+0xdd8,old);return ready;}
inline bool profile_submission_ready(uintptr_t actor){if(!native_submit)return true;int kind=-1;float timer=0;unsigned char blocked=2;return ssc_names::read(actor+0x7dc,kind)&&kind>=0&&kind<=1&&ssc_names::read(actor+0x88,timer)&&std::isfinite(timer)&&ssc_names::read(actor+0x84,blocked)&&blocked==0&&(kind!=0||timer<0);}
inline int last_commit_result=-1;
inline bool commit_skill(uintptr_t actor){if(native_submit){uintptr_t current=0;if(!panel_context||!ssc_names::read(panel_context,current)||current!=actor||!profile_submission_ready(actor)){last_commit_result=0;return false;}native_submit(panel_context);last_commit_result=1;return true;}bool deferred=false;if(!deferred_skill_mode(deferred)){last_commit_result=0;return false;}if(deferred){int pending=-2;const bool ready=ssc_names::read(actor+0x1300,pending)&&pending==-1;const bool ok=ready&&commit_local&&commit_local(actor,6);last_commit_result=ok?1:0;return ok;}int index=-1,old=-2;if(!action_index(actor,index)||!ssc_names::read(actor+0xdd8,old)||!skill_commit_ready(actor)||!write_local(actor+0xdd8,index)){last_commit_result=0;return false;}const bool ok=native_action(actor,6,1,0);write_local(actor+0xdd8,old);last_commit_result=ok?1:0;return ok;}

struct Commit {bool pending=false,accepted=false;uintptr_t actor=0,world=0;int round=0;uint64_t last=0;};inline Commit commit;
using PrepareProfile=void(*)(uintptr_t,uintptr_t,int);
inline PrepareProfile native_prepare=nullptr;
struct PreparedProfile {uintptr_t actor=0,world=0;int round=0,cls=-1,points=0,syringes=0;uint64_t owned=0;};inline PreparedProfile prepared;
inline bool prepare_profile(uintptr_t context){
 if(!native_prepare)return true; // Baseline unit fixtures do not attach native methods.
 using ssc_names::read;const auto& o=ssc_auto::tracking_observation;
 int observed_class=-1;if(!read(o.actor+0x350,observed_class))return false;
 if(prepared.actor&&(prepared.actor!=o.actor||prepared.world!=o.session||prepared.round!=o.round||prepared.cls!=observed_class)){picker.reset();commit={};reroller.reset();prepared={};}
 if(picker.pending||commit.pending||reroller.pending)return true;
 uintptr_t current=0,first=0,last=0;int slot=-1,cls=-1,points=-1,syringes=-1,pending=-2;
 if(!context||!read(context,current)||current!=o.actor||!read(game+0xe23678,slot)||slot<0||!read(o.session+0xa658,first)||!read(o.session+0xa660,last)||!first||last<first||(last-first)%0x34b8||(last-first)/0x34b8>4096||uintptr_t(slot)>=(last-first)/0x34b8||first+uintptr_t(slot)*0x34b8!=o.actor||!read(o.actor+0x350,cls)||!read(o.actor+0x1240,points)||!read(o.actor+0x8d0,syringes)||cls<0||cls>=512||points<0||points>1000||syringes<0||syringes>1000||!read(o.actor+0x1300,pending)||pending!=-1)return false;
 uint64_t hash=14695981039346656037ULL;auto mix=[&](int v){hash=(hash^uint32_t(v))*1099511628211ULL;};uintptr_t a=0,b=0;
 if(!read(o.actor+0x10b8,a)||!read(o.actor+0x10c0,b)||b<a||(b-a)%0x240||(b-a)/0x240>54||(!a&&b))return false;
 for(auto p=a;p<b;p+=0x240){int id=-1,n=0,purchased=0,power=0;if(!read(p,id)||!read(p+0x1f8,n)||!read(p+0x1ec,purchased)||!read(p+0x1f0,power)||id<0||id>=54||n<0||n>300||purchased<0||purchased>1000||power<0||power>1000)return false;mix(id);mix(n);mix(purchased);mix(power);}
 if(!read(o.actor+0x3c0,a)||!read(o.actor+0x3c8,b)||b<a||(b-a)%4||(b-a)/4>54||(!a&&b))return false;
 for(auto p=a;p<b;p+=4){int id=-1;if(!read(p,id)||id<0||id>=54)return false;mix(id+54);}
 // New XP arriving during a reroll chain must not undo that valid native offer.
 const bool same=prepared.actor==o.actor&&prepared.world==o.session&&prepared.round==o.round&&prepared.cls==cls&&prepared.syringes==syringes&&prepared.owned==hash&&(prepared.points==points||(prepared.points>0&&points>prepared.points));
 if(same)return true;
 // This is the same actor-to-profile copy and recalculation used by Ctrl,
 // called directly without its UI-opening wrapper or any input flags.
 native_prepare(o.session,game+0xe09e98,slot);int copied_cls=-1,copied_points=-1;
 if(!read(game+0xe09e98+0x2b0,copied_cls)||!read(game+0xe09e98+0x11a0,copied_points)||copied_cls!=cls||copied_points!=points)return false;
 prepared={o.actor,o.session,o.round,cls,points,syringes,hash};return true;
}
inline int stim_bonus_levels(int rank,int rarity,int base_cap,int grant){
 if(rank<0||rank>1000||rarity<0||rarity>3||base_cap<0||base_cap>1000||grant<0||grant>54)return -1;
 return std::clamp(base_cap+1+rarity-rank,0,grant);
}
inline bool queue_stim(const Offer& offer,int id,uint64_t now){
 using ssc_names::read;
 const auto actor=offer.actor,profile=game+0xe09e98;
 int pending=-2;
 unsigned char stage_profile=2;
 if(!choices_supported||!random.stims||id<0||id>=54||!native_mutation_cost||!native_mutation_blocked||
    native_mutation_blocked()||native_mutation_cost(actor,id)<=0||!read(actor+0x337c,pending)||pending!=-1||
    !read(game+0xfb9b98,stage_profile)||stage_profile>1)return false;
 if(!stage_profile)return write_local(actor+0x337c,id);

 uintptr_t current=0;
 if(!native_submit||!native_prepare||!native_stim_rank||!native_stim_rarity||!native_add_mutation||
    !native_record_stim_bonus||!native_buy||!native_is_mutated||!panel_context||
    !read(panel_context,current)||current!=actor||!profile_submission_ready(actor)||native_is_mutated(profile,id))return false;
 // The manual button stages the post-spend count in the copied profile.
 // Only the native pending mutation spends the live actor's inventory.
 int available=-1,base_cap=-1,grant=-1;
 constexpr uintptr_t profile_syringes=0x830;
 if(!read(actor+0x8d0,available)||available<=0||available!=offer.syringes||
    !read(game+0xfba3a8,base_cap)||!read(game+0xfb9b9c,grant))return false;
 const int levels=stim_bonus_levels(native_stim_rank(profile,id),native_stim_rarity(actor+0xa0,offer.class_id,id),base_cap,grant);
 if(levels<0)return false;
 uintptr_t first=0,last=0,limit=0;
 if(!read(profile+0x320,first)||!read(profile+0x328,last)||!read(profile+0x330,limit)||
    last<first||limit<last||(!first&&limit)||(last-first)%4||(last-first)/4>=54)return false;
 // The manual button updates matching draft offers before staging the native award.
 std::array<std::pair<uintptr_t,int>,3> matching{};
 size_t count=0;
 if(levels){
  if(!read(profile+0x4b8,first)||!read(profile+0x4c0,last)||!read(profile+0x4c8,limit)||
     last<first||limit<last||(!first&&limit)||(last-first)%12||(last-first)/12>matching.size())return false;
  for(auto row=first;row<last;row+=12){
   int skill=-2,value=-1;
   if(!read(row,skill)||skill<-1||skill>=54)return false;
   if(skill!=id)continue;
   if(!read(row+4,value)||value<0||value>1000)return false;
   matching[count++]={row+4,value+levels};
  }
 }
 const auto rebuild_profile=[](){
  prepared={};
  prepare_profile(panel_context);
  return false;
 };
 // Stage both parts with the same native profile calls as a manual syringe.
 // The game alone spends the syringe through its pending mutation command.
 if(!write_local(profile+profile_syringes,available-1))return false;
 native_add_mutation(profile+0x320,&id);
 for(size_t i=0;i<count;++i)if(!write_local(matching[i].first,matching[i].second))return rebuild_profile();
 if(levels){
  if(!native_buy(profile,id,0,0,1,1,0,levels,0))return rebuild_profile();
  native_record_stim_bonus(profile,id,levels);
 }
 if(!write_local(actor+0x337c,id))return rebuild_profile();
 picker.expected_stim_levels=levels;
 commit={true,false,actor,offer.world,offer.round,now};
 commit.accepted=commit_skill(actor);
 return true;
}
inline bool snapshot(Offer& out){
 const auto& o=ssc_auto::tracking_observation;if(!attached||!o.valid||!o.active||o.round<1||!o.actor||!o.session)return false;
 using ssc_names::read;Offer value;value.actor=o.actor;value.world=o.session;value.round=o.round;int cls=0,profile_cls=0;unsigned char special=0;
 if(!read(game+0xe09e98+0x5ec,value.generation)||value.generation<0||value.generation>1024||!read(game+0xfbcb5c,special)||special>1||!read(o.actor+0x350,cls)||!read(game+0xe09e98+0x2b0,profile_cls)||cls!=profile_cls||cls<0||cls>=512||!read(o.actor+0x1240,value.points)||value.points<0||value.points>1000||!read(o.actor+0x8d0,value.syringes)||value.syringes<0||value.syringes>1000)return false;
 value.offline=special==1; // Legacy field name: this is the native deferred-client branch.
 value.class_id=cls;
 auto now=GetTickCount64();value.reroll_known=reroll_supported&&reroll_panel.at&&now>=reroll_panel.at&&now-reroll_panel.at<=200&&reroll_panel.actor==o.actor&&reroll_panel.world==o.session&&reroll_panel.points==value.points&&reroll_panel.generation==value.generation;value.reroll_available=value.reroll_known&&reroll_panel.available;
 uintptr_t first=0,last=0,cap=0;
 if(value.points>0){auto address=game+0xe09e98+0x4b8;if(!read(address,first)||!read(address+8,last)||!read(address+16,cap)||!first||last<first||cap<last||(last-first)%12||(last-first)/12>3)return false;
   for(auto p=first;p<last;p+=12){int id=-1;if(!read(p,id)||id<-1||id>=54)return false;if(id>=0&&(!native_can_learn||native_can_learn(game+0xe09e98,id,0,1,1))&&std::find(value.skills.begin(),value.skills.end(),id)==value.skills.end())value.skills.push_back(id);}}
 // Owned skills, not the bulk-purchase UI scratch table, define mutation choices.
 {uintptr_t a=0,b=0;if(!read(o.actor+0x10b8,a)||!read(o.actor+0x10c0,b)||b<a||(b-a)%0x240||(b-a)/0x240>54||(!a&&b))return false;std::array<bool,54> seen{},excluded{};for(auto p=a;p<b;p+=0x240){int id=-1,n=-1;unsigned char flag=2;if(!read(p,id)||!read(p+0x1f8,n)||!read(p+0x1dc,flag)||id<0||id>=54||n<0||n>300||flag>1||seen[size_t(id)])return false;seen[size_t(id)]=true;int purchased=0,power=0;if(!read(p+0x1ec,purchased)||!read(p+0x1f0,power)||purchased<0||purchased>1000||power<0||power>1000)return false;value.present[size_t(id)]=true;value.allocated[size_t(id)]=purchased+power;value.acquired[size_t(id)]=n;excluded[size_t(id)]=flag!=0;}
  if(!read(o.actor+0x3c0,a)||!read(o.actor+0x3c8,b)||b<a||(b-a)%4||(b-a)/4>54||(!a&&b)){return false;}for(auto p=a;p<b;p+=4){int id=-1;if(!read(p,id)||id<0||id>=54)return false;value.mutated[size_t(id)]=true;}value.owned_known=true;
  uintptr_t profile_mutated=0,profile_end=0;if(!read(game+0xe09e98+0x320,profile_mutated)||!read(game+0xe09e98+0x328,profile_end)||profile_end<profile_mutated||(profile_end-profile_mutated)%4||(profile_end-profile_mutated)/4>54||(!profile_mutated&&profile_end))return false;
  uintptr_t costs=0,cost_end=0;bool cost_table=read(game+0xfb9bc8,costs)&&read(game+0xfb9bd0,cost_end)&&costs&&cost_end>costs&&(cost_end-costs)%4==0&&(cost_end-costs)/4<=54;
  if(random.stims&&value.syringes>0&&cost_table&&choices_supported&&native_mutation_cost&&native_mutation_blocked&&native_is_mutated&&!native_mutation_blocked()){int pending=-1;if(!read(o.actor+0x337c,pending)||pending<-1||pending>=54)return false;if(pending<0)for(int id=0;id<54;++id)if(value.acquired[size_t(id)]>0&&!excluded[size_t(id)]&&!value.mutated[size_t(id)]&&!native_is_mutated(game+0xe09e98,id)&&native_mutation_cost(o.actor,id)>0)value.stims.push_back(uintptr_t(id));}
 }

 out=std::move(value);return true;
}
// The native skill button queues actor request 6 when its deferred-client flag is set;
// the authoritative path executes the checked action directly. Never change that mode.
// Arm the native button, rather than calling the offer generator directly.
// The normal UI path records the cash cost and performs native eligibility checks.
inline void observe_reroll_panel(uintptr_t context){
 const auto& o=ssc_auto::tracking_observation;uintptr_t actor=0;int points=-1,generation=-1;auto now=GetTickCount64();
 if(!random.enabled||!reroll_supported||!o.valid||!o.active||!context||!ssc_names::read(context,actor)||actor!=o.actor||!ssc_names::read(actor+0x1240,points)||!ssc_names::read(game+0xe09e98+0x5ec,generation)){panel_context=0;reroll_panel={};return;}
 panel_context=context;
 auto plan=reroll_plan(actor,o.session,context);if(!plan.valid){reroll_panel={};return;}
 reroll_panel={actor,o.session,points,generation,now,plan.available};
}
inline bool request_reroll(uintptr_t profile,unsigned char a,unsigned char b){
 Offer o;if(profile!=game+0xe09e98||a!=1||b!=1||!snapshot(o)||!o.reroll_known||!o.reroll_available||!native_generate)return false;auto plan=reroll_plan(o.actor,o.world,panel_context);if(!plan.valid||!plan.available||!write_local(o.actor+0x12f8,plan.cost))return false;
 // Exact normal queued payment plus native offer generation; no view/input flags.
 const bool accepted=native_generate(profile,1,1);if(!accepted){int pending=-1;if(ssc_names::read(o.actor+0x12f8,pending)&&pending==plan.cost)write_local(o.actor+0x12f8,-1);}return accepted;
}

extern "C" inline __attribute__((used)) bool ssc_draft_reroll_click(uintptr_t context,unsigned char clicked) noexcept {
 if(clicked){reroll_request={};return true;}
 try{if(!random.enabled||paused||!reroll_request.pending)return false;auto now=GetTickCount64();if(now<reroll_request.at||now-reroll_request.at>1500){reroll_request={};return false;}
 uintptr_t actor=0;unsigned char enabled=0;if(!ssc_names::read(context,actor)||actor!=reroll_request.actor||!ssc_names::read(context+0x11cc,enabled)||enabled!=1)return false;
 Offer o;if(!snapshot(o)||o.actor!=reroll_request.actor||o.world!=reroll_request.world||o.points!=reroll_request.points||o.generation!=reroll_request.generation)return false;reroll_request={};return true;
 }catch(...){reroll_request={};return false;}
}
inline void tick(uint64_t now,bool menu,bool native_panel=false,uintptr_t context=0){
 const auto& o=ssc_auto::tracking_observation;bool fresh=ssc_auto::tracking_at&&now>=ssc_auto::tracking_at&&now-ssc_auto::tracking_at<=500;
 in_session=fresh&&o.valid&&o.round>=1;observe(o.session,in_session,o.active,o.ending&&!o.preparing,o.round,o.last_round,now,menu);
 static uint64_t at=0,unavailable_at=0;if(!random.enabled||!attached){picker.reset();reroller.reset();reroll_panel={};reroll_request={};commit={};prepared={};unavailable_at=0;return;}if(menu||!in_session||!o.active||!native_panel)return;if(at&&now>=at&&now-at<100)return;at=now;
 ssc_perf::Scope draft_timing(ssc_perf::draft);
 if(native_prepare&&!prepare_profile(context)){status=L"Waiting for the native draft profile";return;}
 if(native_prepare)observe_reroll_panel(context);
 Offer next;if(!snapshot(next)){if(!unavailable_at)unavailable_at=now;status=L"Waiting for an eligible native draft";return;}unavailable_at=0;
 const bool was_pending=picker.pending;
 if(!picker.settled(next,now)||!random.enabled){if(!picker.pending)commit={};else if(commit.pending&&!commit.accepted&&now>=commit.last&&now-commit.last>=250&&next.actor==commit.actor&&next.world==commit.world&&next.round==commit.round){commit.last=now;commit.accepted=commit_skill(next.actor);}return;}commit={};
 if(was_pending&&native_prepare){if(!prepare_profile(context)){status=L"Waiting for the native draft profile";return;}observe_reroll_panel(context);if(!snapshot(next))return;}
 if(!reroller.ready(next,now,[&](){Offer verify;if(!snapshot(verify)||verify.actor!=next.actor||verify.world!=next.world||verify.points!=next.points||verify.generation!=next.generation||!verify.reroll_known||!verify.reroll_available||!native_reroll)return false;return native_reroll(game+0xe09e98,1,1);}))return;
 if(next.points<=0&&(next.syringes<=0||next.stims.empty()))status=L"Waiting for the next skill or stim";
 if(((next.points>0&&!next.skills.empty())||(next.syringes>0&&!next.stims.empty()))&&!profile_submission_ready(next.actor)){status=L"Waiting until the game allows drafting";return;}
 if(next.points>0&&!next.skills.empty()&&!skill_purchase_ready(next.actor)){status=L"Waiting for the normal skill action";return;}
 picker.tick(next,now,[](size_t count,size_t& choice){return uniform(count,random_word,choice);},[&](bool skill,uintptr_t choice){Offer verify;if(!snapshot(verify)||verify.offline!=next.offline||verify.actor!=next.actor||verify.world!=next.world||verify.points!=next.points||verify.syringes!=next.syringes||verify.skills!=next.skills||verify.stims!=next.stims)return false;struct Permit{Permit(){selecting=true;}~Permit(){selecting=false;}} permit;
  if(skill){if(!native_buy(game+0xe09e98,int(choice),0,1,0,0,1,0,1))return false;commit={true,false,next.actor,next.world,next.round,now};commit.accepted=commit_skill(next.actor);return true;}return queue_stim(verify,int(choice),now);
 });
}
// Use the native panel update for timing without changing visibility or cursor flags.
// SwapBuffers only updates observation/menu state; it must not mutate the draft profile.
inline void dispatch_panel(uintptr_t context){if(!native_prepare)observe_reroll_panel(context);tick(GetTickCount64(),paused,true,context);}
inline constexpr std::array<unsigned char,45> reroll_bridge_bytes={0xff,0x50,0x10,0x48,0x83,0xec,0x20,0x0f,0xb6,0xd0,0x48,0x89,0xf1,0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xd0,0x48,0x83,0xc4,0x20,0x84,0xc0,0xff,0x25,0,0,0,0,0,0,0,0,0,0,0,0};
inline void make_reroll_bridge(unsigned char* code,uintptr_t callback,uintptr_t continuation){std::memcpy(code,reroll_bridge_bytes.data(),reroll_bridge_bytes.size());std::memcpy(code+15,&callback,8);std::memcpy(code+37,&continuation,8);}
// Skip only the native manual mutation commit block while Random Stims owns it.
// Normal UI rendering and all native eligibility/cost checks remain untouched.
inline size_t make_stim_gate(unsigned char* out,uintptr_t continuation,uintptr_t blocked){
 std::vector<unsigned char> code{0x41,0x52};std::vector<size_t> branches;auto address=[&](uintptr_t p){for(int n=0;n<8;++n)code.push_back(static_cast<unsigned char>(p>>(n*8)));};auto jump=[&](uintptr_t p){code.insert(code.end(),{0xff,0x25,0,0,0,0});address(p);};
 for(auto ptr:{reinterpret_cast<uintptr_t>(&random.enabled),reinterpret_cast<uintptr_t>(&random.stims),reinterpret_cast<uintptr_t>(&in_session)}){code.insert(code.end(),{0x49,0xba});address(ptr);code.insert(code.end(),{0x41,0x80,0x3a,0,0x0f,0x84});branches.push_back(code.size());code.insert(code.end(),{0,0,0,0});}
 code.insert(code.end(),{0x41,0x5a});jump(blocked);auto allow=code.size();code.insert(code.end(),{0x41,0x5a,0x44,0x0f,0x2f,0xb6,0x44,0x02,0,0});jump(continuation);for(auto at:branches){int32_t rel=int32_t(allow-(at+4));std::memcpy(code.data()+at,&rel,4);}std::memcpy(out,code.data(),code.size());return code.size();
}
inline bool attach(){game=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));auto read=[](uintptr_t p,auto& v){return ssc_names::read(p,v);};if(!ssc_compat::supports(128)||!validate_rerolls(game,read)||!validate_actions(game,read)||!validate_choices(game,read)||!validate_silent_drafts(game,read)||!validate_client_commit(game,read)||!validate_profile_prepare(game,read)||!validate_profile_submit(game,read))return false;
 struct Site{uint32_t call,target,length;};const Site sites[]={{0x3e4ca2,0x941710,5},{0x3e1096,0,8}};unsigned char* calls[2]{};
 for(int i=0;i<2;++i){calls[i]=reinterpret_cast<unsigned char*>(game+sites[i].call);if(i==0){int32_t rel=0;std::memcpy(&rel,calls[i]+1,4);if(calls[i][0]!=0xe8||uintptr_t(calls[i]+5+rel)!=game+sites[i].target)return false;}else{const unsigned char expected[]={0x44,0x0f,0x2f,0xb6,0x44,0x02,0,0};if(std::memcmp(calls[i],expected,8))return false;}}
 unsigned char* bridge=nullptr;for(uintptr_t d=0x10000;d<0x60000000&&!bridge;d+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((game+d)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};auto buy=reinterpret_cast<uintptr_t>(buy_hook);std::memcpy(bridge,jump,6);std::memcpy(bridge+6,&buy,8);make_stim_gate(bridge+64,game+0x3e109e,game+0x3e1577);
 DWORD old=0;if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}FlushInstructionCache(GetCurrentProcess(),bridge,4096);
 DWORD protection[2]{};for(int i=0;i<2;++i)if(!VirtualProtect(calls[i],sites[i].length,PAGE_EXECUTE_READWRITE,&protection[i])){for(int j=i-1;j>=0;--j){DWORD ignored;VirtualProtect(calls[j],sites[j].length,protection[j],&ignored);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 reroll_supported=choices_supported=true;native_reroll=request_reroll;native_generate=reinterpret_cast<Reroll>(game+0x926090);native_has_skill=reinterpret_cast<HasSkill>(game+0x942f40);native_world_rule=reinterpret_cast<WorldRule>(game+0x1e77a0);native_world_modifier=reinterpret_cast<WorldModifier>(game+0x1e7730);native_skill_count=reinterpret_cast<SkillCount>(game+0x3affe0);native_can_learn=reinterpret_cast<CanLearn>(game+0x941d20);native_reroll_blocked=reinterpret_cast<bool(*)(uintptr_t)>(game+0x3b0050);native_mutation_cost=reinterpret_cast<MutationCost>(game+0x7d9640);native_mutation_blocked=reinterpret_cast<MutationBlocked>(game+0x3b9910);native_is_mutated=reinterpret_cast<IsMutated>(game+0x1d8140);native_stim_rank=reinterpret_cast<StimRank>(game+0x3b0110);native_stim_rarity=reinterpret_cast<StimRarity>(game+0x924140);native_add_mutation=reinterpret_cast<AddProfileMutation>(game+0x1b0720);native_record_stim_bonus=reinterpret_cast<RecordStimBonus>(game+0x941600);native_can_act=reinterpret_cast<CanAct>(game+0x7df620);native_prepare=reinterpret_cast<PrepareProfile>(game+0x2e0b40);native_submit=reinterpret_cast<SubmitProfile>(game+0x3e5870);native_buy=reinterpret_cast<Buy>(game+0x941710);native_action=reinterpret_cast<Action>(game+0x7e2a60);
 for(int i=0;i<2;++i){int32_t rel=int32_t(bridge+i*64-(calls[i]+5));if(i)calls[i][0]=0xe9;std::memcpy(calls[i]+1,&rel,4);if(i==1)std::memset(calls[i]+5,0x90,3);FlushInstructionCache(GetCurrentProcess(),calls[i],sites[i].length);}for(int i=1;i>=0;--i){DWORD ignored;VirtualProtect(calls[i],sites[i].length,protection[i],&ignored);}return true;
}
}
