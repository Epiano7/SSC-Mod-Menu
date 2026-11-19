#pragma once
#include <cstdint>
namespace ssc_mirror {
struct Guard {uint32_t rva,size;uint64_t hash;};
inline constexpr Guard guards[]={
 {0x814b00,66,0xad40bb96c2eb98a6ULL},
 {0x814b42,481,0x1bef7dc062796e74ULL},
 {0x814d23,30,0x7aedfa93f0003336ULL},
 {0x814d41,980,0x79d2f2af76dfb385ULL},
 {0x815115,3853,0x67c7e76f5624aaeaULL},
 {0x816022,102,0xbd3bdc0568f6ff7fULL},
 {0x816088,5,0x10ed4477e64a8a06ULL},
 {0x81608d,8,0xb3ad817421cc22d0ULL},
 {0x816095,15,0x33ded523db843cbeULL},
 {0x816910,9,0xbd8108b3f5441ac6ULL},
 {0x816919,902,0x7e607e1435afa2adULL},
 {0x816c9f,91,0x2bfbb26882b9fcf8ULL},
 {0x816cfa,85,0x7612dc7b22b93277ULL},
 {0x816d4f,192,0x348198122531159bULL},
 {0x816e0f,236,0x6ef845659f35e55eULL},
};
}
