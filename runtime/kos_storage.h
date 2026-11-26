#pragma once
#include "kos_core.h"
#include "third_party/json.hpp"
namespace ssc_kos {
inline std::string encode(const List& list){
 nlohmann::json j={{"schema",1},{"entries",nlohmann::json::array()}};
 for(const auto& e:list.entries())j["entries"].push_back({{"account",e.account},{"handle",e.handle},{"note",e.note},{"marks",e.marks}});
 return j.dump(2);
}
inline bool decode(const std::string& bytes,List& target){
 if(bytes.size()>65536)return false;
 try{auto j=nlohmann::json::parse(bytes);if(j.at("schema")!=1||!j.at("entries").is_array()||j.at("entries").size()>List::capacity)return false;
  std::vector<Entry> records;
  for(const auto& e:j.at("entries")){
   unsigned marks=1;
   if(e.contains("marks")){
    const auto& value=e.at("marks");
    if(!value.is_number_unsigned())return false;
    const auto count=value.get<uint64_t>();
    if(!count||count>List::max_marks)return false;
    marks=static_cast<unsigned>(count);
   }
   records.push_back({e.at("account").get<std::string>(),e.at("handle").get<std::string>(),e.value("note",std::string{}),marks});
  }
  return target.restore(records);
 }catch(...){return false;}
}
}
