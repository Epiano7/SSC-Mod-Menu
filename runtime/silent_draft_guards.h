#pragma once
#include <cstdint>
namespace ssc_challenge {
struct SilentGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr SilentGuard silent_guards[]={
{0x940c40,328,0x9bd8c19184cfbcccULL},
{0x1e6ff0,41,0x6afc1924becc3ad5ULL},
{0x1e6f80,110,0x724e6d7f581c1810ULL},
{0x3af430,111,0xf6690919930ba664ULL},
{0x93fa20,114,0xd5e15aa30c210099ULL},
{0x3af4a0,69,0x15086c457c6b6b33ULL},
{0x814f54,94,0x6bb4021214da6cc8ULL},
};
template<class Read>bool validate_silent_drafts(uintptr_t base,Read read){for(const auto& g:silent_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
