#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x481920,390,0x36753ad5ae644048ULL},
 {0x743b70,6181,0xb4321b27d5b1c51cULL},
};
}
