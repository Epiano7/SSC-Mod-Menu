#pragma once
#include "chat_tts_core.h"
#include "chat_tts_windows.h"
#include "planned_native_guards.h"
#include "presence_source.h"
#include "piper_speech.h"
#include <thread>
#include <mutex>
#include <condition_variable>
namespace ssc_chat_tts {
inline Settings settings;
inline int voice_choice=0,volume=80,speed_percent=100,backend=0,piper_speaker=0;
inline std::string piper_voice="norman";
class SpeechWorker {
 std::mutex mutex_;std::condition_variable changed_;std::thread thread_;bool exiting_=false,started_=false;uint64_t cancel_=0,revision_=0;
 int selected_=0,loudness_=80,speed_=0,backend_=0,speaker_=0;std::string piper_id_="norman";
 Queue queue_;std::vector<InstalledVoice> voices_;std::string status_="Off";
 void status(const std::string& text){std::lock_guard<std::mutex> lock(mutex_);status_=text.substr(0,160);++revision_;}
 void run(){
  auto inventory=installed_windows_voices();{std::lock_guard<std::mutex> lock(mutex_);voices_=std::move(inventory.voices);++revision_;}
  HRESULT init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);ISpVoice* voice=nullptr;
  HRESULT hr=FAILED(init)?init:CoCreateInstance(CLSID_SpVoice,nullptr,CLSCTX_INPROC_SERVER,IID_ISpVoice,reinterpret_cast<void**>(&voice));
  status(SUCCEEDED(hr)?"Ready":"No compatible Windows speech engine");int prior_voice=-1;ssc_piper::Speech piper;uint64_t observed_cancel=0;
  for(;;){
   std::string text,piper_id;uint64_t generation=0;int selected=0,loudness=80,speed=0,provider=0,speaker=0;bool control=false;
   {std::unique_lock<std::mutex> lock(mutex_);
    if(!changed_.wait_for(lock,std::chrono::seconds(30),[&]{return exiting_||queue_.size()>0||cancel_!=observed_cancel;})){lock.unlock();piper.reset();continue;}
    if(exiting_)break;
    control=observed_cancel!=cancel_;observed_cancel=cancel_;
    if(control){lock.unlock();piper.reset();if(voice)voice->Speak(nullptr,SPF_ASYNC|SPF_PURGEBEFORESPEAK,nullptr);continue;}
    if(!queue_.pop(text))continue;
    generation=cancel_;selected=selected_;loudness=loudness_;speed=speed_;provider=backend_;piper_id=piper_id_;speaker=speaker_;
   }
   auto cancelled=[&](){std::lock_guard<std::mutex> lock(mutex_);return exiting_||generation!=cancel_;};
   if(provider==1){
    status("Generating local Piper speech");ssc_piper::Audio audio;std::string error;
    if(piper.synthesize(piper_id,text,speed,speaker,audio,cancelled,error)){if(!ssc_piper::play(audio,loudness,cancelled)&&!cancelled())status("Could not play local speech audio");else status("Piper ready");}
    else{status(error.empty()?"Piper stopped":error);if(cancelled())piper.reset();}
    continue;
   }
   piper.reset();if(!voice)continue;
   if(selected!=prior_voice){std::wstring id;{std::lock_guard<std::mutex> lock(mutex_);if(selected>=0&&size_t(selected)<voices_.size())id=voices_[size_t(selected)].id;}
    if(!id.empty()){ISpObjectToken* token=nullptr;if(SUCCEEDED(CoCreateInstance(CLSID_SpObjectToken,nullptr,CLSCTX_INPROC_SERVER,IID_ISpObjectToken,reinterpret_cast<void**>(&token)))){if(SUCCEEDED(token->SetId(nullptr,id.c_str(),FALSE)))voice->SetVoice(token);token->Release();}}prior_voice=selected;}
   voice->SetVolume(USHORT(std::clamp(loudness,0,100)));voice->SetRate(std::clamp(speed,-5,5));
   int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);if(length<=0||cancelled())continue;std::wstring wide(size_t(length),0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),wide.data(),length);
   if(loudness>100){ssc_piper::Audio audio;audio.sample_rate=22050;auto result=windows_pcm(voice,wide,speed,audio.pcm,cancelled);if(FAILED(result)){if(!cancelled())status("Could not render boosted Windows speech");}else if(!ssc_piper::play(audio,loudness,cancelled)&&!cancelled())status("Could not play boosted Windows speech");else status("Windows voice ready");continue;}
   voice->Speak(wide.c_str(),SPF_ASYNC|SPF_IS_NOT_XML,nullptr);
   while(voice->WaitUntilDone(25)==S_FALSE){if(cancelled()){voice->Speak(nullptr,SPF_ASYNC|SPF_PURGEBEFORESPEAK,nullptr);break;}}status("Windows voice ready");
  }
  piper.reset();if(voice){voice->Speak(nullptr,SPF_ASYNC|SPF_PURGEBEFORESPEAK,nullptr);voice->Release();}if(SUCCEEDED(init))CoUninitialize();
 }
 void copy_settings(){selected_=voice_choice;loudness_=volume;speed_=synthesis_rate(speed_percent);backend_=backend;piper_id_=piper_voice;speaker_=piper_speaker;}
 public:
 void start(){std::lock_guard<std::mutex> lock(mutex_);if(started_)return;try{thread_=std::thread([this]{run();});started_=true;}catch(...){status_="Could not start local speech";}}
 void configure(const Settings& config){std::lock_guard<std::mutex> lock(mutex_);if((queue_.settings.enabled&&!config.enabled)||backend_!=backend||piper_id_!=piper_voice||speaker_!=piper_speaker){queue_.stop();++cancel_;}queue_.settings=config;copy_settings();changed_.notify_one();}
 void submit(const Message& m,uint64_t now){std::lock_guard<std::mutex> lock(mutex_);if(queue_.push(m,now))changed_.notify_one();}
 void preview(){start();std::lock_guard<std::mutex> lock(mutex_);queue_.settings=settings;copy_settings();queue_.settings.enabled=true;queue_.settings.system=true;queue_.push({GetTickCount64(),0,Channel::system,"","Chat speech preview. Good luck, have fun.",false},GetTickCount64());changed_.notify_one();}
 void clear(){std::lock_guard<std::mutex> lock(mutex_);queue_.stop();++cancel_;changed_.notify_one();}
 void mute(const std::string& name,bool value){std::lock_guard<std::mutex> lock(mutex_);if(value)queue_.muted.insert(name);else queue_.muted.erase(name);queue_.stop();++cancel_;changed_.notify_one();}
 std::vector<std::string> muted(){std::lock_guard<std::mutex> lock(mutex_);return {queue_.muted.begin(),queue_.muted.end()};}
 std::vector<InstalledVoice> voices(){std::lock_guard<std::mutex> lock(mutex_);return voices_;}
 uint64_t revision(){std::lock_guard<std::mutex> lock(mutex_);return revision_;}
 std::string status(){std::lock_guard<std::mutex> lock(mutex_);return status_;}
 void stop(){ {std::lock_guard<std::mutex> lock(mutex_);if(!started_)return;exiting_=true;++cancel_;changed_.notify_one();}if(thread_.joinable())thread_.join();}
};
inline SpeechWorker& speech(){static auto* instance=new SpeechWorker;return *instance;}
inline void (*begin_observation)()=+[](){speech().start();speech().configure(settings);};
inline void (*submit_message)(const Message&,uint64_t)=+[](const Message& m,uint64_t now){speech().submit(m,now);};
// Accepted match chat is a vector<ChatMessage> at world+0xecc0. The global
// fb16c0 histories are community/lobby channels, not the active match feed.
inline uint64_t sampled_at=0,last_world=0,last_count=0,session=0,received=0,source_revision=0;
inline bool observing=false;
inline std::string source_status="Chat: off";
inline void source_state(const std::string& value){if(source_status!=value){source_status=value;++source_revision;}}
inline void stop_observation(){if(observing){speech().clear();observing=false;}last_count=0;last_world=0;}
inline uint64_t message_id(uint64_t index,const std::string& sender,const std::string& text){uint64_t h=14695981039346656037ULL;for(auto c:sender)h=(h^uint8_t(c))*1099511628211ULL;for(auto c:text)h=(h^uint8_t(c))*1099511628211ULL;return (h^index)|1;}
inline void tick(uintptr_t base,uint64_t now){
 if(!settings.enabled||!ssc_planned_native::tts){stop_observation();source_state(settings.enabled?"Chat: unsupported game build":"Chat: off");return;}
 if(now-sampled_at<150)return;
 sampled_at=now;
 uintptr_t world=0,first=0,last=0,capacity=0;
 if(!ssc_names::read(base+ssc_compat::resolve(0xdec5d0),world)||!world){stop_observation();source_state("Chat: waiting for match");return;}
 if(!ssc_names::read(world+0xecc0,first)||!ssc_names::read(world+0xecc8,last)||!ssc_names::read(world+0xecd0,capacity)||last<first||capacity<last||(last-first)%0x70||(last-first)/0x70>4096||(!first&&(last||capacity))){stop_observation();source_state("Chat: history unavailable");return;}
 const uint64_t count=(last-first)/0x70;
 const bool seed=!observing||world!=last_world||count<last_count;
 if(seed){speech().clear();++session;received=0;last_count=count;last_world=world;observing=true;begin_observation();source_state("Chat: listening for new messages");return;}
 if(count==last_count)return;
 begin_observation();
 int local_slot=-1;ssc_names::read(world+0x6b98,local_slot);
 // Bound a burst to the most recent 32 accepted messages. Speech has a second
 // four-message queue limit; neither polling nor playback scales with history.
 for(uint64_t i=std::max(last_count,count>32?count-32:0);i<count;++i){
  const uintptr_t row=first+i*0x70;int slot=-1;uint8_t visible=0,team=0;
  if(!ssc_names::read(row,slot)||!ssc_names::read(row+0x65,visible)||visible!=1||!ssc_names::read(row+0x66,team)||team>1)continue;
  auto sender=ssc_rpc::native_string(row+8),body=ssc_rpc::native_string(row+0x28);
  if(body.empty()||body.size()>256||sender.size()>128||slot>4095)continue;
  // Use the displayed sender copied into accepted chat, never resolve a hidden
  // account name. Local identity is the native sender slot, including anon mode.
  const auto channel=slot<0?Channel::system:team?Channel::team:Channel::public_chat;
  Message m{message_id(i,sender,body),session,channel,sender,body,slot>=0&&local_slot>=0&&slot==local_slot};
  submit_message(m,now);++received;
 }
 last_count=count;source_state("Chat: listening ("+std::to_string(received)+" new)");
}
}
