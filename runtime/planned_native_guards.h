#pragma once
#include "cosmetic_adapter.h"
namespace ssc_planned_native {
struct Guard {uint32_t rva,size;uint64_t hash;};
// Reviewed against Steam build 26346222. These feature guards are checked before
// any name/chat hooks; the existing compatibility manifest remains unchanged.
inline constexpr Guard guards[]={
{0x7dfdd0,1281,0xdc3e5a1fc6b60feaULL},
{0x223850,1204,0x54f4b0b07d868a18ULL},
{0x2eabd0,5775,0x2d68ea2c1cd79617ULL},
{0x223d10,819,0x8ed49c7cc0ee7580ULL},
{0x9b2b60,808,0xa7738e4216efd64eULL},
{0x31be60,4472,0x8c39d8e652e3b99cULL},
{0x31cfe0,242,0x48530eedaafb0ee1ULL},
{0x344350,182,0xcf83c330bb640c9cULL}
};
inline bool kos=false,commands=false,tts=false;
inline bool check(uintptr_t base,const Guard& g){std::vector<unsigned char> bytes(g.size);if(!ssc_names::read_bytes(base+g.rva,bytes.data(),bytes.size()))return false;uint64_t h=14695981039346656037ULL;for(auto c:bytes)h=(h^c)*1099511628211ULL;return h==g.hash;}
inline void initialize(uintptr_t base){kos=check(base,guards[0]);commands=check(base,guards[1])&&check(base,guards[2])&&check(base,guards[3])&&check(base,guards[4]);tts=check(base,guards[4])&&check(base,guards[5])&&check(base,guards[6])&&check(base,guards[7]);}
}
