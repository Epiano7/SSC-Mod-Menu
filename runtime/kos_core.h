#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
namespace ssc_kos {
enum class Visibility { unknown, public_player, anonymous };
struct Player {std::string account,handle;Visibility visibility=Visibility::unknown;bool local=false,bot=false;};
struct Entry {std::string account,handle,note;};
inline bool valid_text(const std::string& s,size_t limit){return !s.empty()&&s.size()<=limit&&std::all_of(s.begin(),s.end(),[](unsigned char c){return c>=32&&c<127;});}
inline std::string canonical(std::string s){for(auto& c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline bool eligible(const Player& p){return p.visibility==Visibility::public_player&&!p.local&&valid_text(p.account,128)&&valid_text(p.handle,64);}
class List {
 std::vector<Entry> entries_;
 public:
 static constexpr size_t capacity=128;
 const std::vector<Entry>& entries()const{return entries_;}
 bool marked(const Player& p)const {if(!eligible(p))return false;const auto key=canonical(p.account);return std::any_of(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==key||e.account=="handle:"+canonical(p.handle);});}
 bool add(const Player& p,const std::string& note={}){
  if(!eligible(p)||note.size()>120||std::any_of(note.begin(),note.end(),[](unsigned char c){return c<32||c>=127;}))return false;
  const auto key=canonical(p.account);auto it=std::find_if(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==key||e.account=="handle:"+canonical(p.handle);});
  if(it!=entries_.end()){it->account=key;it->handle=p.handle;it->note=note;return true;}
  if(entries_.size()>=capacity)return false;
  entries_.push_back({key,p.handle,note});return true;
 }
 bool has_handle(const std::string& handle)const {auto key=canonical(handle);return std::any_of(entries_.begin(),entries_.end(),[&](const Entry& e){return canonical(e.handle)==key;});}
 bool add_handle(const std::string& handle){if(!valid_text(handle,64))return false;if(has_handle(handle))return true;Player p{"handle:"+canonical(handle),handle,Visibility::public_player,false,false};return add(p);}
 bool remove_handle(const std::string& handle){auto key=canonical(handle);auto before=entries_.size();entries_.erase(std::remove_if(entries_.begin(),entries_.end(),[&](const Entry& e){return canonical(e.handle)==key;}),entries_.end());return before!=entries_.size();}
 bool remove(const std::string& account){const auto key=canonical(account);auto before=entries_.size();entries_.erase(std::remove_if(entries_.begin(),entries_.end(),[&](const Entry& e){return e.account==key;}),entries_.end());return before!=entries_.size();}
 // Loading does not certify a currently visible actor. marked() always checks
 // the current visibility, even when this account was previously public.
 bool restore(const std::vector<Entry>& records){
  if(records.size()>capacity)return false;
  List next;
  for(const auto& e:records){Player p{e.account,e.handle,Visibility::public_player,false,false};if(next.marked(p)||!next.add(p,e.note))return false;}
  entries_=std::move(next.entries_);return true;
 }
};
struct Target {Player player;uint64_t generation=0;};
inline bool target_still_public(const Target& selected,const Target& current){return selected.generation==current.generation&&eligible(selected.player)&&eligible(current.player)&&canonical(selected.player.account)==canonical(current.player.account);}
}
