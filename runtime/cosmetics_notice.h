#pragma once
#include <set>
#include <string>
#include <cstdint>
namespace ssc_cosmetic_notice {
struct Policy {
 bool existing_users=false;
 std::set<uint64_t> seen;
 void migrate(bool had_settings,bool welcomed,bool legacy_seen){existing_users=had_settings&&welcomed&&!legacy_seen;}
 bool known(uint64_t id)const{return seen.count(id)!=0;}
 bool consume(uint64_t id){if(!id||!seen.insert(id).second)return false;return existing_users;}
 void parse(const std::string& text){size_t start=0;while(start<text.size()){auto end=text.find(',',start);auto value=text.substr(start,end-start);if(value.size()==17&&value.find_first_not_of("0123456789")==std::string::npos){try{seen.insert(std::stoull(value));}catch(...) {}}if(end==std::string::npos)break;start=end+1;}}
 std::string encode()const{std::string s;for(auto id:seen){if(!s.empty())s+=',';s+=std::to_string(id);}return s;}
};
}
