#pragma once
#include <cstdint>
namespace ssc_challenge {
struct RerollGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr RerollGuard reroll_guards[]={
{0x3d9c20,10386,0x96c8b7456c0be98eULL},
{0x923b00,23398,0x9b7edaa16e063dabULL},
};
}
