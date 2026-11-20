#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3da140,10386,0x7d7c65d2610bb69aULL},
{0x9245f0,23510,0x835a3c6e4d8c1954ULL},
};
}
