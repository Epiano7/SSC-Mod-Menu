#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3d9f20,10386,0x26cd081119bfb460ULL},
{0x923f10,23542,0x4415a35c7547f83cULL},
};
}
