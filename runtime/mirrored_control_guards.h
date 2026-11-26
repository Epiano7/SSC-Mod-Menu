#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x816810,5540,0xc221f1ed384d2029ULL},
 {0x818620,1495,0x875d01830aa1fe83ULL},
};
}
