#pragma once
// Cosmetics-only protocol/cache. No private/beta grant types or gates.
#include "third_party/json.hpp"
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace ssc_shared {
using Json=nlohmann::json;
inline constexpr wchar_t host[]=L"api.epiano7.dev";
inline constexpr wchar_t prefix[]=L"/api/cosmetics/v1";
inline void require(bool value){if(!value)throw std::runtime_error("Invalid cosmetic response");}
inline bool account_valid(const std::string& s){
 if(s.empty()||s.size()>96)return false;
 for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='@'||c=='.'||c=='-'))return false;
 return true;
}
inline void exact(const Json& j,std::initializer_list<const char*> fields){
 require(j.is_object()&&j.size()==fields.size());for(auto k:fields)require(j.contains(k));
}
inline Json parse(const std::string& body){
 require(!body.empty()&&body.size()<=16384);
 auto j=Json::parse(body,[](int depth,Json::parse_event_t,Json&){require(depth<=8);return true;});
 require(j.dump()==body);return j;
}
enum class Mode {solid,rainbow,gradient};
struct Style {Mode mode=Mode::solid;std::array<uint32_t,3> colors{};unsigned count=1;};
inline Style style(const Json& j){
 exact(j,{"mode","colors"});require(j.at("mode").is_string()&&j.at("colors").is_array());
 auto mode=j.at("mode").get<std::string>();Style s;
 if(mode=="solid")s.mode=Mode::solid;else if(mode=="rainbow")s.mode=Mode::rainbow;else {require(mode=="gradient");s.mode=Mode::gradient;}
 const auto& colors=j.at("colors");s.count=unsigned(colors.size());
 require((s.mode==Mode::solid&&s.count==1)||(s.mode==Mode::rainbow&&s.count==0)||(s.mode==Mode::gradient&&s.count>=2&&s.count<=3));
 for(unsigned i=0;i<s.count;++i){require(colors[i].is_number_integer());auto c=colors[i].get<int64_t>();require(c>=0&&c<=0xffffff);s.colors[i]=uint32_t(c);}return s;
}
inline Json encode(const Style& s){
 require(s.count<=3);Json colors=Json::array();for(unsigned i=0;i<s.count;++i)colors.push_back(s.colors[i]);
 Json j={{"mode",s.mode==Mode::solid?"solid":s.mode==Mode::rainbow?"rainbow":"gradient"},{"colors",colors}};
 style(j);return j;
}
struct Time {int64_t wall_ms=0;uint64_t mono_ms=0;};
inline bool fresh(Time start,Time now,uint64_t duration){
 return now.wall_ms>=start.wall_ms&&now.mono_ms>=start.mono_ms&&uint64_t(now.wall_ms-start.wall_ms)<duration&&now.mono_ms-start.mono_ms<duration;
}
struct Pending {uint64_t generation=0,request=0;Time start{};std::set<std::string> accounts;};
class Cache {
 struct Entry {Style appearance;Time start;uint64_t ttl;};
 uint64_t account_=0,generation_=0,request_=0;bool enabled_=false;
 std::map<std::string,Entry> entries_;
public:
 // Called under the integration layer's mutex. Account zero means logged out.
 void context(uint64_t account,bool enabled){
  if(account_!=account||enabled_!=enabled){account_=account;enabled_=enabled;++generation_;++request_;entries_.clear();}
 }
 Pending begin(const std::vector<std::string>& accounts,Time now){
  require(enabled_&&account_&&accounts.size()<=32);Pending p{generation_,++request_,now,{}};
  for(auto& a:accounts){require(account_valid(a)&&p.accounts.insert(a).second);}return p;
 }
 bool accept(const Pending& p,const std::string& body,Time now){
  if(!enabled_||!account_||p.generation!=generation_||p.request!=request_)return false;
  // An outage never calls accept and never refreshes existing leases.
  // Consume a completed request even when malformed; duplicate responses cannot renew.
  ++request_;
  try{
   require(fresh(p.start,now,10000));auto j=parse(body);exact(j,{"ok","profiles"});require(j.at("ok")==true&&j.at("profiles").is_array()&&j.at("profiles").size()<=32);
   std::map<std::string,Entry> next;
   for(auto& profile:j.at("profiles")){
    exact(profile,{"account_key","style","ttl_seconds"});require(profile.at("account_key").is_string()&&profile.at("ttl_seconds").is_number_integer());
    auto account=profile.at("account_key").get<std::string>();auto ttl=profile.at("ttl_seconds").get<int64_t>();
    require(p.accounts.count(account)&&ttl>=1&&ttl<=30);
    // Start at request dispatch, not receipt: latency cannot extend server leases.
    require(next.emplace(account,Entry{style(profile.at("style")),p.start,uint64_t(ttl)*1000}).second);
   }
   entries_.swap(next);return true;
  }catch(...){return false;}
 }
 bool get(const std::string& native_account,Time now,Style& result){
  if(!enabled_||!account_)return false;
  auto it=entries_.find(native_account);
  if(it==entries_.end())return false;
  if(!fresh(it->second.start,now,it->second.ttl)){entries_.erase(it);return false;}
  result=it->second.appearance;return true;
 }
};
}
