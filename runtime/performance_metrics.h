#pragma once
#include <windows.h>
#include <array>
namespace ssc_perf {
enum Kind {overlay,advisor,sharing,tracker,capture,scoring,count};
inline std::array<double,count> milliseconds{};
inline std::array<unsigned,count> calls{};
inline double clock_ms(){static double scale=[](){LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return 1000.0/double(f.QuadPart);}();LARGE_INTEGER t{};QueryPerformanceCounter(&t);return double(t.QuadPart)*scale;}
// Render-thread counters only; exclude native draw functions from advisor scopes.
struct Scope {Kind kind;double start;explicit Scope(Kind k):kind(k),start(clock_ms()){}~Scope(){milliseconds[kind]+=clock_ms()-start;++calls[kind];}};
}
