#pragma once
#include <cstdint>
namespace ssc_commands {
struct ShortcutGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr ShortcutGuard shortcut_guards[]={
{0x318e80,2877,0x0701e81085f5ab82ULL},
{0x9a6db0,225,0x8e23954636de55a5ULL},
{0x31912d,370,0x1239b3302443ec46ULL},
{0x3191d6,104,0xa1875ca9c90c30a7ULL},
};
// Verified native chat rectangle calculation; independent of the input adapter guard.
// A mismatch disables suggestions only, leaving normal chat and command replies intact.
template<class Read>bool validate_popup_anchor(uintptr_t base,Read read){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<88;++i){unsigned char b=0;if(!read(base+0x9a8315+i,b))return false;h=(h^b)*1099511628211ULL;}return h==0x11e58e10e9d10433ULL;}
template<class Read>bool validate_chat_shortcut(uintptr_t base,Read read){for(const auto& g:shortcut_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
