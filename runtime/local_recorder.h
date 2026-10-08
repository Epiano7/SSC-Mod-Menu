#pragma once
#include <windows.h>
#include <atomic>
#include <bcrypt.h>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include "third_party/json.hpp"
namespace ssc_record {
using Json=nlohmann::json;
struct Message { int kind=0;std::string line;std::filesystem::path folder; };
struct State {
    std::mutex mutex;std::condition_variable wake;std::deque<Message> queue;
    std::atomic<bool> enabled{false};std::atomic<unsigned> status{0},dropped{0},written{0};
    bool active=false,match_open=false;std::filesystem::path folder;Json tracking=nullptr,last_build=nullptr;ULONGLONG build_sampled=0;
    std::string match_id;ULONGLONG match_started=0;unsigned segment=0;
    bool launched=false;std::string previous;ULONGLONG started=0,last_sample=0;unsigned sequence=0;
};
inline State& state(){static auto* value=new State;return *value;}
inline std::string new_match_id(){
    unsigned char bytes[16];if(BCryptGenRandom(nullptr,bytes,sizeof(bytes),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)return {};
    std::string id;const char* hex="0123456789abcdef";for(auto b:bytes){id+=hex[b>>4];id+=hex[b&15];}return id;
}
inline uint64_t utc_milliseconds(){FILETIME ft{};GetSystemTimeAsFileTime(&ft);ULARGE_INTEGER value{};value.LowPart=ft.dwLowDateTime;value.HighPart=ft.dwHighDateTime;return (value.QuadPart-116444736000000000ULL)/10000;}
inline Json timeline_stamp(ULONGLONG now){auto& s=state();
    return {{"match_id",s.match_id.empty()?Json(nullptr):Json(s.match_id)},{"match_elapsed_ms",now>=s.match_started?now-s.match_started:0},{"segment",s.segment}};
}
inline std::string executable_hash(){
    wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return {};std::ifstream input(std::filesystem::path(path),std::ios::binary);if(!input)return {};
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;unsigned char digest[32];
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
    bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;char buffer[65536];
    while(ok&&input){input.read(buffer,sizeof(buffer));auto count=input.gcount();if(count>0)ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer),ULONG(count),0)>=0;}
    ok=ok&&input.eof()&&BCryptFinishHash(hash,digest,32,0)>=0;std::string result;
    if(ok){const char* hex="0123456789abcdef";for(auto value:digest){result+=hex[value>>4];result+=hex[value&15];}}
    if(hash){BCryptDestroyHash(hash);}BCryptCloseAlgorithmProvider(algorithm,0);return result;
}
inline void worker(State* s){
    std::ofstream file;size_t bytes=0,limit=10*1024*1024;unsigned serial=0;
    for(;;){Message message;{std::unique_lock<std::mutex> lock(s->mutex);s->wake.wait(lock,[&]{return !s->queue.empty();});message=std::move(s->queue.front());s->queue.pop_front();}
        try {
            if(message.kind==1){file.close();file.clear();bytes=0;std::filesystem::create_directories(message.folder);uintmax_t total=0;
                for(const auto& entry:std::filesystem::directory_iterator(message.folder))if(entry.is_regular_file()&&entry.path().extension()==L".jsonl")total+=entry.file_size();
                if(total>=100*1024*1024){s->status=3;s->enabled=false;continue;}
                SYSTEMTIME now{};GetSystemTime(&now);limit=size_t(std::min<uintmax_t>(10*1024*1024,100*1024*1024-total));wchar_t name[100];swprintf(name,100,L"session-%04u%02u%02u-%02u%02u%02u-%llu-%u-%u.jsonl",now.wYear,now.wMonth,now.wDay,now.wHour,now.wMinute,now.wSecond,GetTickCount64(),GetCurrentProcessId(),++serial);
                file.open(message.folder/name,std::ios::binary|std::ios::out);if(!file){s->status=2;s->enabled=false;continue;}s->status=1;auto header=Json::parse(message.line);auto hash=executable_hash();header["game_sha256"]=hash.empty()?Json(nullptr):Json(hash);message.line=header.dump();
            }
            if(file.is_open()&&!message.line.empty()){
                if(bytes+message.line.size()+1>limit){file.close();s->status=3;s->enabled=false;continue;}
                file<<message.line<<'\n';file.flush();if(!file){file.close();s->status=2;s->enabled=false;continue;}bytes+=message.line.size()+1;++s->written;
            }
            if(message.kind==2){file.close();if(s->status==1||s->status==5)s->status=s->enabled?6:0;}
        }catch(...){file.close();s->status=2;s->enabled=false;}
    }
}
inline void enqueue(Message message,bool control=false){auto& s=state();std::unique_lock<std::mutex> lock(s.mutex,std::defer_lock);
    if(control)lock.lock();else if(!lock.try_lock()){++s.dropped;return;}
    if(s.queue.size()>=(control?2048u:2046u)){++s.dropped;if(!control)return;s.queue.pop_back();}
    s.queue.push_back(std::move(message));s.wake.notify_one();
}
// The UI toggle arms observation; opening a file requires an active round.
inline void start(const std::filesystem::path& folder){auto& s=state();if(s.enabled.exchange(true))return;
    s.folder=folder;s.status=6;s.dropped=0;s.written=0;s.match_id.clear();s.match_started=0;s.segment=0;
}
// Round boundaries are records within the match file, never file closes.
inline void end_round(){auto& s=state();if(!s.active)return;s.active=false;
    auto now=GetTickCount64();Json event={{"event","round_ended"},{"local_build",s.last_build},{"build_sample_age_ms",s.build_sampled&&now>=s.build_sampled?Json(now-s.build_sampled):Json(nullptr)},{"elapsed_ms",now-s.started},{"dropped_events",s.dropped.load()},{"tracking",s.tracking},{"timeline",timeline_stamp(now)}};enqueue({0,event.dump(),{}},true);
}
inline void end_match(){auto& s=state();end_round();if(!s.match_open)return;s.match_open=false;
    if(s.status!=2&&s.status!=3)s.status=5;
    auto now=GetTickCount64();Json event={{"event","recording_stopped"},{"elapsed_ms",now-s.started},{"dropped_events",s.dropped.load()},{"tracking",nullptr},{"timeline",timeline_stamp(now)}};enqueue({2,event.dump(),{}},true);s.match_id.clear();
}
inline void stop(){auto& s=state();s.enabled=false;end_match();if(s.status==6)s.status=0;}
inline void round_state(bool active){auto& s=state();if(!s.enabled||!active){end_round();return;}if(s.active)return;
    if(!s.launched){HMODULE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&state),&module);try{std::thread(worker,&s).detach();s.launched=true;}catch(...){s.enabled=false;s.status=2;return;}}
    if(s.match_id.empty()){s.match_id=new_match_id();s.match_started=GetTickCount64();}
    ++s.segment;s.active=true;s.tracking=nullptr;s.last_build=nullptr;s.build_sampled=0;s.previous.clear();s.last_sample=0;
    if(!s.match_open){s.match_open=true;s.status=4;s.started=GetTickCount64();s.sequence=0;
        Json header={{"schema",4},{"started_utc_ms",utc_milliseconds()},{"event","recording_started"},{"source","native_status_1hz"},{"tracking_schema",1},{"combat_counters_available",false},{"roster_available",false},{"skill_choices_available",false},{"build_schema",1},{"skill_snapshots","per_sample_availability"},{"random_class_choice",nullptr},{"timeline",timeline_stamp(s.started)},{"sampling_interval_ms",1000},{"mod_version","0.1.12"}};
        enqueue({1,header.dump(),s.folder},true);
    }
}
inline Json build_identity(const Json& b){
 Json result=Json::object();if(!b.is_object())return nullptr;
 for(const auto* key:{"class_id","skills","inventory_weapons","skills_status","mutation_status"}){
  auto it=b.find(key);if(it!=b.end())result[key]=*it;
 }return result;
}
inline void sample(int phase,const std::string& mode,const std::string& build,ULONGLONG now,const Json& build_fields=nullptr){auto& s=state();if(!s.enabled||!s.active||phase!=2)return;
    // Native fallback contains game-mode/class/level labels, never player names.
    std::string identity=std::to_string(phase)+"|"+mode+"|"+build+"|"+build_fields.dump()+"|"+s.tracking.dump();
    if(identity==s.previous&&now>=s.last_sample&&now-s.last_sample<1000)return;
    bool build_changed=build_identity(build_fields)!=build_identity(s.last_build);
    Json event={{"build_changed",build_changed},{"event",identity==s.previous?"heartbeat":"observed_state"},{"sequence",++s.sequence},{"elapsed_ms",now>=s.started?now-s.started:0},{"sample_gap_ms",s.last_sample&&now>=s.last_sample?now-s.last_sample:0},{"phase",phase},{"mode_round_label",mode},{"class_level_label",build},{"local_build",build_fields},{"tracking",s.tracking},{"timeline",timeline_stamp(now)},{"dropped_events",s.dropped.load()}};
    enqueue({0,event.dump(),{}});s.previous=identity;s.last_sample=now;s.last_build=build_fields;s.build_sampled=now;
}
}
