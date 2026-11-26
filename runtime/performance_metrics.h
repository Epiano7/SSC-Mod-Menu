#pragma once
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace ssc_perf {
enum Kind {overlay,advisor,sharing,tracker,capture,scoring,present,kos,achievements,draft,overlay_gl,diagnostics_emit,count};
inline bool recording=false;
inline std::array<double,count> milliseconds{};
inline std::array<unsigned,count> calls{};
inline double clock_ms(){static double scale=[](){LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return 1000.0/double(f.QuadPart);}(); LARGE_INTEGER t{};QueryPerformanceCounter(&t);return double(t.QuadPart)*scale;}

struct Scope {
    Kind kind;
    bool active;
    double start;
    explicit Scope(Kind k,bool include=true)
        :kind(k),active(recording&&include),start(active?clock_ms():0){}
    ~Scope(){if(active){milliseconds[kind]+=clock_ms()-start;++calls[kind];}}
};

enum class OverlayPhase : unsigned {
    audio, chat_speech, kos, privacy, private_access, shared_cosmetics,
    game_tracking, challenges, achievements, match_review, recorder,
    training, camera, presence, updater, hud, chat_completion, graphics, count
};
inline constexpr const char* overlay_phase_names[]={
    "audio","chat_speech","kos","privacy","private_access","shared_cosmetics",
    "game_tracking","challenges","achievements","match_review","recorder",
    "training","camera","presence","updater","hud","chat_completion","graphics"
};
using OverlayTimes=std::array<double,unsigned(OverlayPhase::count)>;
static_assert(sizeof(overlay_phase_names)/sizeof(*overlay_phase_names)==OverlayTimes{}.size());
inline OverlayTimes overlay_times{};

// A checkpoint ends one phase and starts the next at the same QPC tick.
class OverlayTimer {
    bool active_;
    OverlayPhase phase_=OverlayPhase::audio;
    double started_,boundary_;
    void record_phase(double at){
        const double elapsed=at-boundary_;
        overlay_times[unsigned(phase_)]+=elapsed;
        if(phase_==OverlayPhase::kos){milliseconds[kos]+=elapsed;++calls[kos];}
        if(phase_==OverlayPhase::achievements){milliseconds[achievements]+=elapsed;++calls[achievements];}
        if(phase_==OverlayPhase::graphics){milliseconds[overlay_gl]+=elapsed;++calls[overlay_gl];}
        boundary_=at;
    }
public:
    explicit OverlayTimer(bool include)
        :active_(recording&&include),started_(active_?clock_ms():0),boundary_(started_){}
    void mark(OverlayPhase next){
        if(active_){record_phase(clock_ms());phase_=next;}
    }
    ~OverlayTimer(){
        if(!active_)return;
        const double finished=clock_ms();
        record_phase(finished);
        milliseconds[overlay]+=finished-started_;
        ++calls[overlay];
    }
};

struct SlowFrame {
    double frame_ms=0,overlay_ms=0,present_ms=0,sharing_ms=0,draft_ms=0,graphics_ms=0;
    bool focused=false,menu_open=false;
    uint64_t ended_tick_ms=0;
    double diagnostics_ms=0;
    OverlayTimes overlay_phases{};
};

struct FrameWindow {
    static constexpr double bucket_ms=0.5;
    std::array<unsigned,400> histogram{};
    std::array<double,count> phase_ms{};
    std::array<unsigned,count> phase_calls{};
    std::array<SlowFrame,3> slowest{},slowest_overlay{};
    OverlayTimes overlay_phase_ms{};
    unsigned frames=0,focused_frames=0,menu_frames=0;
    unsigned over_16ms=0,over_25ms=0,over_33ms=0,over_50ms=0,over_100ms=0;
    double sum_ms=0,max_ms=0;

    void add(double frame,bool focused,bool menu,const std::array<double,count>& times,
             const std::array<unsigned,count>& timing_calls,const OverlayTimes& detail={},uint64_t ended_tick_ms=0){
        if(frame<=0||!std::isfinite(frame))return;
        ++frames;
        focused_frames+=focused;
        menu_frames+=menu;
        over_16ms+=frame>1000.0/60;
        over_25ms+=frame>25;
        over_33ms+=frame>33.333;
        over_50ms+=frame>50;
        over_100ms+=frame>100;
        sum_ms+=frame;
        max_ms=std::max(max_ms,frame);
        const auto bucket=size_t(std::min(frame/bucket_ms,double(histogram.size()-1)));
        ++histogram[bucket];
        for(size_t i=0;i<count;++i){
            phase_ms[i]+=times[i];
            phase_calls[i]+=timing_calls[i];
        }
        for(size_t i=0;i<detail.size();++i)overlay_phase_ms[i]+=detail[i];
        const bool slow=frame>=25&&frame>slowest.back().frame_ms;
        const bool overlay_slow=focused&&times[overlay]>=4&&times[overlay]>slowest_overlay.back().overlay_ms;
        if(!slow&&!overlay_slow)return;
        const SlowFrame item{frame,times[overlay],times[present],times[sharing],times[draft],
            times[overlay_gl],focused,menu,ended_tick_ms,times[diagnostics_emit],detail};
        if(slow)retain(slowest,item,&SlowFrame::frame_ms);
        if(overlay_slow)retain(slowest_overlay,item,&SlowFrame::overlay_ms);
    }

    static void retain(std::array<SlowFrame,3>& rows,SlowFrame item,double SlowFrame::*metric){
        for(auto& row:rows){
            if(item.*metric>row.*metric)std::swap(item,row);
        }
    }

    // Return the upper bound of the percentile's bucket, capped at the measured maximum.
    double percentile(unsigned percent)const{
        if(!frames)return 0;
        const auto rank=(uint64_t(frames)*percent+99)/100;
        uint64_t seen=0;
        for(size_t i=0;i<histogram.size()-1;++i){
            seen+=histogram[i];
            if(seen>=rank)return std::min(max_ms,(i+1)*bucket_ms);
        }
        return max_ms;
    }
};
}
