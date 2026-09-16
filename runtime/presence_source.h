#pragma once
#include "presence_module.h"
#include "cosmetic_adapter.h"

namespace ssc_rpc {
struct RankedFields {
    int mode=0,round=0,kind=-1,login=0,eligibility=0,account_level=0;
    unsigned char active=0,survival=0,singleplayer=0,disabled=0,practice=0,newbie=0,authenticated=0,committed=0;
    uint64_t name_length=0;float bonus=0,normal_threshold=0,bonus_threshold=0;
};
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
inline void sample_rating(uintptr_t base,Snapshot& snapshot){
    if(snapshot.phase!=2)return;
    using ssc_names::read;uintptr_t world=0,first=0,last=0,steam_first=0;int local=-1,slot=-1;
    if(!read(base+0xdda5d0,world)||!world||!read(world+0xa5b8,first)||!read(world+0xa5c0,last)||!first||last<first||(last-first)%0x3460||(last-first)/0x3460>2048
        ||!read(base+0xe11de8,steam_first)||first!=steam_first||!read(base+0xe0e380,local)||local<0||uintptr_t(local)>=(last-first)/0x3460)return;
    auto actor=first+uintptr_t(local)*0x3460;if(!read(actor+0x78,slot)||slot!=local)return;
    RankedFields f;
    if(!read(world+0x128,f.mode)||!read(world+0x3e0,f.round)||!read(world+0x485,f.survival)||!read(world+0x2d2,f.singleplayer)
        ||!read(world+0x2f4,f.disabled)||!read(world+0x121,f.practice)||!read(world+0x30f,f.newbie)
        ||!read(actor+0x81,f.active)||!read(actor+0x7c4,f.kind)||!read(actor+0x6e8,f.name_length)||!read(actor+0x6c8,f.login)
        ||!read(actor+0x6cc,f.authenticated)||!read(actor+0x270,f.committed)||!read(actor+0xf80,f.bonus)
        ||!read(actor+0xa9c,f.account_level)||!read(actor+0x204,f.eligibility)
        ||!read(base+0xfa6684,f.normal_threshold)||!read(base+0xfa6688,f.bonus_threshold)||!ranked(f))return;
    // Values used by the native own-profile "RATING THIS MONTH" display at 5f3a5f.
    float rating=-1,matches=-1;int required=0,profile_loaded=0;
    if(read(base+0xdf4040,profile_loaded)&&profile_loaded&&read(base+0xdf48e4,rating)&&read(base+0xdf48e0,matches)&&read(base+0xfa6658,required))add_rating(snapshot,true,rating,matches,required);
    else add_rating(snapshot,true,-1,-1,0);
}
inline std::string copy_steam_text(const char* value){
    if(!value)return {};
    std::string s;for(size_t i=0;i<256;++i){char c;if(!ssc_names::read(reinterpret_cast<uintptr_t>(value)+i,c))return {};if(!c)return s;s+=c;}return {};
}
inline std::string native_string(uintptr_t address){
    using ssc_names::read;size_t length=0,capacity=0;uintptr_t data=address;
    if(!read(address+16,length)||!read(address+24,capacity)||length>128||capacity<length)return {};
    if(capacity>=16&&(!read(address,data)||!data))return {};
    std::string value;for(size_t i=0;i<length;++i){char c;if(!read(data+i,c))return {};value+=c;}return utf8_limit(value);
}
inline Snapshot native_fallback(uintptr_t base){
    using ssc_names::read;Snapshot s;int session=0;double joining=0;
    if(!read(base+0xfa235c,session)||!read(base+0xfa2370,joining)||!std::isfinite(joining))return s;
    if(session==1)return from_steam("#Status_AtMainMenu","");
    if(session!=2)return s;
    if(joining>0){s.details="Joining game";s.state="Loading";s.phase=3;return s;}
    uintptr_t world=0;if(!read(base+0xdda5d0,world)||!world)return s;
    int mode=-1,round=0,team_size=0;unsigned char survival=0,singleplayer=0;
    if(!read(world+0x128,mode)||!read(world+0x3e0,round)||!read(world+0x480,team_size)||!read(world+0x485,survival)||!read(world+0x2d2,singleplayer))return s;
    s.phase=2;s.state="In game";
    switch(mode){case 0:s.details="FFA Deathmatch";break;case 1:s.details="Capture the Flag";break;case 2:s.details="Checkpoint Racing";break;case 4:s.details="Team Conquest";break;case 5:s.details="Survival Challenge";break;
        case 6:if(survival&&!singleplayer)s.details="Survival Challenge";else s.details=team_size==1?"BR Solos":team_size==2?"BR Duos":"BR Trios";
            if(round>=2&&round<=100)s.details+=" - Round "+std::to_string(round-1);else s.state="Preparing round";break;
        default:s.details="Skillshot City";break;}
    uintptr_t first=0,last=0,steam_first=0;int local=-1,slot=-1,kind=-1,level=-1,class_id=-1;unsigned char active=0;
    if(!read(world+0xa5b8,first)||!read(world+0xa5c0,last)||!first||last<first||(last-first)%0x3460||(last-first)/0x3460>2048
        ||!read(base+0xe11de8,steam_first)||first!=steam_first||!read(base+0xe0e380,local)||local<0||uintptr_t(local)>=(last-first)/0x3460)return s;
    auto actor=first+uintptr_t(local)*0x3460;
    if(!read(actor+0x78,slot)||slot!=local||!read(actor+0x7c4,kind)||kind!=0||!read(actor+0x81,active)||!active||!read(actor+0x858,level)||level<0||level>10000)return s;
    s.state="Level "+std::to_string(level);
    uintptr_t classes=0,end=0;
    if(read(actor+0x350,class_id)&&class_id>=0&&read(base+0xe17de8,classes)&&read(base+0xe17df0,end)&&classes&&end>=classes&&(end-classes)%0x98==0&&(end-classes)/0x98<=512&&uintptr_t(class_id)<(end-classes)/0x98){
        auto label=native_string(classes+uintptr_t(class_id)*0x98+0x18);if(!label.empty())s.state=label+" - "+s.state;
    }
    return s;
}
// Called only on the game/render thread, after the exact executable hash gate.
// No Init, Shutdown, RunCallbacks, SetRichPresence, friend enumeration or game memory writes.
inline Snapshot sample_steam(bool show_rating=true){
    auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));int initialized=0;
    auto fallback=native_fallback(base);if(show_rating)sample_rating(base,fallback);
    if(!ssc_names::read(base+0xfa1164,initialized)||initialized<=0)return fallback;
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
