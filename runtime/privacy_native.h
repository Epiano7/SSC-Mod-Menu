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
 const auto value=*reinterpret_cast<volatile const unsigned char*>(native_base+0xfb08cb);
 return value==0?Detection::off:value==1?Detection::on:Detection::unavailable;
}
inline bool initialize_native(uintptr_t base){
 native_base=0;
 struct Span{uint32_t rva,length;uint64_t hash;};
 const Span spans[]={{0x6a0a40,9088,0xd2c85d286158b268ULL},{0x1f2b80,3466,0x5a374676364d0c14ULL}};
 for(const auto& span:spans){std::vector<unsigned char> bytes(span.length);SIZE_T read=0;
  if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+span.rva),bytes.data(),bytes.size(),&read)||read!=bytes.size()){reader=native_read;return false;}
  uint64_t hash=14695981039346656037ULL;for(auto byte:bytes)hash=(hash^byte)*1099511628211ULL;
  if(hash!=span.hash){reader=native_read;return false;}
 }
 unsigned char value=255;SIZE_T read=0;
 if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+0xfb08cb),&value,1,&read)||read!=1||value>1){reader=native_read;return false;}
 native_base=base;reader=native_read;return true;
}
}
