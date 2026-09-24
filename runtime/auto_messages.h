#pragma once
#include "third_party/json.hpp"
#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
namespace ssc_auto {
using Json=nlohmann::json;
struct Condition {int stat=0,comparison=0,value=1;};
struct Rule {bool enabled=false;int event=0,threshold=5,min_level=0;std::string message="GG!";std::string name="New rule";int repeats=1;bool any=false;std::vector<Condition> conditions{};};
inline const wchar_t* events[]={L"Any round ends",L"Round 3 ends",L"Final round ends",L"Level reaches",L"Health falls below"};
// Persisted trigger IDs remain stable for existing rules and share codes.
inline constexpr int selectable_events[]={0,2,3,4};
inline int event_choice(int event){for(int i=0;i<4;++i)if(selectable_events[i]==event)return i;return event==1?1:0;}
inline int choice_event(int choice){return choice>=0&&choice<4?selectable_events[choice]:0;}
inline const char* variables[]={"roundNumber","level","healthRemaining","maxHealth","className","shieldRemaining","shieldPercent","healthPercent","weaponName","weaponCount","roundsTotal","levelsGained","roundTime","timeAlive","roundDeaths","damageTaken","healthMissing","roundsRemaining","deadTime","alivePercent","weaponTier","weaponType","weaponMagazineSize","weaponFireRate","weaponReloadSeconds","weaponProjectilesPerShot","weapon1","weapon2","weapon3","ammoRemaining","money","syringes","shotsFired","projectilesFired","bulletHits","playerHits","npcHits","impactDamage"};
inline const wchar_t* variable_labels[]={L"Round number",L"Current level",L"Health remaining",L"Maximum health",L"Class name",L"Shield remaining",L"Shield percentage",L"Health percentage",L"Equipped weapon",L"Weapon count",L"Total rounds",L"Levels gained",L"Round time",L"Time alive",L"Deaths this round",L"HP damage taken",L"Missing health",L"Rounds remaining",L"Time dead",L"Time alive percentage",L"Weapon tier",L"Weapon type",L"Weapon magazine size",L"Weapon fire rate",L"Weapon reload seconds",L"Projectiles per shot",L"Inventory weapon 1",L"Inventory weapon 2",L"Inventory weapon 3",L"Ammo remaining",L"Money",L"Syringes",L"Weapon shots fired",L"Weapon projectiles fired",L"Projectile hit events",L"Player hit events",L"NPC hit events",L"Pre-defense impact damage"};
inline const wchar_t* variable_details[]={L"The displayed BR round",L"Your current level",L"Your remaining HP",L"Your maximum HP",L"Your selected class",L"Your current shield points",L"Shield / max HP (no % sign)",L"HP / max HP (no % sign)",L"Your equipped weapon",L"Weapons in your inventory",L"Rounds in this match",L"Net levels gained this round",L"Seconds into the round",L"Observed seconds alive",L"Native deaths this round",L"HP lost, including environment",L"Maximum HP minus current HP",L"Rounds left after this one",L"Observed seconds dead",L"Alive / round time (no % sign)",L"Tier 1-3; 0 if not tiered",L"Equipped weapon type",L"Before actor modifiers",L"RPM before actor modifiers",L"Seconds before modifiers",L"Definition projectile count",L"Inventory slot 1, or None",L"Inventory slot 2, or None",L"Inventory slot 3, or None",L"Loaded ammo before modifiers",L"Your current displayed money",L"Your current syringe counter",L"Primary weapon shots",L"Spawned weapon projectiles",L"Contacts; piercing can hit again",L"Contacts with players",L"Contacts with NPCs",L"Not actual HP damage dealt"};
inline constexpr int variable_count=sizeof(variables)/sizeof(*variables);
inline const char* condition_keys[]={"level","healthRemaining","maxHealth","roundNumber","shieldRemaining","shieldPercent","healthPercent","weaponCount","roundsTotal","levelsGained","roundTime","timeAlive","roundDeaths","damageTaken","healthMissing","roundsRemaining","deadTime","alivePercent","weaponTier","weaponMagazineSize","weaponFireRate","weaponProjectilesPerShot","ammoRemaining","money","syringes","shotsFired","projectilesFired","bulletHits","playerHits","npcHits","impactDamage"};
inline const wchar_t* condition_labels[]={L"Current level",L"Health",L"Maximum health",L"Round number",L"Shield",L"Shield percentage",L"Health percentage",L"Weapon count",L"Total rounds",L"Levels gained",L"Round time (seconds)",L"Time alive (seconds)",L"Deaths this round",L"HP damage taken",L"Missing health",L"Rounds remaining",L"Time dead",L"Time alive percentage",L"Weapon tier",L"Weapon magazine size",L"Weapon fire rate",L"Projectiles per shot",L"Ammo remaining",L"Money",L"Syringes",L"Weapon shots fired",L"Weapon projectiles fired",L"Projectile hit events",L"Player hit events",L"NPC hit events",L"Pre-defense impact damage"};
inline constexpr int condition_count=sizeof(condition_keys)/sizeof(*condition_keys);
inline constexpr uint64_t message_interval_ms=500;
static_assert(std::size(variables)==std::size(variable_labels)&&std::size(variables)==std::size(variable_details));
static_assert(std::size(condition_keys)==std::size(condition_labels));
inline Rule auto_gg(){Rule r;r.name="Auto-GG";return r;}
inline std::vector<Rule> rules={auto_gg()};
inline int selected=0,variable=0;
inline bool enabled=false,import_pending=false,delete_pending=false;
inline Rule imported;
inline std::wstring status=L"";
inline std::filesystem::path diagnostic_dir;
inline Rule& current(){static Rule empty;if(rules.empty())return empty;selected=std::clamp(selected,0,int(rules.size())-1);return rules[selected];}
inline bool printable(const std::string& s){if(s.empty()||s.size()>100)return false;for(unsigned char c:s)if(c<32||c>126)return false;return s.rfind("XL_",0)!=0&&s.rfind("XL100",0)!=0;}
using Values=std::map<std::string,std::string>;
inline bool format(const std::string& text,const Values& values,std::string& out,std::string& error){
 out.clear();error.clear();if(!printable(text)){error="Use 1-100 printable characters";return false;}
 for(size_t i=0;i<text.size();){char c=text[i++];if(c=='{'||c=='['){char close=c=='{'?'}':']';auto end=text.find(close,i);if(end==std::string::npos){error="Unclosed variable";return false;}auto key=text.substr(i,end-i);if(key=="currentLevel")key="level";bool known=false;for(auto name:variables)if(key==name)known=true;if(!known){error="Unknown variable: "+key;return false;}auto it=values.find(key);if(it==values.end()){error="Unavailable: "+key;return false;}out+=it->second;i=end+1;}else if(c=='}'||c==']'){error="Unmatched bracket";return false;}else out+=c;}
 if(!printable(out)){error="Completed message exceeds 100 characters or contains unsupported text";return false;}return true;
}
inline Values examples(){return {{"roundNumber","3"},{"level","8"},{"healthRemaining","42"},{"maxHealth","150"},{"className","Example class"},{"shieldRemaining","75"},{"shieldPercent","50"},{"healthPercent","28"},{"weaponName","UZI"},{"weaponCount","2"},{"roundsTotal","4"},{"levelsGained","5"},{"roundTime","120"},{"timeAlive","95"},{"roundDeaths","1"},{"damageTaken","108"},{"healthMissing","108"},{"roundsRemaining","1"},{"deadTime","25"},{"alivePercent","79"},{"weaponTier","1"},{"weaponType","SMG"},{"weaponMagazineSize","30"},{"weaponFireRate","600"},{"weaponReloadSeconds","1.50"},{"weaponProjectilesPerShot","1"},{"weapon1","Knife"},{"weapon2","UZI"},{"weapon3","None"},{"ammoRemaining","18"},{"money","250"},{"syringes","2"},{"shotsFired","25"},{"projectilesFired","30"},{"bulletHits","12"},{"playerHits","10"},{"npcHits","2"},{"impactDamage","108"}};}
inline bool valid(const Rule& r){if(r.name.empty()||r.name.size()>32||!printable(r.name)||r.repeats<1||r.repeats>5||r.conditions.size()+size_t(r.min_level>0)>3)return false;for(auto& c:r.conditions)if(c.stat<0||c.stat>=condition_count||c.comparison<0||c.comparison>2||c.value<0||c.value>10000)return false;std::string out,error;return r.event>=0&&r.event<5&&r.threshold>=1&&r.threshold<=1000&&r.min_level>=0&&r.min_level<=1000&&format(r.message,examples(),out,error);}
inline Json pack(const Rule& r){Json conditions=Json::array();for(auto& c:r.conditions)conditions.push_back(Json::array({c.stat,c.comparison,c.value}));return Json::array({r.event,r.threshold,r.min_level,r.message,r.name,r.repeats,r.any,conditions});}
inline Rule unpack(const Json& j){if(!j.is_array()||(j.size()!=4&&j.size()!=8)||!j[0].is_number_integer()||!j[1].is_number_integer()||!j[2].is_number_integer()||!j[3].is_string())throw std::runtime_error("Invalid rule");if(j[0]<0||j[0]>4||j[1]<1||j[1]>1000||j[2]<0||j[2]>1000)throw std::runtime_error("Range");Rule r;r.event=j[0].get<int>();r.threshold=j[1].get<int>();r.min_level=j[2].get<int>();r.message=j[3].get<std::string>();if(j.size()==8){r.name=j[4].get<std::string>();if(!j[5].is_number_integer()||j[5]<1||j[5]>5)throw std::runtime_error("Repeat range");r.repeats=j[5].get<int>();r.any=j[6].get<bool>();if(!j[7].is_array()||j[7].size()>3)throw std::runtime_error("Conditions");for(auto& c:j[7]){if(!c.is_array()||c.size()!=3||!c[0].is_number_integer()||!c[1].is_number_integer()||!c[2].is_number_integer()||c[0]<0||c[0]>=condition_count||c[1]<0||c[1]>2||c[2]<0||c[2]>10000)throw std::runtime_error("Condition");r.conditions.push_back({c[0].get<int>(),c[1].get<int>(),c[2].get<int>()});}}else r.name=(r.event==0&&(r.message=="GG"||r.message=="GG!"))?"Auto-GG":"Imported rule";if(!valid(r))throw std::runtime_error("Unsupported rule");return r;}
inline std::string code(const Rule& r){if(!valid(r))return {};auto bytes=Json::to_msgpack(pack(r));const char* alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";std::string out="SSC-AM2:";unsigned bits=0;int count=0;for(auto b:bytes){bits=(bits<<8)|b;count+=8;while(count>=6){count-=6;out+=alphabet[(bits>>count)&63];}}if(count)out+=alphabet[(bits<<(6-count))&63];return out;}
inline bool decode(const std::string& s,Rule& out){try{if(s.size()>512||(s.rfind("SSC-AM1:",0)!=0&&s.rfind("SSC-AM2:",0)!=0))return false;std::string alphabet="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";std::vector<uint8_t> bytes;unsigned bits=0;int count=0;for(size_t i=8;i<s.size();++i){auto v=alphabet.find(s[i]);if(v==std::string::npos)return false;bits=(bits<<6)|unsigned(v);count+=6;if(count>=8){count-=8;bytes.push_back(uint8_t(bits>>count));}}// Bound structure before parsing; a rule is a bounded versioned MessagePack array.
 if(bytes.empty()||(bytes[0]!=0x94&&bytes[0]!=0x98))return false;
 auto candidate=unpack(Json::from_msgpack(bytes));if(s.rfind("SSC-AM2:",0)==0&&code(candidate)!=s)return false;out=candidate;out.enabled=false;return true;}catch(...){return false;}}
inline bool save(const std::filesystem::path& dir){try{Json j={{"schema",2},{"enabled",enabled},{"rules",Json::array()}};for(auto& r:rules)j["rules"].push_back({{"on",r.enabled},{"rule",pack(r)}});auto tmp=dir/L"auto-messages.json.tmp";std::ofstream out(tmp);out<<j.dump(2);out.close();return out&&MoveFileExW(tmp.c_str(),(dir/L"auto-messages.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}catch(...){return false;}}
inline void load(const std::filesystem::path& dir){diagnostic_dir=dir;try{auto p=dir/L"auto-messages.json";if(!std::filesystem::exists(p))return;if(std::filesystem::file_size(p)>16384)throw std::runtime_error("size");std::ifstream in(p);Json j;in>>j;if((j.at("schema")!=1&&j.at("schema")!=2)||!j.at("rules").is_array()||j.at("rules").size()>12)throw std::runtime_error("schema");std::vector<Rule> next;for(auto& e:j.at("rules")){auto r=unpack(e.at("rule"));r.enabled=e.at("on").get<bool>();next.push_back(r);}bool on=j.at("enabled").get<bool>();rules=next;enabled=on;selected=0;}catch(...){status=L"Could not load message rules";}}
inline std::string read_clipboard(HWND window){std::string value;if(!OpenClipboard(window))return value;HANDLE h=GetClipboardData(CF_UNICODETEXT);if(h){auto p=static_cast<const wchar_t*>(GlobalLock(h));if(p){size_t cap=GlobalSize(h)/sizeof(wchar_t),n=0;while(n<cap&&n<=512&&p[n])++n;if(n<cap&&n<=512){for(size_t i=0;i<n;++i){if(p[i]<32||p[i]>126){value.clear();break;}value+=char(p[i]);}}GlobalUnlock(h);}}CloseClipboard();return value;}
inline bool copy_clipboard(HWND window,const std::string& value){if(value.empty())return false;auto h=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));if(!h)return false;auto p=static_cast<wchar_t*>(GlobalLock(h));if(!p){GlobalFree(h);return false;}for(size_t i=0;i<value.size();++i)p[i]=wchar_t(value[i]);p[value.size()]=0;GlobalUnlock(h);if(!OpenClipboard(window)){GlobalFree(h);return false;}bool ok=EmptyClipboard()&&SetClipboardData(CF_UNICODETEXT,h);CloseClipboard();if(!ok)GlobalFree(h);return ok;}
struct Observation {bool valid=false,active=false,ending=false,eligible=true,preparing=true,connected=false;uintptr_t session=0;int round=0,last_round=0;Values values;double damage_total=-1;int death_total=-1;};
// Cumulative observations require seeing the countdown before the round starts.
// Joining/enabling mid-round never fabricates a full-round total.
struct RoundTracker {
 uintptr_t session=0;int round=0,start_level=-1,start_deaths=-1;double start_damage=-1;uint64_t start=0,seen=0,alive_ms=0;bool prepared=false,running=false,finished=false,last_alive=false,complete=false,advanced=false;uint64_t invalid_since=0;Values frozen;
 void reset(){*this=RoundTracker{};}
 static int value(const Values& values,const char* key){auto i=values.find(key);if(i==values.end())return -1;try{return std::stoi(i->second);}catch(...){return -1;}}
 void observe(Observation& o,uint64_t now){
  if(!o.valid){if(o.connected&&session){if(!invalid_since)invalid_since=now;if(now>=invalid_since&&now-invalid_since<=2000)return;}reset();return;}
  invalid_since=0;
  if(session!=o.session||round!=o.round){
   // The native countdown belongs to the previous round, including warmup (round 0).
   bool carry=prepared&&session==o.session&&o.round==round+1&&now>=seen&&now-seen<=2000;
   auto previous_seen=seen;auto previous_damage=start_damage;int previous_deaths=start_deaths;
   reset();session=o.session;round=o.round;
   if(carry){prepared=advanced=true;seen=previous_seen;start_level=value(o.values,"level");start_deaths=previous_deaths;start_damage=previous_damage;}
  }
  if(!o.active&&!o.ending){prepared=o.preparing;start_level=value(o.values,"level");start_deaths=o.death_total;start_damage=o.damage_total;seen=now;return;}
  if(finished){prepared=true;seen=now;start_damage=o.damage_total;start_deaths=o.death_total;for(auto& v:frozen)o.values[v.first]=v.second;return;}
  if(!running){if(!o.active||!prepared||now<seen||now-seen>2000)return;running=true;prepared=false;complete=true;start=seen;last_alive=o.eligible;}
  if(now<seen||now-seen>2000)complete=false;
  // The replicated per-round counters can reset one packet after the round index.
  if(advanced&&now>=start&&now-start<=2000){if(o.damage_total>=0&&o.damage_total<start_damage)start_damage=0;if(o.death_total>=0&&o.death_total<start_deaths)start_deaths=0;}
  else advanced=false;
  if(complete){if(last_alive)alive_ms+=now-seen;o.values["roundTime"]=std::to_string((now-start)/1000);o.values["timeAlive"]=std::to_string(alive_ms/1000);o.values["deadTime"]=std::to_string((now-start-alive_ms)/1000);if(now>start)o.values["alivePercent"]=std::to_string(int(std::lround(100.0*alive_ms/(now-start))));int level=value(o.values,"level");if(start_level>=0&&level>=start_level)o.values["levelsGained"]=std::to_string(level-start_level);}
  if(start_deaths>=0&&o.death_total>=start_deaths)o.values["roundDeaths"]=std::to_string(o.death_total-start_deaths);else start_deaths=-1;if(start_damage>=0&&o.damage_total>=start_damage)o.values["damageTaken"]=std::to_string(int(std::ceil(o.damage_total-start_damage)));else start_damage=-1;
  seen=now;last_alive=o.eligible;
  if(o.ending){finished=true;prepared=true;for(auto key:{"roundTime","timeAlive","levelsGained","roundDeaths","damageTaken","deadTime","alivePercent"}){auto i=o.values.find(key);if(i!=o.values.end())frozen[key]=i->second;}}
 }
};
inline RoundTracker round_tracker;
struct Pending {std::string text;uint64_t expires=0;size_t rule=0;std::string signature;};
struct Engine {
 uintptr_t session=0;int round=0;bool participated=false,ended=false;Values previous;
 std::set<size_t> fired;std::vector<Pending> pending;uint64_t last_send=0,invalid_since=0;
 void reset(){session=0;round=0;participated=ended=false;invalid_since=0;previous.clear();fired.clear();pending.clear();}
 static int number(const Values& v,const char* key){try{auto it=v.find(key);return it==v.end()?-1:std::stoi(it->second);}catch(...){return -1;}}
 void observe(const Observation& o,uint64_t now){
  if(!enabled){reset();return;}if(!o.valid){if(o.connected&&session){if(!invalid_since)invalid_since=now;if(now>=invalid_since&&now-invalid_since<=2000)return;}reset();return;}invalid_since=0;
  if(session!=o.session||round!=o.round){reset();session=o.session;round=o.round;}
  bool was_active=participated;
  if(o.active&&!o.ending&&o.eligible)participated=true;
  bool finish=was_active&&!ended&&o.ending;
  for(size_t i=0;i<rules.size();++i){auto& r=rules[i];if(!r.enabled||fired.count(i))continue;bool trigger=false;
   if(r.event<3)trigger=finish&&(r.event==0||(r.event==1&&o.round==3)||(r.event==2&&o.round==o.last_round));
   else if(participated&&o.active&&!o.ending&&!previous.empty()){const char* key=r.event==3?"level":"healthRemaining";int a=number(previous,key),b=number(o.values,key);trigger=a>=0&&b>=0&&(r.event==3?(a<r.threshold&&b>=r.threshold):(a>=r.threshold&&b<r.threshold));}
   if(!trigger)continue;
   fired.insert(i);if(r.min_level&&number(o.values,"level")<r.min_level)continue;
   bool conditions=r.conditions.empty()||!r.any;bool missing=false;for(auto& c:r.conditions){int value=number(o.values,condition_keys[c.stat]);if(value<0){missing=true;break;}bool pass=c.comparison==0?value>=c.value:c.comparison==1?value<=c.value:value==c.value;if(r.any)conditions=conditions||pass;else conditions=conditions&&pass;}if(missing||!conditions)continue;
   std::string text,error;if(!format(r.message,o.values,text,error)){status=std::wstring(error.begin(),error.end());continue;}
   for(int repeat=0;repeat<r.repeats&&pending.size()<60;++repeat)pending.push_back({text,now+15000+uint64_t(repeat)*message_interval_ms,i,code(r)});
  }
  if(finish)ended=true;
  previous=o.values;
 }
 template<class Sender> void dispatch(uint64_t now,Sender send){while(!pending.empty()){const auto& p=pending.front();if(now<p.expires&&p.rule<rules.size()&&rules[p.rule].enabled&&code(rules[p.rule])==p.signature)break;pending.erase(pending.begin());}if(pending.empty()||(last_send&&now-last_send<message_interval_ms))return;if(send(pending.front().text)){last_send=now;pending.erase(pending.begin());status=L"Message sent";}}
};
inline Engine engine;
}
