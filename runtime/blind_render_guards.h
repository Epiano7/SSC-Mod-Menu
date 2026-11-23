#pragma once
#include <cstdint>
namespace ssc_blind_render {
struct Guard{uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
{0x3bd588,17,0x199524e9c50400eaULL},
{0x3da9dd,17,0x15af5ad93e3b1e7cULL},
{0x3f8d23,17,0xbfdd39e5e130feeeULL},
{0x404ed4,17,0x84eff605dc6776d7ULL},
{0x3dab63,17,0x009bc34023c50fc3ULL},
{0x411c36,17,0xc9b3843a5e076ccbULL},
{0x411cb0,7,0xe7c5dfbc9977a5e3ULL},
{0x408370,12832,0x348271a32e5023ebULL},
};
template<class Read> bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
