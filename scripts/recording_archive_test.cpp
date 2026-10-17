#include "../runtime/statistics_reader.h"
#include <cassert>
#include <iostream>
using namespace ssc_record_archive;
static std::map<std::string,std::string> contents(const std::filesystem::path& p){std::map<std::string,std::string> r;visit(p,[&](const auto& name,const auto& raw){r[name.string()]=ssc_stats::digest(raw);});return r;}
static std::string report(const std::filesystem::path& p){auto r=ssc_stats::read_folder(p);nlohmann::json j={{"files",r.files},{"warnings",r.warnings},{"rounds",nlohmann::json::array()}};for(auto& round:r.rounds){nlohmann::json v={{"number",round.number},{"class",round.player_class},{"complete",round.complete},{"ended",round.ended},{"run",round.run_id},{"started",round.started_utc_ms},{"points",nlohmann::json::array()}};for(auto& point:round.points){nlohmann::json a={point.time,point.gap,point.breaks};for(auto& value:point.values)a.push_back(value?nlohmann::json(*value):nlohmann::json(nullptr));a.push_back(point.countdown?nlohmann::json(*point.countdown):nlohmann::json(nullptr));a.push_back(point.round_time?nlohmann::json(*point.round_time):nlohmann::json(nullptr));v["points"].push_back(a);}j["rounds"].push_back(v);}return j.dump();}
int main(int argc,char** argv){
 assert(argc==3);std::filesystem::path source=argv[1],dir=argv[2];assert(!std::filesystem::exists(dir));std::filesystem::create_directories(dir);
 for(const auto& e:std::filesystem::directory_iterator(source))if(e.path().extension()==L".jsonl"||e.path().filename()==L"recordings.zip")std::filesystem::copy_file(e.path(),dir/e.path().filename());
 auto before=contents(dir);auto summary=report(dir);assert(!before.empty());compact(dir);assert(contents(dir)==before);assert(report(dir)==summary);
 auto zip=read(dir/L"recordings.zip",ssc_record::archive_budget_bytes);compact(dir);assert(read(dir/L"recordings.zip",ssc_record::archive_budget_bytes)==zip);
 // Simulated interruption after archive replacement, before loose-file removal.
 {Zip z(dir/L"recordings.zip");std::string name;auto raw=z.entry(0,name);std::ofstream(dir/name,std::ios::binary)<<raw;}
 assert(report(dir)==summary);compact(dir);assert(contents(dir)==before);
 std::ofstream(dir/L"extra.jsonl")<<"{\"event\":\"test\"}\n";compact(dir);assert(contents(dir).size()==before.size()+1);
 // A damaged archive cannot cause the next loose recording to be deleted.
 {std::ofstream f(dir/L"recordings.zip",std::ios::binary|std::ios::trunc);f<<"broken zip";}
 std::ofstream(dir/L"retained.jsonl")<<"{}\n";bool rejected=false;try{compact(dir);}catch(...){rejected=true;}assert(rejected&&std::filesystem::exists(dir/L"retained.jsonl"));
 // Per-entry expansion limit is enforced before attempting decompression.
 {Zip z;std::string huge(ssc_record::file_budget_bytes+1,'x');assert(mz_zip_writer_add_mem(&z.zip,"huge.jsonl",huge.data(),huge.size(),6));void* b=nullptr;size_t n=0;assert(mz_zip_writer_finalize_heap_archive(&z.zip,&b,&n));std::ofstream f(dir/L"recordings.zip",std::ios::binary|std::ios::trunc);f.write(static_cast<char*>(b),n);mz_free(b);}
 rejected=false;try{contents(dir);}catch(...){rejected=true;}assert(rejected);
 std::cout<<"PASS: "<<before.size()<<" real recordings; byte and Statistics parity, append, restart recovery, corruption preservation and expansion limit\n";
}
