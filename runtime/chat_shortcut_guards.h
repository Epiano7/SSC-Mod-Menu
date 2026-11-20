#pragma once
#include <cstdint>
namespace ssc_commands {
struct ShortcutGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr ShortcutGuard shortcut_guards[]={
{0x3184f0,2877,0xa7df1882ebb0fc50ULL},
{0x9a5270,225,0x7065e6bb025a14f4ULL},
{0x31879d,370,0x1c81aeea76f6bab0ULL},
{0x318846,104,0x9e3a0e180c1e0737ULL},
};
// Verified native chat rectangle calculation; independent of the input adapter guard.
// A mismatch disables suggestions only, leaving normal chat and command replies intact.
template<class Read>bool validate_popup_anchor(uintptr_t base,Read read){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<88;++i){unsigned char b=0;if(!read(base+0x9a67d5+i,b))return false;h=(h^b)*1099511628211ULL;}return h==0x21c58d5f03f2d24cULL;}
template<class Read>bool validate_chat_shortcut(uintptr_t base,Read read){for(const auto& g:shortcut_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
