#include "../runtime/cooldown_pulse.h"
#include <cassert>
#include <limits>
#include <cstdio>
using namespace ssc_cooldown;
int main(){
    Tracker t;Settings cfg;Sample s{1,5,10,10,true};
    assert(t.observe(s,100,cfg)==0); // Already ready at load is not a cooldown event.
    s.elapsed=1;assert(t.observe(s,200,cfg)==0);s.elapsed=10;assert(t.observe(s,300,cfg)==1.f);
    assert(t.observe(s,1000,cfg)==1.f);assert(t.reveal(1000,cfg)==1.f);float fading=t.observe(s,1400,cfg);assert(fading>0&&fading<1.f);assert(t.reveal(1400,cfg)==fading);
    assert(t.observe(s,1800,cfg)==0);assert(t.observe(s,1900,cfg)==0); // No repeat while ready.
    s.elapsed=2;assert(t.observe(s,2000,cfg)==0);s.elapsed=10;assert(t.observe(s,2100,cfg)==1.f);
    s.elapsed=0;assert(t.observe(s,2200,cfg)==0); // Casting cancels an active pulse.
    s.elapsed=10;s.eligible=false;assert(t.observe(s,2300,cfg)==0);s.eligible=true;assert(t.observe(s,2400,cfg)==0); // Condition-only changes do not trigger.
    s.elapsed=0;t.observe(s,2500,cfg);s.elapsed=10;assert(t.observe(s,5000,cfg)==0); // Missed/hidden interval.
    s.elapsed=0;t.observe(s,5100,cfg);s.elapsed=10;assert(t.observe(s,100,cfg)==0); // Clock reset.
    s.elapsed=0;t.observe(s,200,cfg);s.elapsed=10;cfg.enabled=false;assert(t.observe(s,300,cfg)==0);cfg.enabled=true;assert(t.observe(s,400,cfg)==0);
    s.elapsed=0;t.observe(s,500,cfg);Sample other=s;other.actor=2;other.elapsed=10;assert(t.observe(other,600,cfg)==0);s.elapsed=10;assert(t.observe(s,600,cfg)==1.f);
    s.elapsed=std::numeric_limits<float>::quiet_NaN();assert(t.observe(s,700,cfg)==0);s.elapsed=10;assert(t.observe(s,800,cfg)==0);
    t.clear();cfg.opacity=.75f;cfg.duration_ms=1000;s.elapsed=0;t.observe(s,1000,cfg);s.elapsed=10;assert(t.observe(s,1100,cfg)==.75f);assert(t.reveal(1100,cfg)==1.f);t.observe(s,1600,cfg);assert(t.observe(s,2100,cfg)==0);assert(t.reveal(2100,cfg)==0);
    t.clear();for(int i=0;i<100;++i){s.skill=i;s.elapsed=0;t.observe(s,3000+i,cfg);}s.skill=0;s.elapsed=10;assert(t.observe(s,3101,cfg)==0); // Bounded eviction cannot invent a transition.
    std::puts("PASS: cooldown transitions, no initial/condition-only pulses, fade timing, recast, stale samples, identity, disable, invalid data and bounded state");
}
