#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7de740,1281,0xfd024aeb18dc4526ULL},
{0x2232c0,1204,0x8502c39c1a2d4a50ULL},
{0x2ea320,5775,0xe5744646925e55f4ULL},
{0x223780,819,0x243ba381daa24dacULL},
{0x9b14d0,808,0xf03bb8abbe3f92b3ULL},
{0x31b5f0,4492,0x1b79995b487f9cedULL},
{0x31c780,242,0x8466ffec7a0b40fcULL},
{0x343af0,182,0xcf83c330bb640c9cULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
