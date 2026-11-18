#pragma once
#include "piper_assets.h"
#include <mmsystem.h>
#include <wincrypt.h>
namespace ssc_piper {
struct Audio {unsigned sample_rate=0;std::vector<unsigned char> pcm;};
class Speech {
 PipeProcess child_;std::string voice_;uint64_t next_=0;
 public:
 void reset(){child_.reset();voice_.clear();}
 bool active()const{return child_.active();}
 bool load(const std::string& id,const std::function<bool()>& cancel,std::string& error){
  if(voice_==id&&child_.active())return true;
  reset();if(!identifier(id)){error="Choose a downloaded Piper voice";return false;}
  const auto runtime=assets().runtime();if(!verify_runtime(runtime)){error="Download or repair the optional speech engine first";return false;}if(cancel())return false;
  std::vector<std::wstring> args={L"-I",L"-S",L"-B",(runtime/L"piper_worker.py").wstring(),L"serve",L"--root",runtime.wstring(),L"--models",assets().models().wstring(),L"--voice",wide(id)};
  if(!child_.start((runtime/L"python/python.exe").wstring(),args,runtime.wstring())){error="Could not start the local Piper worker";return false;}
  std::string line;if(!child_.line(line,8192,30000,cancel)){reset();error=cancel()?"Stopped":"Piper did not load in time";return false;}
  try{auto reply=Json::parse(line);if(reply.value("type",std::string{})!="ready"){error=reply.value("message",std::string("Piper model failed to load"));reset();return false;}}catch(...){reset();error="Invalid Piper response";return false;}
  voice_=id;return true;
 }
 bool synthesize(const std::string& id,const std::string& text,int rate,int speaker,Audio& audio,const std::function<bool()>& cancel,std::string& error){
  if(text.empty()||text.size()>1536||MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0)<=0){error="Unsupported chat text";return false;}
  if(!load(id,cancel,error))return false;
  const auto serial=++next_;Json request={{"id",serial},{"text",text},{"rate",std::clamp(rate,-5,5)},{"speaker",speaker}};
  if(!child_.send(request.dump())){reset();error="Local speech request failed";return false;}
  std::string line;if(!child_.line(line,3*1024*1024,15000,cancel)){reset();error=cancel()?"Stopped":"Local speech timed out";return false;}
  try{auto response=Json::parse(line);if(response.value("type",std::string{})!="audio"||response.at("id")!=serial){error=response.value("message",std::string("Invalid speech response"));reset();return false;}
   audio.sample_rate=response.at("sample_rate").get<unsigned>();auto encoded=response.at("pcm").get<std::string>();DWORD size=0;
   if(audio.sample_rate<8000||audio.sample_rate>48000||encoded.size()>3*1024*1024||!CryptStringToBinaryA(encoded.c_str(),DWORD(encoded.size()),CRYPT_STRING_BASE64,nullptr,&size,nullptr,nullptr)||!size||size>2*1024*1024||(size&1))throw std::runtime_error("Invalid audio");
   audio.pcm.resize(size);if(!CryptStringToBinaryA(encoded.c_str(),DWORD(encoded.size()),CRYPT_STRING_BASE64,audio.pcm.data(),&size,nullptr,nullptr))throw std::runtime_error("Invalid audio");return !cancel();
  }catch(...){reset();error="Invalid Piper audio";return false;}
 }
};
inline void apply_gain(Audio& audio,int volume){
 for(size_t i=0;i+1<audio.pcm.size();i+=2){int16_t sample=0;std::memcpy(&sample,audio.pcm.data()+i,2);sample=int16_t(std::clamp(int(sample)*std::clamp(volume,0,200)/100,-32768,32767));std::memcpy(audio.pcm.data()+i,&sample,2);}
}
inline bool play(Audio& audio,int volume,const std::function<bool()>& cancel){
 if(audio.pcm.empty()||audio.pcm.size()>2*1024*1024||cancel())return false;
 apply_gain(audio,volume);
 WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=1;format.nSamplesPerSec=audio.sample_rate;format.wBitsPerSample=16;format.nBlockAlign=2;format.nAvgBytesPerSec=audio.sample_rate*2;HWAVEOUT device=nullptr;
 if(waveOutOpen(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR)return false;
 WAVEHDR header{};header.lpData=reinterpret_cast<LPSTR>(audio.pcm.data());header.dwBufferLength=DWORD(audio.pcm.size());bool ok=waveOutPrepareHeader(device,&header,sizeof(header))==MMSYSERR_NOERROR;
 if(ok)ok=waveOutWrite(device,&header,sizeof(header))==MMSYSERR_NOERROR;
 const auto start=GetTickCount64();while(ok&&!(header.dwFlags&WHDR_DONE)){if(cancel()||GetTickCount64()-start>60000){ok=false;break;}Sleep(10);}waveOutReset(device);if(header.dwFlags&WHDR_PREPARED)waveOutUnprepareHeader(device,&header,sizeof(header));waveOutClose(device);return ok;
}
}
