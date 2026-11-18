#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <thread>
#include "statistics_core.h"
#include "recording_archive.h"
namespace ssc_stats {
inline std::string digest(const std::string& bytes){
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;unsigned char result[32];
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Unable to hash recordings");
    bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    ok=ok&&BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())),ULONG(bytes.size()),0)>=0;
    ok=ok&&BCryptFinishHash(hash,result,32,0)>=0;if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
    if(!ok)throw std::runtime_error("Unable to hash recordings");
    std::string out;const char* hex="0123456789abcdef";for(auto c:result){out+=hex[c>>4];out+=hex[c&15];}return out;
}
// Legacy recorder filenames use UTC, not the filesystem modified timestamp.
inline double filename_date(const std::filesystem::path& path){
    unsigned y,m,d,h,n,s;auto name=path.filename().string();if(sscanf(name.c_str(),"session-%4u%2u%2u-%2u%2u%2u-",&y,&m,&d,&h,&n,&s)!=6)return 0;
    if(y<2000||y>2100||m<1||m>12||d<1||d>31||h>23||n>59||s>59)return 0;
    SYSTEMTIME st{};st.wYear=WORD(y);st.wMonth=WORD(m);st.wDay=WORD(d);st.wHour=WORD(h);st.wMinute=WORD(n);st.wSecond=WORD(s);FILETIME ft{};if(!SystemTimeToFileTime(&st,&ft))return 0;
    ULARGE_INTEGER v{};v.LowPart=ft.dwLowDateTime;v.HighPart=ft.dwHighDateTime;return double((v.QuadPart-116444736000000000ULL)/10000);
}
inline std::wstring local_date(double utc){
    if(utc<=0)return L"Date unavailable";
    ULARGE_INTEGER v{};v.QuadPart=uint64_t(utc)*10000+116444736000000000ULL;FILETIME ft{v.LowPart,v.HighPart};SYSTEMTIME st{},local{};
    if(!FileTimeToSystemTime(&ft,&st)||!SystemTimeToTzSpecificLocalTime(nullptr,&st,&local))return L"Date unavailable";
    wchar_t date[128]{},time[128]{};if(!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT,DATE_SHORTDATE,&local,nullptr,date,128,nullptr)||!GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT,TIME_NOSECONDS,&local,nullptr,time,128))return L"Date unavailable";
    return std::wstring(date)+L" "+time;
}
inline Report read_folder(const std::filesystem::path& folder){
    Report result;if(!std::filesystem::exists(folder))return result;
    std::set<std::string> seen;size_t samples=0;
    ssc_record_archive::visit(folder,[&](const std::filesystem::path& path,const std::string& raw){
        if(!seen.insert(digest(raw)).second)return;
        ++result.files;size_t before=result.rounds.size();parse_recording(raw,result);
        for(size_t i=before;i<result.rounds.size();++i){auto& round=result.rounds[i];samples+=round.points.size();if(!round.started_utc_ms)round.started_utc_ms=filename_date(path);}
        if(samples>200000||result.rounds.size()>10000)throw std::runtime_error("Too many samples; select a smaller recording archive");
    });return result;
}
inline std::string folder_stamp(const std::filesystem::path& folder){
 std::vector<std::string> items;if(!std::filesystem::exists(folder))return "missing";
 for(const auto& e:std::filesystem::directory_iterator(folder))if(e.is_regular_file()&&(e.path().extension()==L".jsonl"||e.path().filename()==L"recordings.zip")){if(items.size()>=10001)throw std::runtime_error("Recording entry limit");items.push_back(e.path().filename().string()+":"+std::to_string(e.file_size())+":"+std::to_string(e.last_write_time().time_since_epoch().count()));}
 std::sort(items.begin(),items.end());std::string out;for(const auto& s:items)out+=s+"\n";return folder.lexically_normal().string()+"\n"+out;
}

