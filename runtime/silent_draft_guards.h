#pragma once
#include <cstdint>
namespace ssc_challenge {
struct SilentGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr SilentGuard silent_guards[]={
{0x9426c0,328,0x1773345ca6507408ULL},
{0x1e76f0,41,0x6afc1924becc3ad5ULL},
{0x1e7680,110,0x724e6d7f581c1810ULL},
{0x3afc30,111,0xf6690919930ba664ULL},
{0x9414a0,114,0xc7c730847d729bd8ULL},
{0x3afca0,69,0x15086c457c6b6b33ULL},
{0x816a64,94,0x6bb4021214da6cc8ULL},
};
template<class Read>bool validate_silent_drafts(uintptr_t base,Read read){for(const auto& g:silent_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
