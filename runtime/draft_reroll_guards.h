#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3daad0,10386,0x4d220a86dd2e997aULL},
{0x926090,23542,0xe01a8b594f692998ULL},
};
}
