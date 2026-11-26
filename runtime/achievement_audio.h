#pragma once
#include "sound_module.h"

namespace ssc_achievements::audio {
inline constexpr wchar_t default_name[]=L"itemUnlockedCampaign.wav";
inline std::filesystem::path default_sound,custom_sound;
inline bool custom=false;
inline std::wstring message;

inline void load(const std::filesystem::path& state){
    custom_sound=state/L"achievement-sound.wav";
    std::error_code error;
    custom=std::filesystem::is_regular_file(custom_sound,error);
    message.clear();
}
inline void prepare_default(const std::filesystem::path& source,const std::filesystem::path& state){
    default_sound=source;
    if(state.empty())return;
    constexpr int volume_percent=65;
    const auto cached=state/L"achievement-campaign-default.wav";
    auto temp=cached;
    temp+=L".tmp";
    try{
        auto wave=ssc_sound::gained(ssc_audio::read(source),volume_percent);
        std::filesystem::create_directories(state);
        ssc_audio::write(temp,wave);
        if(!MoveFileExW(temp.c_str(),cached.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Could not save the quiet unlock sound");
        default_sound=cached;
    }catch(...){
        std::error_code error;
        std::filesystem::remove(temp,error);
    }
}
inline bool import_file(const std::filesystem::path& selected){
    if(custom_sound.empty())return false;
    auto temp=custom_sound;
    temp+=L".tmp";
    try{
        const ssc_audio::Wave format{1,44100,16,{}};
        auto wave=ssc_audio::read_import(selected,format);
        std::filesystem::create_directories(custom_sound.parent_path());
        ssc_audio::write(temp,wave);
        if(!MoveFileExW(temp.c_str(),custom_sound.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Could not save the selected sound");
        custom=true;
        message=L"Custom sound saved";
        return true;
    }catch(...){
        std::error_code error;
        std::filesystem::remove(temp,error);
        message=L"Could not import that WAV or MP3 file";
        return false;
    }
}
inline void restore(){
    if(!custom)return;
    std::error_code error;
    std::filesystem::remove(custom_sound,error);
    if(error){message=L"Could not restore the game sound";return;}
    custom=false;
    message.clear();
}
inline void play_unlock(){
    const auto& path=custom?custom_sound:default_sound;
    if(!path.empty())if(auto play=ssc_sound::preview_api()){
        constexpr DWORD async_file=0x00020003;
        constexpr DWORD keep_current=0x00000010;
        play(path.c_str(),nullptr,async_file|keep_current);
    }
}
inline void preview(){
    const auto& path=custom?custom_sound:default_sound;
    message=!path.empty()&&ssc_sound::play_preview(path)?L"":L"Could not play the sound";
}
}
