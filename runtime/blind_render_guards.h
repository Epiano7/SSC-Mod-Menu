#pragma once
#include <cstdint>
namespace ssc_blind_render {
struct Guard{uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
{0x3bd608,17,0xeee1256dee601f3aULL},
{0x3daa6d,17,0x15af5ad93e3b1e7cULL},
{0x3f8db3,17,0xbfdd39e5e130feeeULL},
{0x404f64,17,0x84eff605dc6776d7ULL},
{0x3dabf3,17,0x009bc34023c50fc3ULL},
{0x411cc6,17,0x9054182255ba93ebULL},
{0x411d40,7,0x8fda235fdc420593ULL},
{0x408400,12832,0x6c0bb09834722bd6ULL},
};
template<class Read> bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
