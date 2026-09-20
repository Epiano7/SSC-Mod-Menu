#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdint>

namespace ssc_lab {
struct ShieldRules { float low=0,high=0;bool ready=false; };
struct Target { float health=100,shield=0,max_health=100; };
struct Impact { float health=0,shield=0; };
inline bool valid_target(const Target& t){
    return std::isfinite(t.health)&&std::isfinite(t.shield)&&std::isfinite(t.max_health)
        &&t.health>0&&t.health<=t.max_health&&t.max_health<=1000000&&t.shield>=0&&t.shield<=1000000;
}
// Ordinary hostile player hit: native 0.991.13 health/shield block, after damage
// modifiers and before death-prevention perks. Preserve native float rounding.
inline Impact impact(Target& t,float damage,const ShieldRules& rules){
    Impact out;float hp_damage=damage;
    if(t.shield>0){
        float fraction=std::clamp(t.shield/t.max_health,0.f,1.f);
        float efficiency=(1.f-fraction)*rules.low+fraction*rules.high;
        float excess=std::max(0.f,t.shield-t.health);
        float eligible=std::max(0.f,damage-std::max(0.f,t.health-t.shield));
        float loss=float(double(eligible)/(double(efficiency)*.5+.5));
        float above=std::min(loss,excess);
        loss=std::clamp((loss-above)*.5f+above,0.f,t.shield);
        hp_damage=damage-loss*efficiency;
        out.shield=loss;t.shield-=loss;
    }
    out.health=std::min(t.health,hp_damage);t.health-=out.health;
    return out;
}
struct Shot { unsigned number=0,reloads=0;double time=0;float health=0,shield=0,hp_damage=0,shield_damage=0;bool missed=false; };
struct Trial { std::vector<Shot> shots;bool killed=false,capped=false; };
// A bounded, deterministic magazine estimate. It deliberately excludes travel,
// recovery, burst timing, special effects and reserves. First impact is at t=0.
inline bool misses(unsigned seed,unsigned shot,unsigned percentage){
    uint32_t x=seed+0x9e3779b9u*(shot+1);x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;x^=x>>16;
    return x%100<std::min(percentage,100u);
}
inline Trial simulate(Target target,float damage,int rpm,int magazine,float reload,bool allow_reload,const ShieldRules& rules,unsigned miss_percent=0,unsigned seed=1,int projectiles=1){
    Trial result;
    if(projectiles<1||projectiles>10000||!valid_target(target)||!std::isfinite(damage)||damage<=0||rpm<=0||magazine<=0
       ||!std::isfinite(reload)||reload<0||(target.shield>0&&!rules.ready))return result;
    double time=0,interval=60./rpm;unsigned reloads=0;
    for(unsigned n=0;n<10000;++n){
        if(n){if(n%unsigned(magazine)==0){if(!allow_reload||reload==0)break;time+=std::max(interval,double(reload));++reloads;}else time+=interval;}
        bool missed=true;Impact hit;
        for(int pellet=0;pellet<projectiles&&target.health>0;++pellet){if(misses(seed,n*unsigned(projectiles)+unsigned(pellet),miss_percent))continue;
            missed=false;auto part=impact(target,damage/float(projectiles),rules);hit.health+=part.health;hit.shield+=part.shield;}
        result.shots.push_back({n+1,reloads,time,target.health,target.shield,hit.health,hit.shield,missed});
        if(target.health<=0){result.killed=true;break;}
        if(!missed&&hit.health==0&&hit.shield==0)break;
    }
    result.capped=!result.killed&&result.shots.size()==10000;
    return result;
}
struct Duel { int winner=-1;double time=0;Target players[2];unsigned shots[2]={0,0},reloads[2]={0,0};bool limited=false; };
inline Duel duel(const Trial& a,const Trial& b,Target initial){
    Duel result;result.players[0]=result.players[1]=initial;
    const Trial* trials[]={&a,&b};
    if(a.shots.empty()||b.shots.empty())return result;
    double ta=a.killed?a.shots.back().time:INFINITY,tb=b.killed?b.shots.back().time:INFINITY;
    result.time=std::min(ta,tb);
    double horizon=std::min(a.capped?a.shots.back().time:INFINITY,b.capped?b.shots.back().time:INFINITY);
    if(horizon<result.time){result.time=horizon;result.limited=true;}
    if(result.limited){}
    else if(std::isfinite(result.time))result.winner=std::abs(ta-tb)<1e-8?2:ta<tb?0:1;
    else {result.time=std::max(a.shots.back().time,b.shots.back().time);result.limited=a.capped||b.capped;}
    // Both same-time impacts land before resolving the winner. The independent
    // tracks are valid only for this model (no attacker-health-dependent perks).
    for(int side=0;side<2;++side){for(const auto& shot:trials[side]->shots){if(shot.time>result.time+1e-8)break;
        result.players[1-side].health=shot.health;result.players[1-side].shield=shot.shield;result.shots[side]=shot.number;result.reloads[side]=shot.reloads;}}
    return result;
}

// Playback shares the exact trial timeline used to resolve the duel.
inline Duel duel_at(const Trial& a,const Trial& b,Target initial,double seconds,const Duel* completed=nullptr){
    auto end=completed?*completed:duel(a,b,initial);if(seconds>=end.time)return end;
    Duel frame;frame.time=std::max(0.,seconds);frame.players[0]=frame.players[1]=initial;
    const Trial* tracks[]={&a,&b};
    for(int side=0;side<2;++side){const auto& shots=tracks[side]->shots;
        auto next=std::upper_bound(shots.begin(),shots.end(),frame.time+1e-8,[](double t,const Shot& shot){return t<shot.time;});
        if(next!=shots.begin()){const auto& shot=*std::prev(next);frame.players[1-side].health=shot.health;frame.players[1-side].shield=shot.shield;frame.shots[side]=shot.number;frame.reloads[side]=shot.reloads;}}
    return frame;
}

// Animate only damage that has already landed. No damage is invented during
// reload gaps, and this presentation never changes the duel's result/timing.
inline Target visual_target(const Trial& incoming,Target initial,double event_time,double display_time){
    auto end=std::upper_bound(incoming.shots.begin(),incoming.shots.end(),event_time+1e-8,[](double t,const Shot& shot){return t<shot.time;});
    if(end==incoming.shots.begin())return initial;
    Target shown=initial;shown.health=std::prev(end)->health;shown.shield=std::prev(end)->shield;
    while(end!=incoming.shots.begin()){const auto& shot=*--end;double age=display_time-shot.time;if(age>=.12)break;
        double t=std::clamp(age/.12,0.,1.);double remaining=1-t*t*(3-2*t);
        shown.health+=float(shot.hp_damage*remaining);shown.shield+=float(shot.shield_damage*remaining);}
    return shown;
}

}
