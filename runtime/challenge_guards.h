#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e2300,62,0xd3e07de735a39c05ULL},
{0x7e233e,57,0xa336a54e505b73faULL},
{0x7e2377,176,0x8c34fd91e32ee84aULL},
{0x7e2427,374,0x256addc1a1d2202bULL},
{0x7e259d,49,0x0c7ebf4ae70879afULL},
{0x7e25ce,364,0x59d93db2618a6a7fULL},
{0x7e273a,359,0x7443034e8a91a6adULL},
{0x7e28a1,27,0x4de70ec4220586a3ULL},
{0x7e28bc,1023,0xacefe29fc0c1eee6ULL},
{0x7e2cbb,10,0x34057d276777a889ULL},
{0x7e2cc5,23,0xc41d087e8de37003ULL},
{0x7e2cdc,92,0xead591a137aa287aULL},
{0x940e90,82,0x4a468f57a1bc336eULL},
{0x940ee2,87,0x074cc87324fad16aULL},
{0x940f39,17,0x3b824ea7c4913348ULL},
{0x940f4a,946,0xcdedb68e4509a774ULL},
{0x9412fc,119,0x77cc9077fb4eb850ULL},
{0x941373,12,0x4cecf72f5366572dULL},
{0x9440a0,49,0x173215fe702f42a3ULL},
{0x9440d1,117,0x2f8eacb437b68e17ULL},
{0x944146,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x481f90,387,0x084a992079bd4740ULL};
}
