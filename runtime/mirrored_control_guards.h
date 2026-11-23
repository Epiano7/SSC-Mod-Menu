#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x816440,5540,0xb5bf693393c4c44dULL},
 {0x818250,1495,0x752fe7a72494beb0ULL},
};
}
