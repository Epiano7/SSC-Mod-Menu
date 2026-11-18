#pragma once
#include "audio_import.h"
#include "audio_codec.h"
#include "sound_pool_playback.h"
#include "music_stream.h"
#include "third_party/json.hpp"
#include <map>
#include <set>
#include <atomic>
#include <thread>
#include <memory>
#include <cstring>
#include <commdlg.h>
namespace ssc_sound {
using BufferData=void(__cdecl*)(unsigned,int,const void*,int,int);
inline BufferData original=nullptr;
struct Entry {std::filesystem::path path;std::wstring label;std::string key;unsigned rate=0,channels=0,bits=0;bool imported=false;bool music=false;double seconds=0;};
inline std::wstring duration(double seconds){
 auto total=static_cast<unsigned>(std::max(0.,std::floor(seconds)));auto remain=std::to_wstring(total%60);
 return L"("+std::to_wstring(total/60)+L":"+(remain.size()==1?L"0":L"")+remain+L")";
}
inline std::wstring display_name(const Entry& e){return e.label+L" "+duration(e.seconds)+(e.music?L" [MUSIC]":L"");}
inline ssc_audio::Wave original_wave(const Entry& e){return e.music?ssc_audio::read_ogg(e.path):ssc_audio::read(e.path);}
inline std::vector<Entry> entries;
inline std::map<std::string,ssc_audio::Wave> replacements;
inline std::filesystem::path folder;
inline std::wstring status=L"No sound selected";
using PreviewPlay=BOOL(WINAPI*)(LPCWSTR,HMODULE,DWORD);
inline PreviewPlay preview_player=nullptr;inline bool preview_active=false;
inline PreviewPlay preview_api(){if(!preview_player){static HMODULE lib=LoadLibraryW(L"winmm.dll");auto proc=lib?GetProcAddress(lib,"PlaySoundW"):nullptr;std::memcpy(&preview_player,&proc,sizeof(preview_player));}return preview_player;}
inline void stop_preview(){if(!preview_active)return;if(auto play=preview_api())play(nullptr,nullptr,0);preview_active=false;}
inline bool play_preview(const std::filesystem::path& file){stop_preview();auto play=preview_api();preview_active=play&&play(file.c_str(),nullptr,0x20003);return preview_active;}

inline std::atomic<unsigned> substituted{0},recognized{0};
inline std::atomic<bool> active{false};inline bool attached=false;
inline void request_music_apply();
inline void set_enabled(bool value){bool changed=active!=value;active=value;ssc_sound_pool::enabled=value;ssc_music::enabled=value;if(changed&&value)request_music_apply();}
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
// Verified against the v0.992 native playlist builder (RVA 0x97d480),
// not against files merely left in data/sounds. Other name references in the
// music selector are legacy comparisons, not playlist registrations.
inline std::string music_family(const std::filesystem::path& path){
 auto name=path.stem().wstring();
 for(const auto* base:{L"stabBeat",L"skyCity",L"grooveRide",L"outRunner",L"acidSpray",L"nitrogen",L"onAWire",L"darkTension",L"cyberneticIndustry",L"sonicWarfare",L"epicAssault"}){
  if(name==base||name==std::wstring(base)+L"Intense"){std::wstring stem(base);return "music:"+std::string(stem.begin(),stem.end());}
 }
 return "";
}
inline bool used_music(const std::filesystem::path& path){return path.stem()==L"nightDrive"||!music_family(path).empty();}
// Explicit related clips, including state-dependent alternatives. Do not infer
// families by stripping digits: weapon names such as AK47 are independent clips.
inline std::string family(const Entry& e){
 if(e.music)return music_family(e.path);
 auto name=e.path.stem().wstring();
 auto numbered=[&](const wchar_t* prefix,int lo,int hi,int width=0){for(int i=lo;i<=hi;++i){auto n=std::to_wstring(i);if(width==2&&i<10)n=L"0"+n;if(name==std::wstring(prefix)+n)return true;}return false;};
 if(name==L"killedHuman"||numbered(L"killedHuman",2,3)||numbered(L"comboKillSound",2,10))return "human-kill";
 for(int i=1;i<=6;++i){auto engine=L"engine"+std::to_wstring(i);if(name==engine||name==engine+L"Damaged"||name==engine+L"Muffled")return "vehicle-engines";}
 if(numbered(L"bulletHitNormal",1,6,2))return "bullet-hit-normal";
 if(name==L"bulletHit"||numbered(L"bulletHit",2,5,2))return "bullet-hit";
 if(numbered(L"bulletCar",2,7,2))return "bullet-car";
 if(name==L"bulletWall"||numbered(L"bulletWall",2,8,2)||numbered(L"bulletWallMuffled",1,3,2))return "bullet-wall";
 if(numbered(L"carCollision",1,3))return "car-collision";
 for(const auto& group:std::initializer_list<std::pair<const wchar_t*,std::pair<int,int>>>{{L"alarm",{1,6}},{L"background",{1,3}},{L"bird",{1,3}},{L"computing",{1,2}},{L"drip",{1,2}},{L"fire",{1,4}},{L"frog",{1,2}},{L"horn",{1,3}},{L"hurt",{1,3}},{L"light",{1,5}},{L"machine",{1,3}},{L"rain",{1,7}},{L"stepDirt",{1,2}},{L"stepGrass",{1,2}},{L"stepInside",{1,2}},{L"stepMetal",{1,2}},{L"stepNorm",{1,2}},{L"stepSnow",{1,2}},{L"stepWater",{1,2}},{L"thunder",{1,2}},{L"train",{1,2}},{L"turbine",{1,7}},{L"waves",{1,3}},{L"wind",{1,6}}})if(numbered(group.first,group.second.first,group.second.second)){std::wstring key(group.first);return std::string(key.begin(),key.end());}
 for(const auto& group:std::initializer_list<std::pair<const wchar_t*,std::pair<int,int>>>{{L"copFreeze",{1,10}},{L"fireworkPop",{1,4}},{L"flaskBubble",{1,4}},{L"flyby",{1,3}},{L"gangsterHey",{1,4}},{L"radioTrack",{1,10}},{L"scream",{1,10}},{L"wantedCircle",{1,4}},{L"wantedLevelUp",{1,4}}})if(numbered(group.first,group.second.first,group.second.second,2)){std::wstring key(group.first);return std::string(key.begin(),key.end());}
 if(name==L"die"||name==L"killed")return "death";
 if(name==L"weaponSwitch"||numbered(L"weaponSwitch",2,4,2))return "weapon-switch";
 if(name==L"crateExplodeMuffled"||numbered(L"crateExplode",1,3,2))return "crate-explode";
 if(numbered(L"grenadeExplodeBig",1,3,2))return "grenade-big";
 if(numbered(L"grenadeExplodeDistant",1,4,2))return "grenade-distant";
 if(name==L"grenadeExplode"||numbered(L"grenadeExplode",2,3,2))return "grenade-explode";
 for(const auto* base:{L"bells",L"bonusPointAvailable",L"daggerDanceBegins",L"daggerDanceFired",L"flaskAnimationMix",L"flash",L"zombieGroan",L"zombieSpawn"})if(name==base||name==std::wstring(base)+L"2"||(std::wstring(base)==L"bells"&&name==L"bells1")||(std::wstring(base)==L"flash"&&name==L"flash1")){std::wstring key(base);return std::string(key.begin(),key.end());}
 if(name==L"zombieDeath"||name==L"zombieDeath2")return "zombie-death";
 if(name==L"vehicleExplode"||name==L"vehicleExplode02")return "vehicle-explode";
 return "";
}
// Event lists migrate automatically; ambience and music stay opt-in.
inline bool automatic_group(const std::string& f){static const std::set<std::string> groups={"human-kill","vehicle-engines","death","hurt","weapon-switch","car-collision","horn","bullet-hit-normal","bullet-hit","bullet-car","bullet-wall","stepDirt","stepGrass","stepInside","stepMetal","stepNorm","stepSnow","stepWater","crate-explode","grenade-explode","grenade-big","grenade-distant","vehicle-explode","alarm","copFreeze","wantedCircle","wantedLevelUp","fireworkPop","flaskBubble","zombie-death","zombieGroan","zombieSpawn","bonusPointAvailable","daggerDanceBegins","daggerDanceFired","flaskAnimationMix","flash"};return groups.count(f)!=0;}
inline ssc_sound_pool::Catalog pools;
inline std::map<std::string,int> pool_volume;
inline constexpr const char* music_pool_key="category:music-all";
inline constexpr size_t music_disk_limit=512*1024*1024;
inline std::string pool_key(const Entry& e){if(e.music)return music_pool_key;auto f=family(e);return f.empty()?"clip:"+e.key:"category:"+f;}
inline bool shared_category(const Entry& e){return !e.music&&!family(e).empty();}
// Group membership changes only when the sound catalog is rebuilt. Keep menu
// draws proportional to the selected group, not the entire game's sound list.
inline std::map<std::string,std::vector<size_t>> group_catalog;
inline const Entry* group_catalog_data=nullptr;inline size_t group_catalog_size=~size_t(0);
inline void rebuild_group_catalog(){group_catalog.clear();for(size_t i=0;i<entries.size();++i)if(!entries[i].music)group_catalog[pool_key(entries[i])].push_back(i);group_catalog_data=entries.data();group_catalog_size=entries.size();}
inline const std::vector<size_t>& group_members(const Entry& e){
 if(group_catalog_data!=entries.data()||group_catalog_size!=entries.size())rebuild_group_catalog();
 static const std::vector<size_t> empty;auto found=group_catalog.find(pool_key(e));return found==group_catalog.end()?empty:found->second;
}
inline std::wstring category_label(const Entry& e){
 auto f=family(e);if(f=="human-kill")return L"Human kills";
 if(f.empty())return e.label;
 std::wstring result(f.begin(),f.end());for(auto& c:result)if(c==L'-')c=L' ';
 if(!result.empty()){result[0]=towupper(result[0]);}
 return result;
}
inline size_t native_clip_count(const Entry& e){return group_members(e).size();}
inline std::wstring coverage_label(const Entry& e){
 if(family(e)=="human-kill"){
  size_t base=0,combo=0;for(auto index:group_members(e)){const auto& other=entries[index];if(other.path.stem().wstring().find(L"killedHuman")==0)++base;else ++combo;}
  return L"Covers "+std::to_wstring(base)+L" kill clips + "+std::to_wstring(combo)+L" combo/streak clips.";
 }
 return L"Covers "+std::to_wstring(native_clip_count(e))+L" game clip"+(native_clip_count(e)==1?L".":L"s.");
}
inline size_t saved_custom_count(const Entry& entry){std::set<std::string> keys;for(auto index:group_members(entry)){const auto& e=entries[index];if(e.imported)keys.insert(e.key);}return keys.size();}
// One category row; searches still match any original clip name within it.
inline std::vector<size_t> grouped_entries(std::wstring query){
 std::transform(query.begin(),query.end(),query.begin(),towlower);
 std::map<std::string,size_t> first;std::set<std::string> matched;
 for(size_t i=0;i<entries.size();++i){const auto& e=entries[i];auto key=shared_category(e)?pool_key(e):"file:"+e.path.u8string();
  if(!first.count(key)||e.path.stem()==L"killedHuman")first[key]=i;
  auto label=display_name(e)+L" "+category_label(e);std::transform(label.begin(),label.end(),label.begin(),towlower);
  if(label.find(query)!=std::wstring::npos)matched.insert(key);
 }
 std::vector<size_t> out;for(const auto& key:matched)out.push_back(first.at(key));
 std::sort(out.begin(),out.end(),[](size_t a,size_t b){auto left=shared_category(entries[a])?category_label(entries[a]):display_name(entries[a]);auto right=shared_category(entries[b])?category_label(entries[b]):display_name(entries[b]);return left==right?a<b:left<right;});return out;
}
inline std::set<std::string> pool_migrations,original_groups;
inline bool pool_scope_review_done=false,event_migration_failed=false;
inline size_t pool_count(const Entry& e){auto it=pools.find(pool_key(e));return it==pools.end()?0:it->second.size();}
inline nlohmann::json encode_pools(){auto j=nlohmann::json::object();for(const auto& pool:pools){auto clips=nlohmann::json::array();for(const auto& clip:pool.second)clips.push_back({{"file",clip.file},{"label",clip.label}});j[pool.first]=std::move(clips);}return j;}
inline size_t pool_choice=0,pool_page=0;
inline std::map<std::string,int> volume;
inline std::map<std::string,std::string> families;
inline std::string variant_group(const Entry& e){return e.music?"music:all":family(e);}
inline bool all_variants(const Entry& e){auto f=variant_group(e);return !f.empty()&&families.find(f)!=families.end();}
inline std::string replacement_key(const Entry& e){if(e.music){auto all=families.find("music:all");if(all!=families.end())return all->second;}auto it=families.find(family(e));return it==families.end()?e.key:it->second;}
inline int clip_volume(const Entry& e){if(pool_count(e)){auto v=pool_volume.find(pool_key(e));return v==pool_volume.end()?100:v->second;}auto it=volume.find(replacement_key(e));return it==volume.end()?100:it->second;}
inline bool has_replacement(const Entry& e){if(original_groups.count(pool_key(e)))return false;if(pool_count(e))return true;auto key=replacement_key(e);return std::any_of(entries.begin(),entries.end(),[&](const Entry& item){return item.key==key&&item.imported;});}
inline std::wstring list_label(const Entry& entry){
 if(entry.music)return display_name(entry)+(has_replacement(entry)?L" [CUSTOM]":L"");
 auto count=pool_count(entry);auto label=shared_category(entry)?category_label(entry)+L" [SHARED LIST]":display_name(entry);
 if(original_groups.count(pool_key(entry)))label+=L" [ORIGINAL]";
 else if(count)label+=L" ["+std::to_wstring(count)+L" CUSTOM]";
 else if(shared_category(entry)&&saved_custom_count(entry))label+=L" [SAVED CUSTOM]";
 else if(has_replacement(entry))label+=L" [CUSTOM]";
 return label;
}
inline bool save_options(){try{std::filesystem::create_directories(folder);auto tmp=folder/L"options.json.tmp";std::ofstream out(tmp);out<<nlohmann::json{{"schema",1},{"volume",volume},{"families",families},{"pools",encode_pools()},{"pool_volume",pool_volume},{"pool_migrations",pool_migrations},{"original_groups",original_groups},{"pool_scope_review_done",pool_scope_review_done}}.dump(2);out.close();return out&&MoveFileExW(tmp.c_str(),(folder/L"options.json").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}catch(...){return false;}}
inline void load_options(){event_migration_failed=false;volume.clear();families.clear();pools.clear();pool_volume.clear();pool_migrations.clear();original_groups.clear();pool_scope_review_done=false;try{auto file=folder/L"options.json";if(!std::filesystem::exists(file)||std::filesystem::file_size(file)>256*1024)return;std::ifstream input(file);nlohmann::json j;input>>j;if(j.at("schema")!=1)return;if(j.contains("pool_scope_review_done")&&j["pool_scope_review_done"].is_boolean())pool_scope_review_done=j["pool_scope_review_done"].get<bool>();auto v=j.at("volume").get<std::map<std::string,int>>();auto f=j.at("families").get<std::map<std::string,std::string>>();for(const auto& e:entries){auto it=v.find(e.key);if(it!=v.end()&&it->second>=0&&it->second<=300)volume[e.key]=it->second;auto group=family(e);auto choice=f.find(group);if(!group.empty()&&choice!=f.end()&&choice->second==e.key&&e.imported)families[group]=e.key;auto all=f.find("music:all");if(e.music&&e.imported&&all!=f.end()&&all->second==e.key)families["music:all"]=e.key;}
 if(j.contains("pool_migrations")&&j["pool_migrations"].is_array()&&j["pool_migrations"].size()<=512)for(const auto& key:j["pool_migrations"])if(key.is_string()&&ssc_sound_pool::valid_key(key.get<std::string>()))pool_migrations.insert(key.get<std::string>());
 if(j.contains("original_groups")&&j["original_groups"].is_array()&&j["original_groups"].size()<=512)for(const auto& key:j["original_groups"])if(key.is_string()&&ssc_sound_pool::valid_key(key.get<std::string>()))original_groups.insert(key.get<std::string>());
 size_t total=0;if(j.contains("pools")&&j["pools"].is_object())for(auto it=j["pools"].begin();it!=j["pools"].end();++it){
  if(!ssc_sound_pool::valid_key(it.key())||!it->is_array()||it->empty()||it->size()>ssc_sound_pool::max_clips||total+it->size()>ssc_sound_pool::max_total_clips||std::none_of(entries.begin(),entries.end(),[&](const Entry& entry){return pool_key(entry)==it.key()||(!entry.music&&family(entry)=="death"&&it.key()=="clip:"+entry.key);}))continue;
  std::vector<ssc_sound_pool::Clip> clips;bool valid=true;std::set<std::string> unique;
  for(const auto& item:*it){if(!item.is_object()||!item.contains("file")||!item["file"].is_string()||!item.contains("label")||!item["label"].is_string()){valid=false;break;}auto name=item["file"].get<std::string>(),label=item["label"].get<std::string>();if(!ssc_sound_pool::valid_file(name)||label.size()>320||!unique.insert(name).second){valid=false;break;}clips.push_back({name,label});}
  if(valid){total+=clips.size();pools[it.key()]=std::move(clips);}
 }
 if(j.contains("pool_volume")&&j["pool_volume"].is_object())for(auto it=j["pool_volume"].begin();it!=j["pool_volume"].end();++it)if(pools.count(it.key())&&it->is_number_integer()){int v=it->get<int>();if(v>=0&&v<=300)pool_volume[it.key()]=v;}
 }catch(...) {}}
inline ssc_audio::Wave gained(ssc_audio::Wave wave,int percent){if(wave.bits!=16)throw std::runtime_error("Expected 16-bit replacement");for(size_t i=0;i<wave.raw.size();i+=2){int sample=int(int16_t(ssc_audio::u16(wave.raw.data()+i)));int v=std::clamp(int(std::lround(sample*std::clamp(percent,0,300)/100.0)),-32768,32767);wave.raw[i]=uint8_t(v);wave.raw[i+1]=uint8_t(v>>8);}return wave;}
// Preserve the game's per-track loudness when sharing one music replacement.
inline std::map<std::filesystem::path,double> music_levels;
inline double music_level(const Entry& e){auto found=music_levels.find(e.path);if(found!=music_levels.end())return found->second;auto wave=original_wave(e);double level=ssc_audio::rms(ssc_audio::decode(wave));music_levels[e.path]=level;return level;}
inline ssc_audio::Wave music_track_gain(ssc_audio::Wave wave,double source,double target){
 if(source<1e-8||target<1e-8)return wave;
 double peak=0;for(size_t i=0;i<wave.raw.size();i+=2)peak=std::max(peak,std::abs(double(int16_t(ssc_audio::u16(wave.raw.data()+i)))/32768.));
 if(peak<1e-8)return wave;
 double gain=target/source;if(gain>1)gain=std::min(gain,std::max(1.,std::pow(10.,-1./20)/peak));
 for(size_t i=0;i<wave.raw.size();i+=2){int v=std::clamp(int(std::lround(int16_t(ssc_audio::u16(wave.raw.data()+i))*gain)),-32768,32767);wave.raw[i]=uint8_t(v);wave.raw[i+1]=uint8_t(v>>8);}return wave;
}
inline ssc_audio::Wave effective_wave(const Entry& e){auto key=replacement_key(e);auto wave=ssc_audio::read(folder/(key+".wav"));ssc_audio::Wave reference{e.channels,e.rate,16,{}};wave=ssc_audio::convert_import(wave,reference);if(e.music&&key!=e.key){auto source=std::find_if(entries.begin(),entries.end(),[&](const Entry& item){return item.music&&item.key==key;});if(source!=entries.end())wave=music_track_gain(std::move(wave),music_level(*source),music_level(e));}auto it=volume.find(key);return gained(std::move(wave),it==volume.end()?100:it->second);}


// Upgrade saved single-file effects into the shared category pool once. Keep
// every original replacement and the old options file. Explicitly cleared
// event lists keep using original game audio until enabled again.
inline bool use_saved_pool(const Entry& entry,bool preserve_shared_choice=false){
 const auto key=pool_key(entry);if(!shared_category(entry)||pool_count(entry))return false;
 const auto before=pools;const auto migrated_before=pool_migrations;const auto originals_before=original_groups;const bool reviewed_before=pool_scope_review_done;
 try{
  std::vector<ssc_sound_pool::Clip> clips;std::vector<std::pair<std::string,ssc_audio::Wave>> staged;std::set<std::string> keys,names;
  size_t count=0,total_bytes=0,bytes=0;for(const auto& pool:pools){count+=pool.second.size();if(pool.first==music_pool_key)continue;for(const auto& clip:pool.second){auto n=std::filesystem::file_size(folder/clip.file);if(n>ssc_sound_pool::max_total_bytes-total_bytes)throw std::runtime_error("All pools exceed 128 MiB");total_bytes+=n;}}
  for(const auto& e:entries){if(e.music||!e.imported||pool_key(e)!=key||(preserve_shared_choice&&families.at(family(entry))!=e.key)||!keys.insert(e.key).second)continue;
   auto wave=ssc_audio::read(folder/(e.key+".wav"));if(wave.bits!=16)throw std::runtime_error("Saved sound is not 16-bit PCM");auto v=volume.find(e.key);wave=gained(std::move(wave),v==volume.end()?100:v->second);
   auto hash=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(hash.size()!=64)throw std::runtime_error("Could not identify saved audio");auto file="pool-"+hash+".wav";if(!names.insert(file).second)continue;
   if(clips.size()>=ssc_sound_pool::max_clips||count+clips.size()>=ssc_sound_pool::max_total_clips||wave.raw.size()>ssc_sound_pool::max_pool_bytes-bytes||wave.raw.size()+44>ssc_sound_pool::max_total_bytes-total_bytes)throw std::runtime_error("Saved sounds exceed pool limits; single-file choices kept");
   bytes+=wave.raw.size();total_bytes+=wave.raw.size()+44;auto name=e.label;clips.push_back({file,std::string(name.begin(),name.end())+" (saved replacement)"});staged.emplace_back(file,std::move(wave));
  }
  if(clips.empty())return false;
  std::filesystem::create_directories(folder);auto options=folder/L"options.json",backup=folder/L"options.before-shared-pools.json";
  if(std::filesystem::exists(options)&&!std::filesystem::exists(backup))std::filesystem::copy_file(options,backup);
  for(const auto& item:staged){auto dest=folder/item.first;if(std::filesystem::exists(dest))continue;auto temporary=folder/(item.first+".migration.tmp");ssc_audio::write(temporary,item.second);if(!MoveFileExW(temporary.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not save migrated sound");}
  pools[key]=std::move(clips);pool_migrations.insert(key);original_groups.erase(key);pool_scope_review_done=true;if(!save_options())throw std::runtime_error("Could not save shared sound list");
  pool_choice=pool_page=0;status=L"Saved replacements added to the shared list; originals kept.";return true;
 }catch(const std::exception& error){pools=before;pool_migrations=migrated_before;original_groups=originals_before;pool_scope_review_done=reviewed_before;auto message=std::string(error.what());status.assign(message.begin(),message.end());return false;}
}
// Preserve existing random lists when formerly separate death clips are merged,
// and retain the old broad grenade choice in the new size/distance groups.
inline size_t migrate_list_layout(){
 const std::string marker="category:event-groups-v2";if(pool_migrations.count(marker))return 0;
 auto before=pools;auto gains_before=pool_volume;auto marks_before=pool_migrations;auto originals_before=original_groups;
 try{size_t changed=0;std::vector<std::pair<std::string,ssc_audio::Wave>> staged;
  const std::string death="category:death";
  if(!pools.count(death)&&!original_groups.count(death)){
   std::vector<ssc_sound_pool::Clip> clips;std::set<std::string> files,old_keys;
   for(const auto& e:entries)if(family(e)=="death")old_keys.insert("clip:"+e.key);
   for(const auto& old:old_keys){auto it=pools.find(old);if(it==pools.end())continue;auto gain=pool_volume.count(old)?pool_volume.at(old):100;
    for(const auto& clip:it->second){auto copy=clip;if(gain!=100){auto wave=gained(ssc_audio::read(folder/clip.file),gain);copy.file="pool-"+fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate)+".wav";staged.emplace_back(copy.file,std::move(wave));}if(files.insert(copy.file).second)clips.push_back(std::move(copy));}
   }
   if(!clips.empty()){pools[death]=std::move(clips);for(const auto& old:old_keys){pools.erase(old);pool_volume.erase(old);}pool_migrations.insert(death);++changed;}
  }
  const std::string grenade="category:grenade-explode";for(const auto* child:{"category:grenade-big","category:grenade-distant"}){
   if(original_groups.count(grenade)){original_groups.insert(child);continue;}
   if(!pools.count(child)&&!original_groups.count(child)&&pools.count(grenade)){pools[child]=pools.at(grenade);if(pool_volume.count(grenade))pool_volume[child]=pool_volume.at(grenade);pool_migrations.insert(child);++changed;}
  }
  size_t count=0,bytes=0;for(const auto& pool:pools){if(pool.first==music_pool_key){count+=pool.second.size();continue;}if(pool.second.size()>ssc_sound_pool::max_clips)throw std::runtime_error("Merged sound list exceeds 32 clips; old options kept");count+=pool.second.size();size_t per_pool=0;for(const auto& clip:pool.second){auto item=std::find_if(staged.begin(),staged.end(),[&](const auto& v){return v.first==clip.file;});auto n=item==staged.end()?std::filesystem::file_size(folder/clip.file):item->second.raw.size()+44;if(n>ssc_sound_pool::max_pool_bytes-per_pool||n>ssc_sound_pool::max_total_bytes-bytes)throw std::runtime_error("Merged sound lists exceed memory limits; old options kept");per_pool+=n;bytes+=n;}}
  if(count>ssc_sound_pool::max_total_clips)throw std::runtime_error("Merged sound lists exceed clip limits; old options kept");
  auto options=folder/L"options.json",backup=folder/L"options.before-event-merges.json",shared_backup=folder/L"options.before-shared-pools.json";if(std::filesystem::is_regular_file(options)){if(!std::filesystem::exists(backup))std::filesystem::copy_file(options,backup);if(!std::filesystem::exists(shared_backup))std::filesystem::copy_file(options,shared_backup);}
  for(const auto& item:staged)if(!std::filesystem::exists(folder/item.first))ssc_audio::write(folder/item.first,item.second);
  pool_migrations.insert(marker);if(!save_options())throw std::runtime_error("Could not save merged sound lists");return changed;
 }catch(const std::exception& error){event_migration_failed=true;pools=std::move(before);pool_volume=std::move(gains_before);pool_migrations=std::move(marks_before);original_groups=std::move(originals_before);auto message=std::string(error.what());status.assign(message.begin(),message.end());return 0;}
}
// Event sounds migrate without a button; ambience retains explicit opt-in.
inline size_t migrate_saved_pools(){
 size_t changed=migrate_list_layout();if(event_migration_failed)return 0;std::set<std::string> seen;
 for(const auto& entry:entries){auto key=pool_key(entry);auto group=family(entry);bool automatic=automatic_group(group);
  if(shared_category(entry)&&(automatic||families.count(group))&&seen.insert(key).second&&(!pool_migrations.count(key)||automatic)&&!original_groups.count(key)&&!pool_count(entry)&&saved_custom_count(entry)&&use_saved_pool(entry,!automatic))++changed;
 }return changed;
}
// Repair only untouched pools produced by the earlier automatic conversion.
// Require the exact old saved-audio fingerprints/labels and a backup proving
// that the category was not shared. User-created/edited lists stay as chosen.
inline size_t repair_legacy_pool_scope(){
 if(pool_scope_review_done||pools.empty())return 0;
 auto before=pools;const bool reviewed_before=pool_scope_review_done;
 try{
  auto old_file=folder/L"options.before-shared-pools.json";if(!std::filesystem::is_regular_file(old_file)||std::filesystem::file_size(old_file)>256*1024)return 0;
  nlohmann::json old;{std::ifstream input(old_file);input>>old;}if(!old.is_object()||old.value("schema",0)!=1||!old.contains("families")||!old["families"].is_object()||!old.contains("volume")||!old["volume"].is_object())return 0;
  std::vector<std::string> remove;
  for(const auto& pool:pools){if(pool.first.compare(0,9,"category:")||automatic_group(pool.first.substr(9))||!pool_migrations.count(pool.first)||original_groups.count(pool.first)||pool_volume.count(pool.first))continue;
   auto group=pool.first.substr(9);if(families.count(group)||old["families"].contains(group)||(old.contains("pools")&&old["pools"].is_object()&&old["pools"].contains(pool.first)))continue;
   std::vector<ssc_sound_pool::Clip> expected;std::set<std::string> keys,files;
   for(const auto& e:entries){if(e.music||!e.imported||pool_key(e)!=pool.first||!keys.insert(e.key).second)continue;
    auto wave=ssc_audio::read(folder/(e.key+".wav"));if(wave.bits!=16)throw std::runtime_error("Unexpected saved audio format");int percent=100;
    if(old["volume"].contains(e.key)){if(!old["volume"][e.key].is_number_integer())throw std::runtime_error("Invalid saved volume");percent=old["volume"][e.key].get<int>();if(percent<0||percent>300)throw std::runtime_error("Invalid saved volume");}
    wave=gained(std::move(wave),percent);auto hash=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(hash.size()!=64)throw std::runtime_error("Saved audio fingerprint failed");auto name="pool-"+hash+".wav";if(!files.insert(name).second)continue;expected.push_back({name,std::string(e.label.begin(),e.label.end())+" (saved replacement)"});
   }
   if(expected.empty()||expected.size()!=pool.second.size())continue;
   bool same=true;for(size_t i=0;i<expected.size();++i)if(expected[i].file!=pool.second[i].file||expected[i].label!=pool.second[i].label){same=false;break;}
   if(same)for(const auto& clip:pool.second){auto wave=ssc_audio::read(folder/clip.file);auto hash=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if("pool-"+hash+".wav"!=clip.file){same=false;break;}}
   if(same)remove.push_back(pool.first);
  }
  if(remove.empty())return 0;
  auto options=folder/L"options.json",backup=folder/L"options.before-pool-scope-repair.json";if(std::filesystem::is_regular_file(options)&&!std::filesystem::exists(backup))std::filesystem::copy_file(options,backup);
  for(const auto& key:remove){pools.erase(key);}
  pool_scope_review_done=true;
  if(!save_options()){throw std::runtime_error("Could not save sound scope repair");}
  return remove.size();
 }catch(...){pools=std::move(before);pool_scope_review_done=reviewed_before;return 0;}
}


inline std::atomic<uint64_t> music_requested{0};inline std::atomic<bool> music_busy{false};inline uint64_t music_started=0;
struct MusicResult {uint64_t revision=0;bool applied=false;};inline std::shared_ptr<const MusicResult> music_result;
inline void request_music_apply(){music_requested.fetch_add(1);}
inline void tick_music_apply(){
 static uint64_t reported=0;auto finished=std::atomic_load(&music_result);if(finished&&finished->revision!=reported&&finished->revision==music_requested.load()){reported=finished->revision;status=finished->applied?L"Music ready; applies when the game next opens a track":L"Music update failed; previous live mapping retained";}
 auto revision=music_requested.load();if(!revision||revision==music_started||music_busy.load())return;
 if(!active||!ssc_music::attached){music_started=revision;return;}
 struct Job {Entry target,source;std::string key;int percent=100;};std::vector<Job> jobs;
 std::vector<ssc_sound_pool::Clip> music_clips;auto configured=pools.find(music_pool_key);
 if(configured!=pools.end()&&!original_groups.count(music_pool_key))music_clips=configured->second;
 int music_percent=pool_volume.count(music_pool_key)?pool_volume.at(music_pool_key):100;
 std::vector<std::filesystem::path> targets;for(const auto& e:entries)if(e.music)targets.push_back(e.path);
 if(music_clips.empty())for(const auto& entry:entries)if(entry.music&&has_replacement(entry)){auto key=replacement_key(entry);auto source=std::find_if(entries.begin(),entries.end(),[&](const Entry& e){return e.music&&e.key==key&&e.imported;});if(source!=entries.end())jobs.push_back({entry,*source,key,clip_volume(entry)});}
 auto directory=folder;HMODULE module=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&tick_music_apply),&module))return;
 music_started=revision;music_busy=true;
 try{std::thread([jobs=std::move(jobs),music_clips=std::move(music_clips),targets=std::move(targets),music_percent,directory,revision]{SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);ssc_music::Redirects redirects;ssc_music::Playlists lists;bool ok=true;
  try{
   auto cache_wave=[&](const ssc_audio::Wave& wave){auto digest=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(digest.empty())throw std::runtime_error("Music hash failed");auto cache=directory/L"music-cache";std::filesystem::create_directories(cache);auto destination=cache/(digest+".ogg");bool valid=false;if(std::filesystem::is_regular_file(destination)){try{ssc_audio::ogg_info(destination);valid=true;}catch(...) {}}
    if(!valid){auto temporary=cache/(digest+".live.tmp");ssc_audio::write_ogg(temporary,wave);if(!MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Music cache commit failed");}return destination.wstring();};
   std::vector<std::wstring> files;size_t bytes=0;
   for(const auto& clip:music_clips){if(music_requested.load()!=revision)throw std::runtime_error("Music update superseded");auto n=std::filesystem::file_size(directory/clip.file);if(n>music_disk_limit-bytes)throw std::runtime_error("Music list exceeds 512 MiB");bytes+=n;auto wave=gained(ssc_audio::read(directory/clip.file),music_percent);files.push_back(cache_wave(wave));}
   if(!files.empty())for(const auto& target:targets)lists.emplace(ssc_music::normalized(target),files);
   for(const auto& job:jobs){if(music_requested.load()!=revision)throw std::runtime_error("Music update superseded");auto wave=ssc_audio::read(directory/(job.key+".wav"));wave=ssc_audio::convert_import(wave,ssc_audio::Wave{job.target.channels,job.target.rate,16,{}});
    if(job.key!=job.target.key){auto source=ssc_audio::read_ogg(job.source.path),target=ssc_audio::read_ogg(job.target.path);wave=music_track_gain(std::move(wave),ssc_audio::rms(ssc_audio::decode(source)),ssc_audio::rms(ssc_audio::decode(target)));}
    redirects.emplace(ssc_music::normalized(job.target.path),cache_wave(gained(std::move(wave),job.percent)));
   }
  }catch(...){ok=false;}
  if(ok&&music_requested.load()==revision)ssc_music::publish(std::move(redirects),std::move(lists));else ok=false;
  std::atomic_store(&music_result,std::make_shared<const MusicResult>(MusicResult{revision,ok}));music_busy=false;
 }).detach();status=L"Preparing music in background; applies at the next track change";}catch(...){music_busy=false;status=L"Could not prepare live music update";}
}

inline void set_volume(int percent){if(selection>=entries.size())return;if(pool_count(entries[selection]))pool_volume[pool_key(entries[selection])]=std::clamp(percent,0,300);else volume[replacement_key(entries[selection])]=std::clamp(percent,0,300);status=save_options()?L"Volume saved; restart game to apply":L"Could not save sound options";if(entries[selection].music)request_music_apply();}
inline void toggle_variants(){if(selection>=entries.size())return;auto& e=entries[selection];auto f=variant_group(e);if(f.empty())return;if(all_variants(e))families.erase(f);else if(e.imported)families[f]=e.key;else return;status=save_options()?L"Variant choice saved; restart game to apply":L"Could not save sound options";if(e.music)request_music_apply();}
inline void initialize(const std::filesystem::path& game,const std::filesystem::path& state,bool enabled){
    set_enabled(enabled);folder=state/L"sounds";ssc_sound_pool::clear_prepared();music_levels.clear();entries.clear();replacements.clear();ssc_music::redirects.clear();
    std::error_code ec;
    for(const auto& file:std::filesystem::directory_iterator(game/L"data/sounds",ec)){
        if(file.path().extension()==L".ogg"){
         if(!used_music(file.path()))continue;
         try{auto info=ssc_audio::ogg_info(file.path());std::ifstream in(file.path(),std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(in)),{});
          auto key=fingerprint(bytes.data(),bytes.size(),0x4f4747,info.rate);if(key.empty())continue;
          Entry e{file.path(),readable(file.path()),key,info.rate,info.channels,16,false,true,info.seconds};
          e.imported=std::filesystem::is_regular_file(folder/(key+".wav"),ec);entries.push_back(e);
         }catch(const std::exception&){}continue;
        }
        if(file.path().extension()!=L".wav")continue;
        try{auto wave=ssc_audio::read(file.path());if(wave.bits!=8&&wave.bits!=16)continue;
            auto key=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(key.empty())continue;
            Entry entry{file.path(),readable(file.path()),key,wave.rate,wave.channels,wave.bits,false,false,double(wave.raw.size())/(wave.rate*wave.channels*(wave.bits/8))};
            entry.imported=std::filesystem::is_regular_file(folder/(key+".wav"),ec);entries.push_back(entry);
        }catch(const std::exception&){}
    }
    std::sort(entries.begin(),entries.end(),[](const Entry& a,const Entry& b){return a.label<b.label;});
    rebuild_group_catalog();
    load_options();
    const auto scope_repairs=active?repair_legacy_pool_scope():0;
    const auto migrated=active?migrate_saved_pools():0;
    size_t pool_bytes=0;bool pool_errors=false;
    if(active)for(const auto& config:pools){if(config.first==music_pool_key||original_groups.count(config.first))continue;std::vector<ssc_audio::Wave> waves;size_t bytes=0;for(const auto& clip:config.second){try{auto wave=ssc_audio::read(folder/clip.file);if(wave.bits!=16||wave.raw.size()>ssc_sound_pool::max_pool_bytes-bytes||wave.raw.size()>ssc_sound_pool::max_total_bytes-pool_bytes)throw std::runtime_error("Sound pool memory limit");bytes+=wave.raw.size();pool_bytes+=wave.raw.size();auto v=pool_volume.find(config.first);waves.push_back(gained(std::move(wave),v==pool_volume.end()?100:v->second));}catch(...){pool_errors=true;}}if(!waves.empty())ssc_sound_pool::prepared.emplace(config.first,std::move(waves));}
    for(const auto& e:entries)if(ssc_sound_pool::prepared.count(pool_key(e)))ssc_sound_pool::native_keys[e.key]=pool_key(e);
    for(auto& e:entries){if(!active||!has_replacement(e)||ssc_sound_pool::native_keys.count(e.key))continue;
        try{if(e.music){
         if(pool_count(e))continue;
         auto wave=effective_wave(e);auto hash=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(hash.empty())continue;
         auto cache=folder/L"music-cache";std::filesystem::create_directories(cache);auto dest=cache/(hash+".ogg");
         bool valid=false;if(std::filesystem::is_regular_file(dest)){try{ssc_audio::ogg_info(dest);valid=true;}catch(...) {}}
         if(!valid){auto temp=cache/(hash+".tmp");ssc_audio::write_ogg(temp,wave);if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not save music cache");}
         ssc_music::redirects.emplace(ssc_music::normalized(e.path),dest.wstring());
        }else replacements.emplace(e.key,effective_wave(e));}catch(const std::exception&){}
    }
    // Keep native PCM intact for single-file imports too, so OFF can restore
    // them immediately through the same source-play path as random pools.
    for(const auto& item:replacements){if(item.second.raw.size()>ssc_sound_pool::max_total_bytes-pool_bytes){pool_errors=true;continue;}pool_bytes+=item.second.raw.size();auto key="clip:"+item.first;ssc_sound_pool::prepared[key]={item.second};ssc_sound_pool::native_keys[item.first]=key;}
    ssc_music::publish(ssc_music::redirects);
    if(active&&pools.count(music_pool_key))request_music_apply();
    if(!event_migration_failed)status=pool_errors?L"Some pool clips could not load; originals remain available":scope_repairs?L"Unintended category-wide playback stopped; saved individual choices restored.":migrated?L"Saved custom effects are now in shared sound lists; originals kept.":L"Changes apply after restarting the game";
}
inline void __cdecl buffer_data(unsigned buffer,int fmt,const void* bytes,int length,int rate){
    try{if(!ssc_sound_pool::native_keys.empty()&&bytes&&length>0&&size_t(length)<=ssc_audio::limit&&fmt>=0x1100&&fmt<=0x1103){
        auto key=fingerprint(bytes,length,fmt,rate);ssc_sound_pool::register_buffer(buffer,key);
    }}catch(...){}
    // Never bake replacements into native buffers: the module's OFF switch
    // and cleared pools must always have the original PCM available.
    original(buffer,fmt,bytes,length,rate);
}
inline void import_selected(HWND owner,bool match){
    stop_preview();
    if(selection>=entries.size())return;
    const auto& e=entries[selection];
    std::wstring shared;
    for(const auto& other:entries)if(other.key==e.key){if(!shared.empty())shared+=L", ";shared+=other.label;}
    size_t count=std::count_if(entries.begin(),entries.end(),[&](const Entry& other){return other.key==e.key;});
    if(count>1&&MessageBoxW(owner,(L"These sounds share identical audio: " + shared + L".\n\nImporting replaces all of them. Continue?").c_str(),L"Shared sound",MB_OKCANCEL|MB_ICONINFORMATION)!=IDOK)return;
    wchar_t selected[32768]={};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=owner;dialog.lpstrFilter=L"Audio files\0*.wav;*.mp3\0WAV audio\0*.wav\0MP3 audio\0*.mp3\0\0";dialog.lpstrFile=selected;dialog.nMaxFile=32768;dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;dialog.lpstrTitle=L"Import replacement audio";
    if(!GetOpenFileNameW(&dialog))return;
    try{
        auto reference=original_wave(e),source=ssc_audio::read_import(selected,reference);auto prepared=ssc_audio::prepare(reference,source,match);
        std::filesystem::create_directories(folder);auto dest=folder/(e.key+".wav"),temp=folder/(e.key+".tmp");
        // Imports always target our private directory, never the original or selected source.
        if(std::filesystem::equivalent(e.path,selected))throw std::runtime_error("Choose your replacement, not the original game sound");
        ssc_audio::write(temp,prepared.wave);if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not commit imported sound");
        for(auto& other:entries)if(other.key==e.key)other.imported=true;
        status=prepared.limited?L"Imported; peak limited. Restart game to apply.":L"Imported. Restart game to apply.";if(e.music)request_music_apply();
    }catch(const std::exception& ex){std::string message=ex.what();status.assign(message.begin(),message.end());}
}
// Music is prepared one track at a time and streamed by the native decoder.
// Its disk budget never consumes the OpenAL sound-effect pool budget.
inline bool add_music_files(const std::vector<std::filesystem::path>& paths,bool match){
 if(selection>=entries.size()||!entries[selection].music||paths.empty())return false;
 const auto before=pools;const auto originals_before=original_groups;auto clips=pools[music_pool_key];
 size_t total=0;for(const auto& pool:pools)total+=pool.second.size();
 if(paths.size()>ssc_sound_pool::max_clips-clips.size()||paths.size()>ssc_sound_pool::max_total_clips-total){pools=before;status=L"List limit: 32 tracks / 128 total sounds";return false;}
 try{
  size_t bytes=0;for(const auto& clip:clips){auto n=std::filesystem::file_size(folder/clip.file);if(n>music_disk_limit-bytes)throw std::runtime_error("Music list exceeds 512 MiB");bytes+=n;}
  const auto reference=original_wave(entries[selection]);std::filesystem::create_directories(folder);
  for(const auto& path:paths){auto source=ssc_audio::read_import(path,reference);auto wave=ssc_audio::prepare(reference,source,match).wave;
   auto digest=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(digest.size()!=64)throw std::runtime_error("Could not identify imported music");auto name="pool-"+digest+".wav";
   if(std::any_of(clips.begin(),clips.end(),[&](const ssc_sound_pool::Clip& c){return c.file==name;}))continue;
   if(wave.raw.size()+44>music_disk_limit-bytes){throw std::runtime_error("Music list exceeds 512 MiB");}bytes+=wave.raw.size()+44;
   auto dest=folder/name;if(!std::filesystem::exists(dest)){auto temp=folder/(name+".tmp");ssc_audio::write(temp,wave);if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not save music file");}
   auto label=path.filename().u8string();if(label.size()>320)label="Imported music";clips.push_back({name,label});
  }
  pools[music_pool_key]=std::move(clips);original_groups.erase(music_pool_key);if(!save_options())throw std::runtime_error("Could not save music list");pool_choice=pool_page=0;request_music_apply();status=L"Music list saved; equal chance at each track change.";return true;
 }catch(const std::exception& e){pools=before;original_groups=originals_before;auto message=std::string(e.what());status.assign(message.begin(),message.end());return false;}
}
inline bool add_pool_files(const std::vector<std::filesystem::path>& paths,bool match){
 if(selection>=entries.size()||paths.empty())return false;
 if(entries[selection].music)return add_music_files(paths,match);
 const auto& entry=entries[selection];auto key=pool_key(entry);auto before=pools;auto originals_before=original_groups;auto clips=pools[key];size_t total=0;for(const auto& p:pools)total+=p.second.size();
 if(paths.size()>ssc_sound_pool::max_clips-clips.size()||paths.size()>ssc_sound_pool::max_total_clips-total){status=L"Pool limit: 32 per category / 128 total";pools=before;return false;}
 try{
  auto reference=original_wave(entry);std::vector<std::pair<std::string,ssc_audio::Wave>> staged;size_t bytes=0,total_bytes=0;
  for(const auto& configured:pools)if(configured.first!=music_pool_key)for(const auto& clip:configured.second){auto size=std::filesystem::file_size(folder/clip.file);if(size>ssc_sound_pool::max_total_bytes-total_bytes)throw std::runtime_error("All pools exceed 128 MiB");total_bytes+=size;}
  for(const auto& clip:clips){auto prior=ssc_audio::read(folder/clip.file);if(prior.raw.size()>ssc_sound_pool::max_pool_bytes-bytes)throw std::runtime_error("Pool exceeds 32 MiB");bytes+=prior.raw.size();}
  for(const auto& path:paths){auto source=ssc_audio::read_import(path,reference);auto wave=ssc_audio::prepare(reference,source,match).wave;if(wave.raw.size()>ssc_sound_pool::max_pool_bytes-bytes)throw std::runtime_error("Pool exceeds 32 MiB");auto hash=fingerprint(wave.raw.data(),wave.raw.size(),format(wave),wave.rate);if(hash.size()!=64)throw std::runtime_error("Could not identify imported audio");auto name="pool-"+hash+".wav";if(std::any_of(clips.begin(),clips.end(),[&](const ssc_sound_pool::Clip& clip){return clip.file==name;}))continue;
   if(wave.raw.size()+44>ssc_sound_pool::max_total_bytes-total_bytes){throw std::runtime_error("All pools exceed 128 MiB");}total_bytes+=wave.raw.size()+44;bytes+=wave.raw.size();auto label=path.filename().u8string();if(label.size()>320)label="Imported sound";clips.push_back({name,label});staged.emplace_back(name,std::move(wave));
  }
  std::filesystem::create_directories(folder);
  for(const auto& file:staged){auto destination=folder/file.first;if(std::filesystem::exists(destination))continue;auto temporary=folder/(file.first+".tmp");ssc_audio::write(temporary,file.second);if(!MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not save pool audio");}
  pools[key]=std::move(clips);original_groups.erase(key);if(!save_options())throw std::runtime_error("Could not save sound pool");pool_choice=pool_count(entry)?pool_count(entry)-1:0;pool_page=pool_choice/6;status=L"Pool saved. Each sound has an equal chance; restart to apply.";return true;
 }catch(const std::exception& error){pools=std::move(before);original_groups=std::move(originals_before);auto text=std::string(error.what());status.assign(text.begin(),text.end());return false;}
}
inline void import_pool(HWND owner,bool match){
 stop_preview();
 if(selection>=entries.size())return;
 std::vector<wchar_t> names(65536,0);OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=owner;dialog.lpstrFilter=L"Audio files\0*.wav;*.mp3\0WAV audio\0*.wav\0MP3 audio\0*.mp3\0\0";dialog.lpstrFile=names.data();dialog.nMaxFile=DWORD(names.size());dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER|OFN_ALLOWMULTISELECT;dialog.lpstrTitle=L"Add sounds to the random pool";if(!GetOpenFileNameW(&dialog))return;
 std::filesystem::path first(names.data());const wchar_t* p=names.data()+wcslen(names.data())+1;std::vector<std::filesystem::path> paths;if(!*p)paths.push_back(first);else while(*p&&paths.size()<=ssc_sound_pool::max_clips){paths.push_back(first/p);p+=wcslen(p)+1;}add_pool_files(paths,match);
}
inline void remove_pool_clip(bool all=false){
 if(selection>=entries.size()){return;}auto key=pool_key(entries[selection]);auto it=pools.find(key);if(it==pools.end())return;auto before=pools;auto migrated_before=pool_migrations;auto originals_before=original_groups;pool_migrations.insert(key);
 if(all)pools.erase(it);else if(pool_choice<it->second.size()){it->second.erase(it->second.begin()+pool_choice);if(it->second.empty())pools.erase(it);}else return;
 if(!pools.count(key)&&automatic_group(family(entries[selection])))original_groups.insert(key);
 if(!save_options()){pools=std::move(before);pool_migrations=std::move(migrated_before);original_groups=std::move(originals_before);status=L"Could not save sound pool";return;}
 ssc_sound_pool::disable_pool(key);pool_choice=pool_page=0;if(entries[selection].music){request_music_apply();status=L"Music list saved; applies at the next track change.";}else status=L"Pool stopped now. Restart to load the updated list; files retained.";
}
inline void preview_pool_clip(){
 stop_preview();if(selection>=entries.size())return;auto it=pools.find(pool_key(entries[selection]));if(it==pools.end()||it->second.empty())return;
 if(pool_choice>=it->second.size())pool_choice=0;
 try{auto wave=ssc_audio::read(folder/it->second[pool_choice].file);auto v=pool_volume.find(it->first);wave=gained(std::move(wave),v==pool_volume.end()?100:v->second);auto file=folder/L"preview-pool.wav";ssc_audio::write(file,wave);status=play_preview(file)?L"Playing selected pool sound":L"Could not preview pool sound";}catch(...){status=L"Could not preview pool sound";}
}
inline void preview_original(){
 stop_preview();if(selection>=entries.size())return;
 auto path=entries[selection].path;
 try{if(entries[selection].music){std::filesystem::create_directories(folder);path=folder/L"preview-original.wav";ssc_audio::write(path,original_wave(entries[selection]));}}catch(...){status=L"Could not decode music preview";return;}
 status=play_preview(path)?L"Playing original: "+entries[selection].label:L"Could not preview original sound";
}
inline void preview_custom(HWND owner){
 stop_preview();if(selection>=entries.size()||!has_replacement(entries[selection]))return;
 if(pool_count(entries[selection])){preview_pool_clip();return;}
 try{auto wave=effective_wave(entries[selection]);auto file=folder/L"preview.wav";ssc_audio::write(file,wave);if(!play_preview(file))throw std::runtime_error("Could not preview replacement");status=L"Playing replacement";}catch(...){status=L"Could not preview replacement";}(void)owner;
}
inline void restore_category(){
 if(selection>=entries.size()||(!shared_category(entries[selection])&&!entries[selection].music))return;
 auto key=pool_key(entries[selection]);auto before=pools;auto migrations_before=pool_migrations;auto originals_before=original_groups;
 pools.erase(key);pool_migrations.insert(key);original_groups.insert(key);
 if(!save_options()){pools=std::move(before);pool_migrations=std::move(migrations_before);original_groups=std::move(originals_before);status=L"Could not save original sound choice";return;}
 ssc_sound_pool::disable_pool(key);pool_choice=pool_page=0;if(entries[selection].music)request_music_apply();status=L"Original category restored; saved audio files kept.";
}
inline void remove_selected(){
    if(selection>=entries.size()||entries[selection].key.empty())return;
    std::error_code ec;
    std::filesystem::remove(folder/(entries[selection].key+".wav"),ec);
    if(ec){status=L"Could not remove imported sound";return;}for(auto& other:entries)if(other.key==entries[selection].key)other.imported=false;auto group=variant_group(entries[selection]);families.erase(group);for(auto it=families.begin();it!=families.end();)if(it->second==entries[selection].key)it=families.erase(it);else ++it;volume.erase(entries[selection].key);save_options();ssc_sound_pool::disable_pool("clip:"+entries[selection].key);status=L"Original restored";if(entries[selection].music)request_music_apply();
}
}
