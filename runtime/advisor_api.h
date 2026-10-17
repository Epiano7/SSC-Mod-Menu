#pragma once
#include "advisor_core.h"
#include "third_party/json.hpp"
#include <windows.h>
#include <winhttp.h>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <string>
namespace ssc_advisor {
struct ApiAnswer {std::string key;std::array<float,54> edge{};std::array<unsigned,54> samples{};bool valid=false;};
inline std::string api_key(const Build& b,const std::array<int,54>& offers,unsigned count){
 if(b.player_class<0||b.player_class>=8||!count||count>10)return {};
 std::string offered,owned;std::array<bool,54> seen{};
 for(unsigned i=0;i<count;++i){int id=offers[i];if(id<0||id>=54||seen[size_t(id)]||b.levels[size_t(id)]>0)return {};seen[size_t(id)]=true;}
 for(int i=0;i<54;++i){if(seen[size_t(i)]){if(!offered.empty())offered+=',';offered+=std::to_string(i);}if(b.levels[size_t(i)]>0){if(!owned.empty())owned+=',';owned+=std::to_string(i);}}
 return "classId="+std::to_string(b.player_class+1)+"&offered="+offered+(owned.empty()?"":"&owned="+owned)+"&version=current&limit=5";
}
inline ApiAnswer parse_advice(const std::string& key,const std::string& body){
 ApiAnswer a;a.key=key;try{auto j=nlohmann::json::parse(body);if(!j.contains("candidates")||!j["candidates"].is_array()||j["candidates"].size()>10)return a;
 for(auto& c:j["candidates"]){int id=c.at("skillId").get<int>();float edge=c.at("score").get<float>();if(id<0||id>=54||!std::isfinite(edge))return ApiAnswer{};
  auto n=c.at("alone").at("appearances").get<int>();if(n<0)return ApiAnswer{};a.edge[size_t(id)]=edge;a.samples[size_t(id)]=unsigned(n);
 }a.valid=true;}catch(...){a.valid=false;}return a;
}
inline ApiAnswer fetch_advice(const std::string& key){
 ApiAnswer a; a.key=key;
 HINTERNET session=WinHttpOpen(L"SSC-Mod-Menu-Draft/0.1.15",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);if(!session)return a;
 WinHttpSetTimeouts(session,1500,1500,1500,2500);
 auto connection=WinHttpConnect(session,L"skillshotcity.gg",INTERNET_DEFAULT_HTTPS_PORT,0);
 std::wstring path=L"/api/draft?"+std::wstring(key.begin(),key.end());
 auto request=connection?WinHttpOpenRequest(connection,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr;
 if(request){DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof policy);
  if(WinHttpSendRequest(request,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(request,nullptr)){
   DWORD status=0,size=sizeof status;std::string body;bool ok=WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX)&&status==200;
   while(ok){char bytes[4096];DWORD got=0;if(!WinHttpReadData(request,bytes,sizeof bytes,&got)){ok=false;break;}if(!got)break;if(body.size()+got>131072){ok=false;break;}body.append(bytes,got);}
   if(ok)a=parse_advice(key,body);
  }WinHttpCloseHandle(request);
 }if(connection)WinHttpCloseHandle(connection);WinHttpCloseHandle(session);return a;
}
class ApiService {
 std::mutex mutex;std::condition_variable wake;std::string pending,requested;ApiAnswer answer;uint64_t next=0;
 void run(){for(;;){std::string key;{std::unique_lock<std::mutex> lock(mutex);wake.wait(lock,[&]{return !pending.empty();});key=std::move(pending);pending.clear();}auto result=fetch_advice(key);{std::lock_guard<std::mutex> lock(mutex);answer=std::move(result);}}}
public:
 ApiService(){std::thread([this]{run();}).detach();}
 ApiAnswer request(const std::string& key,uint64_t now){std::lock_guard<std::mutex> lock(mutex);if(!key.empty()&&key!=requested&&now>=next){requested=key;pending=key;next=now+5000;wake.notify_one();}return answer.key==key?answer:ApiAnswer{};}
};
// Process-lifetime service; the DLL is not unloaded while hooks are installed.
inline ApiService& api(){static auto* service=new ApiService;return *service;}
}
