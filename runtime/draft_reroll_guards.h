#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3d9f20,10386,0x13d17ac1e712da8cULL},
{0x923f40,23542,0xe32f8cfbd915ad37ULL},
};
}
