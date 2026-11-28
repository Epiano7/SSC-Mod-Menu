#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e2ba0,62,0xcafe3a45e3b38ae5ULL},
{0x7e2bde,57,0x3210c3e887e25d78ULL},
{0x7e2c17,176,0x39547fab4fa3ca0eULL},
{0x7e2cc7,374,0x19d93c3f52e2d7e2ULL},
{0x7e2e3d,49,0x0c7ebf4ae70879afULL},
{0x7e2e6e,364,0x2e8487e42f65aea5ULL},
{0x7e2fda,359,0xe17946a144e9a11fULL},
{0x7e3141,27,0x4de70ec4220586a3ULL},
{0x7e315c,1046,0xd74c1df2a66cdc4fULL},
{0x7e3572,10,0x34057d276777a889ULL},
{0x7e357c,24,0x13aeb10cc56cba11ULL},
{0x7e3594,92,0x6acbd3142c5d4b1aULL},
{0x9419c0,82,0xc0641a031d44f2f2ULL},
{0x941a12,87,0x074cc87324fad16aULL},
{0x941a69,17,0x3b824ea7c4913348ULL},
{0x941a7a,946,0xc1dcd6de227df5aaULL},
{0x941e2c,119,0x222b23e25d39bbefULL},
{0x941ea3,12,0x4cecf72f5366572dULL},
{0x944bd0,49,0x173215fe702f42a3ULL},
{0x944c01,117,0x2f8eacb437b68e17ULL},
{0x944c76,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x482370,387,0x92c59536107a65cfULL};
}
