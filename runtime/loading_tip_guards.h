#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x482090,390,0x1bc3761734b90178ULL},
 {0x745230,6181,0x87838f5b1919f2c8ULL},
};
}
