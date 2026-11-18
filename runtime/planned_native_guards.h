#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7ddd30,1281,0x3b00f1925db733d0ULL},
{0x2230b0,1204,0xc5e3d9c8eef5b14ULL},
{0x2e9da0,5775,0xbf13e76398075a8dULL},
{0x223570,819,0xe40eaeeea3a14cb2ULL},
{0x9b0930,808,0xc5e1e2b56d45708bULL},
{0x31b070,4492,0xdc812a2ca78c2caaULL},
{0x31c200,242,0x92397d85a1cb2d81ULL},
{0x343550,182,0x84063e969ee06217ULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
