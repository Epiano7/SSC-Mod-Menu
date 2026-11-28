#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x816f40,5540,0x1c65242602ad64d3ULL},
 {0x818d50,1495,0xc9ceb14564224337ULL},
};
}
