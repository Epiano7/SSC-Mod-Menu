#include "performance_metrics.h"
#include <cassert>
#include <iostream>
#include <limits>

int main(){
    using namespace ssc_perf;
    FrameWindow window;
    std::array<double,count> times{};
    std::array<unsigned,count> timing_calls{};
    for(int i=0;i<95;++i)window.add(5.25,true,false,times,timing_calls);
    for(int i=0;i<2;++i)window.add(20,true,false,times,timing_calls);

    times[overlay]=2;
    times[present]=23;
    times[sharing]=0.5;
    times[draft]=0.25;
    timing_calls[sharing]=4;
    window.add(41,false,true,times,timing_calls);
    window.add(252,false,false,times,timing_calls);
    window.add(58,true,false,times,timing_calls);
    window.add(0,true,false,times,timing_calls);
    window.add(std::numeric_limits<double>::quiet_NaN(),true,false,times,timing_calls);
    assert(window.frames==100&&window.focused_frames==98&&window.menu_frames==1);
    assert(window.percentile(50)==5.5&&window.percentile(95)==5.5&&window.percentile(99)==58.5);
    assert(window.percentile(100)==252&&window.max_ms==252);
    assert(window.over_16ms==5&&window.over_25ms==3&&window.over_33ms==3);
    assert(window.over_50ms==2&&window.over_100ms==1);
    assert(window.phase_ms[present]==69&&window.phase_calls[sharing]==12);
    assert(window.slowest[0].frame_ms==252&&window.slowest[1].frame_ms==58&&window.slowest[2].frame_ms==41);
    assert(window.slowest[2].menu_open&&!window.slowest[2].focused);
    assert(window.slowest[2].overlay_ms==2&&window.slowest[2].draft_ms==0.25);
    window={};
    assert(window.frames==0&&window.percentile(99)==0&&window.slowest[0].frame_ms==0);

    {Scope inactive(present);}
    assert(calls[present]==0);
    recording=true;
    {Scope excluded(present,false);}
    {Scope included(present);}
    assert(calls[present]==1&&milliseconds[present]>=0);
    overlay_times.fill(0);
    const auto old_overlay=milliseconds[overlay];
    {
        OverlayTimer timer(true);
        timer.mark(OverlayPhase::privacy);
        timer.mark(OverlayPhase::presence);
    }
    double total=0;
    for(auto amount:overlay_times)total+=amount;
    assert(std::abs(total-(milliseconds[overlay]-old_overlay))<.001);
    assert(calls[overlay]==1);

    window={};times.fill(0);
    OverlayTimes detail{};
    for(int i=0;i<3;++i)window.add(70+i,true,false,times,timing_calls);
    times[overlay]=31.5;times[diagnostics_emit]=.4;
    detail[unsigned(OverlayPhase::presence)]=30.75;
    window.add(59,true,false,times,timing_calls,detail,1200);
    assert(window.slowest[0].frame_ms==72&&window.slowest[2].frame_ms==70);
    const auto& hitch=window.slowest_overlay[0];
    assert(hitch.frame_ms==59&&hitch.overlay_ms==31.5&&hitch.ended_tick_ms==1200);
    assert(hitch.overlay_phases[unsigned(OverlayPhase::presence)]==30.75&&hitch.diagnostics_ms==.4);
    assert(window.overlay_phase_ms[unsigned(OverlayPhase::presence)]==30.75);
    window.add(100,false,false,times,timing_calls,detail,1400);
    assert(window.slowest[0].ended_tick_ms==1400&&window.slowest_overlay[0].ended_tick_ms==1200);
    std::cout<<"PASS frame percentiles, spike attribution, inactive timing and overlay phase isolation\n";
}
