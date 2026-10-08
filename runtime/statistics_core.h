#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <optional>
#include <sstream>
#include <set>
#include <string>
#include <vector>
#include "third_party/json.hpp"

namespace ssc_stats {
using Json=nlohmann::json;
using Value=std::optional<double>;
inline const std::array<const char*,9> classes={{"All classes","Assault","Marksman","Heavy","Caster","Scavenger","Criminal","Scientist","Assassin"}};
struct Point { double time=0;std::array<Value,3> values;bool gap=false;std::array<bool,3> breaks{};Value round_time{},countdown{}; };
struct Round { int number=0;std::string player_class;bool complete=false,ended=false,rollback=false,gaps=false;unsigned dropped=0;std::vector<Point> points;Value end_level;std::string run_id;double started_utc_ms=0; };
struct Report { std::vector<Round> rounds;unsigned files=0,warnings=0;std::string error; };
inline Value numeric(const Json& value){
    if(!value.is_number())return {};
    double v=value.get<double>();return std::isfinite(v)&&v>=0&&v<9007199254740992.0?Value(v):Value{};
}
inline Value metric(const Json& tracking,const char* key){
    const auto a=tracking.find("availability"),m=tracking.find("metrics");
    if(a==tracking.end()||m==tracking.end()||!a->is_object()||!m->is_object())return {};
    if(!a->contains(key)||(*a)[key]!="observed"||!m->contains(key))return {};
    return numeric((*m)[key]);
}
inline std::string class_name(const Json& tracking){
    const auto a=tracking.find("availability"),m=tracking.find("metrics");
    if(a==tracking.end()||m==tracking.end()||!a->is_object()||!m->is_object()||!a->contains("className")||(*a)["className"]!="observed"||!m->contains("className")||!(*m)["className"].is_string())return {};
    auto name=(*m)["className"].get<std::string>();
    return std::find(classes.begin()+1,classes.end(),name)!=classes.end()?name:std::string{};
}
inline bool yes(const Json& obj,const char* key){return obj.contains(key)&&obj[key].is_boolean()&&obj[key].get<bool>();}
// Keep per-bucket extrema and endpoints; never bridge omitted missing intervals.
// This runs on the reader worker and bounds GDI work when the menu paints.
inline void compact_chart(Round& round){
    if(round.points.size()<=1500)return;
    const size_t width=(round.points.size()+159)/160;std::set<size_t> selected;
    for(size_t start=0;start<round.points.size();start+=width){
        size_t end=std::min(start+width,round.points.size());selected.insert(start);selected.insert(end-1);
        for(size_t metric=0;metric<3;++metric){size_t lo=start,hi=start;bool found=false;
            for(size_t i=start;i<end;++i)if(round.points[i].values[metric]){
                if(!found||*round.points[i].values[metric]<*round.points[lo].values[metric])lo=i;
                if(!found||*round.points[i].values[metric]>*round.points[hi].values[metric])hi=i;
                found=true;
            }if(found){selected.insert(lo);selected.insert(hi);}
        }
    }
    std::vector<Point> reduced;size_t previous=0;
    for(auto index:selected){auto point=round.points[index];
        for(size_t i=previous;i<=index;++i)for(size_t m=0;m<3;++m)point.breaks[m]=point.breaks[m]||round.points[i].gap||!round.points[i].values[m]||!round.points[i].round_time;
        reduced.push_back(point);previous=index+1;
    }round.points=std::move(reduced);
}
// Only explicit recorder session identifiers may join files into a local run.
inline void parse_recording(const std::string& input,Report& report){
    struct Sample { double time;Json tracking;unsigned dropped; };
    struct Group {std::map<double,Sample> samples;double previous=-1;bool rollback=false;};
    std::map<int,Group> groups;std::istringstream stream(input);std::string line;
    double interval=10000,started=0;std::string run_id;size_t count=0;
    while(std::getline(stream,line)){
        if(line.size()>1024*1024||++count>200000){throw std::runtime_error("Recording exceeds analysis limits");}
        try{
            auto row=Json::parse(line,[](int depth,Json::parse_event_t,Json&){if(depth>32)throw std::runtime_error("JSON nesting limit");return true;});
            if(!row.is_object()){++report.warnings;continue;}
            const auto event=row.find("event");if(event==row.end()||!event->is_string())continue;
            if(*event=="recording_started"){
                if(row.contains("started_utc_ms")){auto v=numeric(row["started_utc_ms"]);if(v&&*v>=946684800000.0&&*v<=4102444800000.0)started=*v;}
                if(row.contains("timeline")&&row["timeline"].is_object()){auto& t=row["timeline"];if(t.contains("match_id")&&t["match_id"].is_string()){auto id=t["match_id"].get<std::string>();if(id.size()==32&&id.find_first_not_of("0123456789abcdef")==std::string::npos)run_id=id;}}

                if(row.contains("sampling_interval_ms")){auto v=numeric(row["sampling_interval_ms"]);if(v&&*v>=100&&*v<=10000)interval=*v;}
                continue;
            }
            if(*event!="observed_state"&&*event!="heartbeat"&&*event!="round_ended"&&*event!="recording_stopped")continue;
            if(!row.contains("tracking")||!row["tracking"].is_object()||!row.contains("elapsed_ms"))continue;
            const auto elapsed=numeric(row["elapsed_ms"]);const auto& tr=row["tracking"];
            if(!elapsed||!tr.contains("round")||!tr["round"].is_number_integer())continue;
            auto rn=numeric(tr["round"]);if(!rn||*rn>10000)continue;
            auto& group=groups[int(*rn)];if(group.previous>*elapsed)group.rollback=true;group.previous=*elapsed;
            unsigned dropped=0;if(row.contains("dropped_events")){auto d=numeric(row["dropped_events"]);if(d&&*d<=1000000000)dropped=unsigned(*d);}
            group.samples.insert_or_assign(*elapsed,Sample{*elapsed,tr,dropped});
        }catch(...){++report.warnings;}
    }
    for(auto& entry:groups){
        auto& group=entry.second;if(group.samples.empty())continue;
        auto end=std::prev(group.samples.end());bool ended=false;
        for(auto it=group.samples.begin();it!=group.samples.end();++it)if(yes(it->second.tracking,"round_end_observed")){end=it;ended=true;}
        Round round;round.run_id=run_id;round.started_utc_ms=started;round.number=entry.first;round.ended=ended;round.rollback=group.rollback;
        round.complete=ended&&yes(end->second.tracking,"full_round_timing")&&!round.rollback;
        round.player_class=class_name(end->second.tracking);if(ended)round.end_level=metric(end->second.tracking,"level");
        double previous=-1;
        for(const auto& item:group.samples){if(item.first>end->first)break;const auto& sample=item.second;
            Point point;point.time=sample.time;point.round_time=metric(sample.tracking,"roundTime");if(point.round_time){if(*point.round_time<=14400)*point.round_time*=1000;else point.round_time.reset();}if(sample.tracking.contains("round_elapsed_ms")){auto clock=numeric(sample.tracking["round_elapsed_ms"]);if(clock&&*clock<=14400000)point.round_time=clock;}if(sample.tracking.contains("round_countdown_ms")&&sample.tracking["round_countdown_ms"].is_number()){double v=sample.tracking["round_countdown_ms"].get<double>();if(std::isfinite(v)&&v>=-14400000&&v<=7200000)point.countdown=v;}point.values={metric(sample.tracking,"level"),metric(sample.tracking,"xp"),metric(sample.tracking,"money")};
            point.gap=previous>=0&&sample.time-previous>std::max(3000.0,interval*1.5);previous=sample.time;
            if(!round.points.empty()&&point.round_time&&round.points.back().round_time&&*point.round_time<*round.points.back().round_time)point.gap=true;
            round.gaps|=point.gap;round.dropped=std::max(round.dropped,sample.dropped);round.points.push_back(point);
        }
        if(round.rollback||round.gaps||round.dropped)++report.warnings;
        compact_chart(round);report.rounds.push_back(std::move(round));
    }
}
struct Run {std::string id;double started_utc_ms=0;std::vector<size_t> rounds;};
inline std::vector<Run> runs(const Report& report,int filter=0){
    std::vector<Run> out;std::map<std::string,size_t> known;
    for(size_t i=0;i<report.rounds.size();++i){const auto& r=report.rounds[i];
        if(filter>0&&filter<int(classes.size())&&r.player_class!=classes[size_t(filter)])continue;
        auto id=r.run_id.empty()?"legacy-"+std::to_string(i):r.run_id;
        auto found=known.find(id);if(found==known.end()){known[id]=out.size();out.push_back({id,r.started_utc_ms,{i}});}else{auto& run=out[found->second];run.rounds.push_back(i);if(r.started_utc_ms>0&&(run.started_utc_ms==0||r.started_utc_ms<run.started_utc_ms))run.started_utc_ms=r.started_utc_ms;}
    }
    // File names (including recovered archives) do not define match chronology.
    std::stable_sort(out.begin(),out.end(),[](const Run& a,const Run& b){return a.started_utc_ms>b.started_utc_ms;});
    return out;
}
inline double axis_step(double maximum,int intervals){
    double raw=std::max(1.0,maximum/std::max(1,intervals)),power=std::pow(10.0,std::floor(std::log10(raw))),fraction=raw/power;
    return (fraction<=1?1:fraction<=2?2:fraction<=5?5:10)*power;
}
// Each round owns its observed endpoint; no minute rounding or shared duration.
struct RoundAxis {double end=0;Value countdown_start;bool conflict=false;
    void include(const Round& round){for(const auto& p:round.points)if(p.round_time){end=std::max(end,*p.round_time);if(p.countdown){double origin=*p.round_time+*p.countdown;if(origin<0||origin>7200000)conflict=true;else if(countdown_start&&std::abs(*countdown_start-origin)>10)conflict=true;else countdown_start=origin;}}}
    double label(double elapsed)const{return countdown_start&&!conflict?*countdown_start-elapsed:elapsed;}
    bool countdown()const{return countdown_start&&!conflict;}
};
struct Summary {size_t observed=0,complete=0,level_samples=0;Value average;std::string most_played;std::map<std::string,size_t> class_counts;};
inline std::vector<size_t> filtered(const Report& report,int filter){
    std::vector<size_t> result;for(size_t i=0;i<report.rounds.size();++i)if(filter<=0||filter>=int(classes.size())||report.rounds[i].player_class==classes[size_t(filter)])result.push_back(i);return result;
}
inline Summary summarize(const Report& report,int filter=0){
    Summary out;double total=0;for(auto i:filtered(report,filter)){const auto& r=report.rounds[i];++out.observed;if(!r.complete)continue;++out.complete;
        if(r.end_level){++out.level_samples;total+=*r.end_level;}if(!r.player_class.empty())++out.class_counts[r.player_class];}
    if(out.level_samples)out.average=total/out.level_samples;
    size_t maximum=0;for(const auto& entry:out.class_counts)if(entry.second>maximum){maximum=entry.second;out.most_played=entry.first;}else if(entry.second==maximum)out.most_played="Tied";
    return out;
}
}
