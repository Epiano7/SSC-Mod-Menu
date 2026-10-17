#include "../runtime/private_auth.h"
#include <cassert>
#include <iostream>
#include <functional>
using namespace ssc_auth;
template<class F> void rejected(F f){bool no=false;try{f();}catch(...){no=true;}assert(no);}
struct Server {
 unsigned char pub[32]{},secret[64]{};std::string nonce;Clock t{1800000000,100000};Account a{76561198000000001ULL,308600,true};
 Json claims;unsigned challenge_status=200,authorize_status=200,key_status=200;int challenges=0,authorizations=0;
 std::function<void(Json&)> mutate;bool bad_signature=false,drop=false;std::string client;
 Server(){assert(crypto_sign_keypair(pub,secret)==0);unsigned char random[24];randombytes_buf(random,24);nonce=b64(random,24);}
 ~Server(){sodium_memzero(secret,64);}
 std::string token(Json c){auto header=Json{{"alg","EdDSA"},{"typ","JWT"},{"kid","test"}}.dump();auto payload=c.dump();auto msg=b64((const unsigned char*)header.data(),header.size())+"."+b64((const unsigned char*)payload.data(),payload.size());unsigned char sig[64];assert(crypto_sign_detached(sig,nullptr,(const unsigned char*)msg.data(),msg.size(),secret)==0);if(bad_signature)sig[0]^=1;return msg+"."+b64(sig,64);}
 Response request(const wchar_t* path,const std::string& body){
  std::wstring route=path;
  if(route==L"/v1/challenge"){
   ++challenges;if(challenge_status!=200)return {challenge_status,"not JSON"};auto j=parse(body);assert(j.size()==1);client=string(j,"client_public_key");assert(encoded(client,32));
   return {200,Json{{"challenge",nonce},{"identity","ssc-bot-v1:"+nonce},{"expires_at",t.wall+60},{"app_id",a.app}}.dump()};
  }
  if(route==L"/v1/authorize"){
   ++authorizations;if(authorize_status!=200)return {authorize_status,"not JSON"};if(drop)throw std::runtime_error("dropped");auto j=parse(body);assert(j.size()==3&&string(j,"challenge")==nonce);
   std::string hex=string(j,"ticket");assert(hex=="01020304");std::string binary("\1\2\3\4",4),proof="ssc-auth-v1\n"+nonce+"\n"+sha(binary);
   auto sig=un64(string(j,"proof"),64),pk=un64(client,32);assert(crypto_sign_verify_detached(sig.data(),(const unsigned char*)proof.data(),proof.size(),pk.data())==0);
   unsigned char jti[16];randombytes_buf(jti,16);
   claims={{"iss",issuer},{"aud","ssc-mod:308600"},{"appid",308600},{"sub",std::to_string(a.id)},{"permission","bot-highlighting"},{"challenge",nonce},{"iat",t.wall},{"nbf",t.wall},{"exp",t.wall+120},{"jti",b64(jti,16)},{"cnf",{{"jkt",sha("{\"crv\":\"Ed25519\",\"kty\":\"OKP\",\"x\":\""+client+"\"}")}}}};
   if(mutate)mutate(claims);
   return {200,Json{{"authorization",token(claims)},{"expires_at",t.wall+9999}}.dump()};
  }
  assert(route==L"/v1/keys"&&body.empty());if(key_status!=200)return {key_status,""};return {200,Json{{"keys",Json::array({Json{{"kty","OKP"},{"crv","Ed25519"},{"kid","test"},{"x",b64(pub,32)}}})}}.dump()};
 }
};
struct MockSteam {int cancelled=0,requested=0;bool fail=false;std::string identity;
 template<class Valid> void ticket(const std::string& value,Ticket& t,Valid valid){assert(valid());identity=value;++requested;t.handle=7;if(fail)throw std::runtime_error("Steam callback failure");t.bytes={1,2,3,4};}
 void cancel(uint32_t h){assert(h==7);++cancelled;}
};
Grant run(Server& s,MockSteam& steam){Keys keys;return exchange(s,steam,[&]{return s.t;},[&]{return s.a;},s.a,0,keys);}
int main(){assert(sodium_init()>=0);
 Server good;MockSteam steam;auto grant=run(good,steam);assert(steam.cancelled==1&&steam.identity=="ssc-bot-v1:"+good.nonce&&grant.expires==good.t.wall+120);
 // Every bad signed claim must be rejected, not just malformed tokens.
 std::vector<std::function<void(Json&)>> bad={
  [](Json& c){c["iss"]="https://other";},[](Json& c){c["aud"]="ssc-mod:480";},[](Json& c){c["appid"]=480;},[](Json& c){c["appid"]="308600";},
  [](Json& c){c["sub"]="76561198000000002";},[](Json& c){c["sub"]=76561198000000001ULL;},[](Json& c){c["sub"]="076561198000000001";},
  [](Json& c){c["permission"]="admin";},[](Json& c){c["challenge"]=std::string(32,'A');},[](Json& c){c["cnf"]["jkt"]="wrong";},
  [](Json& c){c["exp"]=c["iat"].get<int64_t>()+121;},[](Json& c){c["exp"]=c["iat"];},[](Json& c){c["nbf"]=c["iat"].get<int64_t>()+1;},
  [](Json& c){c["iat"]=1800000001;c["nbf"]=1800000001;},[](Json& c){c["iat"]=1800000000.0;},[](Json& c){c["jti"]="not-random";},[](Json& c){c.erase("cnf");}
 };
 for(auto& mutate:bad){Server s;MockSteam st;s.mutate=mutate;rejected([&]{run(s,st);});assert(st.cancelled==1);}
 {Server s;MockSteam st;s.bad_signature=true;rejected([&]{run(s,st);});assert(st.cancelled==1);}
 for(unsigned status:{400,401,403,404,413,429,503}){
  Server s;MockSteam st;s.authorize_status=status;rejected([&]{run(s,st);});assert(st.cancelled==1);
  Server c;MockSteam ct;c.challenge_status=status;rejected([&]{run(c,ct);});assert(ct.requested==0&&ct.cancelled==0);
 }
 {Server s;MockSteam st;st.fail=true;rejected([&]{run(s,st);});assert(st.cancelled==1);}
 {Server s;MockSteam st;s.drop=true;rejected([&]{run(s,st);});assert(st.cancelled==1);}
 {Server s;MockSteam st;s.key_status=503;rejected([&]{run(s,st);});assert(st.cancelled==1);}
 {Session session;session.begin(good.a,good.t,2);auto response=good.request(L"/v1/challenge",session.request());session.accept_challenge(response.body,good.t);Keys keys=read_keys(good.request(L"/v1/keys","").body);auto exchange_body=session.exchange({1,2,3,4},good.t);auto result=good.request(L"/v1/authorize",exchange_body);
  rejected([&]{verify(result.body,keys,session,good.a,good.t,1);});auto accepted=verify(result.body,keys,session,good.a,good.t,2);assert(accepted.expires==good.t.wall+120);
  rejected([&]{verify(result.body,keys,session,good.a,good.t,2);});
 }
 rejected([]{parse("{\"a\":1,\"a\":2}");});rejected([]{parse("{\"a\":{\"b\":1,\"b\":2}}");});rejected([]{parse("not json");});rejected([]{un64("AA==",1);});
 rejected([]{parse(std::string(17000,' '));});
 // JWT header confusion, malformed compact encodings and stale sessions.
 for(int variant=0;variant<10;++variant){Server s;Session session;session.begin(s.a,s.t,3);session.accept_challenge(s.request(L"/v1/challenge",session.request()).body,s.t);auto body=s.request(L"/v1/authorize",session.exchange({1,2,3,4},s.t));auto keys=read_keys(s.request(L"/v1/keys","").body);
  auto value=parse(body.body);auto token=string(value,"authorization",8192);auto dot=token.find('.');auto bytes=un64(token.substr(0,dot),1024);auto header=parse(std::string(bytes.begin(),bytes.end()));
  if(variant==0)header["alg"]="none";
  if(variant==1)header["alg"]="HS256";
  if(variant==2)header["jku"]="https://attacker/keys";
  if(variant==3)header["kid"]="unknown";
  if(variant==4)header["typ"]="other";
  if(variant==5)header["crit"]=Json::array({"b64"});
  auto raw=header.dump();token=b64((const unsigned char*)raw.data(),raw.size())+token.substr(dot);
  if(variant==6)token="not.a.jwt";
  if(variant==7)token+=".extra";
  value["authorization"]=token;
  auto account=s.a;if(variant==8)++account.id;
  if(variant==9)s.t.mono+=60000;
  rejected([&]{verify(value.dump(),keys,session,account,s.t,3);});
 }
 {Server s;Session session;session.begin(s.a,s.t,0);auto c=parse(s.request(L"/v1/challenge",session.request()).body);c["app_id"]=480;rejected([&]{session.accept_challenge(c.dump(),s.t);});}
 {Server s;Session session;session.begin(s.a,s.t,0);auto c=parse(s.request(L"/v1/challenge",session.request()).body);c["identity"]="different";rejected([&]{session.accept_challenge(c.dump(),s.t);});}
 Access access;auto a=good.a;auto t=good.t;assert(!access.visible(a,t));auto generation=access.generation();
 access.enable(true,a,t);assert(!access.active(a,t)); // settings/hotkey cannot enable without lease
 assert(access.install(grant,generation,t));assert(access.visible(a,t)&&!access.active(a,t));access.enable(true,a,t);assert(access.active(a,t));
 assert(!access.install(grant,generation,t)); // replayed jti
 assert(access.active(a,{t.wall+119,t.mono+119000})); // outage / revoked server membership cannot extend original lease
 assert(!access.active(a,{t.wall+120,t.mono+120000}));
 assert(!access.visible(a,{t.wall+1,t.mono+121000})); // rollback cannot revive
 auto setup=[&](Access& state){state.observe(a,t);assert(state.install(grant,state.generation(),t));state.enable(true,a,t);};
 {Access state;setup(state);assert(!state.active(a,{t.wall,t.mono+120000}));} // suspend elapsed, wall unchanged
 {Access state;setup(state);assert(!state.active(a,{t.wall+120,t.mono+1}));} // wall elapsed, monotonic unchanged
 {Access state;setup(state);assert(!state.active(a,{t.wall-1,t.mono+1}));} // backward clock
 {Access state;setup(state);assert(!state.active(a,{t.wall,t.mono-1}));}
 {Access state;setup(state);auto other=a;other.id++;assert(!state.active(other,t));assert(!state.active(a,t));}
 {Access state;setup(state);auto offline=a;offline.online=false;assert(!state.active(offline,t));assert(!state.active(a,t));}
 {Access state;setup(state);auto old=state.generation();state.clear();assert(!state.install(grant,old,t));assert(!state.active(a,t));}
 {Access state;state.observe(a,t);auto old=state.generation();state.clear();assert(!state.install(grant,old,t));}
 std::cout<<"PASS: signed protocol, exact proof bytes, bad claims/signatures, denial/outage/revocation expiry, ticket cleanup, replay, stale generations, account/logout, clocks and enable bypass\n";
}
