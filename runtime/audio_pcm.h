#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <stdexcept>
namespace ssc_audio {
constexpr size_t limit=64*1024*1024;
struct Wave {unsigned channels=0,rate=0,bits=0;std::vector<unsigned char> raw;};
inline unsigned u16(const unsigned char* p){return p[0]|unsigned(p[1])<<8;}
inline uint32_t u32(const unsigned char* p){return u16(p)|uint32_t(u16(p+2))<<16;}
inline Wave read(const std::filesystem::path& path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);auto length=in.tellg();
    if(!in||length<44||length>std::streamoff(limit))throw std::runtime_error("Invalid or oversized WAV (64 MiB limit)");
    std::vector<unsigned char> bytes(static_cast<size_t>(length));in.seekg(0);in.read(reinterpret_cast<char*>(bytes.data()),length);
    if(!in||std::string(reinterpret_cast<char*>(bytes.data()),4)!="RIFF"||std::string(reinterpret_cast<char*>(bytes.data()+8),4)!="WAVE")throw std::runtime_error("Use an uncompressed PCM WAV");
    const size_t end=size_t(u32(bytes.data()+4))+8;if(end>bytes.size()||end<12)throw std::runtime_error("Truncated WAV");
    Wave result;bool fmt=false,pcm=false;
    for(size_t p=12;p+8<=end;) {
        auto size=u32(bytes.data()+p+4);size_t first=p+8;if(size>end-first)throw std::runtime_error("Truncated WAV chunk");
        std::string tag(reinterpret_cast<char*>(bytes.data()+p),4);
        if(tag=="fmt "){
            if(fmt||size<16||u16(bytes.data()+first)!=1)throw std::runtime_error("Only PCM WAV encoding is supported");
            result.channels=u16(bytes.data()+first+2);result.rate=u32(bytes.data()+first+4);result.bits=u16(bytes.data()+first+14);
            if((result.channels!=1&&result.channels!=2)||result.rate<8000||result.rate>192000||(result.bits!=8&&result.bits!=16&&result.bits!=24&&result.bits!=32)||u16(bytes.data()+first+12)!=result.channels*result.bits/8)throw std::runtime_error("Invalid WAV format");
            fmt=true;
        } else if(tag=="data") {if(pcm||!size)throw std::runtime_error("Invalid WAV data");result.raw.assign(bytes.begin()+first,bytes.begin()+first+size);pcm=true;}
        p=first+size+(size&1);
    }
    if(!fmt||!pcm||result.raw.size()%(result.channels*result.bits/8))throw std::runtime_error("Incomplete WAV");
    return result;
}
inline std::vector<double> decode(const Wave& wave){
    std::vector<double> result;result.reserve(wave.raw.size()/(wave.bits/8));
    for(size_t p=0;p<wave.raw.size();p+=wave.bits/8){int64_t sample=0;
        if(wave.bits==8)sample=int(wave.raw[p])-128;
        else {for(unsigned b=0;b<wave.bits/8;++b)sample|=int64_t(wave.raw[p+b])<<(8*b);if(sample&(int64_t(1)<<(wave.bits-1)))sample-=int64_t(1)<<wave.bits;}
        result.push_back(double(sample)/double(int64_t(1)<<(wave.bits-1)));
    }return result;
}
inline double rms(const std::vector<double>& samples){double sum=0;for(auto x:samples)sum+=x*x;return samples.empty()?0:std::sqrt(sum/samples.size());}
struct Prepared {Wave wave;double gain_db=0;bool limited=false;};
inline Prepared prepare(const Wave& reference,const Wave& input,bool match){
    if(reference.rate!=input.rate||reference.channels!=input.channels)throw std::runtime_error("Replacement must match the original sample rate and channel count");
    auto original=decode(reference),samples=decode(input);double target=rms(original),level=rms(samples),peak=0;
    if(level<1e-8||(match&&target<1e-8))throw std::runtime_error("Silent audio cannot be level matched");
    for(auto x:samples)peak=std::max(peak,std::abs(x));
    double requested=match?target/level:1,gain=std::min(requested,std::pow(10.,-1./20)/peak);
    Prepared result;result.gain_db=20*std::log10(gain);result.limited=gain<requested;result.wave={reference.channels,reference.rate,16,{}};result.wave.raw.reserve(samples.size()*2);
    for(auto x:samples){int v=std::max(-32768,std::min(32767,int(std::round(x*gain*32768))));result.wave.raw.push_back(v&255);result.wave.raw.push_back((v>>8)&255);}return result;
}
inline void write(const std::filesystem::path& path,const Wave& wave){
    std::ofstream out(path,std::ios::binary|std::ios::trunc);auto put16=[&](unsigned n){out.put(n&255);out.put((n>>8)&255);};auto put32=[&](uint32_t n){put16(n&65535);put16(n>>16);};
    out.write("RIFF",4);put32(36+wave.raw.size());out.write("WAVEfmt ",8);put32(16);put16(1);put16(wave.channels);put32(wave.rate);put32(wave.rate*wave.channels*wave.bits/8);put16(wave.channels*wave.bits/8);put16(wave.bits);out.write("data",4);put32(wave.raw.size());out.write(reinterpret_cast<const char*>(wave.raw.data()),wave.raw.size());out.close();if(!out)throw std::runtime_error("Could not save prepared sound");
}
}
