#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x481b40,390,0xb232aef296ef57aeULL},
 {0x743f40,6181,0x2dedb23725e25b34ULL},
};
}
