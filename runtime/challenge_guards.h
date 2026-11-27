#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e2a60,62,0xcafe3a45e3b38ae5ULL},
{0x7e2a9e,57,0x30090a2989fcc489ULL},
{0x7e2ad7,176,0x8c34fd91e32ee84aULL},
{0x7e2b87,374,0x652b445010209d7eULL},
{0x7e2cfd,49,0x0c7ebf4ae70879afULL},
{0x7e2d2e,364,0x8cb0482e822139c3ULL},
{0x7e2e9a,359,0x8d165eed91df4799ULL},
{0x7e3001,27,0x4de70ec4220586a3ULL},
{0x7e301c,1023,0x878ac2797322d69eULL},
{0x7e341b,10,0x34057d276777a889ULL},
{0x7e3425,23,0xc41d087e8de37003ULL},
{0x7e343c,92,0xe1b24d4fe77b2174ULL},
{0x941710,82,0xf324d80af96b425cULL},
{0x941762,87,0x074cc87324fad16aULL},
{0x9417b9,17,0x3b824ea7c4913348ULL},
{0x9417ca,946,0x63b52901ffea21f2ULL},
{0x941b7c,119,0x5794481367666ee5ULL},
{0x941bf3,12,0x4cecf72f5366572dULL},
{0x944920,49,0x173215fe702f42a3ULL},
{0x944951,117,0x2f8eacb437b68e17ULL},
{0x9449c6,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x482340,387,0x55beed09a36dac60ULL};
}
