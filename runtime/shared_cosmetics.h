#pragma once
#include <filesystem>
#include "shared_cosmetics_public.h"
#include "beta_auth.h" // Reuse only HTTPS transport and read-only Steam identity.
#include <condition_variable>
#include <thread>

namespace ssc_shared {
inline Time now(){FILETIME ft{};GetSystemTimeAsFileTime(&ft);ULARGE_INTEGER n{};n.LowPart=ft.dwLowDateTime;n.HighPart=ft.dwHighDateTime;return {int64_t(n.QuadPart/10000)-11644473600000LL,GetTickCount64()};}

struct Network {
 ssc_beta::Http http;
 Network(){http.host=host;}
 auto request(const wchar_t* path,const std::string& body){require(body.size()<=4096);auto result=http.request(path,body,"");return result;}
 void cancel(){http.cancel();}void stop(){http.stop();}
};
enum class Status {off,ready,published,unavailable,account,removed};
inline const wchar_t* label(Status s){switch(s){
 case Status::ready:return L"Receiving shared colors";case Status::published:return L"Your colors are shared";
 case Status::unavailable:return L"Sharing service unavailable - retrying";case Status::account:return L"Sign in and load your game account";
 case Status::removed:return L"Your shared colors have been removed";default:return L"Shared cosmetics are off";}}
template<class Identity=ssc_beta::Identity,class Net=Network> class BasicService {
 mutable std::mutex mutex_;std::condition_variable wake_;std::thread worker_;Identity steam_;Net net_;
 std::filesystem::path directory_;Cache cache_;bool stopping_=false,started_=false,receive_=false,share_=false,remove_=false;
 uint64_t account_=0,generation_=0,next_lookup_=0,next_publish_=0,last_publish_=0;std::string own_,appearance_;Style style_;
 std::vector<std::string> peers_;std::map<std::string,uint64_t> observed_;Status status_=Status::off;
 void work(){
  for(;;){uint64_t id=0,generation=0;std::string own;Style style;bool send=false,remove=false,lookup=false;Pending pending;
   {std::unique_lock<std::mutex> l(mutex_);wake_.wait_for(l,std::chrono::milliseconds(250));if(stopping_)return;
    if(!account_||!account_valid(own_))continue;
    auto time=now();id=account_;generation=generation_;own=own_;style=style_;
    remove=remove_&&time.mono_ms>=next_publish_;send=(share_||remove)&&time.mono_ms>=next_publish_;
    lookup=receive_&&!peers_.empty()&&time.mono_ms>=next_lookup_;
    if(lookup)next_lookup_=time.mono_ms+15000;
    if(send){last_publish_=time.mono_ms;next_publish_=time.mono_ms+15000;}
   }
   auto current=[&]{std::lock_guard<std::mutex> l(mutex_);require(!stopping_&&generation==generation_&&id==account_&&steam_.read()==id);};
   if(send){
    Status result=Status::unavailable;bool sent=false;
    try{current();publish_public(net_,own,remove?nullptr:&style,now,current);sent=true;result=remove?Status::removed:Status::published;}
    catch(...){result=Status::unavailable;}
    {std::lock_guard<std::mutex> l(mutex_);if(!stopping_&&id==account_&&generation==generation_){

      status_=result;
      if(sent){if(remove)remove_=false;next_publish_=now().mono_ms+55000+unsigned(now().mono_ms%10001);}
    }}
   }
   if(lookup){try{
     {std::lock_guard<std::mutex> l(mutex_);require(!stopping_&&generation==generation_&&id==account_&&receive_);pending=cache_.begin(peers_,now());}
     current();auto response=net_.request(L"/api/cosmetics/v1/lookup",Json{{"accounts",pending.accounts}}.dump());current();
     std::lock_guard<std::mutex> l(mutex_);if(generation!=generation_||id!=account_)continue;
     if(response.status==200&&cache_.accept(pending,response.body,now())){if(!share_&&!remove_)status_=Status::ready;}
     else if(!share_&&!remove_)status_=Status::unavailable;
    }catch(...){std::lock_guard<std::mutex> l(mutex_);if(generation==generation_&&!share_&&!remove_)status_=Status::unavailable;}}
  }
 }
public:
 ~BasicService(){stop();}
 void configure(const std::filesystem::path& directory){std::lock_guard<std::mutex> l(mutex_);if(!started_)directory_=directory;}
 void update(const std::string& own,const std::vector<std::string>& peers,const Style& style,bool receive,bool share){
  std::lock_guard<std::mutex> l(mutex_);if(stopping_||directory_.empty())return;
  if(!started_){if(!steam_.attach())return;try{worker_=std::thread([this]{work();});started_=true;}catch(...){status_=Status::unavailable;return;}}
  auto id=steam_.read();const auto encoded_style=encode(style).dump();
  const bool account_change=id!=account_||own!=own_;
  if(account_change||!receive)observed_.clear();
  const bool changed=account_change||receive!=receive_||share!=share_||encoded_style!=appearance_;
  if(changed){++generation_;if(account_change){remove_=false;next_lookup_=next_publish_=0;cache_.context(0,false);}
   else if(share_&&!share){remove_=true;next_publish_=0;}
   else if(!share_&&share){remove_=false;next_publish_=0;}
   // Coalesce edits to at most one exchange per five seconds; failures retry at 15s.
   if(share&&encoded_style!=appearance_)next_publish_=std::min(next_publish_,std::max(last_publish_+5000,now().mono_ms+1500));
   if(receive!=receive_)next_lookup_=0;
  }
  account_=id;own_=own;style_=style;appearance_=encoded_style;receive_=receive;share_=share;
  std::set<std::string> unique;for(auto& peer:peers)if(account_valid(peer)&&unique.size()<32)unique.insert(peer);
  const auto seen_at=now().mono_ms;
  for(auto i=observed_.begin();i!=observed_.end();){
   if(seen_at<i->second||seen_at-i->second>30000)i=observed_.erase(i);
   else {if(unique.size()<32)unique.insert(i->first);++i;}
  }
  peers_={unique.begin(),unique.end()};cache_.context(id,receive);
  if(!id||!account_valid(own))status_=Status::account;else if(!receive&&!share&&!remove_)status_=Status::off;
  wake_.notify_all();
 }
 bool get(const std::string& account,Style& style){std::lock_guard<std::mutex> l(mutex_);if(steam_.read()!=account_){cache_.context(0,false);observed_.clear();return false;}
  // Attributed chat senders may not be in a match roster (main-menu chat).
  // Queue only bounded public account keys; the worker performs all network I/O.
  if(!stopping_&&receive_&&account!=own_&&account_valid(account)&&(observed_.count(account)||observed_.size()<32))observed_[account]=now().mono_ms;
  return cache_.get(account,now(),style);
 }
 Status status()const{std::lock_guard<std::mutex> l(mutex_);return status_;}
 void stop(){
  {std::lock_guard<std::mutex> l(mutex_);stopping_=true;++generation_;cache_.context(0,false);wake_.notify_all();}
  net_.stop();if(worker_.joinable())worker_.join();
 }
};
using Service=BasicService<>;
inline Service& service(){static auto* instance=new Service;return *instance;}
}
