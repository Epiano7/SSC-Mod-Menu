#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e2640,62,0xd3e07de735a39c05ULL},
{0x7e267e,57,0x67c531baacd09825ULL},
{0x7e26b7,176,0x8c34fd91e32ee84aULL},
{0x7e2767,374,0x25aaf6406ab9f806ULL},
{0x7e28dd,49,0x0c7ebf4ae70879afULL},
{0x7e290e,364,0xb1d60d3685b43cc1ULL},
{0x7e2a7a,359,0xd3f20f8b8ae5215dULL},
{0x7e2be1,27,0x4de70ec4220586a3ULL},
{0x7e2bfc,1023,0xfac2874ed3ea8a85ULL},
{0x7e2ffb,10,0x34057d276777a889ULL},
{0x7e3005,23,0xc41d087e8de37003ULL},
{0x7e301c,92,0x3e70f3a425ba4348ULL},
{0x941260,82,0x71c8c14fa7da68ceULL},
{0x9412b2,87,0x074cc87324fad16aULL},
{0x941309,17,0x3b824ea7c4913348ULL},
{0x94131a,946,0x7dc1660e9e8fa3f3ULL},
{0x9416cc,119,0x06dd77b2c02ba4e0ULL},
{0x941743,12,0x4cecf72f5366572dULL},
{0x944470,49,0x173215fe702f42a3ULL},
{0x9444a1,117,0x2f8eacb437b68e17ULL},
{0x944516,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x482220,387,0x3296c799c14ae33aULL};
}
