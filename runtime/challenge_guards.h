#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e0940,62,0xd3e07de735a39c05ULL},
{0x7e097e,57,0xb3dd50c667140194ULL},
{0x7e09b7,176,0x8c34fd91e32ee84aULL},
{0x7e0a67,374,0xd3e22bad42e5edf6ULL},
{0x7e0bdd,49,0xc7ebf4ae70879afULL},
{0x7e0c0e,364,0x5c4fec11b1d6beb7ULL},
{0x7e0d7a,359,0x5b7acbb6b890183ULL},
{0x7e0ee1,27,0x4de70ec4220586a3ULL},
{0x7e0efc,1023,0xe95452e8aa4ac539ULL},
{0x7e12fb,10,0x34057d276777a889ULL},
{0x7e1305,23,0xc41d087e8de37003ULL},
{0x7e131c,92,0x103ede52b8647a0bULL},
{0x93f410,82,0x42d782bc134e3d2cULL},
{0x93f462,87,0x74cc87324fad16aULL},
{0x93f4b9,17,0x3b824ea7c4913348ULL},
{0x93f4ca,946,0x13bd3289934b2e8aULL},
{0x93f87c,119,0xcd75142623c8d6a6ULL},
{0x93f8f3,12,0x4cecf72f5366572dULL},
{0x942620,49,0x173215fe702f42a3ULL},
{0x942651,117,0x2f8eacb437b68e17ULL},
{0x9426c6,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x481770,387,0x614506afbfaca261ULL};
}
