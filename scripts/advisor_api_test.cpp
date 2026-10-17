#include "../runtime/advisor_api.h"
#include <cassert>
#include <iostream>
int main(){using namespace ssc_advisor;Build b;b.player_class=2;b.levels[3]=2;std::array<int,54> offers{};offers[0]=10;offers[1]=36;
 auto key=api_key(b,offers,2);assert(key.find("classId=3")!=std::string::npos&&key.find("owned=3")!=std::string::npos&&key.find("version=current")!=std::string::npos);
 offers[1]=3;assert(api_key(b,offers,2).empty());offers[1]=10;assert(api_key(b,offers,2).empty());offers[1]=-1;assert(api_key(b,offers,2).empty());
 auto good=parse_advice(key,R"({"candidates":[{"skillId":10,"score":2.5,"alone":{"appearances":20}}]})");assert(good.valid&&good.edge[10]==2.5f&&good.samples[10]==20);
 assert(!parse_advice(key,"broken").valid);assert(!parse_advice(key,R"({"candidates":[{"skillId":54,"score":2,"alone":{"appearances":20}}]})").valid);
 assert(!parse_advice(key,R"({"candidates":[{"skillId":10,"score":2,"alone":{"appearances":-1}}]})").valid);
 std::cout<<"PASS: class/owned IDs, upgrades excluded, duplicate/invalid offers, bounded API schema and malformed data rejection\n";
}
