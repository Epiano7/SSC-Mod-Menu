#pragma once
#include "compatibility.h"
#include "presence_module.h"
#include "postgame_context.h"
#include "cosmetic_adapter.h"

namespace ssc_rpc {
struct RankedFields {
    int mode=0,round=0,kind=-1,login=0,eligibility=0,account_level=0;
    unsigned char active=0,survival=0,singleplayer=0,disabled=0,practice=0,newbie=0,authenticated=0,committed=0;
    uint64_t name_length=0;float bonus=0,normal_threshold=0,bonus_threshold=0;
};
// Diagnostic bits describe the existing native predicate; they grant no eligibility.
inline uint32_t ranked_blockers(const RankedFields& f){
 uint32_t bits=0;
 if(!f.active||f.kind!=0)bits|=1;
 if(f.mode!=6||f.survival||f.singleplayer||f.disabled||f.practice||f.newbie)bits|=2;
 if(!f.name_length||f.name_length>1024)bits|=4;
 if(f.login&&!f.authenticated)bits|=8;
 if(f.round>=2&&!f.committed)bits|=16;
 if(!std::isfinite(f.normal_threshold)||!std::isfinite(f.bonus_threshold)||!std::isfinite(f.bonus)||f.normal_threshold<0||f.normal_threshold>1000000||f.bonus_threshold<0||f.bonus_threshold>1000000)bits|=32;
 else if(f.account_level<int(f.bonus>0?f.bonus_threshold:f.normal_threshold))bits|=64;
 if(f.eligibility<=1)bits|=128;
 return bits;
}
inline bool ranked(const RankedFields& f){
    // Native Tab label predicate 7f8720(actor, false), restricted to the local human.
    return f.active&&f.kind==0&&f.mode==6&&!f.survival&&!f.singleplayer&&!f.disabled&&!f.practice&&!f.newbie
        &&f.name_length>0&&f.name_length<=1024&&(!f.login||f.authenticated)&&(f.round<2||f.committed)
        &&std::isfinite(f.normal_threshold)&&std::isfinite(f.bonus_threshold)&&std::isfinite(f.bonus)
        &&f.normal_threshold>=0&&f.normal_threshold<=1000000&&f.bonus_threshold>=0&&f.bonus_threshold<=1000000
        &&f.account_level>=int(f.bonus>0?f.bonus_threshold:f.normal_threshold)&&f.eligibility>1;
}
inline void add_rating(Snapshot& s,bool is_ranked,float rating,float matches,int required){
    if(s.phase!=2||!is_ranked)return;
    std::string label="Ranked";
    if(std::isfinite(matches)&&matches>=0&&required>0&&required<1000&&matches<required)label+=" | Placement matches";
    else if(std::isfinite(matches)&&matches>=required&&required>0&&required<1000&&std::isfinite(rating)&&rating>=0&&rating<=1000000)label+=" | SSC rating: "+std::to_string(int(rating));
    if(s.state!="In a round")label+=" | "+s.state;
    s.state=utf8_limit(label);
}
inline void sample_rating(uintptr_t base,Snapshot& snapshot,ssc_review_context::Rating* observation=nullptr){
    if(observation)*observation={};
    if(snapshot.phase!=2)return;
    using ssc_names::read;
    // Profile history and live ranked eligibility are separate observations.
    // Freeze the former even when match eligibility is not yet available.
    float profile_rating=-1,profile_matches=-1;int profile_required=0,profile_loaded=0;
    const bool profile_valid=read(base+ssc_compat::resolve(0xe031f8),profile_loaded)&&profile_loaded&&
        read(base+ssc_compat::resolve(0xe03a9c),profile_rating)&&read(base+ssc_compat::resolve(0xe03a98),profile_matches)&&
        read(base+ssc_compat::resolve(0xfb5a38),profile_required)&&ssc_review_context::valid_rating(profile_rating,profile_matches,profile_required);
    if(observation&&profile_valid)*observation={true,false,profile_matches<profile_required,profile_rating,profile_matches,profile_required};
    // The actor login field is an authentication state (often 3), not a user ID.
    // Read the loaded account profile's public game ID independently of actors.
    int game_user_id=0;
    auto id_offset=ssc_compat::resolve(0xe02ba8);
    if(observation&&profile_valid&&id_offset&&read(base+id_offset,game_user_id)&&game_user_id>0)observation->player_id=game_user_id;
    uintptr_t world=0,first=0,last=0,steam_first=0;int local=-1,slot=-1;
    if(!read(base+ssc_compat::resolve(0xde95d0),world)||!world||!read(world+0xa650,first)||!read(world+0xa658,last)||!first||last<first||(last-first)%0x34b0||(last-first)/0x34b0>2048
        ||!read(base+ssc_compat::resolve(0xe21140),steam_first)||first!=steam_first||!read(base+ssc_compat::resolve(0xe1d688),local)||local<0||uintptr_t(local)>=(last-first)/0x34b0)return;
    auto actor=first+uintptr_t(local)*0x34b0;if(!read(actor+0x78,slot)||slot!=local)return;
    RankedFields f;
    if(!read(world+0x128,f.mode)||!read(world+0x3e0,f.round)||!read(world+0x485,f.survival)||!read(world+0x2d2,f.singleplayer)
        ||!read(world+0x2f4,f.disabled)||!read(world+0x121,f.practice)||!read(world+0x30f,f.newbie)
        ||!read(actor+0x81,f.active)||!read(actor+0x7dc,f.kind)||!read(actor+0x700,f.name_length)||!read(actor+0x6e0,f.login)
        ||!read(actor+0x6e4,f.authenticated)||!read(actor+0x270,f.committed)||!read(actor+0xf98,f.bonus)
        ||!read(actor+0xab4,f.account_level)||!read(actor+0x204,f.eligibility)
        ||!read(base+ssc_compat::resolve(0xfb5a64),f.normal_threshold)||!read(base+ssc_compat::resolve(0xfb5a68),f.bonus_threshold))return;
    if(observation&&profile_valid){observation->rank_blockers=ranked_blockers(f);}
    if(!ranked(f))return;
    add_rating(snapshot,true,profile_rating,profile_matches,profile_required);
    if(observation&&profile_valid)observation->ranked=true;
}
inline std::string copy_steam_text(const char* value){
    if(!value)return {};
    std::string s;for(size_t i=0;i<256;++i){char c;if(!ssc_names::read(reinterpret_cast<uintptr_t>(value)+i,c))return {};if(!c)return s;s+=c;}return {};
}
inline std::string native_string(uintptr_t address){
    std::array<unsigned char,32> header{};size_t length=0,capacity=0;uintptr_t data=0;
    if(!ssc_names::read_bytes(address,header.data(),header.size()))return {};
    std::memcpy(&length,header.data()+16,8);std::memcpy(&capacity,header.data()+24,8);
    if(length>128||capacity<length)return {};
    if(capacity<16)return utf8_limit(std::string(reinterpret_cast<const char*>(header.data()),length));
    if(!length)return {};
    std::memcpy(&data,header.data(),8);std::string value(length,'\0');
    if(!ssc_names::read_bytes(data,value.data(),length))return {};
    return utf8_limit(value);
}
inline Snapshot native_fallback(uintptr_t base){
    using ssc_names::read;Snapshot s;int session=0;double joining=0;
    if(!read(base+ssc_compat::resolve(0xfb171c),session)||!read(base+ssc_compat::resolve(0xfb1730),joining)||!std::isfinite(joining))return s;
    if(session==1)return from_steam("#Status_AtMainMenu","");
    if(session!=2)return s;
    if(joining>0){s.details="Joining game";s.state="Loading";s.phase=3;return s;}
    uintptr_t world=0;if(!read(base+ssc_compat::resolve(0xde95d0),world)||!world)return s;
    int mode=-1,round=0,team_size=0;unsigned char survival=0,singleplayer=0;
    if(!read(world+0x128,mode)||!read(world+0x3e0,round)||!read(world+0x480,team_size)||!read(world+0x485,survival)||!read(world+0x2d2,singleplayer))return s;
    s.phase=2;s.state="In game";
    switch(mode){case 0:s.details="FFA Deathmatch";break;case 1:s.details="Capture the Flag";break;case 2:s.details="Checkpoint Racing";break;case 4:s.details="Team Conquest";break;case 5:s.details="Survival Challenge";break;
        case 6:if(survival&&!singleplayer)s.details="Survival Challenge";else s.details=team_size==1?"BR Solos":team_size==2?"BR Duos":"BR Trios";
            if(round>=2&&round<=100)s.details+=" - Round "+std::to_string(round-1);else s.state="Preparing round";break;
        default:s.details="Skillshot City";break;}
    uintptr_t first=0,last=0,steam_first=0;int local=-1,slot=-1,kind=-1,level=-1,class_id=-1;unsigned char active=0;
    if(!read(world+0xa650,first)||!read(world+0xa658,last)||!first||last<first||(last-first)%0x34b0||(last-first)/0x34b0>2048
        ||!read(base+ssc_compat::resolve(0xe21140),steam_first)||first!=steam_first||!read(base+ssc_compat::resolve(0xe1d688),local)||local<0||uintptr_t(local)>=(last-first)/0x34b0)return s;
    auto actor=first+uintptr_t(local)*0x34b0;
    if(!read(actor+0x78,slot)||slot!=local||!read(actor+0x7dc,kind)||kind!=0||!read(actor+0x81,active)||!active||!read(actor+0x870,level)||level<0||level>10000)return s;
    s.state="Level "+std::to_string(level);
    uintptr_t classes=0,end=0;
    if(read(actor+0x350,class_id)&&class_id>=0&&read(base+ssc_compat::resolve(0xe16a98),classes)&&read(base+ssc_compat::resolve(0xe16aa0),end)&&classes&&end>=classes&&(end-classes)%0x98==0&&(end-classes)/0x98<=512&&uintptr_t(class_id)<(end-classes)/0x98){
        auto label=native_string(classes+uintptr_t(class_id)*0x98+0x18);if(!label.empty())s.state=label+" - "+s.state;
    }
    return s;
}
// Called only on the game/render thread, after native dependency validation.
// No Init, Shutdown, RunCallbacks, SetRichPresence, friend enumeration or game memory writes.
inline Snapshot sample_steam(bool show_rating=true){
    auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));int initialized=0;
    auto fallback=native_fallback(base);if(show_rating)sample_rating(base,fallback);
    if(!ssc_names::read(base+ssc_compat::resolve(0xfb04fc),initialized)||initialized<=0)return fallback;
    auto dll=GetModuleHandleW(L"steam_api64.dll");if(!dll)return fallback;
    using Interface=void*(*)();using OwnId=uint64_t(*)(void*);using Presence=const char*(*)(void*,uint64_t,const char*);
    auto friends=reinterpret_cast<Interface>(GetProcAddress(dll,"SteamAPI_SteamFriends_v017"));
    auto user=reinterpret_cast<Interface>(GetProcAddress(dll,"SteamAPI_SteamUser_v023"));
    auto own_id=reinterpret_cast<OwnId>(GetProcAddress(dll,"SteamAPI_ISteamUser_GetSteamID"));
    auto presence=reinterpret_cast<Presence>(GetProcAddress(dll,"SteamAPI_ISteamFriends_GetFriendRichPresence"));
    if(!friends||!user||!own_id||!presence)return fallback;
    auto f=friends(),u=user();if(!f||!u)return fallback;auto id=own_id(u);if(!id)return fallback;
    // These two keys only. Native serverIP, serverPass, connect and username are never read.
    auto display=copy_steam_text(presence(f,id,"steam_display"));
    auto detail=display=="#Status_Server"?copy_steam_text(presence(f,id,"status")):std::string{};
    auto snapshot=from_steam(display,detail);if(!snapshot.phase||(snapshot.phase==2&&detail.empty()))return fallback;
    if(show_rating)sample_rating(base,snapshot);
    return snapshot;
}
}
