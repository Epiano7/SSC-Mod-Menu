#pragma once
#include <cstdint>
namespace ssc_blind_render {
struct Guard{uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
{0x3bcdf8,17,0x199524e9c50400eaULL},
{0x3da24d,17,0x15af5ad93e3b1e7cULL},
{0x3f8593,17,0xbfdd39e5e130feeeULL},
{0x404744,17,0x84eff605dc6776d7ULL},
{0x3da3d3,17,0x9bc34023c50fc3ULL},
{0x4114a6,17,0x54fd6756d2dd381bULL},
{0x411520,7,0x9e32a5a7348fe7bULL},
{0x407be0,12832,0x950319827f75de4bULL},
};
template<class Read> bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