// Derived personal-statistics cache only. Never used as official match results,
// ranked ratings or upload evidence. Originals and source limits remain intact.
inline Json cache_json(const Report& report,const std::string& stamp){
 Json out={{"schema",1},{"source",digest(stamp)},{"files",report.files},{"warnings",report.warnings},{"rounds",Json::array()}};
 for(const auto& r:report.rounds){Json points=Json::array();for(const auto& p:r.points){unsigned breaks=0;for(unsigned i=0;i<3;++i)if(p.breaks[i])breaks|=1u<<i;auto val=[](const Value& v){return v?Json(*v):Json(nullptr);};points.push_back(Json::array({p.time,val(p.values[0]),val(p.values[1]),val(p.values[2]),p.gap,breaks,val(p.round_time),val(p.countdown)}));}out["rounds"].push_back(Json::array({r.number,r.player_class,r.complete,r.ended,r.rollback,r.gaps,r.dropped,r.end_level?Json(*r.end_level):Json(nullptr),r.run_id,r.started_utc_ms,std::move(points)}));}
 return out;
}
inline Report decode_cache(const Json& j,const std::string& stamp){
 auto require=[](bool ok){if(!ok)throw std::runtime_error("Invalid statistics cache");};
 require(j.is_object()&&j.value("schema",0)==1&&j.value("source",std::string{})==digest(stamp)&&j.contains("rounds")&&j["rounds"].is_array()&&j["rounds"].size()<=10000);
 auto count=[&](const Json& v,uint64_t limit){require(v.is_number_unsigned()||(v.is_number_integer()&&v.get<int64_t>()>=0));auto n=v.get<uint64_t>();require(n<=limit);return n;};
 auto optional=[&](const Json& v,bool signed_clock=false)->Value{if(v.is_null())return {};require(v.is_number());double n=v.get<double>();require(std::isfinite(n)&&(signed_clock?(n>=-14400000&&n<=7200000):(n>=0&&n<9007199254740992.0)));return n;};
 Report out;out.files=unsigned(count(j.at("files"),10000));out.warnings=unsigned(count(j.at("warnings"),UINT_MAX));size_t samples=0;
 for(const auto& row:j["rounds"]){require(row.is_array()&&row.size()==11);Round r;r.number=int(count(row[0],10000));require(row[1].is_string());r.player_class=row[1].get<std::string>();require(r.player_class.empty()||std::find(classes.begin()+1,classes.end(),r.player_class)!=classes.end());
  for(int i=2;i<6;++i){require(row[size_t(i)].is_boolean());}
  r.complete=row[2];r.ended=row[3];r.rollback=row[4];r.gaps=row[5];r.dropped=unsigned(count(row[6],1000000000));r.end_level=optional(row[7]);require(row[8].is_string());r.run_id=row[8].get<std::string>();require(r.run_id.empty()||(r.run_id.size()==32&&r.run_id.find_first_not_of("0123456789abcdef")==std::string::npos));auto date=optional(row[9]);require(bool(date));r.started_utc_ms=*date;
  require(row[10].is_array()&&row[10].size()<=1500);samples+=row[10].size();require(samples<=200000);double previous=-1;
  for(const auto& value:row[10]){require(value.is_array()&&value.size()==8);Point p;auto time=optional(value[0]);require(bool(time)&&*time>=previous);p.time=*time;previous=p.time;for(int i=0;i<3;++i)p.values[size_t(i)]=optional(value[size_t(i+1)]);require(value[4].is_boolean());p.gap=value[4];unsigned bits=unsigned(count(value[5],7));for(unsigned i=0;i<3;++i)p.breaks[i]=(bits&(1u<<i))!=0;p.round_time=optional(value[6]);if(p.round_time)require(*p.round_time<=14400000);p.countdown=optional(value[7],true);r.points.push_back(p);}
  out.rounds.push_back(std::move(r));
 }
 return out;
}
inline std::shared_ptr<Report> read_cache(const std::filesystem::path& folder,const std::string& stamp){try{
 auto path=folder/L"statistics-cache-v1.json";if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>32*1024*1024)return {};
 auto raw=ssc_record_archive::read(path,32*1024*1024);auto json=Json::parse(raw,[](int depth,Json::parse_event_t,Json&){if(depth>16)throw std::runtime_error("Cache depth limit");return true;});return std::make_shared<Report>(decode_cache(json,stamp));
 }catch(...){return {};}}
