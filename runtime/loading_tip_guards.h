#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x4824d0,390,0x2756b7224ccb79f5ULL},
 {0x745980,6181,0x8e10c3facfe9bc92ULL},
};
}
