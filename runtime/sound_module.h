#pragma once
#include "audio_import.h"
#include "third_party/json.hpp"
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
// Verified native random-selection family: sound IDs 6, 527 and 528.
inline std::string family(const Entry& e){auto name=e.path.filename().wstring();return name==L"killedHuman.wav"||name==L"killedHuman2.wav"||name==L"killedHuman3.wav"?"human-kill":"";}
inline std::map<std::string,int> volume;
inline std::map<std::string,std::string> families;
inline int clip_volume(const Entry& e){auto f=families.find(family(e));auto it=volume.find(f==families.end()?e.key:f->second);return it==volume.end()?100:it->second;}
inline bool all_variants(const Entry& e){auto f=family(e);auto it=families.find(f);return !f.empty()&&it!=families.end();}
inline std::string replacement_key(const Entry& e){auto it=families.find(family(e));return it==families.end()?e.key:it->second;}
inline bool has_replacement(const Entry& e){auto key=replacement_key(e);return std::any_of(entries.begin(),entries.end(),[&](const Entry& item){return item.key==key&&item.imported;});}
inline bool save_options(){try{std::filesystem::create_directories(folder);auto tmp=folder/L"options.json.tmp";std::ofstream out(tmp);out<<nlohmann::json{{"schema",1},{"volume",volume},{"families",families}}.dump(2);out.close();return out&&MoveFileExW(tmp.c_str(),(folder/L"options.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}catch(...){return false;}}
inline void load_options(){volume.clear();families.clear();try{auto file=folder/L"options.json";if(!std::filesystem::exists(file)||std::filesystem::file_size(file)>65536)return;std::ifstream input(file);nlohmann::json j;input>>j;if(j.at("schema")!=1)return;auto v=j.at("volume").get<std::map<std::string,int>>();auto f=j.at("families").get<std::map<std::string,std::string>>();for(const auto& e:entries){auto it=v.find(e.key);if(it!=v.end()&&it->second>=0&&it->second<=300)volume[e.key]=it->second;auto group=family(e);auto choice=f.find(group);if(!group.empty()&&choice!=f.end()&&choice->second==e.key&&e.imported)families[group]=e.key;}}catch(...) {}}
inline ssc_audio::Wave gained(ssc_audio::Wave wave,int percent){if(wave.bits!=16)throw std::runtime_error("Expected 16-bit replacement");for(size_t i=0;i<wave.raw.size();i+=2){int sample=int(int16_t(ssc_audio::u16(wave.raw.data()+i)));int v=std::clamp(int(std::lround(sample*std::clamp(percent,0,300)/100.0)),-32768,32767);wave.raw[i]=uint8_t(v);wave.raw[i+1]=uint8_t(v>>8);}return wave;}
inline ssc_audio::Wave effective_wave(const Entry& e){auto key=replacement_key(e);auto wave=ssc_audio::read(folder/(key+".wav"));ssc_audio::Wave reference{e.channels,e.rate,16,{}};wave=ssc_audio::convert_import(wave,reference);auto it=volume.find(key);return gained(std::move(wave),it==volume.end()?100:it->second);}
inline void set_volume(int percent){if(selection>=entries.size())return;volume[replacement_key(entries[selection])]=std::clamp(percent,0,300);status=save_options()?L"Volume saved; restart game to apply":L"Could not save sound options";}
inline void toggle_variants(){if(selection>=entries.size())return;auto& e=entries[selection];auto f=family(e);if(f.empty())return;if(all_variants(e))families.erase(f);else if(e.imported)families[f]=e.key;else return;status=save_options()?L"Variant choice saved; restart game to apply":L"Could not save sound options";}
inline void initialize(const std::filesystem::path& game,const std::filesystem::path& state,bool enabled){
    active=enabled;folder=state/L"sounds";entries.clear();replacements.clear();
    std::error_code ec;
    for(const auto& file:std::filesystem::directory_iterator(game/L"data/sounds",ec)){
        if(file.path().extension()!=L".wav")continue;
        try{auto wave=ssc_audio::read(file.path());if(wave.bits!=8&&wave.bits!=16)continue;
            auto key=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(key.empty())continue;
            Entry entry{file.path(),readable(file.path()),key,wave.rate,wave.channels,wave.bits,false};
            entry.imported=std::filesystem::is_regular_file(folder/(key+".wav"),ec);entries.push_back(entry);
        }catch(const std::exception&){}
    }
    std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.label<b.label;});
    load_options();
    for(auto& e:entries){if(!active||!has_replacement(e))continue;
        try{replacements.emplace(e.key,effective_wave(e));}catch(const std::exception&){}
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
    const auto& e=entries[selection];
    std::wstring shared;
    for(const auto& other:entries)if(other.key==e.key){if(!shared.empty())shared+=L", ";shared+=other.label;}
    size_t count=std::count_if(entries.begin(),entries.end(),[&](const Entry& other){return other.key==e.key;});
    if(count>1&&MessageBoxW(owner,(L"These sounds share identical audio: " + shared + L".\n\nImporting replaces all of them. Continue?").c_str(),L"Shared sound",MB_OKCANCEL|MB_ICONINFORMATION)!=IDOK)return;
    wchar_t selected[32768]={};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=owner;dialog.lpstrFilter=L"Audio files\0*.wav;*.mp3\0WAV audio\0*.wav\0MP3 audio\0*.mp3\0\0";dialog.lpstrFile=selected;dialog.nMaxFile=32768;dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;dialog.lpstrTitle=L"Import replacement audio";
    if(!GetOpenFileNameW(&dialog))return;
    try{
        auto reference=ssc_audio::read(e.path),source=ssc_audio::read_import(selected,reference);auto prepared=ssc_audio::prepare(reference,source,match);
        std::filesystem::create_directories(folder);auto dest=folder/(e.key+".wav"),temp=folder/(e.key+".tmp");
        // Imports always target our private directory, never the original or selected source.
        if(std::filesystem::equivalent(e.path,selected))throw std::runtime_error("Choose your replacement, not the original game sound");
        ssc_audio::write(temp,prepared.wave);if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not commit imported sound");
        for(auto& other:entries)if(other.key==e.key)other.imported=true;
        status=prepared.limited?L"Imported; peak limited. Restart game to apply.":L"Imported. Restart game to apply.";
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
inline void preview_custom(HWND owner){
    if(selection>=entries.size()||!has_replacement(entries[selection]))return;
    try{auto wave=effective_wave(entries[selection]);auto file=folder/L"preview.wav";ssc_audio::write(file,wave);using Play=BOOL(WINAPI*)(LPCWSTR,HMODULE,DWORD);static HMODULE lib=LoadLibraryW(L"winmm.dll");Play play=nullptr;auto proc=lib?GetProcAddress(lib,"PlaySoundW"):nullptr;std::memcpy(&play,&proc,sizeof(play));if(!play||!play(file.c_str(),nullptr,0x20003))throw std::runtime_error("Could not preview replacement");status=L"Playing replacement";}catch(...){status=L"Could not preview replacement";}(void)owner;
}
inline void remove_selected(){
    if(selection>=entries.size()||entries[selection].key.empty())return;
    std::error_code ec;
    std::filesystem::remove(folder/(entries[selection].key+".wav"),ec);
    if(ec){status=L"Could not remove imported sound";return;}for(auto& other:entries)if(other.key==entries[selection].key)other.imported=false;auto group=family(entries[selection]);if(all_variants(entries[selection]))families.erase(group);volume.erase(entries[selection].key);save_options();status=L"Original restored on next game restart";
}
}
