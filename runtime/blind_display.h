#pragma once
// Final-frame masks use normalized game viewport coordinates. Draw before the mod menu.
namespace ssc_blind {
inline bool scoreboard_drawn=false;
inline uintptr_t completed_world=0;
inline int completed_round=0;
inline void scoreboard_row(uintptr_t,float,float,float,float){if(ssc_challenge::random.blind)scoreboard_drawn=true;}
struct RoundView {bool match=false,intermission=false;};
inline RoundView current_round(uintptr_t base){
    using ssc_names::read;
    int state=0;
    if(!read(base+ssc_compat::resolve(0xfb7734),state))return {true,true};
    if(state!=2){ssc_challenge::blind_locked=false;completed_world=0;completed_round=0;return {};}
    uintptr_t world=0;int mode=0,raw=0,final=0;float countdown=0,ending=0;
    if(!read(base+ssc_compat::resolve(0xdef5d0),world)||!world||
       !read(world+0x128,mode)||mode!=6||!read(world+0x3e0,raw)||
       !read(world+0x46c,final)||!read(world+0x3c0,countdown)||!read(world+0x6ae8,ending)||
       raw<1||raw>101||final<1||final>100||!std::isfinite(countdown)||!std::isfinite(ending))
        return {true,true};
    int round=raw-1;
    if(completed_world!=world||round<completed_round){completed_world=0;completed_round=0;}
    if(round==final&&ending>0){completed_world=world;completed_round=round;}
    if(completed_world==world){ssc_challenge::blind_locked=false;return {true,true};}
    if(world!=ssc_challenge::session||round==0){
        ssc_challenge::session=world;ssc_challenge::blind_locked=false;
    }
    if(round>0&&(ssc_challenge::random.enabled||ssc_challenge::blind_locked)){
        ssc_challenge::blind_locked=!(round==final&&ending>0)&&round<=final;
    }
    return {round>0,ending>0||countdown>0};
}
// Build information is suppressed at validated native draw calls. No screen-space masks.
inline void draw(int,int,bool,bool,bool){}
}
