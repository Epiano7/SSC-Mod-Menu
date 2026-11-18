#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x481600,390,0xc705a28dd4c0f964ULL},
 {0x743850,6181,0xcd95ea3d28bfa536ULL},
};
}
