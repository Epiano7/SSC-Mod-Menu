#pragma once
#include <cstdint>
namespace ssc_commands {
struct ShortcutGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr ShortcutGuard shortcut_guards[]={
{0x318ac0,2877,0x1153b6da7277bd10ULL},
{0x9a6520,225,0x876b96d2c77ccc54ULL},
{0x318d6d,370,0xb8a4d1babfda3c13ULL},
{0x318e16,104,0x03d7ec3469dc47abULL},
};
// Verified native chat rectangle calculation; independent of the input adapter guard.
// A mismatch disables suggestions only, leaving normal chat and command replies intact.
template<class Read>bool validate_popup_anchor(uintptr_t base,Read read){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<88;++i){unsigned char b=0;if(!read(base+0x9a7a85+i,b))return false;h=(h^b)*1099511628211ULL;}return h==0x21c58d5f03f2d24cULL;}
template<class Read>bool validate_chat_shortcut(uintptr_t base,Read read){for(const auto& g:shortcut_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
