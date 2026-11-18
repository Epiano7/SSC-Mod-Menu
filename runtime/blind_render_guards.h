#pragma once
#include <cstdint>
namespace ssc_blind_render {
struct Guard{uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
{0x3bcb28,17,0x4665e3d8dc4e190aULL},
{0x3d9f6d,17,0x25e002aaad9bbbccULL},
{0x3f82a3,17,0xbfdd39e5e130feeeULL},
{0x404454,17,0x84eff605dc6776d7ULL},
{0x3da0f3,17,0xa0e00be362603b08ULL},
{0x4111b6,17,0x74170e413a035bedULL},
{0x411230,7,0x509dd0ebe8f1631aULL},
{0x4078f0,12832,0xbda633d372229de6ULL},
};
template<class Read> bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
