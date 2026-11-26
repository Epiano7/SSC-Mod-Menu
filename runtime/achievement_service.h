#pragma once
#include "achievement_api.h"
#include "achievement_community.h"
#include <windows.h>
#include <winhttp.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>
#include <optional>

namespace ssc_achievements {
struct PendingClaim {Claim claim;uint64_t next=0;unsigned attempts=0;int userid=0;};
struct Saved {Ledger ledger;ImportState imported;std::deque<PendingClaim> pending;};
inline Json pending_json(const std::deque<PendingClaim>& pending){
 Json a=Json::array();for(const auto& p:pending)a.push_back({{"start",p.claim.start},{"finish",p.claim.finish},{"full",p.claim.full},{"chaos",p.claim.chaos},{"blind",p.claim.blind},{"attempts",p.attempts},{"userid",p.userid}});return a;
}
inline bool pending_parse(const Json& j,std::deque<PendingClaim>& pending){
 if(!j.is_array()||j.size()>4)return false;
 try{for(const auto& row:j){
  PendingClaim p;uint64_t attempts=0,userid=0;
  if(!natural(row.at("start"),p.claim.start,4102444800ULL)||!natural(row.at("finish"),p.claim.finish,4102444800ULL)||!p.claim.start||p.claim.finish<p.claim.start||p.claim.finish-p.claim.start>28800)return false;
  if(!natural(row.at("attempts"),attempts,32)||!natural(row.at("userid"),userid,INT_MAX)||!userid)return false;
  p.attempts=unsigned(attempts);p.userid=int(userid);p.claim.full=row.at("full").get<bool>();p.claim.chaos=row.at("chaos").get<bool>();p.claim.blind=row.at("blind").get<bool>();pending.push_back(p);
 }}catch(...){return false;}return true;
}
inline Json pack(const Saved& s){auto j=s.ledger.json();j["api_import"]=s.imported.json();j["pending_results"]=pending_json(s.pending);return j;}
inline bool unpack(const Json& j,Saved& s){Saved next;if(!Ledger::parse(j,next.ledger))return false;if(j.contains("api_import")&&!ImportState::parse(j["api_import"],next.imported))return false;if(j.contains("pending_results")&&!pending_parse(j["pending_results"],next.pending))return false;s=next;return true;}
inline Json read_json(const std::filesystem::path& path){
 std::error_code ec;auto n=std::filesystem::file_size(path,ec);if(ec||n>65536)return nullptr;
 std::ifstream f(path,std::ios::binary);std::string raw((std::istreambuf_iterator<char>(f)),{});if(!f.good()&&!f.eof())return nullptr;
 try{return Json::parse(raw,[](int depth,Json::parse_event_t,Json&){if(depth>12)throw std::runtime_error("Achievement nesting limit");return true;});}catch(...){return nullptr;}
}
inline bool write_saved(const std::filesystem::path& path,const Saved& saved){
 try{
  auto bytes=pack(saved).dump(2);if(bytes.size()>65536)return false;
  std::filesystem::create_directories(path.parent_path());auto tmp=path;tmp+=L".tmp";auto backup=path;backup+=L".bak";
  HANDLE f=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)return false;
  DWORD n=0;bool ok=WriteFile(f,bytes.data(),DWORD(bytes.size()),&n,nullptr)&&n==bytes.size()&&FlushFileBuffers(f);CloseHandle(f);if(!ok){DeleteFileW(tmp.c_str());return false;}
  std::error_code ec;
  if(std::filesystem::exists(path,ec)){
   Saved previous;if(!unpack(read_json(path),previous)){DeleteFileW(tmp.c_str());return false;}
   if(!CopyFileW(path.c_str(),backup.c_str(),FALSE)){DeleteFileW(tmp.c_str());return false;}
  }
  return MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 }catch(...){return false;}
}
inline Json get_achievement_data(const wchar_t* host,const std::wstring& path){
 auto session=WinHttpOpen(L"SSC-Achievements/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!session)return nullptr;
 WinHttpSetTimeouts(session,2500,2500,3500,3500);auto connection=WinHttpConnect(session,host,443,0);
 auto request=connection?WinHttpOpenRequest(connection,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr;
 std::string raw;bool ok=false;
 if(request){
  DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER,disabled=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
  WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof policy);WinHttpSetOption(request,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof disabled);
  if(WinHttpSendRequest(request,L"Accept: application/json\r\n",DWORD(-1),WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(request,nullptr)){
   DWORD status=0,size=sizeof status;ok=WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)&&status==200;
   while(ok){char buffer[4096];DWORD got=0;if(!WinHttpReadData(request,buffer,sizeof buffer,&got)){ok=false;break;}if(!got)break;if(raw.size()+got>1048576){ok=false;break;}raw.append(buffer,got);}
  }WinHttpCloseHandle(request);
 }if(connection)WinHttpCloseHandle(connection);WinHttpCloseHandle(session);
 if(!ok)return nullptr;
 try{return Json::parse(raw,[](int depth,Json::parse_event_t,Json&){if(depth>24)throw std::runtime_error("Achievement API nesting limit");return true;});}catch(...){return nullptr;}
}
inline Json get_profile(int userid){if(userid<=0)return nullptr;return get_achievement_data(L"skillshotcity.gg",L"/api/players/"+std::to_wstring(userid)+L"/profile?limit=10&competitive=false");}
struct Reply {uint64_t account=0,generation=0,utc=0;int userid=0;bool loaded=false,import=false,statistics=false;Saved saved;bool failed=false;Claim claim;Json api;community::Reply community_reply;};
class Service {
 struct Work {uint64_t account=0,generation=0,utc=0;int userid=0;bool load=false,import=false,statistics=false;std::optional<Saved> save;Claim claim;community::Request community_request;};
 std::mutex mutex_;std::condition_variable wake_;std::filesystem::path root_;bool stop_=false;std::deque<Work> pending_;std::deque<Reply> replies_;std::thread worker_;
 std::filesystem::path path(uint64_t account)const{return root_/L"achievements"/(std::to_wstring(account)+L".json");}
 void run(){
  for(;;){Work w;{std::unique_lock<std::mutex> lock(mutex_);wake_.wait(lock,[&]{return stop_||!pending_.empty();});if(stop_&&pending_.empty())return;w=std::move(pending_.front());pending_.pop_front();}
   Reply r;r.account=w.account;r.generation=w.generation;r.utc=w.utc;r.import=w.import;r.statistics=w.statistics;r.claim=w.claim;r.userid=w.userid;
   try{
    if(w.load){
     std::error_code ec;bool exists=std::filesystem::exists(path(w.account),ec);r.loaded=true;
     if(ec||(exists&&!unpack(read_json(path(w.account)),r.saved))){r.failed=true;}
    }else if(w.save){r.failed=!write_saved(path(w.account),*w.save);}
    else if(w.community_request.action!=community::Action::none){r.community_reply=community::perform(root_,w.community_request,w.account);}
    else if(w.statistics){r.api=get_achievement_data(L"api.epiano7.dev",L"/v1/achievements/statistics-v3");}
    else if(w.userid>0){r.api=get_profile(w.userid);r.failed=r.api.is_null();}
   }catch(...){r.failed=true;}
   {std::lock_guard<std::mutex> lock(mutex_);if(replies_.size()<16)replies_.push_back(std::move(r));}
  }
 }
 bool enqueue(Work w){std::lock_guard<std::mutex> lock(mutex_);if(stop_||pending_.size()>=8)return false;pending_.push_back(std::move(w));wake_.notify_one();return true;}
public:
 explicit Service(const std::filesystem::path& root):root_(root),worker_([this]{run();}){}
 ~Service(){{std::lock_guard<std::mutex> lock(mutex_);stop_=true;}wake_.notify_one();worker_.join();}
 Service(const Service&)=delete;Service& operator=(const Service&)=delete;
 bool load(uint64_t account,uint64_t generation){Work w;w.account=account;w.generation=generation;w.load=true;return enqueue(std::move(w));}
 bool save(uint64_t account,uint64_t generation,const Saved& saved){
  std::lock_guard<std::mutex> lock(mutex_);if(stop_)return false;
  for(auto& w:pending_)if(w.account==account&&w.generation==generation&&w.save){w.save=saved;return true;}
  if(pending_.size()>=8)return false;
  Work w;w.account=account;w.generation=generation;w.save=saved;pending_.push_back(std::move(w));wake_.notify_one();return true;
 }
 bool api(uint64_t account,uint64_t generation,int userid,uint64_t utc,bool importing,const Claim& claim={}){
  Work w;w.account=account;w.generation=generation;w.userid=userid;w.utc=utc;w.import=importing;w.claim=claim;return enqueue(std::move(w));
 }
 bool sync_community(uint64_t account,uint64_t generation,community::Request request){
  Work w;w.account=account;w.generation=generation;w.community_request=std::move(request);return enqueue(std::move(w));
 }
 bool statistics(){Work w;w.statistics=true;return enqueue(std::move(w));}
 std::deque<Reply> poll(){std::lock_guard<std::mutex> lock(mutex_);std::deque<Reply> out;out.swap(replies_);return out;}
};
}
