#pragma once
#include "audio_pcm.h"
#include <map>
#include <atomic>
#include <cstring>
#include <commdlg.h>
namespace ssc_sound {
using BufferData=void(__cdecl*)(unsigned,int,const void*,int,int);
inline BufferData original=nullptr;
struct Entry {std::filesystem::path path;std::wstring label;std::string key;unsigned rate=0,channels=0,bits=0;bool imported=false;};
inline std::vector<Entry> entries;
inline std::map<std::string,ssc_audio::Wave> replacements;
inline std::filesystem::path folder;
inline std::wstring status=L"No sound selected";
inline std::atomic<unsigned> substituted{0},recognized{0};
inline bool active=false,attached=false;
inline size_t selection=0;
inline std::string fingerprint(const void* data,size_t size,int format,int rate){
    unsigned char digest[32];BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
    if(size>ssc_audio::limit||BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
    bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok)ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(&format),sizeof(format),0)>=0&&BCryptHashData(hash,reinterpret_cast<PUCHAR>(&rate),sizeof(rate),0)>=0&&BCryptHashData(hash,(PUCHAR)data,static_cast<ULONG>(size),0)>=0&&BCryptFinishHash(hash,digest,32,0)>=0;
    if(hash)BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm,0);if(!ok)return {};
    std::string result;for(auto c:digest){result+="0123456789abcdef"[c>>4];result+="0123456789abcdef"[c&15];}return result;
}
inline int format(const ssc_audio::Wave& w){return w.channels==1?(w.bits==8?0x1100:0x1101):(w.bits==8?0x1102:0x1103);}
inline std::wstring readable(const std::filesystem::path& path){
    auto input=path.stem().wstring();std::wstring result;
    for(size_t i=0;i<input.size();++i){auto c=input[i];if(i&&c>=L'A'&&c<=L'Z'&&input[i-1]>=L'a'&&input[i-1]<=L'z')result+=L' ';result+=c==L'_'?L' ':c;}
    if(!result.empty())result[0]=towupper(result[0]);
    return result;
}
inline void initialize(const std::filesystem::path& game,const std::filesystem::path& state,bool enabled){
    active=enabled;folder=state/L"sounds";entries.clear();replacements.clear();std::map<std::string,unsigned> counts;
    std::error_code ec;
    for(const auto& file:std::filesystem::directory_iterator(game/L"data/sounds",ec)){
        if(file.path().extension()!=L".wav")continue;
        try{auto wave=ssc_audio::read(file.path());if(wave.bits!=8&&wave.bits!=16)continue;
            auto key=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(key.empty())continue;
            Entry entry{file.path(),readable(file.path()),key,wave.rate,wave.channels,wave.bits,false};
            entry.imported=std::filesystem::is_regular_file(folder/(key+".wav"),ec);entries.push_back(entry);++counts[key];
        }catch(const std::exception&){}
    }
    std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.label<b.label;});
    for(auto& e:entries){if(counts[e.key]!=1){e.key.clear();continue;}if(!active||!e.imported)continue;
        try{auto replacement=ssc_audio::read(folder/(e.key+".wav"));if(replacement.rate==e.rate&&replacement.channels==e.channels&&replacement.bits==16)replacements.emplace(e.key,std::move(replacement));}catch(const std::exception&){}
    }
    status=L"Changes apply after restarting the game";
}
inline void __cdecl buffer_data(unsigned buffer,int fmt,const void* bytes,int length,int rate){
    try{if(active&&bytes&&length>0&&size_t(length)<=ssc_audio::limit&&fmt>=0x1100&&fmt<=0x1103){
        auto key=fingerprint(bytes,length,fmt,rate);auto found=replacements.find(key);
        if(found!=replacements.end()){const auto& w=found->second;original(buffer,format(w),w.raw.data(),static_cast<int>(w.raw.size()),w.rate);++substituted;return;}
    }}catch(const std::exception&){}
    original(buffer,fmt,bytes,length,rate);
}
inline void import_selected(HWND owner,bool match){
    if(selection>=entries.size())return;
    const auto& e=entries[selection];if(e.key.empty()){status=L"Duplicate original audio: replacement is ambiguous";return;}
    wchar_t selected[32768]={};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=owner;dialog.lpstrFilter=L"PCM WAV audio\0*.wav\0\0";dialog.lpstrFile=selected;dialog.nMaxFile=32768;dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;dialog.lpstrTitle=L"Import replacement audio (PCM WAV)";
    if(!GetOpenFileNameW(&dialog))return;
    try{
        auto reference=ssc_audio::read(e.path),source=ssc_audio::read(selected);auto prepared=ssc_audio::prepare(reference,source,match);
        std::filesystem::create_directories(folder);auto dest=folder/(e.key+".wav"),temp=folder/(e.key+".tmp");
        // Imports always target our private directory, never the original or selected source.
        if(std::filesystem::equivalent(e.path,selected))throw std::runtime_error("Choose your replacement, not the original game sound");
        ssc_audio::write(temp,prepared.wave);if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not commit imported sound");
        entries[selection].imported=true;status=prepared.limited?L"Imported; peak limited. Restart game to apply.":L"Imported. Restart game to apply.";
    }catch(const std::exception& ex){std::string message=ex.what();status.assign(message.begin(),message.end());}
}
inline void preview_original(){
    if(selection>=entries.size())return;
    using Play=BOOL(WINAPI*)(LPCWSTR,HMODULE,DWORD);
    static HMODULE winmm=LoadLibraryW(L"winmm.dll");
    Play play=nullptr;auto address=winmm?GetProcAddress(winmm,"PlaySoundW"):nullptr;static_assert(sizeof(play)==sizeof(address));std::memcpy(&play,&address,sizeof(play));
    // Asynchronous filename playback owns its data; always use the original path.
    if(play&&play(entries[selection].path.c_str(),nullptr,0x00020000|0x0001|0x0002))status=L"Playing original: "+entries[selection].label;
    else status=L"Could not preview original sound";
}
inline void remove_selected(){
    if(selection>=entries.size()||entries[selection].key.empty())return;
    std::error_code ec;
    std::filesystem::remove(folder/(entries[selection].key+".wav"),ec);
    if(ec){status=L"Could not remove imported sound";return;}entries[selection].imported=false;status=L"Original restored on next game restart";
}
}
