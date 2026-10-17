#include "../runtime/advisor_core.h"
#include <cassert>
#include <iostream>
static void mutations_test(){using namespace ssc_advisor;Build b;b.player_class=2;b.levels[10]=1;b.levels[3]=4;
 auto initial=evaluate(b);auto recommendations=evaluate_mutations(b);assert(recommendations[3].value>recommendations[10].value);
 b.mutated[3]=true;auto after=evaluate(b);assert(after[10].value>initial[10].value);assert(evaluate_mutations(b)[3].evidence==0);
 Build med;med.player_class=2;med.levels[46]=1;med.levels[17]=1; // Only complete mutation requirements can activate a rule.
 for(auto& rule:rules)if(rule.mutation_required){Build x;x.player_class=0;x.levels[size_t(rule.first)]=1;x.levels[size_t(rule.second)]=1;auto without=evaluate(x);for(int id=0;id<54;++id)x.mutated[size_t(id)]=(rule.mutation_mask&(uint64_t(1)<<id))!=0;auto with=evaluate(x);assert(with[size_t(rule.first)].value>=without[size_t(rule.first)].value);}
}
struct RunMutationTests {RunMutationTests(){mutations_test();}} run_mutations;
int main(){using namespace ssc_advisor;Build b;auto s=evaluate(b);assert(choose(s,std::array<int,3>{10,3,-1},2)==-1);b.player_class=2;s=evaluate(b);assert(choose(s,std::array<int,3>{10,10,-1},2)==-1);assert(choose(s,std::array<int,3>{10,54,-1},2)==-1);assert(choose(s,std::array<int,3>{10,3,-1},1)==-1);auto before=s;b.levels[10]=1;s=evaluate(b);bool changed=false;for(unsigned i=0;i<54;++i){assert(std::isfinite(s[i].value));changed|=s[i].value!=before[i].value;}assert(changed);int pick=choose(s,std::array<int,3>{3,36,-1},2);assert(pick==3||pick==36);b.player_class=7;auto assassin=evaluate(b);changed=false;for(unsigned i=0;i<54;++i)changed|=s[i].value!=assassin[i].value;assert(changed);std::cout<<"PASS: class/build-sensitive scoring, invalid context/offers, duplicate rejection\n";}
