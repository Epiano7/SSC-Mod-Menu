#pragma once
#include <cstdint>
namespace ssc_blind_render {
struct Guard{uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
{0x3bda48,17,0x1b3f624ed439139aULL},
{0x3daead,17,0x125ae00f1fd0f91cULL},
{0x3f9193,17,0xbfdd39e5e130feeeULL},
{0x405344,17,0x84eff605dc6776d7ULL},
{0x3db033,17,0x009bc34023c50fc3ULL},
{0x4120a6,17,0x044f2b08a14b42dbULL},
{0x412120,7,0x1b993659ed851707ULL},
{0x4087e0,12832,0x885926dd076df4f1ULL},
};
template<class Read> bool validate(uintptr_t base,Read read){for(const auto& g:guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
