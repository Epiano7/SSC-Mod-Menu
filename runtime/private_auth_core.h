#pragma once
// Protocol-only code. No test grants, persistence, URLs from tokens, or secrets.
#include "third_party/json.hpp"
#include "private_auth_privacy.h"
#ifndef SODIUM_STATIC
#define SODIUM_STATIC
#endif
#include <sodium.h>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace ssc_auth {
using Json=nlohmann::json;
constexpr uint32_t expected_app=308600; // Installed game's steam_appid.txt; checked against ISteamUtils at runtime.
constexpr const char* issuer="https://sscmmauth.epiano7.dev";
struct Clock {int64_t wall;uint64_t mono;}; // Epoch seconds and suspend-inclusive monotonic milliseconds.
struct Account {uint64_t id=0;uint32_t app=0;bool online=false;
 bool valid()const{return online&&app==expected_app&&id!=0;}
 bool operator==(const Account& b)const{return id==b.id&&app==b.app&&online==b.online;}
 bool operator!=(const Account& b)const{return !(*this==b);}
};
inline void require(bool ok){if(!ok)throw std::runtime_error("Authorization rejected");}
inline std::string b64(const unsigned char* p,size_t n){std::string s(sodium_base64_ENCODED_LEN(n,sodium_base64_VARIANT_URLSAFE_NO_PADDING),0);sodium_bin2base64(s.data(),s.size(),p,n,sodium_base64_VARIANT_URLSAFE_NO_PADDING);s.resize(s.size()-1);return s;}
inline std::vector<unsigned char> un64(const std::string& s,size_t maximum){
 require(!s.empty()&&s.size()<=maximum*2+4);std::vector<unsigned char> bytes(maximum);size_t n=0;
 require(sodium_base642bin(bytes.data(),bytes.size(),s.data(),s.size(),nullptr,&n,nullptr,sodium_base64_VARIANT_URLSAFE_NO_PADDING)==0);
 bytes.resize(n);require(b64(bytes.data(),bytes.size())==s);return bytes;
}
inline bool encoded(const std::string& s,size_t bytes){try{return un64(s,bytes).size()==bytes;}catch(...){return false;}}
inline std::string sha(const std::string& s){unsigned char out[32];crypto_hash_sha256(out,reinterpret_cast<const unsigned char*>(s.data()),s.size());return b64(out,32);}
// Reject duplicate keys (including nested objects) rather than accepting the last value.
inline Json parse(const std::string& data,size_t limit=16384){
 require(!data.empty()&&data.size()<=limit);std::vector<std::set<std::string>> objects;bool duplicate=false;
 auto value=Json::parse(data,[&](int depth,Json::parse_event_t event,Json& item){
  require(depth<=12);
  if(event==Json::parse_event_t::object_start)objects.emplace_back();
  if(event==Json::parse_event_t::key){require(!objects.empty());if(!objects.back().insert(item.get<std::string>()).second)duplicate=true;}
  if(event==Json::parse_event_t::object_end)objects.pop_back();
  return true;
 });require(!duplicate&&value.is_object());return value;
}
inline int64_t integer(const Json& j,const char* key){const auto& v=j.at(key);require(v.is_number_integer());if(v.is_number_unsigned())require(v.get<uint64_t>()<=uint64_t(INT64_MAX));return v.get<int64_t>();}
inline std::string string(const Json& j,const char* key,size_t maximum=512){const auto& v=j.at(key);require(v.is_string());auto s=v.get<std::string>();require(s.size()<=maximum);return s;}
struct Session {
 std::array<unsigned char,32> pub{};std::array<unsigned char,64> secret{};
 std::string challenge,thumb;Account account;Clock start{};int64_t challenge_exp=0;uint64_t generation=0;bool pending=false;
 Session()=default;Session(const Session&)=delete;Session& operator=(const Session&)=delete;
 ~Session(){sodium_memzero(secret.data(),secret.size());}
 void begin(Account a,Clock t,uint64_t g){require(a.valid());ssc_auth_privacy::secrets_used=true;require(crypto_sign_keypair(pub.data(),secret.data())==0);account=a;start=t;generation=g;pending=true;
  thumb=sha("{\"crv\":\"Ed25519\",\"kty\":\"OKP\",\"x\":\""+b64(pub.data(),pub.size())+"\"}");}
 std::string request()const{return Json{{"client_public_key",b64(pub.data(),pub.size())}}.dump();}
 bool fresh(Clock t)const{return pending&&t.wall>=start.wall&&t.mono>=start.mono&&t.mono-start.mono<60000&&t.wall<challenge_exp;}
 std::string accept_challenge(const std::string& body,Clock t){auto j=parse(body);challenge=string(j,"challenge",32);require(encoded(challenge,24));
  auto identity=string(j,"identity",64);require(identity=="ssc-bot-v1:"+challenge&&integer(j,"app_id")==account.app);
  challenge_exp=integer(j,"expires_at");require(challenge_exp>t.wall&&challenge_exp-t.wall<=60&&fresh(t));return identity;}
 std::string exchange(const std::vector<unsigned char>& ticket,Clock t){require(fresh(t)&&!ticket.empty()&&ticket.size()<=2560);unsigned char digest[32],signature[64];
  crypto_hash_sha256(digest,ticket.data(),ticket.size());auto message="ssc-auth-v1\n"+challenge+"\n"+b64(digest,32);
  require(crypto_sign_detached(signature,nullptr,reinterpret_cast<const unsigned char*>(message.data()),message.size(),secret.data())==0);
  std::string hex(ticket.size()*2,0);constexpr char digits[]="0123456789abcdef";for(size_t i=0;i<ticket.size();++i){hex[2*i]=digits[ticket[i]>>4];hex[2*i+1]=digits[ticket[i]&15];}
  auto body=Json{{"challenge",challenge},{"ticket",hex},{"proof",b64(signature,64)}}.dump();sodium_memzero(hex.data(),hex.size());require(body.size()<=8192);return body;
 }
};
using Keys=std::map<std::string,std::array<unsigned char,32>>;
inline Keys read_keys(const std::string& body){auto j=parse(body);const auto& list=j.at("keys");require(list.is_array()&&!list.empty()&&list.size()<=16);Keys result;
 for(const auto& k:list){require(string(k,"kty")=="OKP"&&string(k,"crv")=="Ed25519"&&!k.contains("d"));
  if(k.contains("alg"))require(string(k,"alg")=="EdDSA");
  if(k.contains("use"))require(string(k,"use")=="sig");
  auto kid=string(k,"kid",128);require(!kid.empty()&&!result.count(kid));auto key=un64(string(k,"x",43),32);require(key.size()==32);std::array<unsigned char,32> a{};std::copy(key.begin(),key.end(),a.begin());result.emplace(kid,a);
 }return result;
}
struct Grant {Account account;int64_t issued=0,expires=0;uint64_t deadline=0;std::string id;};
inline Grant verify(const std::string& body,const Keys& keys,Session& s,Account current,Clock now,uint64_t generation,bool license=false){
 require(s.pending&&s.generation==generation&&s.account==current&&current.valid()&&s.fresh(now));
 auto response=parse(body);auto jwt=string(response,"authorization",8192);auto p=jwt.find('.'),q=jwt.find('.',p==std::string::npos?0:p+1);
 require(p!=std::string::npos&&q!=std::string::npos&&jwt.find('.',q+1)==std::string::npos);
 auto h=un64(jwt.substr(0,p),1024);auto header=parse(std::string(h.begin(),h.end()));
 require(header.size()==3&&string(header,"alg")=="EdDSA"&&string(header,"typ")=="JWT");
 auto key=keys.find(string(header,"kid",128));require(key!=keys.end());auto sig=un64(jwt.substr(q+1),64);require(sig.size()==64);
 require(crypto_sign_verify_detached(sig.data(),reinterpret_cast<const unsigned char*>(jwt.data()),q,key->second.data())==0);
 auto payload=un64(jwt.substr(p+1,q-p-1),6144);auto claims=parse(std::string(payload.begin(),payload.end()));
 require(string(claims,"iss")==issuer&&string(claims,"aud")==(license?"ssc-bot-license:":"ssc-mod:")+std::to_string(current.app)&&integer(claims,"appid")==current.app);
 require(string(claims,"sub")==std::to_string(current.id)&&string(claims,"permission")=="bot-highlighting");
 if(license){require(string(claims,"auth_method")=="activation_code_device_key"&&claims.at("steam_identity_verified").is_boolean()&&!claims.at("steam_identity_verified").get<bool>());}
 require(string(claims,"challenge")==s.challenge&&string(claims.at("cnf"),"jkt")==s.thumb);
 auto iat=integer(claims,"iat"),nbf=integer(claims,"nbf"),exp=integer(claims,"exp");
 require(iat>=s.start.wall&&nbf==iat&&iat<=now.wall&&now.wall<exp&&exp-iat>0&&exp-iat<=120);
 auto id=string(claims,"jti",22);require(encoded(id,16));
 // Captured before network activity; neither slow responses nor clock rollback extend this deadline.
 require(exp>s.start.wall&&exp-s.start.wall<=180);
 uint64_t deadline=s.start.mono+uint64_t(exp-s.start.wall)*1000-999; // Epoch seconds were floored: expire conservatively
 require(now.mono<deadline);s.pending=false;sodium_memzero(s.secret.data(),s.secret.size());
 return {current,iat,exp,deadline,id};
}
// All visibility and behavior consult this same state. Caller serializes access.
class Access {
 Account account_{};Grant grant_{};Clock last_{};bool lease_=false,enabled_=false,seen_=false;uint64_t generation_=0;
 std::map<std::string,int64_t> accepted_;
public:
 void clear(){lease_=enabled_=false;grant_={};++generation_;}
 uint64_t generation()const{return generation_;}
 Account account()const{return account_;}
 void observe(Account a,Clock t){
  bool backwards=seen_&&(t.wall<last_.wall||t.mono<last_.mono);
  if(a!=account_||!a.valid()||backwards){clear();account_=a;}
  last_=t;seen_=true;
  if(lease_&&(t.wall<grant_.issued||t.wall>=grant_.expires||t.mono>=grant_.deadline))clear();
  for(auto i=accepted_.begin();i!=accepted_.end();)if(i->second<=t.wall)i=accepted_.erase(i);else ++i;
 }
 bool install(const Grant& g,uint64_t generation,Clock t){observe(account_,t);if(generation!=generation_||g.account!=account_||!account_.valid()||accepted_.count(g.id)||t.wall<g.issued||t.wall>=g.expires||t.mono>=g.deadline)return false;
  if(accepted_.size()>=128)return false;
  accepted_[g.id]=g.expires;grant_=g;lease_=true;return true;
 }
 bool visible(Account a,Clock t){observe(a,t);return lease_;}
 bool active(Account a,Clock t){return visible(a,t)&&enabled_;}
 void enable(bool value,Account a,Clock t){enabled_=visible(a,t)&&value;}
};
}
