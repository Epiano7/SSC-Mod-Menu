#pragma once
#include "private_auth_core.h"
#include <ctime>
// Beta enrollment is client-reported identity, not verified Steam ownership.
namespace ssc_beta {
using ssc_auth::Json;using ssc_auth::parse;using ssc_auth::require;using ssc_auth::string;using ssc_auth::integer;
inline void wipe(std::string& s){if(!s.empty())sodium_memzero(s.data(),s.size());s.clear();}
inline bool steam_id(uint64_t id){return (id>>56)==1&&((id>>52)&15)==1&&((id>>32)&0xfffff)==1&&uint32_t(id)!=0;}
enum class State {enroll,checking,accepted,expired,revoked,wrong_account,code_unavailable,unavailable,rate_limited,integration_error,storage_error};
inline const wchar_t* label(State s){switch(s){
 case State::checking:return L"Checking...";case State::accepted:return L"Accepted";case State::expired:return L"Access expired";case State::revoked:return L"Access revoked";
 case State::wrong_account:return L"Wrong Steam account";case State::code_unavailable:return L"Invalid or unavailable code";case State::unavailable:return L"Service unavailable - retry";
 case State::rate_limited:return L"Please wait before retrying";case State::integration_error:return L"Steam account unavailable or integration error";
 case State::storage_error:return L"Could not save access securely - retry";default:return L"Enter your beta access code";}}
struct Result {State state=State::unavailable;bool clear_token=false;std::string token;std::set<std::string> features;int64_t expires=0;};
inline int64_t expiration(const Json& v){
 if(v.is_null())return 0;
 require(v.is_string());auto s=v.get<std::string>();if(s.size()>=6&&s.compare(s.size()-6,6,"+00:00")==0)s=s.substr(0,s.size()-6)+"Z";
 require(s.size()==20||(s.size()>=22&&s.size()<=30));
 require(s[4]=='-'&&s[7]=='-'&&s[10]=='T'&&s[13]==':'&&s[16]==':'&&s.back()=='Z');
 auto number=[&](size_t p,size_t n){int x=0;for(size_t i=p;i<p+n;++i){require(s[i]>='0'&&s[i]<='9');x=x*10+s[i]-'0';}return x;};
 std::tm t{};t.tm_year=number(0,4)-1900;t.tm_mon=number(5,2)-1;t.tm_mday=number(8,2);t.tm_hour=number(11,2);t.tm_min=number(14,2);t.tm_sec=number(17,2);
 if(s.size()>20){require(s[19]=='.');number(20,s.size()-21);}
 auto original=t;auto seconds=_mkgmtime64(&t);require(seconds>0&&t.tm_year==original.tm_year&&t.tm_mon==original.tm_mon&&t.tm_mday==original.tm_mday&&t.tm_hour==original.tm_hour&&t.tm_min==original.tm_min&&t.tm_sec==original.tm_sec);return seconds;
}
inline Result response(unsigned status,const std::string& body,bool redeem,uint64_t account,int64_t now){
 Result r;
 try{auto j=parse(body);if(status!=200||(j.contains("authorized")&&j["authorized"]==false)||(j.contains("ok")&&j["ok"]==false)){std::string code;
   if(j.contains("code"))code=string(j,"code",128);
   if(code=="invalid_token"){r.state=State::enroll;r.clear_token=true;}
   else if(code=="steam_id_mismatch")r.state=State::wrong_account;
   else if(code=="expired")r.state=State::expired;else if(code=="revoked")r.state=State::revoked;
   else if(code=="code_unavailable")r.state=State::code_unavailable;else if(code=="rate_limited"||status==429)r.state=State::rate_limited;
   else if(code=="invalid_app_id"||code=="invalid_steam_id"||code=="invalid_content_type"||code=="invalid_request")r.state=State::integration_error;
   return r;
  }
  require(integer(j,"api_version")==1&&integer(j,"app_id")==308600);
  require(j.at(redeem?"ok":"authorized").is_boolean());if(!j.at(redeem?"ok":"authorized").get<bool>())return r;
  auto& e=j.at("entitlement");require(e.is_object());if(string(e,"steam_id",20)!=std::to_string(account)){r.state=State::wrong_account;return r;}
  require(!string(e,"grant_id",256).empty());require(e.at("features").is_array()&&e["features"].size()<=32);
  for(auto& f:e["features"]){require(f.is_string());auto s=f.get<std::string>();require(s.size()<=128);r.features.insert(s);}
  require(e.at("permanent").is_boolean());r.expires=expiration(e.at("expires_at"));require(e["permanent"].get<bool>()==(r.expires==0));
  if(r.expires&&r.expires<=now){r.state=State::expired;r.features.clear();return r;}
  if(redeem){r.token=string(j,"access_token",8192);require(!r.token.empty());for(unsigned char c:r.token)require(c>=33&&c<=126);}
  r.state=State::accepted;return r;
 }catch(...){wipe(r.token);return {};}
}
inline bool readiness(unsigned status,const std::string& body){try{auto j=parse(body);return status==200&&j.at("ok").is_boolean()&&j["ok"].get<bool>()&&string(j,"service")=="mod-entitlements"&&integer(j,"api_version")==1&&integer(j,"app_id")==308600;}catch(...){return false;}}
// Serialized by Service. A generation binds every result to its account/request.
struct Model {
 uint64_t account=0,generation=0;bool busy=false,enabled=false;State state=State::enroll;std::set<std::string> features;int64_t expires=0,last_wall=0;uint64_t deadline=0;
 void clear(){features.clear();enabled=false;expires=0;deadline=0;++generation;}
 void observe(uint64_t id,int64_t wall,uint64_t mono){if(account!=id){clear();account=id;state=id?State::enroll:State::integration_error;}if((last_wall&&wall<last_wall)||(expires&&(wall>=expires||mono>=deadline))){clear();state=State::expired;}if(!id)state=State::integration_error;last_wall=wall;}
 bool begin(){if(busy||!steam_id(account))return false;clear();busy=true;state=State::checking;return true;}
 bool accept(const Result& r,uint64_t g,int64_t wall,uint64_t mono){if(g!=generation)return false;state=r.state;features.clear();enabled=false;
  if(r.state==State::accepted){features=r.features;expires=r.expires;if(expires){if(expires<=wall){state=State::expired;return false;}deadline=mono+uint64_t(expires-wall)*1000-999;}}return true;}
 bool has(const char* feature)const{return state==State::accepted&&features.count(feature)!=0;}
};
}
