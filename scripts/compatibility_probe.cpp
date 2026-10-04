// Read-only offline probe. Never loads the target as executable code.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "../runtime/compatibility.h"
template<class T> T read_at(const std::vector<unsigned char>& bytes,size_t at){
    if(at>bytes.size()||sizeof(T)>bytes.size()-at)throw std::runtime_error("truncated PE");
    T value;std::memcpy(&value,bytes.data()+at,sizeof value);return value;
}
static void detail(const char* line){std::cerr<<line<<'\n';}
int main(int argc,char** argv){
    try {
        if(argc!=2)throw std::runtime_error("expected snapshot executable path");
        std::ifstream file(argv[1],std::ios::binary|std::ios::ate);
        if(!file)throw std::runtime_error("cannot read snapshot");
        auto length=file.tellg();
        if(length<4096||length>268435456)throw std::runtime_error("invalid executable size");
        std::vector<unsigned char> disk(static_cast<size_t>(length));file.seekg(0);
        if(!file.read(reinterpret_cast<char*>(disk.data()),length))throw std::runtime_error("short read");
        auto dos=read_at<IMAGE_DOS_HEADER>(disk,0);
        if(dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<0)throw std::runtime_error("invalid DOS header");
        size_t at=static_cast<size_t>(dos.e_lfanew);
        auto nt=read_at<IMAGE_NT_HEADERS64>(disk,at);
        if(nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
           nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||
           nt.FileHeader.SizeOfOptionalHeader!=sizeof(IMAGE_OPTIONAL_HEADER64)||
           !nt.FileHeader.NumberOfSections||nt.FileHeader.NumberOfSections>96||
           nt.OptionalHeader.SizeOfImage>536870912||!nt.OptionalHeader.SizeOfImage||
           nt.OptionalHeader.SizeOfHeaders>disk.size()||nt.OptionalHeader.SizeOfHeaders>nt.OptionalHeader.SizeOfImage)
            throw std::runtime_error("unsupported PE layout");
        std::vector<unsigned char> mapped(nt.OptionalHeader.SizeOfImage);
        std::memcpy(mapped.data(),disk.data(),nt.OptionalHeader.SizeOfHeaders);
        std::vector<ssc_compat::Section> sections;
        for(unsigned i=0;i<nt.FileHeader.NumberOfSections;++i){
            auto s=read_at<IMAGE_SECTION_HEADER>(disk,at+sizeof(IMAGE_NT_HEADERS64)+i*sizeof(IMAGE_SECTION_HEADER));
            size_t size=std::max(s.Misc.VirtualSize,s.SizeOfRawData);
            if(s.PointerToRawData>disk.size()||s.SizeOfRawData>disk.size()-s.PointerToRawData||
               s.VirtualAddress<nt.OptionalHeader.SizeOfHeaders||s.VirtualAddress>mapped.size()||size>mapped.size()-s.VirtualAddress)
                throw std::runtime_error("section out of bounds");
            for(const auto& prior:sections)if(size&&prior.size&&s.VirtualAddress<static_cast<uint64_t>(prior.begin)+prior.size&&
                prior.begin<static_cast<uint64_t>(s.VirtualAddress)+size)throw std::runtime_error("overlapping sections");
            std::memcpy(mapped.data()+s.VirtualAddress,disk.data()+s.PointerToRawData,s.SizeOfRawData);
            sections.push_back({s.VirtualAddress,static_cast<uint32_t>(size),(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)!=0});
        }
        using namespace ssc_compat;
        unsigned required=7;for(const auto& f:function_specs)required|=f.groups;
        auto result=inspect(mapped.data(),mapped.size(),sections,detail,function_specs,std::size(function_specs),binding_specs,std::size(binding_specs));
        std::cout<<"{\"schema\":1,\"required_groups\":"<<required<<",\"available_groups\":"<<result
                 <<",\"status\":\""<<(result==required?"compatible":"needs_review")<<"\"}\n";
        return result==required?0:2;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 3;}
}
