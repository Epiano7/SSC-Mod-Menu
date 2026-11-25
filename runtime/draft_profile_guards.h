#pragma once
#include <cstdint>
namespace ssc_challenge {
struct ProfileGuard{uint32_t rva,size;uint64_t hash;};
inline constexpr ProfileGuard profile_guards[]={
{0x2e0730,452,0xb88478bacbc23187ULL},
{0x3e52ff,26,0x8901a9e998b19eb1ULL},
};
template<class Read>bool validate_profile_prepare(uintptr_t base,Read read){for(const auto& g:profile_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
