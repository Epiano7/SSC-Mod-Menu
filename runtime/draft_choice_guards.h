#pragma once
#include "challenge_guards.h"
namespace ssc_challenge {
// Reviewed native syringe eligibility, normal pending mutation command,
// skill-view gates and the existing action request path for build 26668384.
// Existing compatibility fingerprints are unchanged.
inline constexpr Guard choice_guards[]={
{0x7d9790,271,0x2f5f106457f8b819ULL},
{0x3b99a0,56,0xe55ef90839faad5fULL},
{0x1d8150,92,0xd093d93f9da65829ULL},
{0x3e0d00,360,0x46801740bd51b8a7ULL},
{0x3e1126,278,0x7da714fcd620920bULL},
{0x3b9b15,27,0x6b6143118513a45dULL},
{0x3db044,43,0xbf48452f0b4d2a1fULL},
{0x817518,26,0x53ab4e3b487d7229ULL},
{0x7df760,124,0x45a6096fc57a9365ULL},
// Native bonus cap, profile update, and queued mutation consumer.
{0x3e10b0,95,0x0c3a8bc4f5db58f5ULL},
{0x3e13ee,240,0x71a00f6dfb8443c9ULL},
{0x9418b0,266,0xb787247dd77e908aULL},
{0x9243e0,72,0xd460a9db088d5594ULL},
{0x3b01a0,175,0x008876da6e776ec1ULL},
{0x1b0730,140,0x948c8e49ab3c7063ULL},
{0x2db2c0,311,0x23c6f9d2470c5c9cULL},
};
}
