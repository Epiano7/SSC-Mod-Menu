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
 const auto value=*reinterpret_cast<volatile const unsigned char*>(native_base+0xfb48bb);
 return value==0?Detection::off:value==1?Detection::on:Detection::unavailable;
}
inline bool initialize_native(uintptr_t base){
 native_base=0;tournament_validated=false;
 struct Span{uint32_t rva,length;uint64_t hash;};
 const Span spans[]={{0x3ad9b0,41,0x352070099d97ced1ULL},{0x7de9b4,103,0x289f18bb8f1146a9ULL},{0xc7c218,13,0xd544cc964571dddeULL},{0x6a2570,9088,0x7a7668fad22341a1ULL},{0x1f3850,3466,0x6da1857e0cd58475ULL}};
 for(const auto& span:spans){std::vector<unsigned char> bytes(span.length);SIZE_T read=0;
  if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+span.rva),bytes.data(),bytes.size(),&read)||read!=bytes.size()){reader=native_read;return false;}
  uint64_t hash=14695981039346656037ULL;for(auto byte:bytes)hash=(hash^byte)*1099511628211ULL;
  if(hash!=span.hash){reader=native_read;return false;}
 }
 unsigned char value=255;SIZE_T read=0;
 if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(base+0xfb48bb),&value,1,&read)||read!=1||value>1){reader=native_read;return false;}
 tournament_validated=true;native_base=base;reader=native_read;return true;
}
}
