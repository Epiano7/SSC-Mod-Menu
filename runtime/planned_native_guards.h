#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7e0350,1281,0xfecf24621502044eULL},
{0x223920,1204,0x94f8b4ad55f07498ULL},
{0x2ead80,5775,0x0a1d9db53823b52bULL},
{0x223de0,819,0x40240cf3aae69f11ULL},
{0x9b32c0,808,0x0c8067da37b0fad2ULL},
{0x31c010,4472,0x715e549e52f1f4acULL},
{0x31d190,242,0x23014cfabc0182a1ULL},
{0x344500,182,0xcf83c330bb640c9cULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
