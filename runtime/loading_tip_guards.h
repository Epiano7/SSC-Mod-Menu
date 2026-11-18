#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x481900,390,0x9ce9d2200efc19deULL},
 {0x743ad0,6181,0xc837e6de9f736544ULL},
};
}
