#pragma once
#include <cstdint>
namespace ssc_challenge {
struct ProfileGuard{uint32_t rva,size;uint64_t hash;};
inline constexpr ProfileGuard profile_guards[]={
{0x2e0170,452,0x4e0753133b4a8d35ULL},
{0x3e4d1f,26,0xa7566df9bad3d859ULL},
};
template<class Read>bool validate_profile_prepare(uintptr_t base,Read read){for(const auto& g:profile_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
