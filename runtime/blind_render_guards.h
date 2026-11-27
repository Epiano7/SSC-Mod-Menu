#pragma once
#include <cstdint>
namespace ssc_blind_render {
struct Guard{uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
{0x3bd9b8,17,0xeee1256dee601f3aULL},
{0x3dae1d,17,0x15af5ad93e3b1e7cULL},
{0x3f9163,17,0xbfdd39e5e130feeeULL},
{0x405314,17,0x84eff605dc6776d7ULL},
{0x3dafa3,17,0x009bc34023c50fc3ULL},
{0x412076,17,0xc647c27b6085ab70ULL},
{0x4120f0,7,0x1b14f5d4897083b7ULL},
{0x4087b0,12832,0x816e67b8c02e07e6ULL},
};
template<class Read> bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
