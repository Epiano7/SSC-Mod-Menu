#pragma once
#include "achievement_service.h"
#include "achievement_airborne.h"
#include "achievement_account_stats.h"
#include "achievement_jumps.h"
#include "achievement_loadout.h"
#include "achievement_native_events.h"
#include "beta_auth.h"
#include "auto_messages_source.h"
#include "challenge_modes.h"
namespace ssc_achievements {
inline uint64_t utc_now(){return uint64_t(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());}
inline Engine engine;
inline constexpr bool enabled=true;
inline LoadoutProgress loadout_progress;
inline account_stats::Gain xp_gain,cash_gain;
inline bool ready=false,storage_error=false,api_busy=false,import_requested=false;
inline uint64_t account=0,generation=0,saved_revision=0,sampled=0;inline size_t saved_unlocks=0;
inline int game_user_id=0;
inline ImportState imported;
inline Service* service=nullptr;
inline community::Model community_sync;
inline std::wstring status=L"Waiting for Steam";
inline std::deque<PendingClaim> pending_claims;
inline void persist(){if(service&&account&&ready){Saved s{engine.ledger,imported,pending_claims};if(service->save(account,generation,s)){saved_revision=engine.ledger.revision;saved_unlocks=engine.ledger.unlocked();}else status=L"Save queued; retrying";}}
inline void configure(const std::filesystem::path& folder){if(!service)service=new Service(folder);}
inline void request_statistics(){
 const auto now=GetTickCount64();static uint64_t next=0;
 if(service&&now>=next){if(service->statistics())next=now+300000;}
}
inline bool community_request(community::Action action){
 if(!service||!ready||!account||community_sync.busy)return false;
 community::Request request;request.action=action;
 if(action==community::Action::check||action==community::Action::withdraw)request.token=community_sync.token;
 if(action==community::Action::publish){request.body=community::payload(engine.ledger);request.revision=engine.ledger.revision;}
 if(!service->sync_community(account,generation,std::move(request)))return false;
 community_sync.busy=true;++community_sync.revision;return true;
}
inline void community_tick(uint64_t now){
 auto& sync=community_sync;
 if(sync.busy||now<sync.next)return;
 if(!sync.state.empty()){
  if(now>=sync.deadline){sync.clear_pairing();sync.message=L"Steam confirmation expired. Try again.";return;}
  community_request(community::Action::check);
 }else if(sync.loaded&&sync.enabled&&sync.published!=engine.ledger.revision){
  community_request(community::Action::publish);
 }
}
inline void options_changed(){++engine.ledger.revision;persist();}
inline Frame frame(const ssc_auto::Observation& o){
 Frame f;f.connected=o.connected;f.valid=o.valid;f.active=o.active;f.ending=o.ending;f.preparing=o.preparing;f.survivor=o.eligible;
 if(f.valid&&!ssc_combat::multiplayer(o.session)){f.valid=false;f.connected=false;}
 f.session=o.session;f.actor=o.actor;f.round=o.round;f.last_round=o.last_round;f.countdown=o.recording_countdown_seconds;f.deaths=o.death_total;
 auto level=o.values.find("level");if(level!=o.values.end())try{size_t used=0;int n=std::stoi(level->second,&used);if(used==level->second.size())f.level=n;}catch(...){}
 const auto& c=ssc_challenge::chaos;
 f.chaos_max=c.enabled&&c.visual_confirmed&&ssc_challenge::attached&&ssc_challenge::render_supported&&ssc_challenge::view_supported&&ssc_challenge::input_supported&&std::all_of(c.effects.begin(),c.effects.end(),[](bool b){return b;});
 f.draft=ssc_challenge::random.enabled&&ssc_challenge::attached;f.stims=ssc_challenge::random.stims;
 f.blind=ssc_challenge::random.blind&&ssc_challenge::render_supported&&ssc_challenge::blind_supported;
 f.draft_eligible=ssc_challenge::draft_achievement_eligible;
 return f;
}
inline void tick(uint64_t now,bool presence_supported,bool pixel_supported){
 if(!service)return;
 static uint64_t identity_at=0;static ssc_beta::Identity identity;static bool bound=false;
 if(!identity_at||now-identity_at>=1000){
  identity_at=now;if(!bound)bound=identity.attach();auto next=bound?identity.read():0;
  if(next!=account){
   loadout_progress={};xp_gain={};cash_gain={};native_events::capture_live_form(0,0,0,0);native_events::tracking=false;native_events::golden_bullet=false;native_events::survivor_death=false;
   airborne::take_progress(0,false);airborne::take_match_knives({},0,false);jumps::take_progress(false,true);ssc_combat::achievement_bullets_enabled=false;ssc_combat::achievement_bullets.exchange(0);persist();community_sync.reset();account=next;++generation;ready=false;api_busy=storage_error=import_requested=false;pending_claims.clear();game_user_id=0;imported={};engine.replace({});
   if(account){service->load(account,generation);status=L"Loading achievements...";}else status=L"Waiting for Steam";
  }
  if(account&&presence_supported)game_user_id=account_stats::player_id(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)));
 }
 for(auto& r:service->poll()){
  if(r.statistics){completion_rates.replace(r.api);continue;}
  if(r.account!=account||r.generation!=generation)continue;
  if(r.community_reply.action!=community::Action::none){community_sync.accept(r.community_reply,now,completion_rates);continue;}
  if(r.loaded){storage_error=r.failed;ready=!r.failed;if(ready){engine.replace(r.saved.ledger);imported=r.saved.imported;saved_revision=engine.ledger.revision;saved_unlocks=engine.ledger.unlocked();pending_claims=r.saved.pending;for(auto& p:pending_claims)p.next=now+3000;status=L"Tracking achievements";community_request(community::Action::load);}else status=L"Progress file needs recovery; original preserved";continue;}
  if(r.import||r.claim.start){
   api_busy=false;ApiTotals totals;
   if(r.import){
    if(!r.failed&&r.userid==game_user_id&&api_totals(r.api,r.userid,totals)&&imported.import(totals,r.utc)){
     engine.capability(Kind::kills,true);engine.lifetime(Kind::kills,imported.kills,utc_now(),true);++engine.ledger.revision;persist();status=L"Kill history imported";
    }else status=L"API import unavailable; try again";
   }else{
    if(pending_claims.empty()||r.userid!=pending_claims.front().userid||r.claim.start!=pending_claims.front().claim.start||r.claim.finish!=pending_claims.front().claim.finish)continue;
    MatchEvidence e;
    if(!r.failed&&api_match(r.api,r.userid,r.claim.start,r.claim.finish,e)){
     engine.confirmed(r.claim,std::all_of(e.wins.begin(),e.wins.end(),[](bool b){return b;}),e.placement==1,e.kills,e.deaths,utc_now());
     if(imported.add_match(e.id,e.started,e.kills)){engine.capability(Kind::kills,true);engine.lifetime(Kind::kills,imported.kills,utc_now());++engine.ledger.revision;}
     if(!pending_claims.empty())pending_claims.pop_front();
     status=L"Match achievements checked";persist();
    }else if(!pending_claims.empty()){auto& p=pending_claims.front();p.next=now+15000;++p.attempts;status=L"Waiting for the game API to publish the match";if(p.attempts>=32){pending_claims.pop_front();status=L"Match result could not be confirmed";}++engine.ledger.revision;persist();}
   }
  }else if(r.failed){saved_revision=UINT64_MAX;storage_error=true;status=L"Could not save achievements; progress remains in memory";}else storage_error=false;
 }
 if(!ready||!account){airborne::take_progress(0,false);jumps::take_progress(false);ssc_combat::achievement_bullets_enabled=false;ssc_combat::achievement_bullets.exchange(0);return;}
 engine.capability(Kind::kills,true);
 engine.capability(Kind::xp,account_stats::xp_supported||account_stats::profile_supported);
 engine.capability(Kind::cash,account_stats::cash_supported);
 if(!engine.ledger.account_stats_imported&&game_user_id>0){
  account_stats::Historical totals;
  const auto utc=utc_now();
  if(account_stats::history(reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)),totals)&&account_stats::import(engine,totals,utc)){
   imported.done=true;imported.at=utc;imported.kills=std::max(imported.kills,totals.kills);
   persist();
  }
 }
 if(!account_stats::profile_supported&&!imported.done&&game_user_id>0){
  static uint64_t retry_import_at=0;
  if(now>=retry_import_at){import_requested=true;retry_import_at=now+30000;}
 }
 community_tick(now);
 engine.capability(Kind::air_knife,airborne::attached);
 engine.capability(Kind::melee_match,airborne::attached);
 engine.capability(Kind::death_time,native_events::death_attached);
 engine.capability(Kind::double_bullet,native_events::award_attached);
 native_events::tracking=true;
 if(native_events::survivor_death.exchange(false))engine.event(Kind::death_time,1,utc_now());
 if(native_events::golden_bullet.exchange(false))engine.event(Kind::double_bullet,2,utc_now());
 const bool loadout_supported=ssc_challenge::choices_supported;
 for(const auto kind:{Kind::invisibility,Kind::syringes,Kind::syringes_match})engine.capability(kind,loadout_supported);
 engine.capability(Kind::jumps,jumps::attached);
 const auto jump_count=jumps::take_progress(enabled&&jumps::attached);
 if(jump_count)engine.activity(Kind::jumps,jump_count,utc_now());
 const auto& tracked=ssc_auto::tracking_observation;
 const auto tracked_at=ssc_auto::tracking_at;
 const bool fresh=tracked_at&&now>=tracked_at&&now-tracked_at<=500;
 const bool knife_enabled=enabled&&airborne::attached;
 const auto knife_actor=knife_enabled&&fresh&&tracked.valid&&tracked.active&&!tracked.ending&&ssc_combat::multiplayer(tracked.session)?tracked.actor:0;
 const auto knives=airborne::take_progress(knife_actor,knife_enabled);
 if(knives)engine.airborne_knives(knives,true,true,utc_now());
 const bool bullet_source=ssc_combat::attached&&ssc_auto::supported()&&ssc_compat::supports(2)&&ssc_compat::supports(64);
 engine.capability(Kind::bullets,bullet_source);ssc_combat::achievement_bullets_enabled.store(enabled&&bullet_source,std::memory_order_relaxed);
 auto bullets=ssc_combat::achievement_bullets.exchange(0,std::memory_order_relaxed);if(enabled&&bullet_source&&bullets)engine.activity(Kind::bullets,bullets,utc_now());
 if(enabled&&ssc_auto::tracking_at!=sampled){
  sampled=ssc_auto::tracking_at;auto f=frame(ssc_auto::tracking_observation);
  if(!sampled||now<sampled||now-sampled>500)f.valid=false;
  if(native_events::death_attached)native_events::capture_live_form(f.valid&&f.active&&!f.ending?f.actor:0,f.session,f.round,now);
  f.chaos_max=f.chaos_max&&pixel_supported;
  const auto utc=utc_now();
  engine.observe(f,now,utc);
  if(f.valid&&(f.active||f.ending)){
   float xp=-1;
   int cash=-1;
   if(account_stats::xp_supported&&ssc_names::read(f.actor+0x878,xp)&&std::isfinite(xp)&&xp>=0&&xp<=100000000)
    if(auto delta=xp_gain.observe(f,uint64_t(xp),now))engine.activity(Kind::xp,delta,utc);
   if(account_stats::cash_supported&&ssc_names::read(f.actor+0xb58,cash)&&cash>=0&&cash<=100000000)
    if(auto delta=cash_gain.observe(f,uint64_t(cash),now))engine.activity(Kind::cash,delta,utc);
  }else{xp_gain.observe(f,0,now);cash_gain.observe(f,0,now);}
  const auto knife_total=airborne::take_match_knives(f.valid?ssc_names::actor_range():ssc_names::ActorRange{},f.round,f.valid);
  if(knife_total)engine.event(Kind::melee_match,knife_total,utc_now());
  if(loadout_supported){
   const auto inventory=f.valid&&f.active?loadout(f.actor,[](uintptr_t p,auto& v){return ssc_names::read(p,v);}):LoadoutSnapshot{};
   loadout_progress.observe(f,inventory,engine,utc_now());
  }
  if(f.valid&&f.ending){engine.result(f.session,f.round,false,utc_now());auto c=engine.take_claim();if(c.start&&game_user_id>0&&pending_claims.size()<4){pending_claims.push_back({c,now+10000,0,game_user_id});++engine.ledger.revision;persist();}}
 }else if(!enabled){engine.reset_tracking();}
 if(!api_busy&&game_user_id>0){
  while(!pending_claims.empty()&&(pending_claims.front().claim.finish>utc_now()+300||pending_claims.front().claim.finish+604800<utc_now())){pending_claims.pop_front();++engine.ledger.revision;persist();}
  if(import_requested&&!imported.done&&(!ssc_auto::tracking_observation.valid||ssc_auto::tracking_observation.round==0)){import_requested=false;api_busy=service->api(account,generation,game_user_id,utc_now(),true);status=api_busy?L"Importing recorded API totals...":L"API queue busy; try again";}
  else if(!pending_claims.empty()&&now>=pending_claims.front().next){auto& p=pending_claims.front();api_busy=service->api(account,generation,p.userid,utc_now(),false,p.claim);if(!api_busy)p.next=now+1000;}
 }
 static uint64_t save_at=0;if(engine.ledger.revision!=saved_revision&&(engine.ledger.unlocked()!=saved_unlocks||!save_at||now-save_at>=5000)){save_at=now;persist();}
}
inline const wchar_t* availability(size_t i){
 if(i>=catalog.size())return L"Unavailable";
 if(!ready)return status.c_str();
 if(!engine.available(i))return L"Event tracking not connected yet";
 return L"";
}
}
