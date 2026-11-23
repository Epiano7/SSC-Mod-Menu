#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e22a0,62,0xd3e07de735a39c05ULL},
{0x7e22de,57,0xdedc25dad4e5501aULL},
{0x7e2317,176,0x8c34fd91e32ee84aULL},
{0x7e23c7,374,0x3160e9e2c1814430ULL},
{0x7e253d,49,0x0c7ebf4ae70879afULL},
{0x7e256e,364,0x2580e07edfd6f899ULL},
{0x7e26da,359,0x48d6eb3d06d33969ULL},
{0x7e2841,27,0x4de70ec4220586a3ULL},
{0x7e285c,1023,0xfb5975e8eacda46cULL},
{0x7e2c5b,10,0x34057d276777a889ULL},
{0x7e2c65,23,0xc41d087e8de37003ULL},
{0x7e2c7c,92,0x6a677a412975b785ULL},
{0x940dc0,82,0x988cc800fbc2adccULL},
{0x940e12,87,0x074cc87324fad16aULL},
{0x940e69,17,0x3b824ea7c4913348ULL},
{0x940e7a,946,0x1461599b9b786243ULL},
{0x94122c,119,0x20ac04854006427fULL},
{0x9412a3,12,0x4cecf72f5366572dULL},
{0x943fd0,49,0x173215fe702f42a3ULL},
{0x944001,117,0x2f8eacb437b68e17ULL},
{0x944076,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x481f00,387,0x93fc0810d5f2514aULL};
}
