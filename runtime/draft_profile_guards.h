#pragma once
#include <cstdint>
namespace ssc_challenge {
struct ProfileGuard{uint32_t rva,size;uint64_t hash;};
inline constexpr ProfileGuard profile_guards[]={
{0x2dff80,452,0xb02d29cb3b6dd7c4ULL},
{0x3e4aff,26,0xd41aa95513a6b0d1ULL},
};
template<class Read>bool validate_profile_prepare(uintptr_t base,Read read){for(const auto& g:profile_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
