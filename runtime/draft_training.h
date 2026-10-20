#pragma once
#include "draft_training_core.h"
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <atomic>
namespace ssc_training {
inline bool enabled=false;
inline Round round;
inline std::string random_id(){unsigned char bytes[16];if(BCryptGenRandom(nullptr,bytes,sizeof bytes,BCRYPT_USE_SYSTEM_PREFERRED_RNG))return {};std::string out;for(auto b:bytes){out+="0123456789abcdef"[b>>4];out+="0123456789abcdef"[b&15];}return out;}
inline bool valid_id(const std::string& id){return id.size()==32&&id.find_first_not_of("0123456789abcdef")==std::string::npos;}
inline int upload(const std::string& body,DWORD* http=nullptr){
 if(http)*http=0;
 HINTERNET s=WinHttpOpen(L"SSC-Draft-Training/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!s)return 0;
 WinHttpSetTimeouts(s,2000,2000,3000,3000);auto c=WinHttpConnect(s,L"api.epiano7.dev",443,0);
 auto r=c?WinHttpOpenRequest(c,L"POST",L"/api/draft-training/v1/round",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr;int result=0;
 if(r){DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(r,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof policy);
  if(WinHttpSendRequest(r,L"Content-Type: application/json\r\n",DWORD(-1),const_cast<char*>(body.data()),DWORD(body.size()),DWORD(body.size()),0)&&WinHttpReceiveResponse(r,nullptr)){DWORD status=0,size=sizeof status;if(WinHttpQueryHeaders(r,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)){if(http)*http=status;if(status==201||status==200)result=1;else if(status==400||status==409||status==413)result=-1;}}
  WinHttpCloseHandle(r);
 }if(c)WinHttpCloseHandle(c);WinHttpCloseHandle(s);return result;
}
// Disk and network work stay off the render thread. Queue is bounded to 32
// rounds (~8 MiB), survives restart, and is cleared when consent is disabled.
class Service {
 int (*send)(const std::string&,DWORD*)=upload;
 std::mutex mutex;std::condition_variable wake;std::deque<Json> pending;std::filesystem::path folder;bool consent=false,authorized=false,changed=false;
 void receipt(const std::filesystem::path& dir,const char* action,DWORD http=0){
  Json j={{"action",action},{"http_status",http},{"capture_state",capture.load()},{"submitted",submitted.load()},{"dropped",dropped.load()},{"tick",GetTickCount64()}};
  auto p=dir/L"upload-status.json",tmp=dir/L"upload-status.tmp";
  {std::ofstream f(tmp);f<<j.dump();f.close();if(!f)return;}
  MoveFileExW(tmp.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
 }
 void run(){for(;;){std::deque<Json> batch;std::filesystem::path dir;bool allow,access;
  {std::unique_lock<std::mutex> lock(mutex);wake.wait_for(lock,std::chrono::seconds(60),[&]{return changed||!pending.empty();});changed=false;batch.swap(pending);dir=folder;allow=consent;access=authorized;}
  if(dir.empty())continue;
  try{std::filesystem::create_directories(dir);std::vector<std::filesystem::path> files,rejected;
   for(const auto& e:std::filesystem::directory_iterator(dir))if(e.is_regular_file()&&valid_id(e.path().stem().string())){if(e.path().extension()==L".tmp")std::filesystem::remove(e.path());else if(e.path().extension()==L".json")files.push_back(e.path());else if(e.path().extension()==L".rejected")rejected.push_back(e.path());}
   if(!allow){for(const auto& p:files)std::filesystem::remove(p);for(const auto& p:rejected)std::filesystem::remove(p);state=0;receipt(dir,"disabled");continue;}
   for(const auto& j:batch){if(files.size()+rejected.size()>=32){state=3;++dropped;break;}auto id=j.at("round_id").get<std::string>();auto body=j.dump();if(!valid_id(id)||body.size()>max_bytes){++dropped;continue;}auto path=dir/(id+".json"),temp=dir/(id+".tmp");{std::ofstream f(temp,std::ios::binary|std::ios::trunc);f<<body;f.close();if(!f)continue;}std::filesystem::rename(temp,path);files.push_back(path);}
   if(!access){state=5;receipt(dir,"authorization_pending");continue;}
   bool failed=false;
   for(const auto& p:files){ {std::lock_guard<std::mutex> lock(mutex);if(!consent||!authorized)break;}
    if(std::filesystem::file_size(p)>max_bytes){std::filesystem::remove(p);continue;}
    std::ifstream f(p,std::ios::binary);std::string body((std::istreambuf_iterator<char>(f)),{});f.close();
    DWORD http=0;const int result=send(body,&http);if(result>0){receipt(dir,"accepted",http);std::filesystem::remove(p);state=2;}else if(result<0){auto retained=p;retained.replace_extension(L".rejected");std::filesystem::rename(p,retained);state=4;receipt(dir,"rejected_retained",http);}else {failed=true;state=3;receipt(dir,"retry_pending",http);break;}
   }if(files.empty()&&!failed&&state!=2&&state!=4){state=1;receipt(dir,"waiting");}
  }catch(...){state=3;}
 }}
public:
 std::atomic<int> state{0},capture{0};std::atomic<unsigned> submitted{0},dropped{0};
 explicit Service(int (*sender)(const std::string&,DWORD*)=upload):send(sender){std::thread([this]{run();}).detach();}
 void configure(const std::filesystem::path& root,bool allow,bool access=true){if(root.empty())return;std::lock_guard<std::mutex> lock(mutex);auto dir=root/L"draft-training-pending";if(folder==dir&&consent==allow&&authorized==access)return;folder=dir;consent=allow;authorized=access;if(!allow)pending.clear();changed=true;wake.notify_one();}
 void submit(Json j){if(j.is_null())return;std::lock_guard<std::mutex> lock(mutex);if(consent&&pending.size()<4){pending.push_back(std::move(j));++submitted;wake.notify_one();}else ++dropped;}
};
inline Service& service(){static auto* instance=new Service;return *instance;}
inline const wchar_t* status(){switch(service().state.load()){case 1:return L"Waiting for round data";case 2:return L"Round data sent";case 3:return L"Upload queued; retrying in the background";case 4:return L"Rejected round retained for diagnosis";case 5:return L"Waiting for private access; data retained";default:return L"Training uploads off";}}
}
