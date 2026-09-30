#pragma once
#include "beta_core.h"
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <thread>
namespace ssc_beta {
inline constexpr wchar_t beta_host[]=L"api.epiano7.dev";
inline int64_t wall_now(){FILETIME ft{};GetSystemTimeAsFileTime(&ft);ULARGE_INTEGER x{};x.LowPart=ft.dwLowDateTime;x.HighPart=ft.dwHighDateTime;return int64_t(x.QuadPart/10000000ULL)-11644473600LL;}
struct Identity {
 using Interface=void*(*)();Interface user=nullptr,utils=nullptr;bool(*logged)(void*)=nullptr;uint64_t(*id)(void*)=nullptr;uint32_t(*app)(void*)=nullptr;
 bool attach(){auto dll=GetModuleHandleW(L"steam_api64.dll");if(!dll)return false;
  auto current=reinterpret_cast<int(*)()>(GetProcAddress(dll,"SteamAPI_GetHSteamUser"));if(!current||current()<=0)return false;
  user=reinterpret_cast<Interface>(GetProcAddress(dll,"SteamAPI_SteamUser_v023"));utils=reinterpret_cast<Interface>(GetProcAddress(dll,"SteamAPI_SteamUtils_v010"));
  logged=reinterpret_cast<bool(*)(void*)>(GetProcAddress(dll,"SteamAPI_ISteamUser_BLoggedOn"));id=reinterpret_cast<uint64_t(*)(void*)>(GetProcAddress(dll,"SteamAPI_ISteamUser_GetSteamID"));app=reinterpret_cast<uint32_t(*)(void*)>(GetProcAddress(dll,"SteamAPI_ISteamUtils_GetAppID"));return user&&utils&&logged&&id&&app;
 }
 uint64_t read()const{if(!user||!utils||!logged||!id||!app)return 0;auto u=user(),v=utils();if(!u||!v||!logged(u)||app(v)!=308600)return 0;auto n=id(u);return steam_id(n)?n:0;}
};
struct Stored {uint64_t account=0;std::string token;};
// DPAPI is bound to the Windows user (or this user's Wine/Proton prefix).
// Never fall back to plaintext if secure storage is unavailable.
struct Storage {
 std::filesystem::path path;const wchar_t* description=L"SSC beta access";
 Stored load(){Stored s;if(path.empty())return s;std::ifstream in(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});if(bytes.empty()||bytes.size()>32768)return s;
  DATA_BLOB source{DWORD(bytes.size()),reinterpret_cast<BYTE*>(bytes.data())},plain{};
  if(!CryptUnprotectData(&source,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&plain))return s;
  ssc_auth_privacy::secrets_used=true;
  try{auto j=parse(std::string(reinterpret_cast<char*>(plain.pbData),plain.cbData));auto a=string(j,"steam_id",20);require(a.size()==17&&a.find_first_not_of("0123456789")==std::string::npos);s.account=std::stoull(a);require(steam_id(s.account));s.token=string(j,"token",8192);require(!s.token.empty());for(unsigned char c:s.token)require(c>=33&&c<=126);}catch(...){wipe(s.token);s.account=0;}
  SecureZeroMemory(plain.pbData,plain.cbData);LocalFree(plain.pbData);return s;
 }
 bool save(const Stored& s){if(path.empty())return false;auto text=Json{{"steam_id",std::to_string(s.account)},{"token",s.token}}.dump();DATA_BLOB plain{DWORD(text.size()),reinterpret_cast<BYTE*>(text.data())},cipher{};
  bool ok=CryptProtectData(&plain,description,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&cipher)!=0;wipe(text);if(!ok)return false;
  auto tmp=path;tmp+=L".tmp";HANDLE f=CreateFileW(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_HIDDEN,nullptr);DWORD written=0;
  ok=f!=INVALID_HANDLE_VALUE;if(ok){ok=WriteFile(f,cipher.pbData,cipher.cbData,&written,nullptr)&&written==cipher.cbData&&FlushFileBuffers(f);CloseHandle(f);}LocalFree(cipher.pbData);
  if(ok){ok=MoveFileExW(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;}
  if(!ok)DeleteFileW(tmp.c_str());
  return ok;
 }
 void remove(){if(!path.empty())DeleteFileW(path.c_str());}
};
struct Reply {unsigned status=0;std::string body;};
struct Http {
 const wchar_t* host=beta_host;
 std::mutex mutex;std::condition_variable timer;HINTERNET pending=nullptr;bool done=true,stopped=false;
 void cancel(){std::lock_guard<std::mutex> l(mutex);if(pending){WinHttpCloseHandle(pending);pending=nullptr;}}
 void stop(){std::lock_guard<std::mutex> l(mutex);stopped=true;if(pending){WinHttpCloseHandle(pending);pending=nullptr;}timer.notify_all();}
 Reply request(const wchar_t* path,const std::string& body,const std::string& token){
  struct Handle {HINTERNET h;~Handle(){if(h)WinHttpCloseHandle(h);}};
  Handle session{WinHttpOpen(L"SSCModBeta/1",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};require(session.h);
  require(WinHttpSetTimeouts(session.h,2500,2500,2500,2500));Handle connection{WinHttpConnect(session.h,host,INTERNET_DEFAULT_HTTPS_PORT,0)};require(connection.h);
  HINTERNET req=WinHttpOpenRequest(connection.h,body.empty()?L"GET":L"POST",path,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);require(req);
  {std::lock_guard<std::mutex> l(mutex);if(stopped){WinHttpCloseHandle(req);throw std::runtime_error("Stopped");}pending=req;done=false;}
  std::thread watchdog([this]{std::unique_lock<std::mutex> l(mutex);if(!timer.wait_for(l,std::chrono::seconds(10),[this]{return done||stopped;})){if(pending){WinHttpCloseHandle(pending);pending=nullptr;}}});
  Reply reply;
  try{DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER,disabled=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
   require(WinHttpSetOption(req,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof redirects));require(WinHttpSetOption(req,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof disabled));
   std::wstring headers=L"Content-Type: application/json\r\nAccept: application/json\r\n";
   if(!token.empty()){headers+=L"Authorization: Bearer ";headers.append(token.begin(),token.end());headers+=L"\r\n";}
   bool sent=WinHttpSendRequest(req,headers.c_str(),DWORD(headers.size()),body.empty()?WINHTTP_NO_REQUEST_DATA:const_cast<char*>(body.data()),DWORD(body.size()),DWORD(body.size()),0)!=0;
   SecureZeroMemory(headers.data(),headers.size()*sizeof(wchar_t));require(sent&&WinHttpReceiveResponse(req,nullptr));DWORD status=0,n=sizeof status;
   require(WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&n,WINHTTP_NO_HEADER_INDEX));reply.status=status;
   char buffer[2048];do{n=0;require(WinHttpReadData(req,buffer,sizeof buffer,&n));require(reply.body.size()+n<=16384);reply.body.append(buffer,n);}while(n);
  }catch(...){wipe(reply.body);reply.status=0;}
  {std::lock_guard<std::mutex> l(mutex);done=true;if(pending){WinHttpCloseHandle(pending);pending=nullptr;}timer.notify_all();}watchdog.join();return reply;
 }
};
template<class SteamType,class HttpType,class StorageType> class BasicService {
 std::mutex mutex_;std::condition_variable wake_;Model model_;SteamType steam_;StorageType storage_;HttpType http_;Stored stored_;
 bool started_=false,stopping_=false,queued_=false;uint64_t attempted_=0,redeemed_=0,queued_account_=0;std::string code_,version_="0.1.8-beta";
 void observe(){model_.observe(steam_.read(),wall_now(),GetTickCount64());}
 void current(uint64_t id,uint64_t generation){std::lock_guard<std::mutex> l(mutex_);observe();require(!stopping_&&model_.account==id&&model_.generation==generation);}
 void worker(){auto initial=storage_.load();{std::lock_guard<std::mutex> l(mutex_);stored_=std::move(initial);}for(;;){std::string code,token;uint64_t id=0,g=0;bool redeem=false;
   {std::unique_lock<std::mutex> l(mutex_);wake_.wait_for(l,std::chrono::milliseconds(100),[this]{return stopping_||queued_;});if(stopping_)break;observe();
    if(!queued_&&model_.account&&attempted_!=model_.account){attempted_=model_.account;if(stored_.account==model_.account&&!stored_.token.empty()){queued_=true;queued_account_=model_.account;}else if(stored_.account)model_.state=State::wrong_account;else{queued_=true;queued_account_=model_.account;}}
    if(!queued_||model_.busy)continue;
    queued_=false;if(queued_account_!=model_.account){wipe(code_);model_.state=State::wrong_account;continue;}if(!model_.begin())continue;id=model_.account;g=model_.generation;code.swap(code_);redeem=!code.empty();if(stored_.account==id)token=stored_.token;
   }
   Result result;bool saved=true;
   try{auto health=http_.request(L"/api/mod-auth/status","","");require(readiness(health.status,health.body));current(id,g);
    if(!redeem&&token.empty()){result.state=State::enroll;}
    else {auto body=Json{{"app_id",308600},{"steam_id",std::to_string(id)},{"client_version",version_},{"platform","windows"}};if(redeem)body["code"]=code;
     auto payload=body.dump();auto reply=http_.request(redeem?L"/api/mod-auth/redeem":L"/api/mod-auth/check",payload,redeem?"":token);wipe(payload);result=response(reply.status,reply.body,redeem,id,wall_now());wipe(reply.body);
     // A redeemed token is saved before /check. Never spend the single-use code again.
     if(redeem&&result.state==State::accepted){Stored next{id,result.token};saved=storage_.save(next);
      {std::lock_guard<std::mutex> l(mutex_);wipe(stored_.token);stored_=next;++redeemed_;}
      auto checkbody=Json{{"app_id",308600},{"steam_id",std::to_string(id)},{"client_version",version_},{"platform","windows"}}.dump();wipe(result.token);
      current(id,g);auto checked=http_.request(L"/api/mod-auth/check",checkbody,next.token);result=response(checked.status,checked.body,false,id,wall_now());wipe(checked.body);wipe(next.token);
     }else if(!redeem&&result.state==State::accepted){saved=storage_.save(Stored{id,token});}
    }
   }catch(...){result={};}
   wipe(code);wipe(token);
   {std::lock_guard<std::mutex> l(mutex_);observe();model_.busy=false;
    if(!stopping_&&g==model_.generation){if(result.clear_token){wipe(stored_.token);stored_.account=0;storage_.remove();}if(!saved)result.state=State::storage_error;model_.accept(result,g,wall_now(),GetTickCount64());}wipe(result.token);
   }
  }}
public:
 void configure(const std::filesystem::path& directory){std::lock_guard<std::mutex> l(mutex_);if(!started_)storage_.path=directory/L"beta-access.bin";}
 void start(){std::lock_guard<std::mutex> l(mutex_);if(started_||stopping_||storage_.path.empty()||!steam_.attach())return;started_=true;try{std::thread([this]{worker();}).detach();}catch(...){started_=false;model_.state=State::unavailable;}}
 bool request(std::string code={}){std::lock_guard<std::mutex> l(mutex_);observe();if(stopping_||!started_||model_.busy||queued_||!model_.account){wipe(code);return false;}
  auto first=code.find_first_not_of(" \t\r\n"),last=code.find_last_not_of(" \t\r\n");if(first==std::string::npos)wipe(code);else code=code.substr(first,last-first+1);
  if(code.size()>4096){wipe(code);return false;}ssc_auth_privacy::secrets_used=true;code_.swap(code);wipe(code);queued_=true;queued_account_=model_.account;attempted_=model_.account;model_.clear();model_.state=State::checking;wake_.notify_all();return true;
 }
 uint64_t redeemed(){std::lock_guard<std::mutex> l(mutex_);return redeemed_;}
 State state(){std::lock_guard<std::mutex> l(mutex_);observe();return model_.state;}
 bool busy(){std::lock_guard<std::mutex> l(mutex_);return queued_||model_.busy;}
 bool available(){std::lock_guard<std::mutex> l(mutex_);observe();return !stopping_&&model_.has("beta");}
 bool active(){std::lock_guard<std::mutex> l(mutex_);observe();return !stopping_&&model_.has("beta")&&model_.enabled;}
 void enable(bool on){std::lock_guard<std::mutex> l(mutex_);observe();model_.enabled=on&&!stopping_&&model_.has("beta");}
 void invalidate(){std::lock_guard<std::mutex> l(mutex_);model_.clear();model_.state=State::enroll;attempted_=0;http_.cancel();}
 void stop(){std::lock_guard<std::mutex> l(mutex_);stopping_=true;model_.clear();wipe(code_);wipe(stored_.token);http_.stop();wake_.notify_all();}
};
// Ordinary beta is closed in this build, including programmatic entry points.
class Service : public BasicService<Identity,Http,Storage> {
public:
 void start(){} bool request(std::string code={}){wipe(code);return false;}
 bool available(){return false;} bool active(){return false;} void enable(bool){}
};
inline Service& service(){static auto* s=new Service;return *s;}
inline bool available(){return service().available();}inline bool active(){return service().active();}
}
