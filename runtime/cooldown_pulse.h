#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace ssc_cooldown {
struct Settings {bool enabled=true;float opacity=1.f;unsigned duration_ms=1500;};
inline Settings settings;
struct Sample {uintptr_t actor=0;int skill=-1;float elapsed=0,total=0;bool eligible=false;};
struct Entry {uintptr_t actor=0;int skill=-1;uint64_t seen=0,started=0;bool cooling=false,pulsing=false;};
struct Tracker {
    std::array<Entry,32> entries{};
    void clear(){entries={};}
    float reveal(uint64_t now,const Settings& cfg)const{
        if(!cfg.enabled)return 0;
        float result=0;unsigned duration=std::clamp(cfg.duration_ms,250u,5000u);
        for(const auto& e:entries){
            if(!e.pulsing||now<e.seen||now-e.seen>1000||now<e.started||now-e.started>=duration)continue;
            float t=std::clamp((float(now-e.started)/duration-.6f)/.4f,0.f,1.f);
            result=std::max(result,1-t*t*(3-2*t));
        }
        return result;
    }
    float observe(const Sample& s,uint64_t now,const Settings& cfg){
        if(!cfg.enabled){clear();return 0;}
        if(!s.actor||s.skill<0||s.skill>4096||!std::isfinite(s.elapsed)||!std::isfinite(s.total)||s.elapsed<0||s.total<=0||s.total>1000000){
            for(auto& e:entries)if(e.actor==s.actor&&e.skill==s.skill)e={};
            return 0;
        }
        Entry* entry=nullptr;
        for(auto& e:entries)if(e.actor==s.actor&&e.skill==s.skill){entry=&e;break;}
        if(!entry){entry=&*std::min_element(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.seen<b.seen;});*entry={s.actor,s.skill,now,0,s.elapsed<s.total,false};return 0;}
        auto& e=*entry;
        if(now<e.seen||now-e.seen>1000){e={s.actor,s.skill,now,0,s.elapsed<s.total,false};return 0;}
        bool cooling=s.elapsed<s.total;
        if(e.cooling&&!cooling&&s.eligible){e.started=now;e.pulsing=true;}
        if(cooling||!s.eligible)e.pulsing=false;
        e.cooling=cooling;e.seen=now;
        if(!e.pulsing)return 0;
        unsigned duration=std::clamp(cfg.duration_ms,250u,5000u);
        if(now-e.started>=duration){e.pulsing=false;return 0;}
        float t=float(now-e.started)/duration;
        float fade=std::clamp((t-.6f)/.4f,0.f,1.f);fade=fade*fade*(3-2*fade);
        float opacity=std::isfinite(cfg.opacity)?std::clamp(cfg.opacity,.25f,1.f):.5f;
        return opacity*(1-fade);
    }
};
inline Tracker tracker;
}
