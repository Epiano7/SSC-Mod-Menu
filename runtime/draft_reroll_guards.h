#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3dab60,10386,0xacbf231b1f6c78b5ULL},
{0x926330,23558,0x496ce70c97767c95ULL},
};
}
