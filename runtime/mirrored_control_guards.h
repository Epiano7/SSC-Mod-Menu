#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x8164d0,5540,0x2dd671b72f7c7124ULL},
 {0x8182e0,1495,0x03b7d4ff6b280c4dULL},
};
}
