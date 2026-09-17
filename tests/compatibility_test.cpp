#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include "../runtime/compatibility.h"
static void report(const char* line){std::cout<<line<<"\n";}
int main(int argc,char** argv){
    using namespace ssc_compat;
    std::vector<unsigned char> image(1024,0xcc);std::vector<Section> sections={{32,400,true},{512,400,false}};
    const unsigned char bytes[]={0x10,0x21,0x32,0x43,0x54,0x65,0x76,0x87};std::memcpy(image.data()+64,bytes,8);
    FunctionSpec f{64,1,8,0,"10 21 32 43",0,{}};f.hash=fingerprint(image.data()+64,f);
    Binding b{64,64,0,0,0,1,true};
    auto check=[&](){return inspect(image.data(),image.size(),sections,nullptr,&f,1,&b,1);};
    assert(check()==7&&resolve(64)==64);
    // Pure relocation, including an unrelated change, leaves the dependency valid.
    std::memcpy(image.data()+128,bytes,8);std::memset(image.data()+64,0xcc,8);image[600]=42;
    assert(check()==7&&resolve(64)==128);
    // Changing bytes outside the short locator still rejects the function body.
    image[135]^=1;assert(check()==6);image[135]^=1;
    std::memcpy(image.data()+192,bytes,8);assert(check()==6);std::memset(image.data()+192,0xcc,8);
    std::memset(image.data()+128,0xcc,8);assert(check()==6);
    // RIP-relative targets must be in the expected data section.
    std::memcpy(image.data()+64,bytes,8);f.ignored={4,5,6,7};int32_t delta=512-(64+8);std::memcpy(image.data()+68,&delta,4);f.hash=fingerprint(image.data()+64,f);
    Binding data{512,64,0,4,8,1,false};b=data;assert(check()==7&&resolve(512)==512);
    delta=-1000;std::memcpy(image.data()+68,&delta,4);assert(check()==6);
    std::cout<<"Compatibility tests passed: relocation, independent groups, changed body, ambiguous/missing signature, target bounds\n";
    if(argc>1){
        std::ifstream file(argv[1],std::ios::binary|std::ios::ate);assert(file);auto size=file.tellg();assert(size>4096);std::vector<unsigned char> disk(static_cast<size_t>(size));file.seekg(0);file.read(reinterpret_cast<char*>(disk.data()),size);assert(file);
        auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(disk.data());assert(dos->e_magic==IMAGE_DOS_SIGNATURE);
        auto nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(disk.data()+dos->e_lfanew);std::vector<unsigned char> mapped(nt->OptionalHeader.SizeOfImage);std::memcpy(mapped.data(),disk.data(),nt->OptionalHeader.SizeOfHeaders);std::vector<Section> ranges;auto sec=IMAGE_FIRST_SECTION(nt);
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){assert(sec[i].PointerToRawData+sec[i].SizeOfRawData<=disk.size());std::memcpy(mapped.data()+sec[i].VirtualAddress,disk.data()+sec[i].PointerToRawData,sec[i].SizeOfRawData);ranges.push_back({sec[i].VirtualAddress,sec[i].Misc.VirtualSize,(sec[i].Characteristics&IMAGE_SCN_MEM_EXECUTE)!=0});}
        auto now=GetTickCount64();auto result=inspect(mapped.data(),mapped.size(),ranges,report,function_specs,std::size(function_specs),binding_specs,std::size(binding_specs));
        std::cout<<"Native compatibility groups="<<result<<" duration="<<GetTickCount64()-now<<"ms\n";assert(result==7);
        for(const auto& binding:binding_specs)assert(resolve(binding.key)==binding.key);
    }
}
