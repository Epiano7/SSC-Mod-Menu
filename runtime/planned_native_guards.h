#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7de0d0,1281,0xb6e31305e2d930daULL},
{0x223170,1204,0x9367b1043bcee60aULL},
{0x2ea110,5775,0x45295a346aa5b8a6ULL},
{0x223630,819,0x24ec5c6b00bad033ULL},
{0x9b0c60,808,0xf19c441f2b6d1aeaULL},
{0x31b3e0,4492,0x681fb55b340a3882ULL},
{0x31c570,242,0xecf1ef7bb1460001ULL},
{0x3438c0,182,0x84063e969ee06217ULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
