#pragma once
#include <windows.h>
#include <algorithm>
#include <memory>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include "recording_limits.h"
#include "third_party/miniz/implementation.h"
namespace ssc_record_archive {
inline std::recursive_mutex& mutex(){static std::recursive_mutex value;return value;}
inline void require(bool ok){if(!ok)throw std::runtime_error("Recording archive operation failed");}
inline std::string read(const std::filesystem::path& path,uintmax_t limit){
    auto size=std::filesystem::file_size(path);require(size<=limit);
    std::ifstream f(path,std::ios::binary);require(bool(f));std::string data(size,'\0');
    f.read(data.data(),std::streamsize(size));require(size_t(f.gcount())==size);return data;
}
struct Zip {
    mz_zip_archive zip{};std::string data;bool opened=false,writer=false;
    explicit Zip(const std::filesystem::path& path){if(std::filesystem::exists(path)){data=read(path,ssc_record::archive_budget_bytes);require(mz_zip_reader_init_mem(&zip,data.data(),data.size(),0));opened=true;}}
    Zip(){writer=true;require(mz_zip_writer_init_heap(&zip,0,0));opened=true;}
    Zip(const Zip&)=delete;Zip& operator=(const Zip&)=delete;
    ~Zip(){if(opened){if(writer)mz_zip_writer_end(&zip);else mz_zip_reader_end(&zip);}}
    unsigned count(){return opened?mz_zip_reader_get_num_files(&zip):0;}
    std::string entry(unsigned i,std::string& name){
        mz_zip_archive_file_stat st{};require(mz_zip_reader_file_stat(&zip,i,&st));
        name=st.m_filename;require(!name.empty()&&name.size()<250&&name.find_first_of("/\\:")==std::string::npos&&std::filesystem::path(name).extension()==L".jsonl");
        require(st.m_uncomp_size<=ssc_record::file_budget_bytes&&!st.m_is_directory&&!st.m_is_encrypted);
        std::string raw(size_t(st.m_uncomp_size),'\0');require(mz_zip_reader_extract_to_mem(&zip,i,raw.data(),raw.size(),0));return raw;
    }
};
template<class Callback> inline void visit(const std::filesystem::path& folder,Callback callback){
    std::lock_guard<std::recursive_mutex> lock(mutex());if(!std::filesystem::exists(folder))return;
    Zip archive(folder/L"recordings.zip");require(archive.count()<=10000);uintmax_t expanded=0;unsigned count=0;
    auto emit=[&](const std::string& name,const std::string& raw){expanded+=raw.size();require(expanded<=ssc_record::archive_budget_bytes&&++count<=10000);callback(std::filesystem::path(name),raw);};
    for(unsigned i=0;i<archive.count();++i){std::string name;auto raw=archive.entry(i,name);emit(name,raw);}
    std::vector<std::filesystem::path> loose;for(const auto& e:std::filesystem::directory_iterator(folder))if(e.is_regular_file()&&e.path().extension()==L".jsonl")loose.push_back(e.path());
    std::sort(loose.begin(),loose.end());for(const auto& p:loose)emit(p.filename().string(),read(p,ssc_record::file_budget_bytes));
}
// Call only with this process's match writer closed. Never edits the old ZIP
// in place. A crash before removal leaves duplicates, deduplicated by Statistics.
inline void compact(const std::filesystem::path& folder){
    std::lock_guard<std::recursive_mutex> lock(mutex());
    std::vector<std::filesystem::path> loose;for(const auto& e:std::filesystem::directory_iterator(folder))if(e.is_regular_file()&&e.path().extension()==L".jsonl")loose.push_back(e.path());
    if(loose.empty())return;
    auto target=folder/L"recordings.zip";auto temp=folder/(L"recordings-"+std::to_wstring(GetCurrentProcessId())+L".zip.tmp");
    Zip old(target),out;std::set<std::string> names;uintmax_t expanded=0;
    require(old.count()+loose.size()<=10000);
    for(unsigned i=0;i<old.count();++i){std::string name;auto raw=old.entry(i,name);expanded+=raw.size();require(expanded<=ssc_record::archive_budget_bytes&&names.insert(name).second);require(mz_zip_writer_add_from_zip_reader(&out.zip,&old.zip,i));}
    for(const auto& p:loose){auto name=p.filename().string();auto raw=read(p,ssc_record::file_budget_bytes);
        if(!names.insert(name).second){int index=mz_zip_reader_locate_file(&old.zip,name.c_str(),nullptr,0);require(index>=0);std::string prior;require(old.entry(unsigned(index),prior)==raw);continue;}
        expanded+=raw.size();require(expanded<=ssc_record::archive_budget_bytes);require(mz_zip_writer_add_mem(&out.zip,name.c_str(),raw.data(),raw.size(),6));
    }
    void* bytes=nullptr;size_t size=0;require(mz_zip_writer_finalize_heap_archive(&out.zip,&bytes,&size));
    std::unique_ptr<void,decltype(&mz_free)> allocation(bytes,&mz_free);require(size<=ssc_record::archive_budget_bytes);
    HANDLE h=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);require(h!=INVALID_HANDLE_VALUE);
    DWORD written=0;bool ok=WriteFile(h,bytes,DWORD(size),&written,nullptr)&&written==size&&FlushFileBuffers(h);CloseHandle(h);require(ok);
    {Zip verify(temp);require(verify.count()==names.size());for(unsigned i=0;i<verify.count();++i){std::string name;verify.entry(i,name);}
     for(const auto& p:loose){int index=mz_zip_reader_locate_file(&verify.zip,p.filename().string().c_str(),nullptr,0);require(index>=0);std::string name;require(verify.entry(unsigned(index),name)==read(p,ssc_record::file_budget_bytes));}}
    require(MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH));
    // The replacement is durable and every original was compared byte-for-byte.
    for(const auto& p:loose)std::filesystem::remove(p);
}
inline void try_compact(const std::filesystem::path& folder) noexcept {try{compact(folder);}catch(...){OutputDebugStringA("SSC: recording compression failed; originals preserved\n");}}
}
