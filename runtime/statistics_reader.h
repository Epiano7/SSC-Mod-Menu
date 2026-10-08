#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <thread>
#include "statistics_core.h"
namespace ssc_stats {
inline std::string digest(const std::string& bytes){
    BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;unsigned char result[32];
    if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("Unable to hash recordings");
    bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;
    ok=ok&&BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())),ULONG(bytes.size()),0)>=0;
    ok=ok&&BCryptFinishHash(hash,result,32,0)>=0;if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
    if(!ok)throw std::runtime_error("Unable to hash recordings");
    std::string out;const char* hex="0123456789abcdef";for(auto c:result){out+=hex[c>>4];out+=hex[c&15];}return out;
}
// Legacy recorder filenames use UTC, not the filesystem modified timestamp.
inline double filename_date(const std::filesystem::path& path){
    unsigned y,m,d,h,n,s;auto name=path.filename().string();if(sscanf(name.c_str(),"session-%4u%2u%2u-%2u%2u%2u-",&y,&m,&d,&h,&n,&s)!=6)return 0;
    if(y<2000||y>2100||m<1||m>12||d<1||d>31||h>23||n>59||s>59)return 0;
    SYSTEMTIME st{};st.wYear=WORD(y);st.wMonth=WORD(m);st.wDay=WORD(d);st.wHour=WORD(h);st.wMinute=WORD(n);st.wSecond=WORD(s);FILETIME ft{};if(!SystemTimeToFileTime(&st,&ft))return 0;
    ULARGE_INTEGER v{};v.LowPart=ft.dwLowDateTime;v.HighPart=ft.dwHighDateTime;return double((v.QuadPart-116444736000000000ULL)/10000);
}
inline std::wstring local_date(double utc){
    if(utc<=0)return L"Date unavailable";
    ULARGE_INTEGER v{};v.QuadPart=uint64_t(utc)*10000+116444736000000000ULL;FILETIME ft{v.LowPart,v.HighPart};SYSTEMTIME st{},local{};
    if(!FileTimeToSystemTime(&ft,&st)||!SystemTimeToTzSpecificLocalTime(nullptr,&st,&local))return L"Date unavailable";
    wchar_t date[128]{},time[128]{};if(!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT,DATE_SHORTDATE,&local,nullptr,date,128,nullptr)||!GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT,TIME_NOSECONDS,&local,nullptr,time,128))return L"Date unavailable";
    return std::wstring(date)+L" "+time;
}
inline Report read_folder(const std::filesystem::path& folder){
    Report result;if(!std::filesystem::exists(folder))return result;
    std::vector<std::filesystem::path> paths;for(const auto& f:std::filesystem::directory_iterator(folder))if(f.is_regular_file()&&f.path().extension()==L".jsonl"){paths.push_back(f.path());if(paths.size()>10000)throw std::runtime_error("Too many recording files");}
    std::sort(paths.begin(),paths.end());std::set<std::string> seen;uintmax_t total=0;size_t samples=0;
    for(const auto& path:paths){
        auto size=std::filesystem::file_size(path);total+=size;if(total>100*1024*1024||size>10*1024*1024)throw std::runtime_error("Recording budget exceeded; move older files to an archive");
        std::ifstream stream(path,std::ios::binary);if(!stream){++result.warnings;continue;}
        std::string raw(size,'\0');stream.read(raw.data(),std::streamsize(size));raw.resize(size_t(stream.gcount()));
        if(!seen.insert(digest(raw)).second)continue;
        ++result.files;size_t before=result.rounds.size();parse_recording(raw,result);
        for(size_t i=before;i<result.rounds.size();++i){auto& round=result.rounds[i];samples+=round.points.size();if(!round.started_utc_ms)round.started_utc_ms=filename_date(path);}
        if(samples>200000||result.rounds.size()>10000)throw std::runtime_error("Too many samples; select a smaller recording archive");
    }return result;
}
struct Reader {std::atomic<bool> busy{false};std::atomic<unsigned> revision{0};std::shared_ptr<const Report> result=std::make_shared<Report>();};
inline Reader& reader(){static auto* value=new Reader;return *value;}
inline std::shared_ptr<const Report> snapshot(){return std::atomic_load(&reader().result);}
inline void refresh(const std::filesystem::path& folder){
    auto& state=reader();if(state.busy.exchange(true))return;
    HMODULE module=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&reader),&module)){state.busy=false;return;}
    try{std::thread([folder,&state]{
        auto report=std::make_shared<Report>();try{*report=read_folder(folder);}catch(const std::exception&){report->rounds.clear();report->error="Could not read this archive. Check file access and the 100 MiB limit.";}catch(...){report->error="Could not read recordings";}
        std::atomic_store(&state.result,std::shared_ptr<const Report>(report));state.busy=false;++state.revision;
    }).detach();}catch(...){state.busy=false;}
}
}
