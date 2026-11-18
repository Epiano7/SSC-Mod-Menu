#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x8149c0,66,0x3e2f9cc73ffe0204ULL},
 {0x814a02,481,0x6f4672d9ea785a4aULL},
 {0x814be3,30,0xea908f599a0674b0ULL},
 {0x814c01,980,0x3e7ea2af560e05eaULL},
 {0x814fd5,3853,0x9c10b8198429dca8ULL},
 {0x815ee2,102,0x3be6b1679284ba84ULL},
 {0x815f48,5,0x10ed4477e64a8a06ULL},
 {0x815f4d,8,0xb3ad817421cc22d0ULL},
 {0x815f55,15,0x33ded523db843cbeULL},
 {0x8167d0,9,0xbd8108b3f5441ac6ULL},
 {0x8167d9,902,0xb6480631a99e37f8ULL},
 {0x816b5f,91,0xec64379b73b69c50ULL},
 {0x816bba,85,0x1528e857d7a7c43cULL},
 {0x816c0f,192,0x7f43af83ffb3d6dbULL},
 {0x816ccf,236,0xf2dc6265b10c6cb1ULL},
};
}
