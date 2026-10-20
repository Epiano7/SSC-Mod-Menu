#include "../runtime/cosmetics_notice.h"
#include <cassert>
#include <iostream>
int main(){using ssc_cosmetic_notice::Policy;constexpr uint64_t id=76561198025298853ULL;Policy old;old.migrate(true,true,false);assert(old.consume(id));assert(!old.consume(id));Policy update;update.existing_users=old.existing_users;update.parse(old.encode());assert(!update.consume(id));assert(update.consume(id+1));Policy fresh;fresh.migrate(false,false,false);assert(!fresh.consume(id));Policy acknowledged;acknowledged.migrate(true,true,true);assert(!acknowledged.consume(id));assert(!fresh.consume(0));std::cout<<"PASS: existing-user migration once per stable account, update persistence, legacy acknowledgement and fresh-install suppression\n";}
