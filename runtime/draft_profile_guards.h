#pragma once
#include <cstdint>
namespace ssc_challenge {
struct ProfileGuard{uint32_t rva,size;uint64_t hash;};
inline constexpr ProfileGuard profile_guards[]={
{0x2dff60,452,0xe1da9d9b7179b349ULL},
{0x3e4adf,26,0x74d432cba89a5751ULL},
};
template<class Read>bool validate_profile_prepare(uintptr_t base,Read read){for(const auto& g:profile_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
