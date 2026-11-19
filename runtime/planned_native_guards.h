#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7de170,1281,0xc98143f7e13a860eULL},
{0x223190,1204,0x9cbc8d0772de5c3fULL},
{0x2ea130,5775,0x6e92c2f75497b67bULL},
{0x223650,819,0x97a6358b16f13a3dULL},
{0x9b0e30,808,0x4a15fa72e2d057ddULL},
{0x31b400,4492,0x9a4c3331bd604de0ULL},
{0x31c590,242,0x186e2c03108d8c81ULL},
{0x3438e0,182,0x84063e969ee06217ULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
