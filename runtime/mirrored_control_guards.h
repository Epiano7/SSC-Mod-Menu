#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x816ca0,5540,0xc925b0cfe87bc1f9ULL},
 {0x818ab0,1495,0xfdc6f912f85a1494ULL},
};
}
