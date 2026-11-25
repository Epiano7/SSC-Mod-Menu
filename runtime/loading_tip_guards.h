#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x482120,390,0x6b51c0f2e69139c3ULL},
 {0x7452b0,6181,0x55df720fcb5e2019ULL},
};
}
