#pragma once
#include "challenge_guards.h"
namespace ssc_challenge {
// Reviewed native syringe eligibility, normal pending mutation command,
// skill-view gates and the existing action request path for build 26668384.
// Existing compatibility fingerprints are unchanged.
inline constexpr Guard choice_guards[]={
{0x7d9640,271,0x87c501a6c1df4994ULL},
{0x3b9910,56,0xcae9707a286067efULL},
{0x1d8140,92,0x25a173f5452aa139ULL},
{0x3e0c70,360,0x47c1bca53552df56ULL},
{0x3e1096,278,0x2c7e694ad1d9d99cULL},
{0x3b9a85,27,0xd9a81e5cbb7e580dULL},
{0x3dafb4,43,0x57da9b6166d3fe66ULL},
{0x817278,26,0x53ab4e3b487d7229ULL},
{0x7df620,124,0x597f5a9496dfdf38ULL},
// Native bonus cap, profile update, and queued mutation consumer.
{0x3e1020,95,0xdfc5f75bdec4717eULL},
{0x3e135e,240,0xe6e6bb0408dd8294ULL},
{0x941600,266,0x0c658c8c597312b9ULL},
{0x924140,72,0x27ebbbf61911e2d6ULL},
{0x3b0110,175,0x008876da6e776ec1ULL},
{0x1b0720,140,0x948c8e49ab3c7063ULL},
{0x2db230,311,0xa19f7907a77b77c7ULL},
};
}
