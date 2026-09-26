#pragma once
#include <windows.h>
#include <filesystem>
#include <map>
#include <string>
#include <atomic>
#include <algorithm>
#include <cstring>
namespace ssc_music {
using Open=HANDLE(WINAPI*)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
inline Open original=CreateFileW;
// Immutable once the game import is attached; UI edits apply on restart.
inline std::map<std::wstring,std::wstring> redirects;
inline std::atomic<unsigned> redirected{0};
inline bool attached=false;
inline std::wstring normalized(const std::filesystem::path& path){
 auto s=std::filesystem::absolute(path).lexically_normal().wstring();std::replace(s.begin(),s.end(),L'/',L'\\');CharLowerBuffW(s.data(),DWORD(s.size()));return s;
}
inline HANDLE WINAPI open(LPCWSTR path,DWORD access,DWORD sharing,LPSECURITY_ATTRIBUTES security,DWORD creation,DWORD flags,HANDLE template_file){
 if(path&&!redirects.empty()&&wcslen(path)>=4&&!_wcsicmp(path+wcslen(path)-4,L".ogg")&&creation==OPEN_EXISTING&&(access&GENERIC_READ)&&!(access&(GENERIC_WRITE|DELETE|FILE_WRITE_DATA|FILE_APPEND_DATA))){
  try{auto found=redirects.find(normalized(path));if(found!=redirects.end()){
   auto handle=original(found->second.c_str(),access,sharing,security,creation,flags,template_file);
   if(handle!=INVALID_HANDLE_VALUE){++redirected;return handle;}
  }}catch(...){}
 }
 return original(path,access,sharing,security,creation,flags,template_file);
}
inline bool attach(){
 auto base=reinterpret_cast<unsigned char*>(GetModuleHandleW(nullptr));auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
 auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);auto size=nt->OptionalHeader.SizeOfImage;auto dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
 if(dir.VirtualAddress>=size||dir.Size>size-dir.VirtualAddress)return false;
 for(size_t off=0;off+sizeof(IMAGE_IMPORT_DESCRIPTOR)<=dir.Size;off+=sizeof(IMAGE_IMPORT_DESCRIPTOR)){
  auto d=reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base+dir.VirtualAddress+off);if(!d->Name)break;
  if(!d->OriginalFirstThunk)continue;
  for(size_t i=0;;++i){size_t nr=d->OriginalFirstThunk+i*8,ar=d->FirstThunk+i*8;if(nr+8>size||ar+8>size)return false;
   auto n=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+nr),a=reinterpret_cast<IMAGE_THUNK_DATA64*>(base+ar);if(!n->u1.AddressOfData)break;
   if(IMAGE_SNAP_BY_ORDINAL64(n->u1.Ordinal))continue;
   auto rva=n->u1.AddressOfData;
   if(rva+2>=size||!memchr(base+rva+2,0,size-rva-2))return false;
   if(strcmp(reinterpret_cast<char*>(base+rva+2),"CreateFileW"))continue;
   if(a->u1.Function!=reinterpret_cast<ULONGLONG>(CreateFileW))return false;
   DWORD protection;
   if(!VirtualProtect(&a->u1.Function,8,PAGE_READWRITE,&protection))return false;
   original=CreateFileW;auto previous=InterlockedCompareExchangePointer(reinterpret_cast<PVOID volatile*>(&a->u1.Function),reinterpret_cast<PVOID>(open),reinterpret_cast<PVOID>(original));
   DWORD unused;VirtualProtect(&a->u1.Function,8,protection,&unused);return attached=previous==reinterpret_cast<PVOID>(original);
  }
 }return false;
}
}
