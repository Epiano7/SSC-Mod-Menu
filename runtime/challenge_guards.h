#pragma once
#include <cstdint>
namespace ssc_challenge {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard action_guards[]={
{0x7e0fb0,62,0xd3e07de735a39c05ULL},
{0x7e0fee,57,0x48b7b26af3290c7bULL},
{0x7e1027,176,0x8c34fd91e32ee84aULL},
{0x7e10d7,374,0xb57c2609583113b3ULL},
{0x7e124d,49,0xc7ebf4ae70879afULL},
{0x7e127e,364,0x2ee847525fac1a2bULL},
{0x7e13ea,359,0x116281ec66fd7507ULL},
{0x7e1551,27,0x4de70ec4220586a3ULL},
{0x7e156c,1023,0xcad2cbcd2f5863beULL},
{0x7e196b,10,0x34057d276777a889ULL},
{0x7e1975,23,0xc41d087e8de37003ULL},
{0x7e198c,92,0x90abdbfb6b130ffULL},
{0x93fc40,82,0xe82c2f842ace37daULL},
{0x93fc92,87,0x74cc87324fad16aULL},
{0x93fce9,17,0x3b824ea7c4913348ULL},
{0x93fcfa,946,0x73ec831c2d5313b2ULL},
{0x9400ac,119,0x8ed8a1f709af674bULL},
{0x940123,12,0x4cecf72f5366572dULL},
{0x942e50,49,0x173215fe702f42a3ULL},
{0x942e81,117,0x2f8eacb437b68e17ULL},
{0x942ef6,25,0xf0b43a67e9ea2f18ULL}
};
inline constexpr Guard text_guard={0x4819b0,387,0xd5275447674c0590ULL};
}
