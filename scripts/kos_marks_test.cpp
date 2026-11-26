#include "kos_storage.h"
#include <cassert>
#include <iostream>

int main(){
 using namespace ssc_kos;
 List list;
 assert(decode(R"({"schema":1,"entries":[{"account":"handle:test","handle":"Test","note":"saved"}]})",list));
 assert(list.entries().size()==1&&list.entries().front().marks==1);
 assert(list.add_handle("TEST")==2);
 const Player public_player{"account","Test",Visibility::public_player,false,false};
 assert(list.add(public_player)==3);
 const auto& entry=list.entries().front();
 assert(list.entries().size()==1&&entry.account=="account"&&entry.note=="saved"&&entry.marks==3);
 assert(list.add_handle("test")==4&&list.marked(public_player));
 assert(decode(encode(list),list)&&list.entries().front().marks==4);

 auto bad=nlohmann::json::parse(encode(list));
 for(const auto& value:{nlohmann::json(-1),nlohmann::json(0),nlohmann::json(1.5),nlohmann::json(uint64_t(1)<<40),nlohmann::json("4")}){
  bad["entries"][0]["marks"]=value;
  assert(!decode(bad.dump(),list)&&list.entries().front().marks==4);
 }
 auto max=nlohmann::json::parse(encode(list));
 max["entries"][0]["marks"]=List::max_marks;
 assert(decode(max.dump(),list)&&list.add(public_player)==0&&list.entries().front().marks==List::max_marks);
 auto player=public_player;
 player.visibility=Visibility::anonymous;
 assert(!list.marked(player)&&!list.add(player));
 player=public_player;player.local=true;
 assert(!list.marked(player)&&!list.add(player));
 assert(list.remove_handle("test")&&!list.marked(public_player));
 for(size_t i=0;i<List::capacity;++i)assert(list.add_handle("Name "+std::to_string(i))==1);
 assert(list.add_handle("Name 2")==2&&list.add_handle("Overflow")==0);
 const auto command=parse_count("Public Name 3 ");
 assert(command.valid&&command.handle=="Public Name"&&command.marks==3);
 assert(parse_count("Public Name").valid&&parse_count("Public Name").marks==0);
 for(const auto* invalid:{"Public Name 0","Public Name -2","Public Name 10000","Public Name 123456789012345"})assert(!parse_count(invalid).valid);
 List queue;
 assert(queue.add_handle("Test",3)==3&&queue.increase("handle:test")==4);
 assert(queue.add(public_player,{},2)==2&&queue.spend(public_player));
 assert(queue.entries().size()==1&&queue.entries().front().marks==1);
 auto elsewhere=public_player;elsewhere.account="wrong";assert(!queue.spend(elsewhere));
 elsewhere=public_player;elsewhere.visibility=Visibility::anonymous;assert(!queue.spend(elsewhere));
 assert(queue.spend(public_player)&&queue.entries().empty()&&!queue.spend(public_player));
 std::cout<<"PASS: legacy migration, repeated marks, identity upgrade, persistence, removal, limits and privacy\n";
}
