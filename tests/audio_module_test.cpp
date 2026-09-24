#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include "../runtime/sound_module.h"
#include <cassert>
#include <iostream>
static std::vector<unsigned char> received;
static void __cdecl capture(unsigned,int,const void* bytes,int length,int){received.assign((const unsigned char*)bytes,(const unsigned char*)bytes+length);}
static ssc_audio::Wave tone(double amplitude){ssc_audio::Wave w{1,44100,16,{}};for(int i=0;i<4410;++i){int v=int(32767*amplitude*std::sin(i*.13));w.raw.push_back(v&255);w.raw.push_back((v>>8)&255);}return w;}
int main(int argc,char** argv){
    assert(argc==3);auto root=std::filesystem::path(argv[1]);std::filesystem::create_directories(root/"game/data/sounds");
    auto reference=tone(.2),input=tone(.05);auto prepared=ssc_audio::prepare(reference,input,true);
    assert(std::abs(ssc_audio::rms(ssc_audio::decode(prepared.wave))-ssc_audio::rms(ssc_audio::decode(reference)))<.00002);
    auto transient=input;transient.raw[0]=255;transient.raw[1]=127;assert(ssc_audio::prepare(reference,transient,true).limited);
    auto mismatch=input;mismatch.rate=22050;bool rejected=false;try{ssc_audio::prepare(reference,mismatch,true);}catch(...){rejected=true;}assert(rejected);
    ssc_audio::write(root/"game/data/sounds/test.wav",reference);auto reread=ssc_audio::read(root/"game/data/sounds/test.wav");assert(reread.raw==reference.raw);
    ssc_sound::initialize(root/"game",root/"state",false);assert(ssc_sound::entries.size()==1);
    auto key=ssc_sound::entries[0].key;std::filesystem::create_directories(root/"state/sounds");ssc_audio::write(root/"state/sounds"/(key+".wav"),prepared.wave);
    ssc_sound::initialize(root/"game",root/"state",true);assert(ssc_sound::replacements.size()==1);ssc_sound::original=capture;
    ssc_sound::buffer_data(1,0x1101,reference.raw.data(),reference.raw.size(),reference.rate);assert(received==prepared.wave.raw&&ssc_sound::substituted==1);
    ssc_sound::buffer_data(2,0x1101,input.raw.data(),input.raw.size(),input.rate);assert(received==input.raw);
    ssc_sound::initialize(root/"game",root/"state",false);ssc_sound::buffer_data(1,0x1101,reference.raw.data(),reference.raw.size(),reference.rate);assert(received==reference.raw);
    ssc_audio::write(root/"game/data/sounds/duplicate.wav",reference);ssc_sound::initialize(root/"game",root/"state",true);assert(ssc_sound::replacements.size()==1);assert(ssc_sound::entries[0].key==ssc_sound::entries[1].key);
    ssc_sound::buffer_data(3,0x1101,reference.raw.data(),reference.raw.size(),reference.rate);assert(received==prepared.wave.raw);
    ssc_sound::selection=0;ssc_sound::remove_selected();assert(!ssc_sound::entries[0].imported&&!ssc_sound::entries[1].imported);
    ssc_sound::initialize(root/"game",root/"state",true);assert(ssc_sound::replacements.empty());
    assert(ssc_audio::read(root/"game/data/sounds/test.wav").raw==reference.raw);
    {std::ofstream bad(root/"broken.wav",std::ios::binary);bad<<"RIFF";}
    rejected=false;try{ssc_audio::read(root/"broken.wav");}catch(...){rejected=true;}assert(rejected);
    auto mp3=ssc_audio::read_mp3(argv[2]);assert(mp3.channels==2&&mp3.rate==22050&&mp3.bits==16&&!mp3.raw.empty());
    auto imported=ssc_audio::read_import(argv[2],reference);assert(imported.rate==reference.rate&&imported.channels==reference.channels&&ssc_audio::rms(ssc_audio::decode(imported))>.01);
    auto matched=ssc_audio::prepare(reference,imported,true);assert(std::abs(ssc_audio::rms(ssc_audio::decode(matched.wave))-ssc_audio::rms(ssc_audio::decode(reference)))<.0001);
    auto converted=ssc_audio::convert_import(mismatch,reference);assert(converted.rate==44100&&converted.channels==1&&converted.raw.size()==mismatch.raw.size()*2);
    auto stereo=input;stereo.channels=2;converted=ssc_audio::convert_import(stereo,reference);assert(converted.channels==1&&converted.raw.size()==stereo.raw.size()/2);
    rejected=false;try{ssc_audio::read_mp3(root/"broken.wav");}catch(...){rejected=true;}assert(rejected);
    // Random variants have different fingerprints and may use different sample rates.
    auto source=tone(.1),variant=tone(.15),third=tone(.18);third.rate=22050;
    ssc_audio::write(root/"game/data/sounds/killedHuman.wav",source);ssc_audio::write(root/"game/data/sounds/killedHuman2.wav",variant);ssc_audio::write(root/"game/data/sounds/killedHuman3.wav",third);
    ssc_sound::initialize(root/"game",root/"state",true);auto find_name=[&](const wchar_t* name){return size_t(std::find_if(ssc_sound::entries.begin(),ssc_sound::entries.end(),[&](const auto& e){return e.path.filename()==name;})-ssc_sound::entries.begin());};
    ssc_sound::selection=find_name(L"killedHuman.wav");auto selected_key=ssc_sound::entries[ssc_sound::selection].key;ssc_audio::write(root/"state/sounds"/(selected_key+".wav"),input);
    ssc_sound::initialize(root/"game",root/"state",true);ssc_sound::selection=find_name(L"killedHuman.wav");ssc_sound::toggle_variants();ssc_sound::set_volume(150);ssc_sound::initialize(root/"game",root/"state",true);
    assert(ssc_sound::replacements.size()==3);for(auto name:{L"killedHuman.wav",L"killedHuman2.wav",L"killedHuman3.wav"}){auto e=ssc_sound::entries[find_name(name)];assert(ssc_sound::all_variants(e)&&ssc_sound::has_replacement(e)&&ssc_sound::clip_volume(e)==150);auto actual=ssc_sound::replacements.at(e.key);assert(actual.rate==e.rate&&actual.channels==e.channels);assert(ssc_audio::rms(ssc_audio::decode(actual))>.05);}
    auto loud=ssc_sound::gained(tone(.9),300);for(double v:ssc_audio::decode(loud))assert(v>=-1&&v<=1);assert(ssc_audio::rms(ssc_audio::decode(ssc_sound::gained(input,0)))==0);
    ssc_sound::selection=find_name(L"killedHuman2.wav");ssc_sound::remove_selected();ssc_sound::initialize(root/"game",root/"state",true);assert(ssc_sound::replacements.size()==1);assert(ssc_audio::read(root/"game/data/sounds/killedHuman2.wav").raw==variant.raw);
    std::cout<<"PASS: normalization, clipping ceiling, format validation, buffer substitution, unknown passthrough, disable/restart, shared audio import/removal, format conversion, corrupt compressed audio and original preservation\n";
}
