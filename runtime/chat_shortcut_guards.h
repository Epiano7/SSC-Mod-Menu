#pragma once
#include <cstdint>
namespace ssc_commands {
struct ShortcutGuard {uint32_t rva,size;uint64_t hash;};
inline constexpr ShortcutGuard shortcut_guards[]={
{0x317f70,2877,0x50c5aae640276352ULL},
{0x9a46d0,225,0x5bad6701cca5b8cULL},
{0x31821d,370,0x6b5ee60fbc4334e2ULL},
{0x3182c6,104,0xeeed3d82502d0d8ULL},
};
// Verified native chat rectangle calculation; independent of the input adapter guard.
// A mismatch disables suggestions only, leaving normal chat and command replies intact.
template<class Read>bool validate_popup_anchor(uintptr_t base,Read read){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<88;++i){unsigned char b=0;if(!read(base+0x9a5c35+i,b))return false;h=(h^b)*1099511628211ULL;}return h==0x21c58d5f03f2d24cULL;}
template<class Read>bool validate_chat_shortcut(uintptr_t base,Read read){for(const auto& g:shortcut_guards){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<g.size;++i){unsigned char b=0;if(!read(base+g.rva+i,b))return false;h=(h^b)*1099511628211ULL;}if(h!=g.hash)return false;}return true;}
}
