#pragma once
#include <cstdint>
namespace ssc_challenge {
// Native skill-button branch: client request versus direct authoritative action.
inline constexpr uint32_t client_commit_rva=0x3e4356,client_commit_size=42;
inline constexpr uint64_t client_commit_hash=0x5c7ad7ca6d1d4c07ULL;
template<class Read> bool validate_client_commit(uintptr_t base,Read read){uint64_t h=14695981039346656037ULL;for(uint32_t i=0;i<client_commit_size;++i){unsigned char b=0;if(!read(base+client_commit_rva+i,b))return false;h=(h^b)*1099511628211ULL;}return h==client_commit_hash;}
}
