#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7dfa30,1281,0xefc9bbfc73b65d3cULL},
{0x223810,1204,0xa6a85af5b5fa860bULL},
{0x2ea870,5775,0x802350272f8da40eULL},
{0x223cd0,819,0x75c391c182cab580ULL},
{0x9b2650,808,0x8f31f75a25460397ULL},
{0x31bb40,4492,0xfbef61dc09bfe650ULL},
{0x31ccd0,242,0x22f449ace0cefee3ULL},
{0x344040,182,0xcf83c330bb640c9cULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
