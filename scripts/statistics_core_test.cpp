#include "../runtime/statistics_reader.h"
#include <cassert>
#include <iostream>
using namespace ssc_stats;
Json sample(double time,int level=7,bool ended=false){return {{"event","observed_state"},{"elapsed_ms",time},{"tracking",{{"round",1},{"round_end_observed",ended},{"full_round_timing",true},{"metrics",{{"level",level},{"money",0},{"className","Caster"}}},{"availability",{{"level","observed"},{"money","observed"},{"className","observed"}}}}}};}
int main(int argc,char** argv){
    if(argc==3&&std::string(argv[1])=="--inspect"){
        auto report=read_folder(argv[2]);auto summary=summarize(report);
        for(const auto& run:runs(report)){Json axes=Json::array();for(auto index:run.rounds){RoundAxis axis;axis.include(report.rounds[index]);axes.push_back({{"round",report.rounds[index].number},{"countdown",axis.countdown()},{"start",axis.label(0)},{"end",axis.label(axis.end)}});}std::cout<<Json({{"started_utc_ms",run.started_utc_ms},{"axes",axes}}).dump()<<'\n';}
        std::cout<<Json({{"files",report.files},{"observed",summary.observed},{"complete",summary.complete},{"average_end_level",summary.average?Json(*summary.average):Json(nullptr)},{"class_counts",summary.class_counts},{"warnings",report.warnings}}).dump()<<'\n';return 0;
    }
    Report report;parse_recording(sample(0).dump()+"\n"+sample(1000,9,true).dump(),report);
    assert(report.rounds.size()==1&&report.rounds[0].complete);assert(summarize(report).average==9);assert(summarize(report,1).observed==0);
    assert(!report.rounds[0].points[0].values[1]&&report.rounds[0].points[0].values[2]==0);
    Report gap;parse_recording("{\"event\":\"recording_started\",\"sampling_interval_ms\":1000}\n"+sample(0).dump()+"\n"+sample(20000,7,true).dump(),gap);assert(gap.rounds[0].gaps);
    Report rollback;parse_recording(sample(1000).dump()+"\n"+sample(500,7,true).dump(),rollback);assert(!rollback.rounds[0].complete&&rollback.rounds[0].rollback);
    Report duplicate;parse_recording(sample(0).dump()+"\n"+sample(0,9,true).dump(),duplicate);assert(summarize(duplicate).average==9&&duplicate.rounds[0].points.size()==1);
    auto invalid=sample(0,7,true);invalid["tracking"]["metrics"]["level"]="7";Report bad;parse_recording(invalid.dump()+"\nnull\n{",bad);assert(!summarize(bad).average&&bad.warnings==2);
    Round dense;for(int i=0;i<10000;++i)dense.points.push_back({double(i),{double(i%10),1.0,2.0},false,{},double(i)});
    dense.points[22].values[0]=999.0;dense.points[23].values[1].reset();compact_chart(dense);assert(dense.points.size()<=1280&&dense.points.front().time==0&&dense.points.back().time==9999);
    assert(std::any_of(dense.points.begin(),dense.points.end(),[](const Point& p){return p.values[0]==999;}));
    assert(std::any_of(dense.points.begin(),dense.points.end(),[](const Point& p){return p.breaks[1]&&!p.breaks[0];}));
    // Run grouping is explicit; partial starts retain their actual round clock.
    Report grouped;Json header={{"event","recording_started"},{"started_utc_ms",1791228600000LL},{"timeline",{{"match_id",std::string(32,'a')}}}};
    auto late=sample(0);late["tracking"]["round"]=2;late["tracking"]["round_elapsed_ms"]=75000;
    parse_recording(header.dump()+"\n"+late.dump(),grouped);late["tracking"]["round"]=3;parse_recording(header.dump()+"\n"+late.dump(),grouped);
    assert(runs(grouped).size()==1&&runs(grouped)[0].rounds.size()==2&&grouped.rounds[0].points[0].round_time==75000);
    parse_recording(sample(0).dump(),grouped);parse_recording(sample(0).dump(),grouped);assert(runs(grouped).size()==3);
    assert(!grouped.rounds.back().points[0].round_time);assert(axis_step(16247,6)==5000);
    assert(filename_date("session-20261005-140000-1.jsonl")>0&&filename_date("random.jsonl")==0);
    assert(local_date(1791228600000)!=L"Date unavailable");
    Report match_file;std::string match_rows=header.dump()+"\n";
    for(int rn=1;rn<=3;++rn){auto event=sample(rn*1000,7+rn,true);event["event"]="round_ended";event["tracking"]["round"]=rn;match_rows+=event.dump()+"\n";}
    match_rows+="{\"event\":\"recording_stopped\",\"tracking\":null,\"elapsed_ms\":4000}";
    parse_recording(match_rows,match_file);assert(match_file.rounds.size()==3&&runs(match_file).size()==1&&summarize(match_file).complete==3);
    Round timed;Point start;start.round_time=60000;start.countdown=120000;Point finish;finish.round_time=245123;finish.countdown=-65123;timed.points={start,finish};
    RoundAxis axis;axis.include(timed);assert(axis.end==245123&&axis.countdown()&&axis.label(0)==180000&&axis.label(axis.end)==-65123);
    Round shorter=timed;shorter.points.pop_back();RoundAxis short_axis;short_axis.include(shorter);assert(short_axis.end==60000);
    RoundAxis legacy_axis;legacy_axis.include(dense);assert(!legacy_axis.countdown());
    auto signed_row=sample(1000);signed_row["tracking"]["round_elapsed_ms"]=245123;signed_row["tracking"]["round_countdown_ms"]=-65123;Report signed_report;parse_recording(signed_row.dump(),signed_report);assert(signed_report.rounds[0].points[0].countdown==-65123);
    // A recovered filename or insertion order must never hide the latest match.
    Report ordering;Round old=timed;old.run_id="old";old.started_utc_ms=1000;old.player_class="Caster";ordering.rounds.push_back(old);
    Round latest=old;latest.run_id="latest";latest.started_utc_ms=3000;ordering.rounds.push_back(latest);
    Round recovered=old;recovered.run_id="recovered";recovered.started_utc_ms=2000;ordering.rounds.push_back(recovered);
    Round unknown=old;unknown.run_id="undated";unknown.started_utc_ms=0;ordering.rounds.push_back(unknown);
    ordering.rounds.push_back(latest);auto ordered=runs(ordering);assert(ordered.size()==4&&ordered[0].id=="latest"&&ordered[0].rounds.size()==2&&ordered[1].id=="recovered"&&ordered[2].id=="old"&&ordered[3].id=="undated");
    assert(runs(ordering,4)[0].id=="latest");
    assert(argc==2);auto folder=std::filesystem::path(argv[1]);assert(!std::filesystem::exists(folder));std::filesystem::create_directories(folder);
    {std::ofstream a(folder/"one.jsonl"),b(folder/"copy.jsonl");auto bytes=sample(0,8,true).dump();a<<bytes;b<<bytes;}
    auto loaded=read_folder(folder);assert(loaded.files==1&&loaded.rounds.size()==1&&summarize(loaded).average==8);
    refresh(folder);auto deadline=GetTickCount64()+5000;while(reader().busy&&GetTickCount64()<deadline)Sleep(10);assert(!reader().busy&&snapshot()->rounds.size()==1);
    std::cout<<"Statistics core, file deduplication and background-reader tests passed\n";
}
