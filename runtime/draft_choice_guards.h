#pragma once
#include "challenge_guards.h"
namespace ssc_challenge {
// Reviewed native syringe eligibility, normal pending mutation command,
// skill-view gates and the existing action request path for build 26668384.
// Existing compatibility fingerprints are unchanged.
inline constexpr Guard choice_guards[]={
{0x7d9200,271,0xb0cfcfafdbbc41c2ULL},
{0x3b97f0,56,0x89442f41f8fa728eULL},
{0x1d8090,92,0x4f8918a8e530f508ULL},
{0x3e0b50,360,0x8a9c1c2d7e4075dfULL},
{0x3e0f76,278,0x91608a39684305d2ULL},
{0x3b9965,27,0xf759b13ef80081e5ULL},
{0x3dae94,43,0x170e2b99b175dc3eULL},
{0x816de8,26,0x53ab4e3b487d7229ULL},
{0x7df1e0,124,0x9693af392a965fcbULL},
// Native bonus cap, profile update, and queued mutation consumer.
{0x3e0f00,95,0x161643dd0fe51e6aULL},
{0x3e123e,240,0xa3d263c3afb76614ULL},
{0x941150,266,0x2f7634b8c8f26451ULL},
{0x923ca0,72,0x594e7b961175c370ULL},
{0x3afff0,175,0x008876da6e776ec1ULL},
{0x1b0670,140,0x948c8e49ab3c7063ULL},
{0x2db110,311,0x1a8fd25c9bed714dULL},
};
}