inline void write_cache(const std::filesystem::path& folder,const std::string& stamp,const Report& report){try{
 auto raw=cache_json(report,stamp).dump();if(raw.size()>32*1024*1024)return;auto temporary=folder/(L"statistics-cache-"+std::to_wstring(GetCurrentProcessId())+L".tmp");{std::ofstream out(temporary,std::ios::binary|std::ios::trunc);out<<raw;out.close();if(!out)return;}if(!MoveFileExW(temporary.c_str(),(folder/L"statistics-cache-v1.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))DeleteFileW(temporary.c_str());
 }catch(...) {}}

// Cache each recording by its content digest. A newly finished match no longer
// forces JSON parsing of every older recording. This is derived local data only.
using EntryCache=std::map<std::string,Report>;
inline EntryCache read_entry_cache(const std::filesystem::path& folder){EntryCache out;try{
 auto path=folder/L"statistics-entries-v1.json";if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>32*1024*1024)return out;
 auto j=Json::parse(ssc_record_archive::read(path,32*1024*1024),[](int depth,Json::parse_event_t,Json&){if(depth>20)throw std::runtime_error("Cache nesting limit");return true;});
 if(j.value("schema",0)!=1||!j.contains("entries")||!j["entries"].is_object()||j["entries"].size()>10000)return out;
 size_t samples=0,rounds=0;for(auto it=j["entries"].begin();it!=j["entries"].end();++it){const auto& key=it.key();if(key.size()!=64||key.find_first_not_of("0123456789abcdef")!=std::string::npos)throw std::runtime_error("Cache identity");auto report=decode_cache(it.value(),key);rounds+=report.rounds.size();for(const auto& r:report.rounds)samples+=r.points.size();if(samples>200000||rounds>10000)throw std::runtime_error("Entry cache limit");out.emplace(key,std::move(report));}
 }catch(...){out.clear();}return out;}
inline void write_entry_cache(const std::filesystem::path& folder,const EntryCache& cache){try{
 Json j={{"schema",1},{"entries",Json::object()}};for(const auto& [key,report]:cache)j["entries"][key]=cache_json(report,key);auto raw=j.dump();if(raw.size()>32*1024*1024)return;
 auto temporary=folder/(L"statistics-entries-"+std::to_wstring(GetCurrentProcessId())+L".tmp");{std::ofstream f(temporary,std::ios::binary|std::ios::trunc);f<<raw;f.close();if(!f)return;}if(!MoveFileExW(temporary.c_str(),(folder/L"statistics-entries-v1.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))DeleteFileW(temporary.c_str());
 }catch(...) {}}
inline Report read_folder_cached(const std::filesystem::path& folder){
 Report result;if(!std::filesystem::exists(folder))return result;auto old=read_entry_cache(folder);EntryCache used;std::set<std::string> seen;size_t samples=0;
 ssc_record_archive::visit(folder,[&](const std::filesystem::path& path,const std::string& raw){auto key=digest(raw);if(!seen.insert(key).second)return;++result.files;
  auto found=old.find(key);Report parsed;if(found!=old.end())parsed=std::move(found->second);else parse_recording(raw,parsed);result.warnings+=parsed.warnings;used[key]=parsed;
  for(auto& round:parsed.rounds){samples+=round.points.size();if(!round.started_utc_ms)round.started_utc_ms=filename_date(path);result.rounds.push_back(std::move(round));}
  if(samples>200000||result.rounds.size()>10000)throw std::runtime_error("Recording limits");
 });write_entry_cache(folder,used);return result;
}
struct Reader {std::string stamp;std::atomic<bool> busy{false};std::atomic<unsigned> revision{0};std::shared_ptr<const Report> result=std::make_shared<Report>();};
inline Reader& reader(){static auto* value=new Reader;return *value;}
inline std::shared_ptr<const Report> snapshot(){return std::atomic_load(&reader().result);}
inline void refresh(const std::filesystem::path& folder,bool force=false){
    auto& state=reader();if(state.busy.exchange(true))return;
    HMODULE module=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&reader),&module)){state.busy=false;return;}
    try{std::thread([folder,force,&state]{
        SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
        auto report=std::make_shared<Report>();try{auto stamp=folder_stamp(folder);if(!force&&stamp==state.stamp){state.busy=false;return;}auto cached=force?std::shared_ptr<Report>{}:read_cache(folder,stamp);if(cached)report=std::move(cached);else {*report=read_folder_cached(folder);if(folder_stamp(folder)==stamp)write_cache(folder,stamp,*report);}state.stamp=std::move(stamp);}catch(const std::exception&){report->rounds.clear();report->error="Could not read this archive. Check file access and the 512 MiB limit.";}catch(...){report->error="Could not read recordings";}
        std::atomic_store(&state.result,std::shared_ptr<const Report>(report));state.busy=false;++state.revision;
    }).detach();}catch(...){state.busy=false;}
}
struct ComparisonResult {std::shared_ptr<const Report> report;int filter=0,metric=0;Comparison chart;};
inline std::atomic<bool> comparison_busy{false};inline std::shared_ptr<const ComparisonResult> comparison_result;
inline void request_comparison(std::shared_ptr<const Report> report,int filter,int metric){
 auto cached=std::atomic_load(&comparison_result);if(cached&&cached->report==report&&cached->filter==filter&&cached->metric==metric)return;
 if(comparison_busy.exchange(true))return;
 HMODULE module=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&request_comparison),&module)){comparison_busy=false;return;}
 try{std::thread([report,filter,metric]{SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);try{auto result=std::make_shared<ComparisonResult>();result->report=report;result->filter=filter;result->metric=metric;result->chart=compare(*report,filter,metric);std::atomic_store(&comparison_result,std::shared_ptr<const ComparisonResult>(result));}catch(...){}comparison_busy=false;++reader().revision;}).detach();}catch(...){comparison_busy=false;}
}

}
