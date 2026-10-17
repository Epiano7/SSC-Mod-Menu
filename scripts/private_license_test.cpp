#include "../runtime/private_auth.h"
#include <cassert>
#include <functional>
#include <iostream>
using namespace ssc_auth;
using namespace ssc_auth::license;
namespace {
std::string iso(int64_t n){std::tm t{};_gmtime64_s(&t,&n);char b[32];strftime(b,sizeof b,"%Y-%m-%dT%H:%M:%SZ",&t);return b;}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(...){rejected=true;}assert(rejected);}
struct Server {
 std::array<unsigned char,32> pub{};std::array<unsigned char,64> secret{};Clock now{1800000000,100000};Account account{76561198000000001ULL,308600,true};
 std::string nonce,device,purpose,code="SSCB-"+std::string(43,'A');bool enrolled=false,drop=false,bad_sig=false,revoked=false;unsigned error=0;int activation=0,renewal=0;int challenge_skew=0;std::function<void(Json&)> mutate;
 Server(){crypto_sign_keypair(pub.data(),secret.data());}~Server(){sodium_memzero(secret.data(),64);}
 ssc_beta::Reply request(const wchar_t* route,const std::string& body){std::wstring path=route;
  assert(path!=L"/readyz"&&path!=L"/v1/authorize"&&path!=L"/v1/challenge");
  if(error)return {error,"<html>proxy error</html>"};
  if(path==L"/v2/license/status")return {200,Json{{"ok",true},{"api_version",2},{"app_id",308600},{"authentication","activation_code_device_key"},{"steam_identity_verified",false},{"permission","bot-highlighting"},{"max_token_ttl_seconds",120}}.dump()};
  if(path==L"/v1/keys")return {200,Json{{"keys",Json::array({Json{{"kty","OKP"},{"crv","Ed25519"},{"kid","fixture"},{"x",b64(pub.data(),32)}}})}}.dump()};
  auto j=parse(body);
  if(path==L"/v2/license/challenge"){
   assert(j.size()==3&&string(j,"steam_id")==std::to_string(account.id));device=string(j,"device_public_key");purpose=string(j,"purpose");unsigned char random[24];randombytes_buf(random,24);nonce=b64(random,24);
   return {200,Json{{"ok",true},{"api_version",2},{"challenge",nonce},{"purpose",purpose},{"expires_at",iso(now.wall+60+challenge_skew)},{"app_id",308600},{"authentication","activation_code_device_key"},{"steam_identity_verified",false}}.dump()};
  }
  bool activate=path==L"/v2/license/activate";assert(activate||path==L"/v2/license/renew");assert(j.size()==(activate?3:2));assert(purpose==(activate?"activate":"renew"));assert(string(j,"challenge")==nonce);auto used=nonce;nonce.clear();
  auto msg="ssc-bot-license-v2\n"+purpose+"\n"+used+"\n"+(activate?sha(string(j,"code")):"-");auto pk=un64(device,32),sig=un64(string(j,"proof"),64);assert(crypto_sign_verify_detached(sig.data(),reinterpret_cast<const unsigned char*>(msg.data()),msg.size(),pk.data())==0);
  if(activate){++activation;if(enrolled||string(j,"code")!=code)return {403,R"({"code":"activation_denied"})"};enrolled=true;}else{++renewal;if(!enrolled||revoked)return {403,R"({"code":"device_denied"})"};}
  if(drop){drop=false;throw std::runtime_error("lost response");}
  unsigned char r[16];randombytes_buf(r,16);auto thumb=sha("{\"crv\":\"Ed25519\",\"kty\":\"OKP\",\"x\":\""+device+"\"}");
  Json claims={{"iss",issuer},{"aud","ssc-bot-license:308600"},{"appid",308600},{"sub",std::to_string(account.id)},{"permission","bot-highlighting"},{"auth_method","activation_code_device_key"},{"steam_identity_verified",false},{"challenge",used},{"iat",now.wall},{"nbf",now.wall},{"exp",now.wall+120},{"jti",b64(r,16)},{"cnf",{{"jkt",thumb}}}};if(mutate)mutate(claims);
  auto h=Json{{"alg","EdDSA"},{"typ","JWT"},{"kid","fixture"}}.dump(),c=claims.dump();auto jwt=b64(reinterpret_cast<const unsigned char*>(h.data()),h.size())+"."+b64(reinterpret_cast<const unsigned char*>(c.data()),c.size());unsigned char sign[64];crypto_sign_detached(sign,nullptr,reinterpret_cast<const unsigned char*>(jwt.data()),jwt.size(),secret.data());if(bad_sig)sign[0]^=1;jwt+="."+b64(sign,64);
  return {200,Json{{"ok",true},{"api_version",2},{"authorization",jwt},{"expires_at",iso(now.wall+120)},{"device_id",thumb},{"authentication","activation_code_device_key"},{"steam_identity_verified",false}}.dump()};
 }
};
Grant run(Server& s,Device& d,std::string code){return license::exchange(s,d,code,s.account,0,[&]{return s.now;},[&]{return s.account;});}
}
namespace {
std::mutex fixture_mutex;Server fixture;std::atomic<uint64_t> local_id{76561198000000001ULL};std::array<unsigned char,32> durable{};bool stored=false;int saves=0;std::atomic<bool> stall{false},entered{false},release_request{false};
struct FakeSteam {bool attach(){return true;}Account account()const{auto id=local_id.load();return {id,308600,id!=0};}void shutdown(){}};
struct FakeStore {std::filesystem::path directory;bool load(uint64_t,Device& d,bool create){std::lock_guard<std::mutex> l(fixture_mutex);if(!stored&&!create)return false;if(!stored){randombytes_buf(durable.data(),32);stored=true;++saves;}d.seed=durable;d.derive();return true;}};
struct FakeNetwork {ssc_beta::Reply request(const wchar_t* p,const std::string& b){
 if(stall){entered=true;while(!release_request)Sleep(5);}
 std::lock_guard<std::mutex> l(fixture_mutex);fixture.now=clock_now();assert(stored);return fixture.request(p,b);
 }void cancel(){release_request=true;}void stop(){release_request=true;}};
using Worker=license::Service<FakeSteam,FakeNetwork,FakeStore>;
template<class F> void until(F ready){auto deadline=GetTickCount64()+5000;while(!ready()&&GetTickCount64()<deadline)Sleep(10);assert(ready());}
Worker* worker(){auto* w=new Worker;w->configure("fixture-only");w->start();return w;}
void lifecycle(){
 auto* first=worker();until([&]{return !first->busy();});assert(!first->available());
 assert(first->request("  "+fixture.code+"  "));until([&]{return first->available();});assert(first->active()&&saves==1&&first->cleared()>0);first->stop();assert(!first->available());Sleep(150);
 int before;{std::lock_guard<std::mutex> l(fixture_mutex);before=fixture.activation;}
 auto* second=worker();until([&]{return second->available();});assert(second->active());{std::lock_guard<std::mutex> l(fixture_mutex);assert(fixture.activation==before&&fixture.renewal>=1);}
 local_id=0;assert(!second->available()&&!second->active());second->enable(true);assert(!second->active());second->stop();Sleep(150);local_id=76561198000000001ULL;
 // An activation succeeds on the server but its reply is lost: same saved key,
 // one redemption, immediate renewal, no second code required.
 {std::lock_guard<std::mutex> l(fixture_mutex);fixture.enrolled=false;fixture.drop=true;stored=false;}
 auto* lost=worker();assert(lost->request(fixture.code));until([&]{return lost->available();});assert(saves==2);lost->stop();Sleep(150);
 // Pending renewal must not install after logout, even if its reply is valid.
 stall=true;release_request=false;entered=false;auto* stale=worker();until([]{return entered.load();});local_id=0;assert(!stale->available());release_request=true;until([&]{return !stale->busy();});assert(!stale->available());stale->stop();Sleep(150);stall=false;local_id=76561198000000001ULL;
 // Non-JSON timeout/proxy failures and rate limiting remain closed.
 {std::lock_guard<std::mutex> l(fixture_mutex);fixture.error=429;}
 auto* limited=worker();until([&]{return limited->state()==State::limited;});assert(!limited->available());limited->stop();Sleep(150);
 {std::lock_guard<std::mutex> l(fixture_mutex);fixture.error=503;}
 auto* outage=worker();until([&]{return outage->state()==State::unavailable;});assert(!outage->available());outage->stop();Sleep(150);
}
}
int main(int argc,char** argv){assert(argc==2&&sodium_init()>=0);Device d;randombytes_buf(d.seed.data(),32);d.derive();
 auto hinted=[](int64_t when){auto c=Json{{"iat",when}}.dump();return Json{{"authorization","e30."+b64(reinterpret_cast<const unsigned char*>(c.data()),c.size())+".invalid"}}.dump();};
 assert(pending_issue_time(hinted(101),100)==101);assert(pending_issue_time(hinted(103),100)==100);assert(pending_issue_time("not JSON",100)==100);
 Server s;auto grant=run(s,d,s.code);assert(s.activation==1&&grant.account==s.account);auto renewed=run(s,d,"");assert(s.renewal==1&&renewed.id!=grant.id);
 Access gate;gate.observe(s.account,s.now);assert(gate.install(grant,gate.generation(),s.now));gate.enable(true,s.account,s.now);assert(gate.active(s.account,s.now));assert(!gate.install(grant,gate.generation(),s.now));assert(!gate.active(s.account,{s.now.wall,s.now.mono+120000}));gate.enable(true,s.account,s.now);assert(!gate.active(s.account,s.now));
 for(auto mutate:std::vector<std::function<void(Json&)>>{
 [](Json& c){c["aud"]="ssc-mod:308600";},[](Json& c){c["iss"]="https://evil";},[](Json& c){c["sub"]="other";},[](Json& c){c["appid"]=480;},[](Json& c){c["permission"]="beta";},[](Json& c){c["auth_method"]="steam_ticket";},[](Json& c){c["steam_identity_verified"]=true;},[](Json& c){c["steam_identity_verified"]=0;},[](Json& c){c["challenge"]="other";},[](Json& c){c["cnf"]["jkt"]="wrong";},[](Json& c){c["exp"]=1800000121;},[](Json& c){c["exp"]=1800000000;},[](Json& c){c["nbf"]=1800000001;},[](Json& c){c["jti"]="bad";}}){Server bad;bad.mutate=mutate;rejects([&]{run(bad,d,bad.code);});}
 {Server skew;skew.challenge_skew=1;auto g=run(skew,d,skew.code);assert(g.expires==skew.now.wall+120);}
 {Server bad;bad.challenge_skew=3;rejects([&]{run(bad,d,bad.code);});}
 {Server bad;bad.bad_sig=true;rejects([&]{run(bad,d,bad.code);});}
 for(unsigned status:{400,401,403,404,413,429,503}){Server bad;bad.error=status;rejects([&]{run(bad,d,bad.code);});}
 rejects([&]{run(s,d,s.code);});s.revoked=true;rejects([&]{run(s,d,"");});
 {Server lost;lost.drop=true;rejects([&]{run(lost,d,lost.code);});assert(lost.enrolled);auto g=run(lost,d,"");assert(g.account==lost.account&&lost.activation==1);}
 {Server wrong;rejects([&]{run(wrong,d,"SSCB-"+std::string(43,'B'));});}
 {Server stale;rejects([&]{license::exchange(stale,d,stale.code,stale.account,0,[&]{return stale.now;},[]{return Account{};});});}
 {Access a;a.observe(s.account,s.now);assert(a.install(grant,a.generation(),s.now));a.enable(true,s.account,s.now);assert(!a.active(s.account,{s.now.wall-1,s.now.mono+1}));}
 {Access a;a.observe(s.account,s.now);auto generation=a.generation();a.observe({},s.now);assert(!a.install(grant,generation,s.now));a.enable(true,s.account,s.now);assert(!a.active(s.account,s.now));}
 std::string padded=" \t"+s.code+"\r\n";trim(padded);assert(padded==s.code&&code_valid(padded));assert(!code_valid("sscb-"+std::string(43,'A')));
 Store store;store.directory=argv[1];std::filesystem::create_directories(store.directory);Device saved;assert(!store.load(s.account.id,saved,false));assert(store.load(s.account.id,saved,true));Device again;assert(store.load(s.account.id,again,false)&&saved.pub==again.pub);Device other;assert(!store.load(s.account.id+1,other,false));
 auto file=store.directory/(L"private-device-"+std::to_wstring(s.account.id)+L".bin");std::ifstream in(file,std::ios::binary);std::string raw((std::istreambuf_iterator<char>(in)),{});assert(raw.find(b64(saved.seed.data(),32))==std::string::npos);
 assert(!ssc_beta::service().request(s.code)&&!ssc_beta::available()&&!ssc_beta::active());
 {Server scoped;scoped.mutate=[](Json& c){c["permission"]="draft-advisor";};auto g=run(scoped,d,scoped.code);assert(g.draft_only);
  Access a;a.observe(scoped.account,scoped.now);assert(a.install(g,a.generation(),scoped.now));a.enable(true,scoped.account,scoped.now);
  assert(a.visible(scoped.account,scoped.now)&&!a.bot_visible(scoped.account,scoped.now)&&!a.active(scoped.account,scoped.now));
  assert(run(scoped,d,"").draft_only);assert(!a.visible(scoped.account,{scoped.now.wall+121,scoped.now.mono+121000}));}
 lifecycle();
 {std::lock_guard<std::mutex> l(fixture_mutex);fixture.error=0;fixture.mutate=[](Json& c){c["permission"]="draft-advisor";};}
 auto* scopedWorker=worker();until([&]{return scopedWorker->draft_available();});assert(!scopedWorker->available()&&!scopedWorker->active());
 scopedWorker->enable(true);assert(!scopedWorker->active()&&scopedWorker->state()==State::accepted);
 scopedWorker->stop();assert(!scopedWorker->draft_available());Sleep(150);
 std::cout<<"PASS: service activation/restart/lost-response/stale/shutdown, v2 proof/signatures/claims, activation/renewal, lost response recovery, errors, revocation, replay, expiry/clocks/account gates, DPAPI persistence/account separation, beta closed\n";
}
