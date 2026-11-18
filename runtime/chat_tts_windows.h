#pragma once
#include <windows.h>
#include <sapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <cmath>
namespace ssc_chat_tts {
// Invoke off the game/render thread. This uses installed Windows voices only;
// no voice packages, web requests, or legacy voice redistribution is involved.
struct InstalledVoice {std::wstring id,label;};
struct VoiceInventory {HRESULT result=E_FAIL;std::vector<InstalledVoice> voices;};
inline VoiceInventory installed_windows_voices(){
 VoiceInventory out;const HRESULT init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(FAILED(init)){out.result=init;return out;}
 ISpObjectTokenCategory* category=nullptr;IEnumSpObjectTokens* tokens=nullptr;
 out.result=CoCreateInstance(CLSID_SpObjectTokenCategory,nullptr,CLSCTX_INPROC_SERVER,IID_ISpObjectTokenCategory,reinterpret_cast<void**>(&category));
 if(SUCCEEDED(out.result))out.result=category->SetId(SPCAT_VOICES,FALSE);
 if(SUCCEEDED(out.result))out.result=category->EnumTokens(nullptr,nullptr,&tokens);
 if(SUCCEEDED(out.result)){
  ULONG count=0;out.result=tokens->GetCount(&count);
  for(ULONG i=0;SUCCEEDED(out.result)&&i<count&&i<64;++i){
   ISpObjectToken* token=nullptr;if(FAILED(tokens->Item(i,&token))||!token)continue;
   wchar_t* id=nullptr;wchar_t* label=nullptr;
   if(SUCCEEDED(token->GetId(&id))&&id&&SUCCEEDED(token->GetStringValue(nullptr,&label))&&label&&wcslen(id)<=1024&&wcslen(label)<=128)out.voices.push_back({id,label});
   CoTaskMemFree(id);CoTaskMemFree(label);token->Release();token=nullptr;
  }
 }
 if(tokens)tokens->Release();
 if(category)category->Release();
 std::stable_sort(out.voices.begin(),out.voices.end(),[](const auto& a,const auto& b){return (a.label.find(L"David")!=std::wstring::npos)>(b.label.find(L"David")!=std::wstring::npos);});
 CoUninitialize();return out;
}
// Percent controls retain the natural timbre: Windows/Piper use their nearest
// supported synthesis rate rather than changing pitch through playback rate.
inline int synthesis_rate(int percent){return std::clamp(int(std::lround(6*std::log2(std::clamp(percent,50,200)/100.))),-5,5);}
inline int legacy_speed_percent(int rate){return std::clamp(int(std::lround(100*std::pow(2.,std::clamp(rate,-5,5)/6.))),50,200);}
inline HRESULT windows_pcm(ISpVoice* voice,const std::wstring& text,int rate,std::vector<unsigned char>& pcm,const std::function<bool()>& cancel){
 pcm.clear();if(!voice||text.empty()||text.size()>512||cancel())return E_INVALIDARG;
 IStream* memory=nullptr;ISpStream* stream=nullptr;HRESULT hr=CreateStreamOnHGlobal(nullptr,TRUE,&memory);
 if(SUCCEEDED(hr))hr=CoCreateInstance(CLSID_SpStream,nullptr,CLSCTX_INPROC_SERVER,IID_ISpStream,reinterpret_cast<void**>(&stream));
 WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=1;format.nSamplesPerSec=22050;format.wBitsPerSample=16;format.nBlockAlign=2;format.nAvgBytesPerSec=44100;
 static const GUID wave_format={0xc31adbae,0x527f,0x4ff5,{0xa2,0x30,0xf6,0x2b,0xb6,0x1f,0xf7,0x0c}};
 if(SUCCEEDED(hr))hr=stream->SetBaseStream(memory,wave_format,&format);
 if(SUCCEEDED(hr))hr=voice->SetOutput(stream,TRUE);
 if(SUCCEEDED(hr)){voice->SetVolume(100);voice->SetRate(std::clamp(rate,-10,10));hr=voice->Speak(text.c_str(),SPF_ASYNC|SPF_IS_NOT_XML,nullptr);}
 const auto started=GetTickCount64();
 while(SUCCEEDED(hr)){auto waited=voice->WaitUntilDone(25);if(waited!=S_FALSE){hr=waited;break;}STATSTG stat{};if(cancel()||GetTickCount64()-started>15000||FAILED(memory->Stat(&stat,STATFLAG_NONAME))||stat.cbSize.QuadPart>2*1024*1024){hr=E_ABORT;break;}}
 voice->Speak(nullptr,SPF_ASYNC|SPF_PURGEBEFORESPEAK,nullptr);voice->SetOutput(nullptr,FALSE);
 if(SUCCEEDED(hr)&&!cancel()){STATSTG stat{};LARGE_INTEGER zero{};if(FAILED(memory->Stat(&stat,STATFLAG_NONAME))||!stat.cbSize.QuadPart||stat.cbSize.QuadPart>2*1024*1024||(stat.cbSize.QuadPart&1))hr=E_FAIL;else{pcm.resize(size_t(stat.cbSize.QuadPart));ULONG got=0;hr=memory->Seek(zero,STREAM_SEEK_SET,nullptr);if(SUCCEEDED(hr))hr=memory->Read(pcm.data(),ULONG(pcm.size()),&got);if(got!=pcm.size())hr=E_FAIL;}}else hr=E_ABORT;
 if(stream)stream->Release();
 if(memory)memory->Release();
 if(FAILED(hr))pcm.clear();
 return hr;
}
}