#pragma once
#include "third_party/json.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <climits>
#include <deque>
#include <limits>
#include <string>
#include <vector>

namespace ssc_achievements {
using Json=nlohmann::json;
enum class Kind {level,chaos_round,chaos_sweep,draft_round,blind_match,kdr_match,death_time,air_knife,cash,xp,kills,jumps,bullets,syringes,melee_match,invisibility,double_bullet,syringes_match};
enum class Rarity {common,uncommon,rare,epic,legendary,mythic};
inline constexpr const wchar_t* rarity_names[]={L"Common",L"Uncommon",L"Rare",L"Epic",L"Legendary",L"Mythic"};
struct Definition {
 const char* id;const char* name;const char* description;Kind kind;uint64_t target;
 bool event_required=false;Rarity rarity=Rarity::common;unsigned tier=0;
};
inline constexpr std::array<Definition,88> catalog={{
 {"level_17","Not bad!","Reach level 17 in a round played from the start.",Kind::level,17,false,Rarity::rare},
 {"level_18","I voted!","Reach level 18 in a round played from the start.",Kind::level,18,false,Rarity::epic},
 {"level_19","Ok you're definitely on scav","Reach level 19 in a round played from the start.",Kind::level,19,false,Rarity::legendary},
 {"level_20","HOW???","Reach level 20 in a round played from the start.",Kind::level,20,false,Rarity::mythic},
 {"chaos_round","CHAOS, CHAOS!","Complete a full round with every Chaos effect active.",Kind::chaos_round,1,false,Rarity::rare},
 {"chaos_sweep","Jevil sends his regards","Win all three rounds with every Chaos effect active throughout.",Kind::chaos_sweep,3,false,Rarity::mythic},
 {"draft_round","Dealer's Choice","Complete a full round with Random Drafts and Random Stims on.",Kind::draft_round,1,false,Rarity::uncommon},
 {"blind_match","Eyes Closed","Win a full three-round match with Blind Drafts on throughout.",Kind::blind_match,3,false,Rarity::legendary},
 {"kdr_40","I'm [TITLE CARD]","Finish a full match with at least 40 kills and a 40+ KDR.",Kind::kdr_match,40,false,Rarity::mythic},
 {"death_minus_120","o7","Die as a survivor between -1:55 and -2:00.",Kind::death_time,1,true,Rarity::rare},
 {"air_knife_3","Lore-Accurate Assassin","Knife three enemy players in one airborne sequence.",Kind::air_knife,3,true,Rarity::mythic},
 {"cash_10000","Cash Collector","Collect $10,000 in multiplayer matches.",Kind::cash,10000,true,Rarity::common,1},
 {"cash_100000","Cash Collector","Collect $100,000 in multiplayer matches.",Kind::cash,100000,true,Rarity::common,2},
 {"cash_300000","Cash Collector","Collect $300,000 in multiplayer matches.",Kind::cash,300000,true,Rarity::uncommon,3},
 {"cash_1000000","Cash Collector","Collect $1,000,000 in multiplayer matches.",Kind::cash,1000000,true,Rarity::uncommon,4},
 {"cash_2000000","Cash Collector","Collect $2,000,000 in multiplayer matches.",Kind::cash,2000000,true,Rarity::rare,5},
 {"cash_4000000","Cash Collector","Collect $4,000,000 in multiplayer matches.",Kind::cash,4000000,true,Rarity::rare,6},
 {"cash_7500000","Cash Collector","Collect $7,500,000 in multiplayer matches.",Kind::cash,7500000,true,Rarity::rare,7},
 {"cash_12000000","Cash Collector","Collect $12,000,000 in multiplayer matches.",Kind::cash,12000000,true,Rarity::epic,8},
 {"cash_20000000","Cash Collector","Collect $20,000,000 in multiplayer matches.",Kind::cash,20000000,true,Rarity::epic,9},
 {"cash_35000000","Cash Collector","Collect $35,000,000 in multiplayer matches.",Kind::cash,35000000,true,Rarity::legendary,10},
 {"cash_60000000","Cash Collector","Collect $60,000,000 in multiplayer matches.",Kind::cash,60000000,true,Rarity::legendary,11},
 {"cash_100000000","Cash Collector","Collect $100,000,000 in multiplayer matches.",Kind::cash,100000000,true,Rarity::mythic,12},
 {"xp_100000","XP Gain","Earn 100,000 XP in multiplayer matches.",Kind::xp,100000,true,Rarity::common,1},
 {"xp_250000","XP Gain","Earn 250,000 XP in multiplayer matches.",Kind::xp,250000,true,Rarity::common,2},
 {"xp_500000","XP Gain","Earn 500,000 XP in multiplayer matches.",Kind::xp,500000,true,Rarity::uncommon,3},
 {"xp_1000000","XP Gain","Earn 1,000,000 XP in multiplayer matches.",Kind::xp,1000000,true,Rarity::uncommon,4},
 {"xp_2000000","XP Gain","Earn 2,000,000 XP in multiplayer matches.",Kind::xp,2000000,true,Rarity::rare,5},
 {"xp_3000000","XP Gain","Earn 3,000,000 XP in multiplayer matches.",Kind::xp,3000000,true,Rarity::rare,6},
 {"xp_5000000","XP Gain","Earn 5,000,000 XP in multiplayer matches.",Kind::xp,5000000,true,Rarity::rare,7},
 {"xp_7500000","XP Gain","Earn 7,500,000 XP in multiplayer matches.",Kind::xp,7500000,true,Rarity::epic,8},
 {"xp_10000000","XP Gain","Earn 10,000,000 XP in multiplayer matches.",Kind::xp,10000000,true,Rarity::epic,9},
 {"xp_15000000","XP Gain","Earn 15,000,000 XP in multiplayer matches.",Kind::xp,15000000,true,Rarity::legendary,10},
 {"xp_20000000","XP Gain","Earn 20,000,000 XP in multiplayer matches.",Kind::xp,20000000,true,Rarity::legendary,11},
 {"xp_30000000","XP Gain","Earn 30,000,000 XP in multiplayer matches.",Kind::xp,30000000,true,Rarity::mythic,12},
 {"xp_45000000","XP Gain","Earn 45,000,000 XP in multiplayer matches.",Kind::xp,45000000,true,Rarity::mythic,13},
 {"xp_65000000","XP Gain","Earn 65,000,000 XP in multiplayer matches.",Kind::xp,65000000,true,Rarity::mythic,14},
 {"xp_100000000","XP Gain","Earn 100,000,000 XP in multiplayer matches.",Kind::xp,100000000,true,Rarity::mythic,15},
 {"kills_100","City Slayer","Kill 100 enemy players.",Kind::kills,100,true,Rarity::common,1},
 {"kills_500","City Slayer","Kill 500 enemy players.",Kind::kills,500,true,Rarity::common,2},
 {"kills_1000","City Slayer","Kill 1,000 enemy players.",Kind::kills,1000,true,Rarity::uncommon,3},
 {"kills_2500","City Slayer","Kill 2,500 enemy players.",Kind::kills,2500,true,Rarity::uncommon,4},
 {"kills_5000","City Slayer","Kill 5,000 enemy players.",Kind::kills,5000,true,Rarity::rare,5},
 {"kills_10000","City Slayer","Kill 10,000 enemy players.",Kind::kills,10000,true,Rarity::epic,6},
 {"kills_25000","City Slayer","Kill 25,000 enemy players.",Kind::kills,25000,true,Rarity::legendary,7},
 {"kills_50000","City Slayer","Kill 50,000 enemy players.",Kind::kills,50000,true,Rarity::legendary,8},
 {"kills_75000","City Slayer","Kill 75,000 enemy players.",Kind::kills,75000,true,Rarity::legendary,9},
 {"kills_100000","City Slayer","Kill 100,000 enemy players.",Kind::kills,100000,true,Rarity::mythic,10},
 {"kills_200000","City Slayer","Kill 200,000 enemy players.",Kind::kills,200000,true,Rarity::mythic,11},
 {"jumps_100","Boing","Jump 100 times.",Kind::jumps,100,true,Rarity::common,1},
 {"jumps_300","Boing","Jump 300 times.",Kind::jumps,300,true,Rarity::common,2},
 {"jumps_1000","Boing","Jump 1,000 times.",Kind::jumps,1000,true,Rarity::uncommon,3},
 {"jumps_2000","Boing","Jump 2,000 times.",Kind::jumps,2000,true,Rarity::uncommon,4},
 {"jumps_5000","Boing","Jump 5,000 times.",Kind::jumps,5000,true,Rarity::rare,5},
 {"jumps_10000","Boing","Jump 10,000 times.",Kind::jumps,10000,true,Rarity::rare,6},
 {"jumps_25000","Boing","Jump 25,000 times.",Kind::jumps,25000,true,Rarity::epic,7},
 {"jumps_50000","Boing","Jump 50,000 times.",Kind::jumps,50000,true,Rarity::epic,8},
 {"jumps_100000","Boing","Jump 100,000 times.",Kind::jumps,100000,true,Rarity::legendary,9},
 {"jumps_300000","Boing","Jump 300,000 times.",Kind::jumps,300000,true,Rarity::legendary,10},
 {"jumps_1000000","Boing","Jump 1,000,000 times.",Kind::jumps,1000000,true,Rarity::mythic,11},
 {"bullets_1000","Shots Fired","Fire 1,000 gun projectiles.",Kind::bullets,1000,true,Rarity::common,1},
 {"bullets_3000","Shots Fired","Fire 3,000 gun projectiles.",Kind::bullets,3000,true,Rarity::common,2},
 {"bullets_10000","Shots Fired","Fire 10,000 gun projectiles.",Kind::bullets,10000,true,Rarity::uncommon,3},
 {"bullets_25000","Shots Fired","Fire 25,000 gun projectiles.",Kind::bullets,25000,true,Rarity::uncommon,4},
 {"bullets_50000","Shots Fired","Fire 50,000 gun projectiles.",Kind::bullets,50000,true,Rarity::rare,5},
 {"bullets_100000","Shots Fired","Fire 100,000 gun projectiles.",Kind::bullets,100000,true,Rarity::rare,6},
 {"bullets_250000","Shots Fired","Fire 250,000 gun projectiles.",Kind::bullets,250000,true,Rarity::epic,7},
 {"bullets_500000","Shots Fired","Fire 500,000 gun projectiles.",Kind::bullets,500000,true,Rarity::epic,8},
 {"bullets_1000000","Shots Fired","Fire 1,000,000 gun projectiles.",Kind::bullets,1000000,true,Rarity::legendary,9},
 {"bullets_3000000","Shots Fired","Fire 3,000,000 gun projectiles.",Kind::bullets,3000000,true,Rarity::legendary,10},
 {"bullets_10000000","Shots Fired","Fire 10,000,000 gun projectiles.",Kind::bullets,10000000,true,Rarity::mythic,11},
 {"syringes_10","Syringe Expert","Use 10 syringes.",Kind::syringes,10,true,Rarity::common,1},
 {"syringes_25","Syringe Expert","Use 25 syringes.",Kind::syringes,25,true,Rarity::common,2},
 {"syringes_50","Syringe Expert","Use 50 syringes.",Kind::syringes,50,true,Rarity::common,3},
 {"syringes_100","Syringe Expert","Use 100 syringes.",Kind::syringes,100,true,Rarity::uncommon,4},
 {"syringes_250","Syringe Expert","Use 250 syringes.",Kind::syringes,250,true,Rarity::uncommon,5},
 {"syringes_500","Syringe Expert","Use 500 syringes.",Kind::syringes,500,true,Rarity::rare,6},
 {"syringes_1000","Syringe Expert","Use 1,000 syringes.",Kind::syringes,1000,true,Rarity::rare,7},
 {"syringes_3000","Syringe Expert","Use 3,000 syringes.",Kind::syringes,3000,true,Rarity::epic,8},
 {"syringes_6000","Syringe Expert","Use 6,000 syringes.",Kind::syringes,6000,true,Rarity::epic,9},
 {"syringes_10000","Syringe Expert","Use 10,000 syringes.",Kind::syringes,10000,true,Rarity::legendary,10},
 {"syringes_25000","Syringe Expert","Use 25,000 syringes.",Kind::syringes,25000,true,Rarity::legendary,11},
 {"syringes_50000","Syringe Expert","Use 50,000 syringes.",Kind::syringes,50000,true,Rarity::mythic,12},
 {"syringes_100000","Syringe Expert","Use 100,000 syringes.",Kind::syringes,100000,true,Rarity::mythic,13},
 {"melee_match_10","#1 Melee Advocate","Get 10 knife kills in one match.",Kind::melee_match,10,true,Rarity::epic},
 {"draft_invisibility","You have turned to the dark side","Draft Invisibility.",Kind::invisibility,1,true,Rarity::common},
 {"double_bullet","2 Birds, 1 Shot","Kill two players with one bullet.",Kind::double_bullet,2,true,Rarity::legendary},
 {"syringes_match_6","Slide the supplier","Get six syringes in one match, applied or unused.",Kind::syringes_match,6,true,Rarity::rare},
}};
inline size_t index(const std::string& id){for(size_t i=0;i<catalog.size();++i)if(id==catalog[i].id)return i;return catalog.size();}
struct Progress {uint64_t value=0,unlocked_at=0;};
struct Ledger {
 std::array<Progress,catalog.size()> items{};
 bool notifications=true,sync=false,tracking=true,account_stats_imported=false;
 uint64_t revision=0;
 size_t unlocked()const{return size_t(std::count_if(items.begin(),items.end(),[](const Progress& p){return p.unlocked_at!=0;}));}
 Json json()const {
  Json a=Json::object();for(size_t i=0;i<items.size();++i)if(items[i].value||items[i].unlocked_at)a[catalog[i].id]={{"progress",items[i].value},{"unlocked_at",items[i].unlocked_at}};
  return {{"schema",1},{"tracking",tracking},{"notifications",notifications},{"sync",sync},{"account_stats_imported",account_stats_imported},{"achievements",a}};
 }
 static bool parse(const Json& j,Ledger& out){
  if(!j.is_object()||j.value("schema",0)!=1||!j.contains("achievements")||!j["achievements"].is_object()||j["achievements"].size()>128)return false;
  Ledger next;
  try{
   // Legacy OFF becomes a notification preference. Progress always tracks.
   if(j.contains("tracking")&&!j.at("tracking").get<bool>())next.notifications=false;
   if(j.contains("notifications"))next.notifications=next.notifications&&j.at("notifications").get<bool>();
   if(j.contains("sync"))next.sync=j.at("sync").get<bool>();
   if(j.contains("account_stats_imported"))next.account_stats_imported=j.at("account_stats_imported").get<bool>();
   for(auto it=j["achievements"].begin();it!=j["achievements"].end();++it){
    size_t i=index(it.key());if(i==catalog.size())continue;const auto& v=it.value();
    if(!v.is_object()||!v.at("progress").is_number_integer()||!v.at("unlocked_at").is_number_integer())return false;
    if(v.at("progress")<0||v.at("unlocked_at")<0)return false;
    auto count=v.at("progress").get<uint64_t>(),at=v.at("unlocked_at").get<uint64_t>();
    if(count>catalog[i].target||at>4102444800ULL||(at&&count<catalog[i].target))return false;
    next.items[i]={count,at};
   }
  }catch(...){return false;}
  for(size_t i=0;i<catalog.size();++i){
   const auto& d=catalog[i];auto& p=next.items[i];
   if(!d.tier||p.unlocked_at)continue;
   uint64_t total=0,earned=0;
   for(size_t j=0;j<catalog.size();++j){
    if(catalog[j].kind!=d.kind)continue;
    total=std::max(total,next.items[j].value);
    if(next.items[j].unlocked_at&&catalog[j].target>=d.target)
     earned=earned?std::min(earned,next.items[j].unlocked_at):next.items[j].unlocked_at;
   }
   p.value=std::min(d.target,total);
   if(total>=d.target)p.unlocked_at=earned?earned:static_cast<uint64_t>(std::time(nullptr));
  }
  out=next;return true;
 }
};
struct TierDisplay {size_t earned=catalog.size(),next=catalog.size(),shown=catalog.size();};
inline TierDisplay tier_display(const Ledger& ledger,size_t row){
 TierDisplay out;out.shown=row;
 if(row>=catalog.size())return out;
 const auto& base=catalog[row];
 for(size_t i=row;i<catalog.size();++i){
  if(i!=row&&(!base.tier||catalog[i].kind!=base.kind))break;
  if(ledger.items[i].unlocked_at)out.earned=i;
  else if(out.next==catalog.size())out.next=i;
 }
 out.shown=out.earned!=catalog.size()?out.earned:row;
 return out;
}
inline constexpr unsigned catalog_wire_version=3;
struct CompletionRates {
 std::array<int,catalog.size()> rates{},unlocked_counts{};
 uint64_t participants=0,revision=0;
 CompletionRates(){rates.fill(-1);unlocked_counts.fill(-1);}
 int unlocked(size_t id)const{return id<unlocked_counts.size()?unlocked_counts[id]:-1;}
 int percent(size_t id)const{return id<rates.size()?rates[id]:-1;}
 bool replace(const Json& data){
  try{
   if(data.at("schema")!=1||data.at("catalog")!=catalog_wire_version||data.at("population")!="synced_accounts")return false;
   const auto& entries=data.at("achievements");
   if(!data.at("participants").is_number_unsigned()||!entries.is_object()||entries.size()>catalog.size())return false;
   const auto participants=data.at("participants").get<uint64_t>();
   if(participants>100000000)return false;
   std::array<int,catalog.size()> values{},counts{};values.fill(-1);counts.fill(-1);
   for(auto it=entries.begin();it!=entries.end();++it){
    const auto id=index(it.key());if(id==catalog.size()||!it->at("unlocked").is_number_unsigned())return false;
    const auto unlocked=it->at("unlocked").get<uint64_t>();
    if(unlocked>participants)return false;
    counts[id]=int(unlocked);
    if(participants)values[id]=int((unlocked*10000+participants/2)/participants);
   }
   if(values!=rates||counts!=unlocked_counts||participants!=this->participants){
    rates=values;unlocked_counts=counts;this->participants=participants;++revision;
   }
   return true;
  }catch(...){return false;}
 }
};
inline CompletionRates completion_rates;
struct Frame {
 bool valid=false,connected=false,active=false,ending=false,preparing=false,survivor=false;
 uintptr_t session=0,actor=0;int round=0,last_round=0,level=-1,kills=-1,deaths=-1;
 double countdown=NAN;
 bool chaos_max=false,draft=false,stims=false,blind=false,draft_eligible=true;
};
struct Claim {uint64_t start=0,finish=0;bool full=false,chaos=false,blind=false;};
inline constexpr uint64_t notice_lifetime_ms=6000;
struct Notice {size_t item=0;uint64_t started=0;bool preview=false,sound_played=false;};
// A new flight starts with zero kills. Presentation may keep the best attempt,
// but every unlock must come from this single-flight count.
struct AirborneKnifeCombo {
 struct Flight {
  uintptr_t world=0,actor=0;
  int round=0,deaths=-1;
  bool operator==(const Flight& other)const{
   return world==other.world&&actor==other.actor&&round==other.round&&deaths==other.deaths;
  }
 };
 Flight flight{};
 std::array<uintptr_t,3> victims{};
 unsigned count=0,best=0;
 void land(){flight={};victims={};count=0;}
 void kill(Flight current,uintptr_t victim,int source,bool airborne,bool enemy){
  if(!airborne||!current.world||!current.actor||current.round<1||current.deaths<0){land();return;}
  if(!(current==flight)){land();flight=current;}
  if(source!=0||!enemy||!victim||victim==current.actor||count>=victims.size())return;
  if(std::find(victims.begin(),victims.end(),victim)!=victims.end())return;
  victims[count++]=victim;
  best=std::max(best,count);
 }
 unsigned take_best(){const auto value=best;best=0;return value;}
};

class Engine {
 uintptr_t session_=0,actor_=0;int round_=-1;
 uint64_t last_=0,prepared_at_=0;
 bool prepared_=false,running_=false,ended_=false,complete_=false,prior_survivor_=false;
 bool chaos_=false,draft_=false,blind_=false,match_good_=false;
 int prior_deaths_=-1,start_kills_=-1,start_deaths_=-1;
 uint64_t match_kills_=0,match_deaths_=0;
 unsigned ended_mask_=0,chaos_wins_=0,chaos_rounds_=0,blind_rounds_=0;
 uint64_t match_start_=0;Claim claim_;
 int current_kills_=-1,current_deaths_=-1;
 std::array<bool,catalog.size()> available_{};
 std::deque<Notice> notices_;
 void invalidate(){complete_=false;chaos_=draft_=blind_=false;match_good_=false;}
 void update(Kind kind,uint64_t value,uint64_t utc,bool importing=false){
  size_t highest=catalog.size();
  for(size_t i=0;i<catalog.size();++i){
   const auto& d=catalog[i];if(d.kind!=kind)continue;
   const bool earned=ledger.items[i].unlocked_at!=0;
   advance(i,value,utc,!importing);
   if(importing&&!earned&&ledger.items[i].unlocked_at)highest=i;
  }
  if(highest<catalog.size()&&ledger.notifications&&notices_.size()<catalog.size())notices_.push_back({highest,0,false});
 }
 void start_round(const Frame& f,bool carry){
  round_=f.round;prepared_=carry;running_=ended_=complete_=false;
  chaos_=draft_=blind_=false;prior_survivor_=f.survivor;prior_deaths_=f.deaths;
  start_kills_=current_kills_=f.kills;start_deaths_=current_deaths_=f.deaths;
 }
public:
 Ledger ledger;
 Engine(){for(size_t i=0;i<catalog.size();++i)available_[i]=!catalog[i].event_required;}
 void reset_tracking(){claim_={};session_=actor_=0;round_=-1;last_=prepared_at_=0;prepared_=running_=ended_=complete_=prior_survivor_=false;chaos_=draft_=blind_=match_good_=false;prior_deaths_=start_kills_=start_deaths_=current_kills_=current_deaths_=-1;match_kills_=match_deaths_=0;ended_mask_=chaos_wins_=chaos_rounds_=blind_rounds_=0;match_start_=0;}
 Claim take_claim(){auto c=claim_;claim_={};return c;}
 void confirmed(const Claim& c,bool won_all,bool won,uint64_t kills,uint64_t deaths,uint64_t utc){if(!c.full)return;if(c.chaos&&won_all)update(Kind::chaos_sweep,3,utc);if(c.blind&&won)update(Kind::blind_match,3,utc);if(kills>=40&&kills>=40*std::max<uint64_t>(1,deaths))update(Kind::kdr_match,40,utc);}
 void replace(const Ledger& next){ledger=next;notices_.clear();reset_tracking();}
 void challenge_changed(bool draft){if(running_&&!ended_){if(draft)draft_=blind_=false;else chaos_=false;}}
 void capability(Kind kind,bool value){for(size_t i=0;i<catalog.size();++i)if(catalog[i].kind==kind)available_[i]=value;}
 bool available(size_t i)const{return i<available_.size()&&available_[i];}
 bool full_round()const{return running_&&complete_;}
 void advance(size_t i,uint64_t value,uint64_t utc,bool announce=true){
  if(i>=catalog.size()||!available_[i])return;
  auto& p=ledger.items[i];if(p.unlocked_at)return;
  auto next=std::max(p.value,std::min(value,catalog[i].target));if(next==p.value&&next<catalog[i].target)return;
  if(next>p.value){p.value=next;++ledger.revision;}
  if(next==catalog[i].target&&utc>0&&utc<=4102444800ULL){
   p.unlocked_at=utc;++ledger.revision;if(announce&&ledger.notifications&&notices_.size()<catalog.size())notices_.push_back({i,0,false});
  }
 }
 void preview(){if(notices_.size()<catalog.size())notices_.push_back({0,0,true});}
 Notice* notice(uint64_t now){
  while(!notices_.empty()){
   auto& n=notices_.front();if(!ledger.notifications&&!n.preview){notices_.pop_front();continue;}if(!n.started)n.started=now?now:1;
   if(now>=n.started&&now-n.started>=notice_lifetime_ms){notices_.pop_front();continue;}
   return &n;
  }return nullptr;
 }
 bool has_notice()const{return !notices_.empty();}
 void observe(const Frame& f,uint64_t now,uint64_t utc){
  if(!f.valid){
   if(running_&&(!f.connected||!last_||now<last_||now-last_>2000))invalidate();
   if(!f.connected){reset_tracking();}
   return;
  }
  if(!f.session||!f.actor||f.round<0||f.round>3||f.last_round!=3){invalidate();return;}
  bool new_match=session_!=f.session||actor_!=f.actor||f.round<round_;
  if(new_match){reset_tracking();session_=f.session;actor_=f.actor;match_good_=f.round<=1;}
  bool recent=last_&&now>=last_&&now-last_<=2000;
  if(last_&&!recent)invalidate();
  if(round_!=f.round){
   bool carry=recent&&prepared_&&f.round==round_+1;
   if(round_>=1&&!ended_)match_good_=false;
   start_round(f,carry);
  }
  // Countdown snapshots belong to the preceding native round (warmup included).
  if(f.preparing&&!f.active&&(!running_||ended_)){prepared_=true;prepared_at_=now;prior_survivor_=f.survivor;prior_deaths_=f.deaths;last_=now;return;}
  if(f.active&&!running_&&!ended_){
   running_=true;complete_=prepared_&&recent&&now>=prepared_at_&&now-prepared_at_<=2000;
   if(!complete_)match_good_=false;
   chaos_=complete_&&f.chaos_max;draft_=complete_&&f.draft&&f.stims&&f.draft_eligible;
   blind_=draft_&&f.blind;
   if(f.round==1){match_start_=utc;match_good_=match_good_&&complete_;match_kills_=match_deaths_=0;ended_mask_=chaos_wins_=chaos_rounds_=blind_rounds_=0;}
  }
  if(running_&&!ended_){
   if(!f.chaos_max)chaos_=false;
   if(!f.draft||!f.stims||!f.draft_eligible)draft_=blind_=false;
   if(!f.blind)blind_=false;
   if(f.kills>=0)current_kills_=f.kills;
   if(f.deaths>=0)current_deaths_=f.deaths;
   // Replicated counters may reset one sample after round number advances.
   if(start_kills_>=0&&current_kills_>=0&&current_kills_<start_kills_){start_kills_=0;}
   if(start_deaths_>=0&&current_deaths_>=0&&current_deaths_<start_deaths_){start_deaths_=0;}
   if(f.active&&complete_&&f.survivor&&f.level>=0&&f.level<=100)update(Kind::level,uint64_t(f.level),utc);

  }
  prior_survivor_=f.survivor;prior_deaths_=f.deaths;
  last_=now;
 }
 // Result attribution is supplied only by a verified native result source.
 // Repeated packets, delayed confirmations and post-round frames are idempotent.
 void result(uintptr_t session,int round,bool won,uint64_t utc){
  if(session!=session_||round!=round_||round<1||round>3||!running_||ended_||!last_)return;
  ended_=true;prepared_=true;prepared_at_=last_;
  unsigned bit=1u<<unsigned(round-1);if(ended_mask_&bit)return;ended_mask_|=bit;
  if(!complete_)match_good_=false;
  if(complete_&&chaos_){chaos_rounds_|=bit;update(Kind::chaos_round,1,utc);if(won)chaos_wins_|=bit;}
  if(complete_&&draft_)update(Kind::draft_round,1,utc);
  if(complete_&&blind_)blind_rounds_|=bit;
  if(start_kills_>=0&&current_kills_>=start_kills_)match_kills_+=uint64_t(current_kills_-start_kills_);
  if(start_deaths_>=0&&current_deaths_>=start_deaths_)match_deaths_+=uint64_t(current_deaths_-start_deaths_);
  if(round==3){claim_={match_start_,utc,match_good_&&ended_mask_==7,chaos_rounds_==7,blind_rounds_==7};}
  if(match_good_&&round==3&&ended_mask_==7){
   if(chaos_wins_==7)update(Kind::chaos_sweep,3,utc);
   if(won&&blind_rounds_==7)update(Kind::blind_match,3,utc);
   if(match_kills_>=40&&match_kills_>=40*std::max<uint64_t>(1,match_deaths_))update(Kind::kdr_match,40,utc);
  }
 }
 void activity(Kind kind,uint64_t count,uint64_t utc){
  if(kind!=Kind::jumps&&kind!=Kind::bullets&&kind!=Kind::syringes&&kind!=Kind::xp&&kind!=Kind::cash)return;
  uint64_t total=0;for(size_t i=0;i<catalog.size();++i)if(catalog[i].kind==kind)total=std::max(total,ledger.items[i].value);
  if(count>1000000||total>UINT64_MAX-count)return;
  lifetime(kind,total+count,utc);
 }
 void lifetime(Kind kind,uint64_t total,uint64_t utc,bool importing=false){
  if(kind==Kind::kills||kind==Kind::jumps||kind==Kind::bullets||kind==Kind::syringes||kind==Kind::xp||kind==Kind::cash)update(kind,total,utc,importing);
 }
 void survivor_death(double clock,bool survivor,uint64_t utc){if(survivor&&clock>=-120&&clock<=-115)update(Kind::death_time,1,utc);}
 void event(Kind kind,uint64_t count,uint64_t utc){update(kind,count,utc);}
 void airborne_knives(unsigned count,bool enemy_player,bool airborne,uint64_t utc){if(enemy_player&&airborne)update(Kind::air_knife,std::min(count,3u),utc);}
};
inline std::string progress_text(const Definition& d,const Progress& p){return std::to_string(p.value)+" / "+std::to_string(d.target);}
} // namespace ssc_achievements
