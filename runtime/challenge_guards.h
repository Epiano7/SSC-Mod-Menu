#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e0590,62,0x2331050801eca1b5ULL},
{0x7e05ce,57,0x808363261b05e145ULL},
{0x7e0607,176,0x8c34fd91e32ee84aULL},
{0x7e06b7,374,0x22fa146bdb2c974bULL},
{0x7e082d,49,0xc7ebf4ae70879afULL},
{0x7e085e,364,0xc8883215cd821fbdULL},
{0x7e09ca,359,0xa872bfd45ebf2a8aULL},
{0x7e0b31,27,0x4de70ec4220586a3ULL},
{0x7e0b4c,1023,0x406b0bb16ba693bbULL},
{0x7e0f4b,10,0x34057d276777a889ULL},
{0x7e0f55,23,0xc41d087e8de37003ULL},
{0x7e0f6c,92,0x83fd597cf1afeba4ULL},
{0x93f0e0,82,0x18550d5d10166892ULL},
{0x93f132,87,0x74cc87324fad16aULL},
{0x93f189,17,0x3b824ea7c4913348ULL},
{0x93f19a,946,0x28312aa059105653ULL},
{0x93f54c,119,0xc9fa96886199ed6eULL},
{0x93f5c3,12,0x4cecf72f5366572dULL},
{0x9422f0,49,0x173215fe702f42a3ULL},
{0x942321,117,0x2f8eacb437b68e17ULL},
{0x942396,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x481470,387,0x37a86fea53f22376ULL};
}
