#pragma once
#include "challenge_modes.h"
namespace ssc_mirror {
// Each row is a permutation with no unchanged direction. Values identify the
// original input feeding each output: Up, Down, Left, Right.
inline constexpr std::array<std::array<unsigned char,4>,9> permutations={{{1,0,3,2},{1,2,3,0},{1,3,0,2},{2,0,3,1},{2,3,0,1},{2,3,1,0},{3,0,1,2},{3,2,0,1},{3,2,1,0}}};
struct Scramble {
 unsigned index=0;bool ready=false,was_active=false,last_shuffle=true;uint64_t last_at=0,elapsed=0,next_retry=0;uintptr_t world=0;int round=0;unsigned interval=60000;
 void reset(){*this={};}
 template<class Next>void tick(uint64_t now,bool enabled,bool active,bool shuffle,unsigned period,uintptr_t session,int rn,Next next){
  period=period==120000?120000:60000;
  if(!enabled){reset();return;}
  if(world!=session||rn<round||shuffle!=last_shuffle||period!=interval){reset();world=session;last_shuffle=shuffle;interval=period;}
  if(last_at&&now<last_at){elapsed=0;was_active=false;}
  if(last_at&&now>=last_at&&now-last_at<=10000&&was_active&&active)elapsed+=now-last_at;
  last_at=now;round=rn;was_active=active;
  if(!shuffle){index=0;ready=true;elapsed=0;return;}
  if(!active)return;
  if(!ready&&now<next_retry)return;
  if(!ready||elapsed>=period){std::array<unsigned,9> candidates{};size_t count=0;for(unsigned p=0;p<permutations.size();++p){bool changed=true;if(ready)for(size_t d=0;d<4;++d)changed=changed&&permutations[p][d]!=permutations[index][d];if(changed)candidates[count++]=p;}size_t selected=0;if(ssc_challenge::uniform(count,next,selected)){index=candidates[selected];ready=true;next_retry=0;}else next_retry=now+1000;if(elapsed>=period)elapsed%=period;}
 }
 std::array<unsigned char,4> transform(const std::array<unsigned char,4>& input)const{std::array<unsigned char,4> output{};const auto& p=permutations[index<permutations.size()?index:0];for(size_t i=0;i<4;++i)output[i]=input[p[i]];return output;}
};inline Scramble scramble;
}
