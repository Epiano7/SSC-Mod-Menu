#pragma once
#include <cstdint>
namespace ssc_loading_tips {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x481920,390,0x4fb153f79dd89941ULL},
 {0x743b70,6181,0x2e998804c7f159bcULL},
};
}
