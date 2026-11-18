#pragma once
#include "audio_pcm.h"
#include <map>
#include <set>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include <bcrypt.h>
namespace ssc_sound_pool {
constexpr size_t max_clips=32,max_total_clips=128,max_pool_bytes=32*1024*1024,max_total_bytes=128*1024*1024;
struct Clip {std::string file,label;};
using Catalog=std::map<std::string,std::vector<Clip>>;
inline bool valid_file(const std::string& name){return name.size()==73&&name.compare(0,5,"pool-")==0&&name.compare(69,4,".wav")==0&&name.find_first_not_of("0123456789abcdef",5)==69;}
inline bool valid_key(const std::string& key){return key.size()<=96&&(key.compare(0,9,"category:")==0||key.compare(0,5,"clip:")==0)&&key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-")==std::string::npos;}
template<class Next> bool choose(size_t count,Next next,size_t& index){if(!count||count>max_clips)return false;const uint32_t n=uint32_t(count),threshold=uint32_t(0-n)%n;for(int tries=0;tries<32;++tries){uint32_t word=0;if(!next(word))return false;if(word>=threshold){index=word%n;return true;}}return false;}
inline bool random_word(uint32_t& value){return BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&value),sizeof(value),BCRYPT_USE_SYSTEM_PREFERRED_RNG)>=0;}
inline int format(const ssc_audio::Wave& wave){return wave.channels==1?(wave.bits==8?0x1100:0x1101):(wave.bits==8?0x1102:0x1103);}
using BufferData=void(__cdecl*)(unsigned,int,const void*,int,int);
using GenBuffers=void(__cdecl*)(int,unsigned*);using DeleteBuffers=void(__cdecl*)(int,const unsigned*);
using SourcePlay=void(__cdecl*)(unsigned);using SourceStop=void(__cdecl*)(unsigned);using Sourcei=void(__cdecl*)(unsigned,int,int);using GetSourcei=void(__cdecl*)(unsigned,int,int*);
using DeleteSources=void(__cdecl*)(int,const unsigned*);
using CurrentContext=void*(__cdecl*)();using DestroyContext=void(__cdecl*)(void*);
inline BufferData buffer_data_original=nullptr;inline GenBuffers gen_buffers=nullptr;inline DeleteBuffers delete_buffers=nullptr;
inline DeleteSources delete_sources_original=nullptr;
inline SourcePlay play_original=nullptr;inline SourceStop stop_source=nullptr;inline Sourcei source_i=nullptr;inline GetSourcei get_source_i=nullptr;inline CurrentContext current_context=nullptr;inline DestroyContext destroy_original=nullptr;
inline bool (*next_random)(uint32_t&)=random_word;
inline bool attached=false;
inline std::atomic<bool> enabled{true};
inline std::set<std::string> disabled_pools;

