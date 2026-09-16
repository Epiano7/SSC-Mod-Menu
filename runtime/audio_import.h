#pragma once
#include "audio_pcm.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
namespace ssc_audio {
template<class T> struct ComOwner {
    T* value=nullptr;
    ~ComOwner(){if(value)value->Release();}
    T* operator->()const{return value;}
};
inline void media_check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("Could not decode audio with Windows Media Foundation");}
inline Wave read_mp3(const std::filesystem::path& path){
    if(!std::filesystem::is_regular_file(path)||std::filesystem::file_size(path)>limit)throw std::runtime_error("Invalid or oversized audio (64 MiB limit)");
    struct Platform {
        bool com=false,mf=false;
        ~Platform(){if(mf)MFShutdown();if(com)CoUninitialize();}
    } platform;
    HRESULT hr=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    platform.com=SUCCEEDED(hr);if(FAILED(hr)&&hr!=RPC_E_CHANGED_MODE)media_check(hr);
    media_check(MFStartup(MF_VERSION));platform.mf=true;
    ComOwner<IMFSourceReader> reader;
    media_check(MFCreateSourceReaderFromURL(path.c_str(),nullptr,&reader.value));
    media_check(reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS,FALSE));
    media_check(reader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM,TRUE));
    ComOwner<IMFMediaType> requested,actual;
    media_check(MFCreateMediaType(&requested.value));
    media_check(requested->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Audio));
    media_check(requested->SetGUID(MF_MT_SUBTYPE,MFAudioFormat_PCM));
    media_check(requested->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,16));
    media_check(reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,nullptr,requested.value));
    media_check(reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM,&actual.value));
    Wave wave;
    media_check(actual->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS,&wave.channels));
    media_check(actual->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND,&wave.rate));
    media_check(actual->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE,&wave.bits));
    if((wave.channels!=1&&wave.channels!=2)||wave.bits!=16||wave.rate<8000||wave.rate>192000)throw std::runtime_error("Unsupported decoded audio format");
    for(unsigned reads=0;;++reads){
        if(reads>100000)throw std::runtime_error("Audio decoder exceeded import limit");
        DWORD flags=0;ComOwner<IMFSample> sample;
        media_check(reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM,0,nullptr,&flags,nullptr,&sample.value));
        if(flags&(MF_SOURCE_READERF_ERROR|MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))throw std::runtime_error("Audio format changed during decoding");
        if(sample.value){
            ComOwner<IMFMediaBuffer> buffer;media_check(sample->ConvertToContiguousBuffer(&buffer.value));
            BYTE* bytes=nullptr;DWORD length=0;media_check(buffer->Lock(&bytes,nullptr,&length));
            struct Unlock {IMFMediaBuffer* buffer;~Unlock(){buffer->Unlock();}} unlock{buffer.value};
            if(length>limit-wave.raw.size())throw std::runtime_error("Decoded audio exceeds 64 MiB limit");
            wave.raw.insert(wave.raw.end(),bytes,bytes+length);
        }
        if(flags&MF_SOURCE_READERF_ENDOFSTREAM)break;
    }
    if(wave.raw.empty()||wave.raw.size()%(wave.channels*2))throw std::runtime_error("Incomplete decoded audio");
    return wave;
}
// Convert imports once, outside the game's audio callback. Stored overrides
// remain ordinary PCM WAV files at the original sound's rate/channel count.
inline Wave convert_import(const Wave& input,const Wave& reference){
    if(input.rate==reference.rate&&input.channels==reference.channels)return input;
    auto samples=decode(input);size_t frames=samples.size()/input.channels;
    size_t count=static_cast<size_t>(std::ceil(double(frames)*reference.rate/input.rate));
    if(!frames||count>limit/(reference.channels*2))throw std::runtime_error("Converted audio exceeds 64 MiB limit");
    Wave out{reference.channels,reference.rate,16,{}};out.raw.reserve(count*reference.channels*2);
    auto at=[&](size_t frame,unsigned channel){
        if(reference.channels==1&&input.channels==2)return (samples[frame*2]+samples[frame*2+1])*.5;
        return samples[frame*input.channels+(input.channels==1?0:channel)];
    };
    for(size_t i=0;i<count;++i){double pos=double(i)*input.rate/reference.rate;size_t a=std::min(size_t(pos),frames-1),b=std::min(a+1,frames-1);double t=pos-a;
        for(unsigned c=0;c<out.channels;++c){int v=int(std::round((at(a,c)*(1-t)+at(b,c)*t)*32768));v=std::clamp(v,-32768,32767);out.raw.push_back(v&255);out.raw.push_back((v>>8)&255);}
    }
    return out;
}
inline Wave read_import(const std::filesystem::path& path,const Wave& reference){
    auto extension=path.extension().wstring();std::transform(extension.begin(),extension.end(),extension.begin(),::towlower);
    if(extension!=L".wav"&&extension!=L".mp3")throw std::runtime_error("Choose a WAV or MP3 file");
    return convert_import(extension==L".mp3"?read_mp3(path):read(path),reference);
}
}
