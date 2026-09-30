#pragma once
#include "private_auth_core.h"
namespace ssc_auth {
struct Response {unsigned status=0;std::string body;};
struct Ticket {
 uint32_t handle=0;std::vector<unsigned char> bytes;
 ~Ticket(){if(!bytes.empty())sodium_memzero(bytes.data(),bytes.size());}
};
// Adapters are supplied at compile time. Production uses only the fixed HTTPS
// transport and genuine Steam adapter; mocks exist exclusively in tests.
template<class Network,class Steam,class Now,class Current>
Grant exchange(Network& http,Steam& steam,Now now,Current current,Account account,uint64_t generation,Keys& keys){
 Session session;session.begin(account,now(),generation);
 auto response=http.request(L"/v1/challenge",session.request());require(response.status==200);
 auto identity=session.accept_challenge(response.body,now());
 require(current()==account);
 Ticket ticket;
 struct Cancel {Steam& steam;Ticket& ticket;~Cancel(){if(ticket.handle)steam.cancel(ticket.handle);}} cancel{steam,ticket};
 steam.ticket(identity,ticket,[&]{return session.fresh(now())&&current()==account;});
 require(current()==account);
 auto body=session.exchange(ticket.bytes,now());
 struct Wipe {std::string& value;~Wipe(){if(!value.empty())sodium_memzero(value.data(),value.size());}} wipe{body};
 response=http.request(L"/v1/authorize",body);
 if(ticket.handle){steam.cancel(ticket.handle);ticket.handle=0;}
 require(response.status==200&&current()==account);
 // Refresh from the fixed endpoint on each renewal. Keep only public keys in memory.
 auto public_keys=http.request(L"/v1/keys","");require(public_keys.status==200);keys=read_keys(public_keys.body);
 Wipe wipe_grant{response.body};
 return verify(response.body,keys,session,current(),now(),generation);
}
}
