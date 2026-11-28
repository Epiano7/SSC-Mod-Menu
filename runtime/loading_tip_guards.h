#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x482500,390,0x6d0b28fed590b424ULL},
 {0x7459c0,6181,0x7db51697f141748cULL},
};
}
