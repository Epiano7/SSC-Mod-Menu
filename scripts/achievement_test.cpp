#include "achievement_api.h"
#include <cassert>
#include <iostream>
using namespace ssc_achievements;
int checks=0;
void check(bool good){++checks;if(!good){std::cerr<<"Failed check "<<checks<<"\n";std::abort();}}
bool unlocked(const Engine& e,const char* id){return e.ledger.items[index(id)].unlocked_at!=0;}
Frame initial(){Frame f;f.valid=f.connected=f.preparing=f.survivor=true;f.session=10;f.actor=20;f.round=0;f.last_round=3;f.level=0;f.deaths=f.kills=0;f.chaos_max=f.draft=f.stims=f.blind=true;return f;}
void begin(Engine& e,Frame& f,uint64_t& now,int n){f.active=false;f.ending=false;f.preparing=true;now+=100;e.observe(f,now,1000000+now/1000);f.round=n;f.active=true;f.preparing=false;f.level=1;now+=100;e.observe(f,now,1000000+now/1000);}
void finish(Engine& e,Frame& f,uint64_t& now,int kills=0){f.kills=kills;f.active=true;now+=100;e.observe(f,now,1000000+now/1000);f.active=false;f.ending=true;f.preparing=false;now+=100;e.observe(f,now,1000000+now/1000);e.result(f.session,f.round,false,1000000+now/1000);}
Json profile(){
 Json filters;for(const char* k:{"gameVersion","minRating","maxRating","since","until","groupId","region","mode","classId","ranked","topPlayers"})filters[k]=nullptr;filters["competitive"]=false;
 return {{"filters",filters},{"player",{{"userid",7},{"bot",false}}},{"summary",{{"kills",6000},{"matches",100}}},{"recentMatches",Json::array()}};
}
int main(){
 {Engine e;auto f=initial();uint64_t now=100;begin(e,f,now,1);check(e.full_round());f.level=20;e.observe(f,now+=100,1000100);for(auto id:{"level_17","level_18","level_19","level_20"})check(unlocked(e,id));
  auto rev=e.ledger.revision;e.observe(f,now+=100,1000100);check(rev==e.ledger.revision);
  check(e.has_notice());auto n=e.notice(now);check(n&&n->item==index("level_17"));e.notice(now+6000);check(e.notice(now+6000)->item==index("level_18"));
 }
 {Engine e;auto f=initial();f.round=1;f.preparing=false;f.active=true;f.level=20;e.observe(f,100,1000);check(!e.full_round());check(!unlocked(e,"level_17"));uint64_t now=100;finish(e,f,now);check(!unlocked(e,"chaos_round"));}
 {Engine e;auto f=initial();uint64_t now=100;begin(e,f,now,1);f.level=20;e.observe(f,now+=3000,1000);check(!unlocked(e,"level_17"));check(!e.full_round());}
 {Engine e;auto f=initial();uint64_t now=100;begin(e,f,now,1);e.challenge_changed(true);finish(e,f,now);check(!unlocked(e,"draft_round"));check(unlocked(e,"chaos_round"));begin(e,f,now,2);finish(e,f,now);check(unlocked(e,"draft_round"));}
 {Engine e;auto f=initial();uint64_t now=100;begin(e,f,now,1);e.challenge_changed(false);finish(e,f,now);check(!unlocked(e,"chaos_round"));check(unlocked(e,"draft_round"));}
 {Engine e;auto f=initial();uint64_t now=100;
  for(int i=1;i<=3;++i){begin(e,f,now,i);finish(e,f,now);auto rev=e.ledger.revision;e.result(f.session,f.round,true,1000000+now/1000);check(rev==e.ledger.revision);}
  auto c=e.take_claim();check(c.full&&c.chaos&&c.blind&&c.start&&c.finish>=c.start);check(!unlocked(e,"chaos_sweep"));check(!unlocked(e,"blind_match"));
  e.confirmed(c,true,true,40,1,1000200);check(unlocked(e,"chaos_sweep"));check(unlocked(e,"blind_match"));check(unlocked(e,"kdr_40"));auto rev=e.ledger.revision;e.confirmed(c,true,true,40,1,1000200);check(rev==e.ledger.revision);check(!e.take_claim().start);
 }
 {Engine e;auto f=initial();uint64_t now=100;for(int i=1;i<=3;++i){begin(e,f,now,i);if(i==2)e.challenge_changed(false);finish(e,f,now);}auto c=e.take_claim();check(c.full&&!c.chaos&&c.blind);e.confirmed(c,false,true,39,0,1000200);check(!unlocked(e,"kdr_40"));check(!unlocked(e,"chaos_sweep"));}
 {Engine e;auto f=initial();uint64_t now=100;f.round=1;f.active=true;f.preparing=false;e.observe(f,now,1000000);finish(e,f,now);for(int i=2;i<=3;++i){begin(e,f,now,i);finish(e,f,now);}check(!e.take_claim().full);}
 {Engine e;e.capability(Kind::death_time,true);auto f=initial();uint64_t now=100;begin(e,f,now,1);e.survivor_death(-120,true,1000100);check(unlocked(e,"death_minus_120"));}
 {Engine e;e.capability(Kind::death_time,true);auto f=initial();uint64_t now=100;begin(e,f,now,1);e.survivor_death(-121,true,1000100);check(!unlocked(e,"death_minus_120"));}
 {Engine e;e.capability(Kind::death_time,true);e.survivor_death(-115,false,1000);e.survivor_death(-114,true,1000);check(!unlocked(e,"death_minus_120"));e.survivor_death(-115,true,1000);check(unlocked(e,"death_minus_120"));}
 {Engine e;Ledger migrated;auto data=e.ledger.json();data["achievements"]["jumps_10000"]={{"progress",305},{"unlocked_at",0}};check(Ledger::parse(data,migrated));check(migrated.items[index("jumps_100")].unlocked_at!=0);auto tier=tier_display(migrated,index("jumps_100"));check(tier.earned==index("jumps_300")&&tier.next==index("jumps_1000")&&migrated.items[tier.next].value==305);}
 {CompletionRates stats;Json result={{"schema",1},{"catalog",catalog_wire_version},{"population","synced_accounts"},{"participants",uint64_t(8)},{"achievements",{{"jumps_100",{{"unlocked",uint64_t(3)}}}}}};check(stats.replace(result));check(stats.percent(index("jumps_100"))==3750);result["achievements"]["jumps_100"]["unlocked"]=uint64_t(9);check(!stats.replace(result));}
 {Engine e;e.airborne_knives(3,true,true,1000);check(!unlocked(e,"air_knife_3"));e.capability(Kind::air_knife,true);e.airborne_knives(3,false,true,1000);check(!unlocked(e,"air_knife_3"));e.airborne_knives(2,true,true,1000);check(!unlocked(e,"air_knife_3"));e.airborne_knives(3,true,true,1000);check(unlocked(e,"air_knife_3"));}
 {Ledger l;l.items[0]={17,1000};l.notifications=false;l.tracking=false;Ledger loaded;check(Ledger::parse(Json::parse(l.json().dump()),loaded));check(loaded.items[0].unlocked_at==1000&&!loaded.notifications&&loaded.tracking);auto j=l.json();j["achievements"]["level_17"]["unlocked_at"]=-1;check(!Ledger::parse(j,loaded));j=l.json();j["achievements"]["level_17"]["progress"]=16;check(!Ledger::parse(j,loaded));j=l.json();j["schema"]=2;check(!Ledger::parse(j,loaded));}
 {auto j=profile();ApiTotals totals;check(api_totals(j,7,totals)&&totals.kills==6000);check(!api_totals(j,8,totals));j["filters"]["ranked"]=true;check(!api_totals(j,7,totals));j=profile();j["summary"]["kills"]=-1;check(!api_totals(j,7,totals));j=profile();j["player"]["bot"]=true;check(!api_totals(j,7,totals));}
 {ImportState s;ApiTotals t{6000,100};check(s.import(t,1000));check(!s.import(t,1001));check(!s.add_match("abc",999,20));check(s.add_match("abc",1001,20));check(!s.add_match("abc",1001,20));check(s.kills==6020);ImportState copy;check(ImportState::parse(Json::parse(s.json().dump()),copy));check(copy.done&&copy.receipts.size()==1&&copy.kills==6020);}
 {check(iso_seconds("2026-11-19T12:00:00Z")>0);check(iso_seconds("2026-11-19T12:00:00.296439+00:00")==iso_seconds("2026-11-19T12:00:00Z"));check(!iso_seconds("2026-02-30T12:00:00Z"));check(!iso_seconds("2026-11-19T12:00:00+01:00"));check(!iso_seconds("2026-11-19T12:00:00.Z"));}
 {auto j=profile();j["recentMatches"].push_back({{"match",{{"id","12345678-1234-1234-1234-123456789abc"},{"startedAt","2026-11-19T12:00:00Z"},{"endedAt","2026-11-19T12:20:00Z"}}},{"player",{{"userid",7},{"kills",45},{"deaths",1},{"placement",1},{"roundWins",{1,1,1}}}}});MatchEvidence m;
  auto a=iso_seconds("2026-11-19T12:00:30Z"),b=iso_seconds("2026-11-19T12:20:00Z");check(api_match(j,7,a,b,m)&&m.kills==45);check(!api_match(j,7,a,b+300,m));j["recentMatches"].push_back(j["recentMatches"][0]);check(!api_match(j,7,a,b,m));}
 {Engine e;e.ledger.notifications=false;e.capability(Kind::kills,true);e.lifetime(Kind::kills,5000,1000);check(unlocked(e,"kills_5000")&&!e.has_notice());e.preview();check(e.has_notice());check(e.notice(100)->preview);check(!e.notice(6100));}
 {Engine e;e.advance(index("level_17"),17,0);check(!unlocked(e,"level_17"));e.advance(index("level_17"),17,1000);check(unlocked(e,"level_17"));}
 {Engine e;auto f=initial();uint64_t now=100;for(int i=1;i<=3;++i){begin(e,f,now,i);finish(e,f,now);}e.replace({});check(!e.take_claim().start);}
 {auto p=profile();p["player"]["userid"]=7.1;ApiTotals totals;check(!api_totals(p,7,totals));}
 {Engine e;auto f=initial();uint64_t now=100;begin(e,f,now,1);f.active=false;f.ending=f.preparing=true;now+=100;e.observe(f,now,1000100);e.result(f.session,f.round,false,1000100);check(unlocked(e,"chaos_round"));check(unlocked(e,"draft_round"));}
 {Engine e;e.advance(index("level_17"),17,1000);check(e.has_notice());e.ledger.notifications=false;check(!e.notice(100));e.preview();check(e.notice(200)&&e.notice(200)->preview);}
 std::cout<<"PASS "<<checks<<" achievement eligibility, persistence, API and idempotency checks\n";
}
