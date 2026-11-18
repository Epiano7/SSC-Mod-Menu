#pragma once
#include "chat_commands_core.h"
namespace ssc_commands {
struct Completion {std::string text,label;};
inline std::string folded(std::string s){for(auto& c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline std::vector<Completion> complete(const Router& router,const std::string& input,const std::vector<std::string>& names){
 std::vector<Completion> out;if(!router.enabled||input.empty()||input.size()>256||input[0]!='/'||input.find_first_of("\r\n")!=std::string::npos)return out;
 auto add=[&](const std::string& value,const std::string& label){if(value.size()<=256&&out.size()<8&&std::none_of(out.begin(),out.end(),[&](const Completion& c){return c.text==value;}))out.push_back({value,label});};
 auto space=input.find(' ');auto name=folded(input.substr(1,space==std::string::npos?space:space-1));
 if(space==std::string::npos){for(const auto& b:router.bindings)if(b.name.compare(0,name.size(),name)==0)add("/"+b.name+" ","/"+b.name);return out;}
 auto found=std::find_if(router.bindings.begin(),router.bindings.end(),[&](const Binding& b){return b.name==name;});if(found==router.bindings.end())return out;
 const auto head=input.substr(0,space+1),tail=input.substr(space+1);auto second=tail.find(' ');auto verb=folded(tail.substr(0,second));
 if(found->action=="tts"||found->action=="kos"){
  std::vector<std::string> verbs=found->action=="tts"?std::vector<std::string>{"on","off","mute","unmute","mutelist","volume"}:std::vector<std::string>{"add","remove","list"};
  if(second==std::string::npos){for(const auto& v:verbs)if(v.compare(0,verb.size(),verb)==0)add(head+v+" ",head+v);}
  bool wants_name=(found->action=="kos"&&(verb=="add"||verb=="remove"))||(found->action=="tts"&&(verb=="mute"||verb=="unmute"));
  std::string partial,prefix;if(wants_name&&second!=std::string::npos){prefix=head+tail.substr(0,second+1);partial=tail.substr(second+1);}else if(found->action=="kos"&&second==std::string::npos){prefix=head;partial=tail;}else return out;
  auto key=folded(partial);for(const auto& n:names)if(!n.empty()&&n.size()<=64&&n.find_first_of("\r\n")==std::string::npos&&folded(n).compare(0,key.size(),key)==0)add(prefix+n,n);
 }
 return out;
}
}
