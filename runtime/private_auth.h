#pragma once
#include "private_auth_exchange.h"
#include <windows.h>
#include <winhttp.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>

namespace ssc_auth {
inline Clock clock_now(){FILETIME ft{};GetSystemTimeAsFileTime(&ft);ULARGE_INTEGER x{};x.LowPart=ft.dwLowDateTime;x.HighPart=ft.dwHighDateTime;return {int64_t(x.QuadPart/10000000ULL)-11644473600LL,GetTickCount64()};}
struct Internet {
 HINTERNET h=nullptr;explicit Internet(HINTERNET p):h(p){}~Internet(){if(h)WinHttpCloseHandle(h);}operator HINTERNET()const{return h;}
 Internet(const Internet&)=delete;Internet& operator=(const Internet&)=delete;
};
struct Https {
 Response request(const wchar_t* path,const std::string& body){
  require(body.size()<=8192);
  Internet session(WinHttpOpen(L"SSCModAuthorization/1",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));require(session.h);
  require(WinHttpSetTimeouts(session,4000,4000,4000,4000));
  Internet connection(WinHttpConnect(session,L"sscmmauth.epiano7.dev",INTERNET_DEFAULT_HTTPS_PORT,0));require(connection.h);
  Internet request(WinHttpOpenRequest(connection,body.empty()?L"GET":L"POST",path,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));require(request.h);
  DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER,disabled=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
  require(WinHttpSetOption(request,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects)));
  require(WinHttpSetOption(request,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof(disabled)));
  // No SECURITY_FLAGS override, decompression, cookies, or redirects. OS certificate checks remain enabled.
  const wchar_t* headers=body.empty()?L"Accept: application/json\r\n":L"Content-Type: application/json\r\nAccept: application/json\r\n";
  require(WinHttpSendRequest(request,headers,DWORD(-1),body.empty()?WINHTTP_NO_REQUEST_DATA:const_cast<char*>(body.data()),DWORD(body.size()),DWORD(body.size()),0));
  require(WinHttpReceiveResponse(request,nullptr));DWORD status=0,size=sizeof(status);
  require(WinHttpQueryHeaders(request,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX));
  Response result;result.status=status;if(status!=200)return result; // Errors may be non-JSON.
  auto started=GetTickCount64();char buffer[4096];DWORD n=0;
  do{require(GetTickCount64()-started<10000);require(WinHttpReadData(request,buffer,sizeof(buffer),&n));require(result.body.size()+n<=16384);result.body.append(buffer,n);}while(n);
  return result;
 }
};
// Windows x64 CCallbackBase ABI: vptr, uint8 flags, padding, int callback id.
// Both Run overload slots intentionally share a handler: this is a normal
// callback, never a call-result. Extra ABI arguments are ignored safely.
struct Callback;
struct CallbackTable {void (*run)(Callback*,void*);void (*run_result)(Callback*,void*);int (*size)(Callback*);};
struct Callback {CallbackTable* table=nullptr;uint8_t flags=0;uint8_t padding[3]{};int id=168;};
static_assert(offsetof(Callback,id)==12&&sizeof(Callback)==16,"Steam callback ABI");
struct WebTicket {uint32_t handle;int result;int length;unsigned char data[2560];};
static_assert(sizeof(WebTicket)==2572,"Steam ticket callback ABI");
class Steam {
 using Interface=void*(*)();using Logged=bool(*)(void*);using Id=uint64_t(*)(void*);using App=uint32_t(*)(void*);
 using Get=uint32_t(*)(void*,const char*);using Cancel=void(*)(void*,uint32_t);using Register=void(*)(Callback*,int);using Unregister=void(*)(Callback*);
 Interface user_=nullptr,utils_=nullptr;Logged logged_=nullptr;Id id_=nullptr;App app_=nullptr;Get get_=nullptr;Cancel cancel_=nullptr;Register register_=nullptr;Unregister unregister_=nullptr;
 std::mutex mutex_;std::condition_variable ready_;uint32_t pending_=0;WebTicket result_{};bool received_=false;
 struct OwnedCallback:Callback {Steam* owner=nullptr;};OwnedCallback callback_,connected_,disconnected_;
 static void dispatch(Callback* base,void* data){auto self=static_cast<OwnedCallback*>(base)->owner;
  if(base->id!=168){++self->connection_epoch;self->ready_.notify_all();return;}
  if(!data)return;
  auto& t=*static_cast<WebTicket*>(data);
  std::lock_guard<std::mutex> lock(self->mutex_);if(!self->pending_||t.handle!=self->pending_||self->received_)return;
  self->result_=t;self->received_=true;self->ready_.notify_all();
 }
 static int callback_size(Callback* base){return base->id==168?int(sizeof(WebTicket)):base->id==101?1:4;}
 inline static CallbackTable table_{dispatch,dispatch,callback_size};
public:
 std::atomic<bool> stopped{false},attached{false};std::atomic<uint64_t> connection_epoch{0};
 bool attach(){
  auto dll=GetModuleHandleW(L"steam_api64.dll");if(!dll)return false;
  auto current_user=reinterpret_cast<int(*)()>(GetProcAddress(dll,"SteamAPI_GetHSteamUser"));
  if(!current_user||current_user()<=0)return false; // Never initialize/reinitialize Steam on the game's behalf.
  user_=reinterpret_cast<Interface>(GetProcAddress(dll,"SteamAPI_SteamUser_v023"));utils_=reinterpret_cast<Interface>(GetProcAddress(dll,"SteamAPI_SteamUtils_v010"));
  logged_=reinterpret_cast<Logged>(GetProcAddress(dll,"SteamAPI_ISteamUser_BLoggedOn"));id_=reinterpret_cast<Id>(GetProcAddress(dll,"SteamAPI_ISteamUser_GetSteamID"));app_=reinterpret_cast<App>(GetProcAddress(dll,"SteamAPI_ISteamUtils_GetAppID"));
  get_=reinterpret_cast<Get>(GetProcAddress(dll,"SteamAPI_ISteamUser_GetAuthTicketForWebApi"));cancel_=reinterpret_cast<Cancel>(GetProcAddress(dll,"SteamAPI_ISteamUser_CancelAuthTicket"));
  register_=reinterpret_cast<Register>(GetProcAddress(dll,"SteamAPI_RegisterCallback"));unregister_=reinterpret_cast<Unregister>(GetProcAddress(dll,"SteamAPI_UnregisterCallback"));
  if(!user_||!utils_||!logged_||!id_||!app_||!get_||!cancel_||!register_||!unregister_)return false;
  if(!account().valid())return false;
  callback_.table=&table_;callback_.owner=this;register_(&callback_,168);
  connected_.table=disconnected_.table=&table_;connected_.owner=disconnected_.owner=this;connected_.id=101;disconnected_.id=103;
  register_(&connected_,101);register_(&disconnected_,103);attached=true;return true;
 }
 Account account()const{if(stopped||!user_||!utils_||!logged_||!id_||!app_)return {};auto u=user_(),v=utils_();if(!u||!v||!logged_(u))return {};return {id_(u),app_(v),true};}
 template<class Valid> void ticket(const std::string& identity,Ticket& output,Valid valid){
  std::unique_lock<std::mutex> lock(mutex_);require(!stopped&&attached);auto user=user_();require(user);
  received_=false;sodium_memzero(&result_,sizeof(result_));pending_=get_(user,identity.c_str());output.handle=pending_;require(pending_!=0);
  auto deadline=GetTickCount64()+20000;
  while(!received_&&!stopped&&GetTickCount64()<deadline){lock.unlock();bool ok=valid();lock.lock();if(!ok)break;ready_.wait_for(lock,std::chrono::milliseconds(100));}
  require(!stopped&&received_&&result_.handle==pending_&&result_.result==1&&result_.length>0&&result_.length<=2560);
  output.bytes.assign(result_.data,result_.data+result_.length);sodium_memzero(&result_,sizeof(result_));received_=false;
 }
 void cancel(uint32_t handle){std::lock_guard<std::mutex> lock(mutex_);if(handle&&handle==pending_){if(auto u=user_())cancel_(u,handle);pending_=0;received_=false;sodium_memzero(&result_,sizeof(result_));}}
 void shutdown(){stopped=true;ready_.notify_all();{std::lock_guard<std::mutex> lock(mutex_);if(pending_){if(auto u=user_())cancel_(u,pending_);pending_=0;}sodium_memzero(&result_,sizeof(result_));}if(attached){unregister_(&callback_);unregister_(&connected_);unregister_(&disconnected_);attached=false;}}
};
}
#include "private_license.h"
