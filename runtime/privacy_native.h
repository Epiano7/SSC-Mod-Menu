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
 const auto value=*reinterpret_cast<volatile const unsigned char*>(native_base+0xfb68bb);
 return value==0?Detection::off:value==1?Detection::on:Detection::unavailable;
}
inline bool initialize_native(uintptr_t base){
 native_base=0;tournament_validated=false;
 struct Span{uint32_t rva,length;uint64_t hash;};
 const Span spans[]={{0x3adf80,41,0x00d564f29bf8b3b9ULL},{0x7dfd04,103,0x0b0cc4f317fc4aa5ULL},{0xc7e538,13,0xd544cc964571dddeULL},{0x6a2b50,9088,0xf57571686e3b125dULL},{0x1f3de0,3466,0x7e24af7cbe251685ULL}};
 for(const auto& span:spans){std::vector<unsigned char> bytes(span.length);SIZE_T read=0;
  if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+span.rva),bytes.data(),bytes.size(),&read)||read!=bytes.size()){reader=native_read;return false;}
  uint64_t hash=14695981039346656037ULL;for(auto byte:bytes)hash=(hash^byte)*1099511628211ULL;
  if(hash!=span.hash){reader=native_read;return false;}
 }
 unsigned char value=255;SIZE_T read=0;
 if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+0xfb68bb),&value,1,&read)||read!=1||value>1){reader=native_read;return false;}
 tournament_validated=true;native_base=base;reader=native_read;return true;
}
}
