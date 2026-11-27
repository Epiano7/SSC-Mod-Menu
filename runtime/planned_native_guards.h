#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7e0210,1281,0x792e259e190c4eabULL},
{0x223910,1204,0x244f0d5354dcd66dULL},
{0x2eacf0,5775,0x27e8bb1e0b8382ceULL},
{0x223dd0,819,0xac124a224ed88998ULL},
{0x9b3010,808,0x4c5524e79b16de48ULL},
{0x31bf80,4472,0x9295a8e89c83cde9ULL},
{0x31d100,242,0x4b9d7aada69e915bULL},
{0x344470,182,0xcf83c330bb640c9cULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
