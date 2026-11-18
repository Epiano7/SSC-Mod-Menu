#pragma once
#include <algorithm>
#include <cctype>
#include <functional>
#include <string>
#include <vector>
namespace ssc_commands {
enum class Channel { unchanged, team, public_chat };
struct Binding {std::string name,action;};
struct Result {bool consumed=false;std::string outgoing,reply,action,argument;Channel channel=Channel::unchanged;};
inline bool valid_name(const std::string& s){return !s.empty()&&s.size()<=20&&std::all_of(s.begin(),s.end(),[](unsigned char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_';});}
class Router {
 public:
 bool enabled=false;char prefix='/';Channel channel=Channel::unchanged;
 std::vector<Binding> bindings={{"team","team"},{"all","all"},{"help","help"},{"summary","summary"},{"record","record"},{"kos","kos"},{"tts","tts"}};
 bool valid()const {
  if(prefix<33||prefix>126||std::isalnum(static_cast<unsigned char>(prefix))||bindings.size()>32)return false;
  for(size_t i=0;i<bindings.size();++i){if(!valid_name(bindings[i].name)||!valid_name(bindings[i].action))return false;for(size_t j=0;j<i;++j)if(bindings[j].name==bindings[i].name)return false;}
  return true;
 }
 Result route(const std::string& text,const std::function<std::string(const std::string&,const std::string&)>& execute={}){
  Result r;r.outgoing=text;r.channel=channel;
  const auto first=text.find_first_not_of(" \t");
  if(first!=std::string::npos&&text.compare(first,2,"./")==0){r.outgoing.erase(first,1);return r;}
  const bool slash=first!=std::string::npos&&text[first]=='/';
  if(first==std::string::npos||(!slash&&(!enabled||text[first]!=prefix)))return r;
  // An invalid configuration must never leak a prefixed private command.
  r.consumed=true;r.outgoing.clear();
  if(!valid()||text.size()>256||text.find_first_of("\r\n")!=std::string::npos){r.reply="Invalid local command";return r;}
  if(!enabled){r.reply="Local commands are off";return r;}
  const auto split=text.find_first_of(" \t",first+1);std::string name=text.substr(first+1,split==std::string::npos?std::string::npos:split-first-1);
  std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(std::tolower(c));});
  auto binding=std::find_if(bindings.begin(),bindings.end(),[&](const Binding& b){return b.name==name;});
  if(binding==bindings.end()){r.reply="Unknown local command; use "+std::string(1,prefix)+"help";return r;}
  if(split!=std::string::npos){auto start=text.find_first_not_of(" \t",split);if(start!=std::string::npos){auto end=text.find_last_not_of(" \t");r.argument=text.substr(start,end-start+1);}}
  r.action=binding->action;
  if((r.action=="team"||r.action=="all")&&r.argument.empty()){channel=r.action=="team"?Channel::team:Channel::public_chat;r.channel=channel;r.reply=channel==Channel::team?"Team chat selected":"Public chat selected";}
  else if(r.action=="help"){r.reply="Local commands:";size_t count=0;for(const auto& b:bindings){r.reply+=(count++%3==0?"\n":"   ");r.reply+="/"+b.name;}if(std::any_of(bindings.begin(),bindings.end(),[](const Binding& b){return b.action=="tts";}))r.reply+="\n/tts on|off|mute NAME|unmute NAME|mutelist|volume 0-200";}
  else r.reply=execute?execute(r.action,r.argument):"Command unavailable in this build";
  return r;
 }
};
}
