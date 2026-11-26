#pragma once
#include <algorithm>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>
namespace ssc_kos {
enum class Visibility { unknown, public_player, anonymous };
struct Player {std::string account,handle;Visibility visibility=Visibility::unknown;bool local=false,bot=false;};
struct Entry {std::string account,handle,note;unsigned marks=1;};
inline bool valid_text(const std::string& s,size_t limit){return !s.empty()&&s.size()<=limit&&std::all_of(s.begin(),s.end(),[](unsigned char c){return c>=32&&c<127;});}
inline std::string canonical(std::string s){for(auto& c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline bool eligible(const Player& p){return p.visibility==Visibility::public_player&&!p.local&&valid_text(p.account,128)&&valid_text(p.handle,64);}
class List {
 std::vector<Entry> entries_;
 public:
 static constexpr size_t capacity=128;
 static constexpr unsigned max_marks=9999;
 const std::vector<Entry>& entries()const{return entries_;}
 bool marked(const Player& p)const {if(!eligible(p))return false;const auto key=canonical(p.account);return std::any_of(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==key||e.account=="handle:"+canonical(p.handle);});}
 unsigned add(const Player& p,const std::string& note={},unsigned count=0){
  if(!eligible(p)||count>max_marks||note.size()>120||std::any_of(note.begin(),note.end(),[](unsigned char c){return c<32||c>=127;}))return 0;
  const auto key=canonical(p.account);auto it=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==key||e.account=="handle:"+canonical(p.handle);});
  if(it!=entries_.end()){
   if(!count&&it->marks>=max_marks)return 0;
   it->account=key;it->handle=p.handle;if(!note.empty())it->note=note;
   it->marks=count?count:it->marks+1;
   return it->marks;
  }
  if(entries_.size()>=capacity)return 0;
  entries_.push_back({key,p.handle,note,count?count:1});return entries_.back().marks;
 }
 bool has_handle(const std::string& handle)const {auto key=canonical(handle);return std::any_of(entries_.begin(),entries_.end(),[&](const Entry& e){return canonical(e.handle)==key;});}
 unsigned add_handle(const std::string& handle,unsigned count=0){
  if(!valid_text(handle,64)||count>max_marks)return 0;
  const auto key=canonical(handle);
  auto existing=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& e){return canonical(e.handle)==key;});
  if(existing!=entries_.end()){
   if(!count&&existing->marks>=max_marks)return 0;
   existing->marks=count?count:existing->marks+1;
   return existing->marks;
  }
  return add({"handle:"+key,handle,Visibility::public_player,false,false},{},count);
 }
 unsigned increase(const std::string& account){
  auto it=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==account;});
  return it==entries_.end()||it->marks>=max_marks?0:++it->marks;
 }
 bool spend(const Player& p){
  if(!eligible(p))return false;
  const auto account=canonical(p.account),handle="handle:"+canonical(p.handle);
  auto it=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==account;});
  if(it==entries_.end())it=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==handle;});
  if(it==entries_.end())return false;
  if(--it->marks==0)entries_.erase(it);
  return true;
 }
 bool remove_handle(const std::string& handle){auto key=canonical(handle);auto before=entries_.size();entries_.erase(std::remove_if(entries_.begin(),entries_.end(),[&](const Entry& e){return canonical(e.handle)==key;}),entries_.end());return before!=entries_.size();}
 bool remove(const std::string& account){const auto key=canonical(account);auto before=entries_.size();entries_.erase(std::remove_if(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==key;}),entries_.end());return before!=entries_.size();}
 // Loading does not certify a currently visible actor. marked() always checks
 // the current visibility, even when this account was previously public.
 bool restore(const std::vector<Entry>& records){
  if(records.size()>capacity)return false;
  List next;
  for(const auto& e:records){
   Player p{e.account,e.handle,Visibility::public_player,false,false};
   if(!e.marks||e.marks>max_marks||next.marked(p)||!next.add(p,e.note))return false;
   next.entries_.back().marks=e.marks;
  }
  entries_=std::move(next.entries_);return true;
 }
};
struct CountArgument {std::string_view handle;unsigned marks=0;bool valid=true;};
inline CountArgument parse_count(std::string_view argument){
 const auto last=argument.find_last_not_of(" \t");
 if(last==std::string_view::npos)return {argument};
 argument=argument.substr(0,last+1);
 const auto split=argument.find_last_of(" \t");
 if(split==std::string_view::npos)return {argument};
 auto tail=argument.substr(split+1);
 if(tail.empty())return {argument};
 const bool signed_count=tail.front()=='-'||tail.front()=='+';
 const auto digits=tail.substr(signed_count?1:0);
 if(digits.empty()||!std::all_of(digits.begin(),digits.end(),[](char c){return c>='0'&&c<='9';}))return {argument};
 if(signed_count)return {{},0,false};
 unsigned count=0;
 for(char c:tail){
  count=count*10+unsigned(c-'0');
  if(count>List::max_marks)return {{},0,false};
 }
 const auto handle_end=argument.find_last_not_of(" \t",split);
 if(!count||handle_end==std::string_view::npos)return {{},0,false};
 return {argument.substr(0,handle_end+1),count,true};
}
struct Target {Player player;uint64_t generation=0;};
inline bool target_still_public(const Target& selected,const Target& current){return selected.generation==current.generation&&eligible(selected.player)&&eligible(current.player)&&canonical(selected.player.account)==canonical(current.player.account);}
}
