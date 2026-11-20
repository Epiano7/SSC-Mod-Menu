#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x815150,5540,0xe6adaa23463d484cULL},
 {0x816f60,1495,0xe0ca634da446619dULL},
};
}
