#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3d9f00,10386,0xf172a99c0263cb42ULL},
{0x923da0,23542,0x7d34992ef62ecaa7ULL},
};
}
