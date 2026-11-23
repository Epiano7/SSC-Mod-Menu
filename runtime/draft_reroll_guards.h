#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3da690,10386,0x7fbb62f5d0da6ef2ULL},
{0x925770,23510,0x3af1752dd2cb2b76ULL},
};
}
