#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x4823b0,390,0x9436a2352ca30918ULL},
 {0x745540,6181,0xda4d67ad041d1000ULL},
};
}
