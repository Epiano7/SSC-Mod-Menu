#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7dfa90,1281,0xf3d532593b6cd96fULL},
{0x223850,1204,0xa79c8816ad124168ULL},
{0x2ea8e0,5775,0x845cece9d040f066ULL},
{0x223d10,819,0x832ae89de089bbb1ULL},
{0x9b2780,808,0x2ecde29b3eaeb611ULL},
{0x31bbc0,4492,0xd682eb9e16364a9dULL},
{0x31cd50,242,0x8178657fb03158ecULL},
{0x3440c0,182,0xcf83c330bb640c9cULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
