#pragma once
#include "privacy.h"
#include <windows.h>
#include <vector>
namespace ssc_privacy {
inline uintptr_t native_base=0;
inline Detection native_read(){
 if(!native_base)return Detection::unavailable;
 // The game stores this byte in its process-lifetime settings block. The
 // settings and native account-copy functions are validated before enabling it.
 const auto value=*reinterpret_cast<volatile const unsigned char*>(native_base+0xfb6a33);
 return value==0?Detection::off:value==1?Detection::on:Detection::unavailable;
}
inline bool initialize_native(uintptr_t base){
 native_base=0;tournament_validated=false;
 struct Span{uint32_t rva,length;uint64_t hash;};
 const Span spans[]={{0x3ae330,41,0x73f902f05aff70a3ULL},{0x7e0484,103,0x3cc4dc3ee15187adULL},{0xc7e580,13,0xd544cc964571dddeULL},{0x6a3000,9088,0x8a7f58172a65f410ULL},{0x1f3e90,3466,0x0d0f3435d6660034ULL}};
 for(const auto& span:spans){std::vector<unsigned char> bytes(span.length);SIZE_T read=0;
  if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+span.rva),bytes.data(),bytes.size(),&read)||read!=bytes.size()){reader=native_read;return false;}
  uint64_t hash=14695981039346656037ULL;for(auto byte:bytes)hash=(hash^byte)*1099511628211ULL;
  if(hash!=span.hash){reader=native_read;return false;}
 }
 unsigned char value=255;SIZE_T read=0;
 if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+0xfb6a33),&value,1,&read)||read!=1||value>1){reader=native_read;return false;}
 tournament_validated=true;native_base=base;reader=native_read;return true;
}
}
