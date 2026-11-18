#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x814530,66,0xebb737f061d168a7ULL},
 {0x814572,481,0x69912621783a7d44ULL},
 {0x814753,30,0x90a1de8bafc208d5ULL},
 {0x814771,980,0x54cebfcd4334dfedULL},
 {0x814b45,3853,0xc12ed34f167a8490ULL},
 {0x815a52,102,0xe08a675f1e041308ULL},
 {0x815ab8,5,0x10ed4477e64a8a06ULL},
 {0x815abd,8,0xb3ad817421cc22d0ULL},
 {0x815ac5,15,0x33ded523db843cbeULL},
 {0x816340,9,0xbd8108b3f5441ac6ULL},
 {0x816349,902,0x4015ff8987fe78f0ULL},
 {0x8166cf,91,0x4061d7ca2ab5fac7ULL},
 {0x81672a,85,0x6012697ca90d6ea0ULL},
 {0x81677f,192,0xdbfe89d45adf06c1ULL},
 {0x81683f,236,0x8f4739015cb4d21ULL},
};
}
