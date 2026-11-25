#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3da720,10386,0x883e0bb82bcb6f91ULL},
{0x925820,23542,0x57a813a5a2acbba7ULL},
};
}
