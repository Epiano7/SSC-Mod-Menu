#pragma once
#include <cstdint>
namespace ssc_challenge {
struct SilentGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr SilentGuard silent_guards[]={
{0x9431f0,328,0x011feb1a581180d2ULL},
{0x1e77b0,41,0x6afc1924becc3ad5ULL},
{0x1e7740,110,0x724e6d7f581c1810ULL},
{0x3b0070,111,0xf6690919930ba664ULL},
{0x941fd0,114,0x12bed8ee672611b4ULL},
{0x3b00e0,69,0x15086c457c6b6b33ULL},
{0x8174d4,94,0x6bb4021214da6cc8ULL},
};
template<class Read>bool validate_silent_drafts(uintptr_t base,Read read){for(const auto& g:silent_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
