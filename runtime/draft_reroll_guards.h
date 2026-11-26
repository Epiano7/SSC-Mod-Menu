#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3da9b0,10386,0xd119f15149aa8f70ULL},
{0x925bf0,23542,0xfc3fa9ea01c2c906ULL},
};
}
