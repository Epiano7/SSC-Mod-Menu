#pragma once
#include <cstdint>
namespace ssc_challenge {
// Reviewed build 26731514: native Ctrl-release profile submission, including
// normal alive checks, skill-vector serialization and the normal world request.
// Action 6 alone does not transmit the locally edited profile.
template<class Read>bool validate_profile_submit(uintptr_t base,Read read){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<0x143;++i){unsigned char b=0;if(!read(base+0x3e49b0+i,b))return false;h=(h^b)*1099511628211ULL;}return h==0xd0fb0ff95129a988ULL;}
}
