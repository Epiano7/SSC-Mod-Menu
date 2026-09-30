#pragma once
#include "beta_auth.h"
namespace ssc_auth {
namespace license {
inline void wipe(std::string& s){ssc_beta::wipe(s);}
enum class State {idle,checking,accepted,expired,code_denied,device_denied,unavailable,limited,storage,account};
inline const wchar_t* label(State s){switch(s){case State::expired:return L"Access expired - waiting for renewal";case State::checking:return L"Checking private access...";case State::accepted:return L"Private access activated";case State::code_denied:return L"Invalid, expired or already used code";case State::device_denied:return L"Account or device not approved";case State::unavailable:return L"Server unavailable - try again later";case State::limited:return L"Rate limited - please wait before retrying";case State::storage:return L"Secure device storage unavailable";case State::account:return L"Sign in to Steam to activate";default:return L"Enter your private activation code";}}
struct Failure {State state;};
inline void ok(const ssc_beta::Reply& r){if(r.status==200)return;State s=State::unavailable;if(r.status==429)s=State::limited;
 try{auto j=parse(r.body);auto c=string(j,"code",64);if(c=="activation_denied")s=State::code_denied;else if(c=="denied"||c=="device_denied"||c=="device_limit")s=State::device_denied;else if(c=="rate_limited"||c=="busy")s=State::limited;}catch(...){}throw Failure{s};}
inline void metadata(const Json& j){require(j.at("ok").is_boolean()&&j.at("ok").get<bool>()&&integer(j,"api_version")==2&&string(j,"authentication")=="activation_code_device_key"&&j.at("steam_identity_verified").is_boolean()&&!j.at("steam_identity_verified").get<bool>());}
inline bool code_valid(const std::string& c){return c.size()==48&&c.substr(0,5)=="SSCB-"&&encoded(c.substr(5),32);}
inline void trim(std::string& c){auto a=c.find_first_not_of(" \t\r\n"),b=c.find_last_not_of(" \t\r\n");if(a==std::string::npos){wipe(c);return;}auto t=c.substr(a,b-a+1);wipe(c);c.swap(t);}
struct Device {
 std::array<unsigned char,32> pub{},seed{};std::array<unsigned char,64> secret{};
 Device()=default;Device(const Device&)=delete;Device& operator=(const Device&)=delete;
 ~Device(){sodium_memzero(seed.data(),seed.size());sodium_memzero(secret.data(),secret.size());}
 void derive(){ssc_auth_privacy::secrets_used=true;require(crypto_sign_seed_keypair(pub.data(),secret.data(),seed.data())==0);}
 std::string thumb()const{return sha("{\"crv\":\"Ed25519\",\"kty\":\"OKP\",\"x\":\""+b64(pub.data(),32)+"\"}");}
};
// No Proton DPAPI equivalence claim: device enrollment is blocked until a native
// secret-store/permissions bridge has been validated for that environment.
struct Store {
 std::filesystem::path directory;
 bool supported()const{return !GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"wine_get_version");}
 bool load(uint64_t id,Device& key,bool create){
  if(!supported())throw Failure{State::storage};
  ssc_beta::Storage storage;storage.description=L"SSC private device";storage.path=directory/(L"private-device-"+std::to_wstring(id)+L".bin");
  std::error_code ec;bool exists=std::filesystem::exists(storage.path,ec);if(ec)throw Failure{State::storage};
  if(exists){auto saved=storage.load();try{require(saved.account==id);auto bytes=un64(saved.token,32);require(bytes.size()==32);std::copy(bytes.begin(),bytes.end(),key.seed.begin());sodium_memzero(bytes.data(),bytes.size());wipe(saved.token);key.derive();return true;}catch(...){wipe(saved.token);throw Failure{State::storage};}}
  if(!create)return false;
  randombytes_buf(key.seed.data(),32);key.derive();ssc_beta::Stored saved{id,b64(key.seed.data(),32)};
  bool good=storage.save(saved);auto roundtrip=good?storage.load():ssc_beta::Stored{};good=good&&roundtrip.account==id&&roundtrip.token==saved.token;wipe(saved.token);wipe(roundtrip.token);if(!good)throw Failure{State::storage};return true;
 }
};
// An untrusted timestamp may only delay validation, never grant access. This
// handles a server clock up to two seconds ahead without relaxing iat <= now.
inline int64_t pending_issue_time(const std::string& body,int64_t now){try{auto envelope=parse(body);auto jwt=string(envelope,"authorization",8192);auto a=jwt.find('.'),b=jwt.find('.',a==std::string::npos?0:a+1);require(a!=std::string::npos&&b!=std::string::npos);auto bytes=un64(jwt.substr(a+1,b-a-1),6144);auto claims=parse(std::string(bytes.begin(),bytes.end()));auto iat=integer(claims,"iat");return iat>now&&iat-now<=2?iat:now;}catch(...){return now;}}
struct Network {
 ssc_beta::Http http;std::atomic<bool> cancelled{false};
 Network(){http.host=L"sscmmauth.epiano7.dev";}
 ssc_beta::Reply request(const wchar_t* p,const std::string& b){require(b.size()<=8192);cancelled=false;auto reply=http.request(p,b,"");
  if(reply.status==200&&(std::wstring(p)==L"/v2/license/activate"||std::wstring(p)==L"/v2/license/renew")){
   auto due=pending_issue_time(reply.body,clock_now().wall);auto until=GetTickCount64()+2500;
   while(!cancelled&&clock_now().wall<due&&GetTickCount64()<until)Sleep(20);
  }return reply;
 }
 void cancel(){cancelled=true;http.cancel();}void stop(){cancelled=true;http.stop();}
};
struct Identity {ssc_beta::Identity steam;bool attach(){return steam.attach();}Account account()const{auto id=steam.read();return {id,expected_app,id!=0};}void shutdown(){}};
// Shared production grant validator and Access state remain the sole bot gate.
template<class Net,class Now,class Current>
Grant exchange(Net& net,Device& key,const std::string& code,Account a,uint64_t generation,Now now,Current current){
 auto status=net.request(L"/v2/license/status","");ok(status);auto st=parse(status.body);metadata(st);require(integer(st,"app_id")==expected_app&&string(st,"permission")=="bot-highlighting"&&integer(st,"max_token_ttl_seconds")==120);require(current()==a);
 auto public_keys=net.request(L"/v1/keys","");ok(public_keys);auto keys=read_keys(public_keys.body);require(current()==a);
 Session session;session.account=a;session.start=now();session.generation=generation;session.pending=true;session.pub=key.pub;session.thumb=key.thumb();
 const std::string purpose=code.empty()?"renew":"activate";
 auto response=net.request(L"/v2/license/challenge",Json{{"steam_id",std::to_string(a.id)},{"device_public_key",b64(key.pub.data(),32)},{"purpose",purpose}}.dump());ok(response);require(current()==a);
 auto j=parse(response.body);metadata(j);require(integer(j,"app_id")==expected_app&&string(j,"purpose")==purpose);session.challenge=string(j,"challenge",32);require(encoded(session.challenge,24));session.challenge_exp=ssc_beta::expiration(j.at("expires_at"));auto t=now();require(session.challenge_exp>t.wall&&session.challenge_exp-t.wall<=62);
 // Server/local epoch-second boundaries can differ slightly. Never extend
 // the local 60-second wall/monotonic challenge lifetime to match server time.
 session.challenge_exp=std::min(session.challenge_exp,session.start.wall+60);require(session.fresh(t));
 auto message="ssc-bot-license-v2\n"+purpose+"\n"+session.challenge+"\n"+(code.empty()?"-":sha(code));unsigned char signature[64];require(crypto_sign_detached(signature,nullptr,reinterpret_cast<const unsigned char*>(message.data()),message.size(),key.secret.data())==0);
 Json payload={{"challenge",session.challenge},{"proof",b64(signature,64)}};if(!code.empty())payload["code"]=code;auto body=payload.dump();payload.clear();
 struct Clean {std::string& s;~Clean(){wipe(s);}} clean{body};
 require(current()==a&&session.fresh(now()));response=net.request(code.empty()?L"/v2/license/renew":L"/v2/license/activate",body);Clean token{response.body};ok(response);require(current()==a);
 auto envelope=parse(response.body);metadata(envelope);require(string(envelope,"device_id",43)==session.thumb);auto grant=verify(response.body,keys,session,a,now(),generation,true);require(ssc_beta::expiration(envelope.at("expires_at"))==grant.expires);return grant;
}
template<class SteamType=Identity,class NetworkType=Network,class StoreType=Store> class Service {
 std::mutex mutex_;std::condition_variable wake_;Access access_;SteamType steam_;NetworkType http_;StoreType store_;std::atomic<bool> stopping_{false};bool started_=false,queued_=false,busy_=false;std::string code_;uint64_t queued_account_=0,clear_code_=0,retry_after_=0;State state_=State::idle;
 void observe(){access_.observe(steam_.account(),clock_now());}
 void work(){Account previous{};uint64_t next=0;unsigned failures=0;
  while(!stopping_){Account account;uint64_t generation;std::string code;
   {std::unique_lock<std::mutex> l(mutex_);wake_.wait_for(l,std::chrono::milliseconds(100),[&]{return stopping_||queued_;});if(stopping_)break;observe();account=steam_.account();if(account!=previous){if(previous.valid()){++clear_code_;wipe(code_);queued_=false;}previous=account;next=0;state_=account.valid()?State::idle:State::account;}
    if(!account.valid())continue;
    if(!queued_&&GetTickCount64()<next)continue;
    if(queued_){queued_=false;if(queued_account_!=account.id){wipe(code_);++clear_code_;continue;}code.swap(code_);}
    generation=access_.generation();busy_=true;state_=State::checking;
   }
   bool success=false,have=false;State result=State::unavailable;Grant grant;Device key;
   auto current=[&]{std::lock_guard<std::mutex> l(mutex_);observe();require(!stopping_&&access_.generation()==generation);return steam_.account();};
   try{have=store_.load(account.id,key,!code.empty());if(!have)result=State::idle;else{
     try{grant=exchange(http_,key,code,account,generation,clock_now,current);success=true;}catch(...){
      if(code.empty())throw;
      // The saved key survives every failure. First try renewal with a fresh
      // challenge: the one-use code may already have enrolled this device.
      auto error=std::current_exception();try{std::rethrow_exception(error);}catch(const Failure& f){if(f.state!=State::unavailable)throw;}catch(...){}
      try{grant=exchange(http_,key,"",account,generation,clock_now,current);success=true;}catch(...){std::rethrow_exception(error);}
     }
    }}catch(const Failure& f){result=f.state;}catch(...){result=State::unavailable;}
   bool submitted=!code.empty();wipe(code);
   {std::lock_guard<std::mutex> l(mutex_);observe();busy_=false;
    if(!stopping_&&generation==access_.generation()&&account==steam_.account()){
     if(success){bool already=access_.visible(account,clock_now());success=access_.install(grant,generation,clock_now());if(success){if(!already)access_.enable(true,account,clock_now());state_=State::accepted;++clear_code_;}}
     if(!success){state_=result;if(result==State::limited)retry_after_=GetTickCount64()+60000;}
    }else if(submitted)++clear_code_;
   }
   if(success){failures=0;next=GetTickCount64()+55000+randombytes_uniform(10001);}else if(!have&&!submitted){next=GetTickCount64()+60000;}else{next=GetTickCount64()+std::min(60000u,5000u<<std::min(failures++,4u))+randombytes_uniform(1001);}
  }
 }
public:
 void configure(const std::filesystem::path& path){std::lock_guard<std::mutex> l(mutex_);if(!started_)store_.directory=path;}
 void start(){std::lock_guard<std::mutex> l(mutex_);if(started_||stopping_||store_.directory.empty())return;if(sodium_init()<0||!steam_.attach())return;started_=true;try{std::thread([this]{work();}).detach();}catch(...){started_=false;state_=State::unavailable;}}
 bool request(std::string code){trim(code);std::lock_guard<std::mutex> l(mutex_);observe();if(GetTickCount64()<retry_after_){wipe(code);state_=State::limited;return false;}if(stopping_||busy_||queued_){wipe(code);return false;}if(!code_valid(code)){wipe(code);state_=State::code_denied;return false;}if(!started_||!steam_.account().valid()){wipe(code);state_=State::account;return false;}ssc_auth_privacy::secrets_used=true;code_.swap(code);wipe(code);queued_account_=steam_.account().id;queued_=true;state_=State::checking;wake_.notify_all();return true;}
 bool available(){std::lock_guard<std::mutex> l(mutex_);return started_&&!stopping_&&access_.visible(steam_.account(),clock_now());}
 bool active(){std::lock_guard<std::mutex> l(mutex_);return started_&&!stopping_&&access_.active(steam_.account(),clock_now());}
 void enable(bool on){std::lock_guard<std::mutex> l(mutex_);access_.enable(started_&&!stopping_&&on,steam_.account(),clock_now());}
 State state(){std::lock_guard<std::mutex> l(mutex_);observe();if(state_==State::accepted&&!access_.visible(steam_.account(),clock_now()))state_=State::expired;return state_;}
 bool busy(){std::lock_guard<std::mutex> l(mutex_);return busy_||queued_;}
 uint64_t cleared(){std::lock_guard<std::mutex> l(mutex_);return clear_code_;}
 void invalidate(){std::lock_guard<std::mutex> l(mutex_);access_.clear();wipe(code_);queued_=false;++clear_code_;http_.cancel();wake_.notify_all();}
 void stop(){stopping_=true;invalidate();http_.stop();steam_.shutdown();wake_.notify_all();}
};
}
using Service=license::Service<>;
inline Service& service(){static auto* s=new Service;return *s;}
inline bool available(){return service().available();}inline bool active(){return service().active();}
}
