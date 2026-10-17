#pragma once
#include "advisor_data.h"
#include <array>
#include <algorithm>
#include <cmath>
namespace ssc_advisor {
struct Build {int player_class=-1;std::array<int,54> levels{};std::array<bool,54> mutated{};};
struct Score {float value=0;unsigned evidence=0;};
inline uint64_t mutations(const Build& b){uint64_t mask=0;for(int i=0;i<54;++i)if(b.mutated[size_t(i)])mask|=uint64_t(1)<<i;return mask;}
inline bool same_build(const Build& a,const Build& b){return a.player_class==b.player_class&&a.levels==b.levels&&a.mutated==b.mutated;}
inline float reference(int a,int b){for(const auto& p:pairs)if((p.first==a&&p.second==b)||(p.first==b&&p.second==a)){if(p.rating>=0)return float(p.rating-5)/5;if(p.category>=-1)return float(p.category);break;}return 0;}
// Community scores describe mature builds, not measured current win chances.
// Use them as contextual priors; do not import old damage/cooldown numbers.
inline std::array<Score,54> evaluate(const Build& b){
 std::array<Score,54> out{};if(b.player_class<0||b.player_class>=8)return out;
 for(int candidate=0;candidate<54;++candidate){auto& s=out[size_t(candidate)];s.value=2*reference(candidate,54+b.player_class);s.evidence=1;
  float sum=0;int count=0;for(int owned=0;owned<54;++owned)if(b.levels[size_t(owned)]>0&&owned!=candidate){sum+=reference(candidate,owned);++count;}
  if(count)s.value+=sum/std::sqrt(float(count));
  for(const auto& r:rules){int other=r.first==candidate?r.second:r.second==candidate?r.first:-1;if(other<0)continue;bool held=other>=54?other==54+b.player_class:b.levels[size_t(other)]>0;if(!held)continue;
   // Do not assume a future mutation or promise a mutation-dependent loop.
   if(r.mutation_required&&(mutations(b)&r.mutation_mask)!=r.mutation_mask)continue;
   s.value+=.25f*r.priority;++s.evidence;
  }
 }
 return out;
}
// Marginal mutation value: only already-owned, unmutated skills are eligible.
// Reward newly enabled interactions, not interactions that still need another
// missing mutation. The class/pair prior breaks ties; it is not measured DPS.
inline std::array<Score,54> evaluate_mutations(const Build& b){
 auto prior=evaluate(b);std::array<Score,54> out{};if(b.player_class<0||b.player_class>=8)return out;
 auto before=mutations(b);
 for(int id=0;id<54;++id){if(b.levels[size_t(id)]<=0||b.mutated[size_t(id)])continue;
  auto& score=out[size_t(id)];score.evidence=1;score.value=.1f*prior[size_t(id)].value;
  auto after=before|(uint64_t(1)<<id);
  for(const auto& rule:rules){if(!rule.mutation_required||!(rule.mutation_mask&(uint64_t(1)<<id)))continue;
   if(b.levels[size_t(rule.first)]<=0||b.levels[size_t(rule.second)]<=0)continue;
   if((before&rule.mutation_mask)!=rule.mutation_mask&&(after&rule.mutation_mask)==rule.mutation_mask){score.value+=rule.priority;++score.evidence;}
  }
 }
 return out;
}
template<size_t N> inline int choose(const std::array<Score,54>& scores,const std::array<int,N>& offers,unsigned count){
 if(count<2||count>offers.size())return -1;
 int best=-1;float high=-INFINITY;
 for(unsigned i=0;i<count;++i){int id=offers[i];if(id<0||id>=54)return -1;for(unsigned j=0;j<i;++j)if(offers[j]==id)return -1;if(!scores[size_t(id)].evidence||!std::isfinite(scores[size_t(id)].value))return -1;
  if(scores[size_t(id)].value>high+.0001f){high=scores[size_t(id)].value;best=id;}
 }
 return best;
}
}

