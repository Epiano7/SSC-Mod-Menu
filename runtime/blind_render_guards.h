#pragma once
#include <cstdint>
namespace ssc_blind_render {
struct Guard{uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
{0x3bce18,17,0x199524e9c50400eaULL},
{0x3da26d,17,0x15af5ad93e3b1e7cULL},
{0x3f85b3,17,0xbfdd39e5e130feeeULL},
{0x404764,17,0x84eff605dc6776d7ULL},
{0x3da3f3,17,0x9bc34023c50fc3ULL},
{0x4114c6,17,0x82e6bbcd871af91bULL},
{0x411540,7,0x8af5eb618cba5a9bULL},
{0x407c00,12832,0xe590e80bdb711ad3ULL},
};
template<class Read> bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
