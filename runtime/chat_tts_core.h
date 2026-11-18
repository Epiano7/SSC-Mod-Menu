#pragma once
#include <algorithm>
#include <cstdint>
#include <deque>
#include <set>
#include <string>
namespace ssc_chat_tts {
enum class Channel { unknown, public_chat, team, server, system };
struct Message {uint64_t id=0,session=0;Channel channel=Channel::unknown;std::string sender,text;bool own=false;};
struct Settings {bool enabled=false,public_chat=true,team=true,server=false,system=false,own=false,read_names=false;};
class Queue {
 std::deque<Message> pending_;
 std::deque<uint64_t> seen_;
 uint64_t session_=0;
 bool session_set_=false;
 std::string previous_text_,previous_sender_;
 uint64_t previous_at_=0;
 public:
 Settings settings;
 std::set<std::string> muted;
 static constexpr size_t capacity=4;
 size_t size()const{return pending_.size();}
 void stop(){pending_.clear();}
 bool allowed(const Message& m)const{
  if(muted.count(m.sender)||(m.own&&!settings.own))return false;
  return m.channel==Channel::public_chat?settings.public_chat:m.channel==Channel::team?settings.team:m.channel==Channel::server?settings.server:m.channel==Channel::system?settings.system:false;
 }
 bool push(const Message& m,uint64_t now){
  if(!settings.enabled)return false;
  if(!session_set_||m.session!=session_){pending_.clear();seen_.clear();previous_text_.clear();previous_sender_.clear();previous_at_=0;session_=m.session;session_set_=true;}
  if(!m.id||m.text.empty()||m.text.size()>256||m.sender.size()>128||muted.count(m.sender)||(m.own&&!settings.own))return false;
  if(!allowed(m)||std::find(seen_.begin(),seen_.end(),m.id)!=seen_.end())return false;
  if(previous_text_==m.text&&previous_sender_==m.sender&&now>=previous_at_&&now-previous_at_<5000)return false;
  seen_.push_back(m.id);if(seen_.size()>128)seen_.pop_front();
  previous_text_=m.text;previous_sender_=m.sender;previous_at_=now;
  // Drop overload; never let chat flood create minutes of delayed speech.
  if(pending_.size()>=capacity)return false;
  pending_.push_back(m);return true;
 }
 bool pop(std::string& text){
  if(!settings.enabled){stop();return false;}
  while(!pending_.empty()){
   const auto m=std::move(pending_.front());pending_.pop_front();if(!allowed(m))continue;
   text=settings.read_names&&!m.sender.empty()?m.sender+": "+m.text:m.text;return true;
  }
  return false;
 }
};
}
