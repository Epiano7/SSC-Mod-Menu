#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e09e0,62,0xd3e07de735a39c05ULL},
{0x7e0a1e,57,0x94bab1184e5e4c09ULL},
{0x7e0a57,176,0x8c34fd91e32ee84aULL},
{0x7e0b07,374,0x324de94e6b6ed40dULL},
{0x7e0c7d,49,0xc7ebf4ae70879afULL},
{0x7e0cae,364,0xd30fd59ca7a9300fULL},
{0x7e0e1a,359,0x6a98203df8366484ULL},
{0x7e0f81,27,0x4de70ec4220586a3ULL},
{0x7e0f9c,1023,0x4f1e3028d83b9544ULL},
{0x7e139b,10,0x34057d276777a889ULL},
{0x7e13a5,23,0xc41d087e8de37003ULL},
{0x7e13bc,92,0x4a7f94f766274ab4ULL},
{0x93f580,82,0xf6004874a41cb35cULL},
{0x93f5d2,87,0x74cc87324fad16aULL},
{0x93f629,17,0x3b824ea7c4913348ULL},
{0x93f63a,946,0x9494723ce3a224ccULL},
{0x93f9ec,119,0x41cb8e2a0477db9bULL},
{0x93fa63,12,0x4cecf72f5366572dULL},
{0x942790,49,0x173215fe702f42a3ULL},
{0x9427c1,117,0x2f8eacb437b68e17ULL},
{0x942836,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x481790,387,0x7f2a23a6766c989aULL};
}