inline std::map<std::string,std::vector<ssc_audio::Wave>> prepared;
inline std::map<std::string,std::string> native_keys;
struct Context {std::map<unsigned,std::string> native_buffers;std::map<std::string,std::vector<unsigned>> pools;struct Source {unsigned native=0,custom=0;};std::map<unsigned,Source> sources;};
inline std::map<uintptr_t,Context> contexts;
inline std::mutex mutex;
inline unsigned played=0;
inline void disable_pool(const std::string& key){std::lock_guard<std::mutex> lock(mutex);disabled_pools.insert(key);}
inline bool allowed(const std::string& key){return enabled.load()&&!disabled_pools.count(key); }
inline void clear_prepared(){std::lock_guard<std::mutex> lock(mutex);prepared.clear();native_keys.clear();contexts.clear();disabled_pools.clear();played=0;}
// Called only while the game is loading a native buffer. PCM is already decoded;
// custom OpenAL buffers are uploaded here, never by file I/O on the play path.
inline bool register_buffer(unsigned buffer,const std::string& key){
 if(!attached||!current_context||!gen_buffers||!delete_buffers||!buffer_data_original)return false;
 auto handle=reinterpret_cast<uintptr_t>(current_context());if(!handle)return false;
 std::lock_guard<std::mutex> lock(mutex);auto match=native_keys.find(key);auto existing=contexts.find(handle);
 if(match==native_keys.end()){if(existing!=contexts.end())existing->second.native_buffers.erase(buffer);return false;}
 auto data=prepared.find(match->second);if(data==prepared.end()||data->second.empty()||data->second.size()>max_clips)return false;
 if(existing==contexts.end()){if(contexts.size()>=4)return false;existing=contexts.emplace(handle,Context{}).first;}
 auto& context=existing->second;auto found=context.pools.find(match->second);
 if(found==context.pools.end()){
  std::vector<unsigned> ids(data->second.size(),0);gen_buffers(int(ids.size()),ids.data());
  if(std::any_of(ids.begin(),ids.end(),[](unsigned id){return id==0;})){for(auto id:ids)if(id)delete_buffers(1,&id);return false;}
  for(size_t i=0;i<ids.size();++i){const auto& wave=data->second[i];buffer_data_original(ids[i],format(wave),wave.raw.data(),int(wave.raw.size()),int(wave.rate));}
  context.pools.emplace(match->second,std::move(ids));
 }
 if(context.native_buffers.size()>=4096&&context.native_buffers.find(buffer)==context.native_buffers.end())return false;
 context.native_buffers[buffer]=match->second;return true;
}
// The engine compares AL_BUFFER with its original asset records when finding
// and recycling voices. Keep that identity stable while custom PCM is bound.
// Internal routing always uses get_source_i directly to see the real binding.
inline void __cdecl get_source_integer(unsigned source,int param,int* value){
 if(!get_source_i)return;
 get_source_i(source,param,value);
 if(param!=0x1009||!value||!current_context)return;
 std::lock_guard<std::mutex> lock(mutex);auto context=contexts.find(reinterpret_cast<uintptr_t>(current_context()));if(context==contexts.end())return;
 auto tracked=context->second.sources.find(source);if(tracked!=context->second.sources.end()&&unsigned(*value)==tracked->second.custom&&context->second.native_buffers.count(tracked->second.native))*value=int(tracked->second.native);
}
inline void __cdecl delete_sources(int count,const unsigned* sources){
 if(count>0&&count<=4096&&sources&&current_context){std::lock_guard<std::mutex> lock(mutex);auto context=contexts.find(reinterpret_cast<uintptr_t>(current_context()));if(context!=contexts.end())for(int i=0;i<count;++i)context->second.sources.erase(sources[i]);}
 if(delete_sources_original)delete_sources_original(count,sources);
}
// Game buffer rebinds invalidate our source's previous custom/native pairing.
inline void __cdecl source_integer(unsigned source,int param,int value){
 if(!source_i)return;
 source_i(source,param,value);
 if(param!=0x1009||!current_context||!get_source_i)return;
 std::lock_guard<std::mutex> lock(mutex);auto context=contexts.find(reinterpret_cast<uintptr_t>(current_context()));if(context==contexts.end())return;
 auto tracked=context->second.sources.find(source);if(tracked==context->second.sources.end())return;
 // A playing engine may be assigned its existing logical buffer repeatedly.
 // OpenAL rejects that operation and leaves the custom buffer bound. Retain
 // its identity in that case; invalidate only after a real binding change.
 // Do not consume alGetError: preserve the driver's result for the caller.
 int bound=0;get_source_i(source,0x1009,&bound);
 if(unsigned(bound)!=tracked->second.custom)context->second.sources.erase(tracked);
}
inline void __cdecl delete_native_buffers(int count,const unsigned* buffers){
 if(delete_buffers)delete_buffers(count,buffers);
 if(count<=0||count>4096||!buffers||!current_context)return;
 std::lock_guard<std::mutex> lock(mutex);auto context=contexts.find(reinterpret_cast<uintptr_t>(current_context()));if(context==contexts.end())return;
 for(int i=0;i<count;++i){context->second.native_buffers.erase(buffers[i]);for(auto it=context->second.sources.begin();it!=context->second.sources.end();)if(it->second.native==buffers[i])it=context->second.sources.erase(it);else ++it;}
}
inline void __cdecl source_play(unsigned source){
 if(!play_original)return;
 try{if(attached&&current_context&&get_source_i&&source_i&&stop_source){
  auto handle=reinterpret_cast<uintptr_t>(current_context());
  std::lock_guard<std::mutex> lock(mutex);auto context_it=contexts.find(handle);
  if(context_it!=contexts.end()){
   auto& context=context_it->second;
   // Restore disabled/cleared custom sounds on the owning OpenAL thread, even
   // when this callback is for an unrelated source. Never rebind reused sources.
   for(auto it=context.sources.begin();it!=context.sources.end();){auto native=context.native_buffers.find(it->second.native);
    if(native!=context.native_buffers.end()&&allowed(native->second)){++it;continue;}
    int bound=0;get_source_i(it->first,0x1009,&bound);
    if(unsigned(bound)==it->second.custom){int state=0;get_source_i(it->first,0x1010,&state);if(state==0x1012||state==0x1013)stop_source(it->first);source_i(it->first,0x1009,int(it->second.native));if(state==0x1012&&it->first!=source)play_original(it->first);}
    it=context.sources.erase(it);
   }
   int buffer=0;get_source_i(source,0x1009,&buffer);auto native=context.native_buffers.find(unsigned(buffer));auto state=context.sources.find(source);
   unsigned original=0;
   if(native!=context.native_buffers.end())original=unsigned(buffer);
   else if(state!=context.sources.end()&&state->second.custom==unsigned(buffer)){original=state->second.native;native=context.native_buffers.find(original);}
   if(original&&native!=context.native_buffers.end()&&allowed(native->second)){
    auto pool=context.pools.find(native->second);size_t choice=0;
    if(pool!=context.pools.end()&&choose(pool->second.size(),next_random,choice)&&(context.sources.size()<1024||state!=context.sources.end())){
     int playing=0;get_source_i(source,0x1010,&playing);if(playing==0x1012||playing==0x1013)stop_source(source);
     unsigned custom=pool->second[choice];source_i(source,0x1009,int(custom));context.sources[source]={original,custom};++played;
    }else if(unsigned(buffer)!=original){int playing=0;get_source_i(source,0x1010,&playing);if(playing==0x1012||playing==0x1013)stop_source(source);source_i(source,0x1009,int(original));context.sources.erase(source);}
   }else if(state!=context.sources.end()){if(state->second.custom==unsigned(buffer)){int playing=0;get_source_i(source,0x1010,&playing);if(playing==0x1012||playing==0x1013)stop_source(source);source_i(source,0x1009,int(state->second.native));}context.sources.erase(state);}
  }
 }}catch(...){}
 play_original(source);
}
inline void __cdecl destroy_context(void* context){
 {std::lock_guard<std::mutex> lock(mutex);contexts.erase(reinterpret_cast<uintptr_t>(context));}
 // OpenAL owns and frees its buffers on context destruction. Do not issue buffer
 // calls against a context which may no longer be current on this thread.
 if(destroy_original)destroy_original(context);
}
}
