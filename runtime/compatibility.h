#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstdio>
namespace ssc_compat {
struct FunctionSpec {uint32_t key,groups,length,anchor;const char* pattern;uint64_t hash;std::vector<uint32_t> ignored;};
struct Binding {uint32_t key,function,offset,displacement,instruction_size,groups;bool code;};
#include "compatibility_data.h"
struct Section {uint32_t begin,size;bool executable;};
inline std::map<uint32_t,uint32_t> addresses;
inline unsigned available=0;
inline bool initialized=false;
inline uint32_t resolve(uint32_t key){
    // Unit fixtures use the validated baseline layout without initializing the resolver.
    if(!initialized)return key;
    auto it=addresses.find(key);return it==addresses.end()?0:it->second;
}
inline bool supports(unsigned group){return !initialized||(available&group)==group;}
inline bool contains(const std::vector<Section>& sections,uint32_t at,size_t size,bool executable){
    for(const auto& s:sections)if(s.executable==executable&&at>=s.begin&&at-s.begin<=s.size&&size<=s.size-(at-s.begin))return true;
    return false;
}
inline std::vector<int> parse(const char* text){
    std::vector<int> p;while(*text){if(*text==' '){++text;continue;}if(text[0]=='?'&&text[1]=='?'){p.push_back(-1);text+=2;}else{unsigned v=0;if(std::sscanf(text,"%2x",&v)!=1)return {};p.push_back(int(v));text+=2;}}return p;
}
inline bool matches(const unsigned char* data,const std::vector<int>& p){for(size_t i=0;i<p.size();++i)if(p[i]>=0&&data[i]!=p[i])return false;return true;}
inline uint64_t fingerprint(const unsigned char* bytes,const FunctionSpec& s){
    uint64_t h=14695981039346656037ULL;size_t skip=0;
    for(uint32_t i=0;i<s.length;++i){bool masked=skip<s.ignored.size()&&s.ignored[skip]==i;if(masked)++skip;h=(h^(masked?0:bytes[i]))*1099511628211ULL;}return h;
}
// Input is a mapped PE image. No patches are installed until resolution completes.
inline unsigned inspect(const unsigned char* image,size_t size,const std::vector<Section>& sections,void(*report)(const char*),const FunctionSpec* specs,size_t count,const Binding* bindings,size_t binding_count){
    initialized=true;available=7;addresses.clear();std::map<uint32_t,uint32_t> functions;
    auto fail=[&](unsigned groups,uint32_t key,const char* reason){available&=~groups;if(report){char line[180];std::snprintf(line,sizeof(line),"Compatibility groups=%u dependency=%08X: %s",groups,key,reason);report(line);}};
    for(size_t i=0;i<count;++i){const auto& f=specs[i];auto p=parse(f.pattern);if(p.empty()){fail(f.groups,f.key,"invalid locator");continue;}
        auto fixed=std::find_if(p.begin(),p.end(),[](int v){return v>=0;});if(fixed==p.end()){fail(f.groups,f.key,"empty locator");continue;}
        size_t first=size_t(fixed-p.begin());uint32_t match=0;unsigned hits=0;
        for(const auto& section:sections){if(!section.executable||section.begin>size||section.size>size-section.begin||section.size<p.size())continue;
            size_t end=section.begin+section.size-p.size()+1,pos=section.begin;
            while(pos<end){auto found=static_cast<const unsigned char*>(std::memchr(image+pos+first,*fixed,end-pos));if(!found)break;pos=size_t(found-image)-first;
                if(matches(image+pos,p)){++hits;if(pos>=f.anchor)match=uint32_t(pos-f.anchor);}++pos;if(hits>1)break;}
            if(hits>1)break;
        }
        if(hits!=1||!contains(sections,match,f.length,true)){fail(f.groups,f.key,hits>1?"ambiguous locator":"locator missing");continue;}
        if(fingerprint(image+match,f)!=f.hash){fail(f.groups,f.key,"function changed");continue;}
        functions[f.key]=match;
    }
    for(size_t i=0;i<binding_count;++i){const auto& b=bindings[i];auto f=functions.find(b.function);if(f==functions.end()){fail(b.groups,b.key,"dependency unavailable");continue;}
        uint64_t at=uint64_t(f->second)+b.offset;int64_t target=at;
        if(!b.code){if(at+b.displacement+4>size){fail(b.groups,b.key,"reference out of range");continue;}int32_t delta;std::memcpy(&delta,image+at+b.displacement,4);target=int64_t(at+b.instruction_size)+delta;}
        if(target<=0||uint64_t(target)>=size||!contains(sections,uint32_t(target),b.code?1:4,b.code)){fail(b.groups,b.key,"target outside expected section");continue;}
        auto found=addresses.find(b.key);if(found!=addresses.end()&&found->second!=target){fail(b.groups,b.key,"references disagree");continue;}addresses[b.key]=uint32_t(target);
    }
    return available;
}
inline unsigned initialize(void(*report)(const char*)){
    auto image=reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    initialized=true;available=0;addresses.clear();
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>4096)return 0;
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(image+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||nt->FileHeader.NumberOfSections>96)return 0;
    std::vector<Section> sections;auto s=IMAGE_FIRST_SECTION(nt);size_t size=nt->OptionalHeader.SizeOfImage;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(s[i].VirtualAddress>size||s[i].Misc.VirtualSize>size-s[i].VirtualAddress)return 0;sections.push_back({s[i].VirtualAddress,s[i].Misc.VirtualSize,(s[i].Characteristics&IMAGE_SCN_MEM_EXECUTE)!=0});}
    return inspect(image,size,sections,report,function_specs,std::size(function_specs),binding_specs,std::size(binding_specs));
}
}
