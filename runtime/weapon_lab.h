#pragma once
#include "presence_source.h"
#include <limits>
#include "weapon_simulation.h"

namespace ssc_lab {
// Read-only base definitions from the same table as the native Weapons screen.
// No actor enumeration, progression writes, or per-frame sampling.
struct Weapon {
    std::string name;
    int rpm=0,magazine=0;
    float hit=0,reload=0,guard=0,burst_size=0,burst_delay=0,extra_dps=0;
    int projectiles=1,tier=-1,category=-1;
};
struct Metrics {
    double displayed_dps=std::numeric_limits<double>::quiet_NaN(),interval=0;
    double empty=std::numeric_limits<double>::quiet_NaN();
    double sustained=std::numeric_limits<double>::quiet_NaN();
};
inline bool valid(const Weapon& w){
    auto bounded=[](float v,float hi){return std::isfinite(v)&&v>=0&&v<=hi;};
    return !w.name.empty()&&w.name.size()<=128&&w.rpm>=0&&w.rpm<=100000&&w.magazine>=0&&w.magazine<=10000
        &&bounded(w.hit,1e8f)&&bounded(w.reload,3600)&&bounded(w.guard,1000)
        &&bounded(w.burst_size,10000)&&bounded(w.burst_delay,3600)&&bounded(w.extra_dps,1e8f)
        &&(!(w.burst_size>0&&w.burst_delay>0)||w.burst_size<=w.magazine);
}
inline Metrics calculate(const Weapon& w,bool guards=false){
    Metrics m;if(!valid(w)||w.rpm==0||w.magazine==0)return m;
    // Preserve float arithmetic and rounding from the native browser helper.
    float hit=w.hit*(guards?w.guard:1.f);
    float cycle=(60.f/float(w.rpm))*float(w.magazine);
    if(w.burst_size>0&&w.burst_delay>0)cycle+=(float(w.magazine)/w.burst_size-1.f)*w.burst_delay;
    m.displayed_dps=std::floor(double(float(w.magazine)*hit/cycle)+double(w.extra_dps)+.5);
    m.interval=60./w.rpm;
    // Simple magazine model only: first shot at t=0, reload after the last shot.
    // Burst timing and added damage effects need separate validation.
    if(!(w.burst_size>0&&w.burst_delay>0)&&w.extra_dps==0){
        m.empty=(w.magazine-1)*m.interval;
        // A zero reload may denote a consumable or melee item, not infinite ammo.
        if(w.reload>0){double cycle_seconds=m.empty+std::max(double(w.reload),m.interval);
            m.sustained=double(w.magazine)*hit/cycle_seconds;}
    }
    return m;
}
inline std::vector<Weapon> weapons;
inline size_t left=0,right=1,extra_columns[2]={0,1};
inline int comparison_count=2;
inline size_t& column(int i){return i==0?left:i==1?right:extra_columns[std::clamp(i-2,0,1)];}
inline bool guards=false;
inline ShieldRules shield_rules;
inline Target target;
inline int tab=0,editing=-1;
inline size_t history_page=0;
inline std::wstring edit_buffer;
inline bool edit_error=false,allow_reload=true;
inline int damage_percent=100,reload_percent=100;
inline Trial trials[2];
inline unsigned miss_percent[2]={20,20},seed=1;
inline bool miss_enabled[2]={false,false},duel_shown=false;
inline Duel duel_result,duel_complete;
inline bool duel_playing=false;
inline double duel_elapsed=0,duel_visual_time=0;
inline bool duel_animating(){return duel_shown&&(duel_playing||duel_visual_time<duel_elapsed+.12);}
inline void advance_duel(double seconds){
    if(!duel_shown)return;
    seconds=std::max(0.,seconds);
    if(duel_playing){duel_elapsed=std::min(duel_elapsed+seconds,duel_complete.time);
        duel_result=duel_at(trials[0],trials[1],target,duel_elapsed,&duel_complete);
        if(duel_elapsed>=duel_complete.time)duel_playing=false;}
    duel_visual_time=std::min(duel_visual_time+seconds,duel_elapsed+.12);
}
inline size_t shot_cursor[2]={0,0};
inline const wchar_t* exclusion(const Weapon& w){
    if(!valid(w)||w.rpm<=0||w.magazine<=0||w.hit<=0)return L"No damage model";
    if(w.extra_dps>0)return L"Damage-over-time effects not modeled";
    if(w.burst_size>0&&w.burst_delay>0)return L"Burst timing not modeled";
    if(w.projectiles!=1&&!(w.name=="Minigun"&&w.projectiles==3))return L"Multiple impacts not modeled";
    if(w.reload==0)return L"Melee / explosives not modeled";
    if(target.shield>0&&!shield_rules.ready)return L"Shield rules unavailable";
    if(guards&&target.shield>0)return L"Set shield to zero for guards";
    return nullptr;
}
inline void recompute(){
    history_page=0;duel_shown=false;duel_playing=false;
    for(int col=0;col<2;++col){trials[col]={};shot_cursor[col]=0;auto index=col?right:left;
        if(index<weapons.size()&&!exclusion(weapons[index])){const auto& w=weapons[index];
            trials[col]=simulate(target,w.hit*(guards?w.guard:1.f)*(damage_percent/100.f),w.rpm,w.magazine,w.reload*(reload_percent/100.f),allow_reload,shield_rules,miss_enabled[col]?miss_percent[col]:0,seed+unsigned(col)*1234567,w.projectiles);}}
}
inline void start_duel(){recompute();duel_complete=duel(trials[0],trials[1],target);duel_elapsed=duel_visual_time=0;duel_shown=true;duel_playing=true;advance_duel(0);}
inline void begin_edit(int field){editing=field;edit_error=false;edit_buffer.clear();}
inline bool commit_edit(bool recalculate=true){
    if(editing<0)return true;
    if(edit_buffer.empty()){editing=-1;return true;}
    unsigned long value=wcstoul(edit_buffer.c_str(),nullptr,10);
    unsigned long low=editing==1?0:1,high=editing==1?200:1000;
    if(value<low||value>high||(editing==2&&value>target.max_health)){edit_error=true;return false;}
    if(editing==0){bool full=target.health==target.max_health;float ratio=target.shield/target.max_health;target.max_health=float(value);target.health=full?target.max_health:std::min(target.health,target.max_health);target.shield=ratio*target.max_health;}
    if(editing==1)target.shield=target.max_health*float(value)/100.f;
    if(editing==2)target.health=float(value);
    if(editing==3)damage_percent=int(value);
    if(editing==4)reload_percent=int(value);
    editing=-1;edit_error=false;if(recalculate)recompute();return true;
}
inline int picker=-1,sort_mode=0;
inline size_t picker_page=0;
inline std::wstring query;
inline std::wstring wide_name(const std::string& name){int n=MultiByteToWideChar(CP_UTF8,0,name.data(),int(name.size()),nullptr,0);std::wstring out(n,0);MultiByteToWideChar(CP_UTF8,0,name.data(),int(name.size()),out.data(),n);return out;}
inline std::wstring metadata(const Weapon& w){
    const wchar_t* names[]={L"Melee",L"Explosive",L"Pistol",L"SMG",L"Shotgun",L"Rifle",L"LMG",L"Sniper",L"Misc"};
    if(w.category<0||w.category>8)return {};
    return (w.category>=2&&w.tier>=0&&w.tier<=2?L"T"+std::to_wstring(w.tier+1)+L" ":L"")+std::wstring(names[w.category]);
}
inline std::wstring label(const Weapon& w){auto suffix=metadata(w);return wide_name(w.name)+(suffix.empty()?L"":L" ("+suffix+L")");}
inline std::vector<size_t> filtered(){
    auto lower=[](std::wstring text){std::transform(text.begin(),text.end(),text.begin(),towlower);return text;};
    auto find=lower(query);std::vector<size_t> result;
    for(size_t i=0;i<weapons.size();++i)if(lower(label(weapons[i])).find(find)!=std::wstring::npos)result.push_back(i);
    std::stable_sort(result.begin(),result.end(),[&](size_t a,size_t b){
        if(sort_mode==1&&weapons[a].rpm!=weapons[b].rpm)return weapons[a].rpm>weapons[b].rpm;
        if(sort_mode==2){auto x=calculate(weapons[a],guards).displayed_dps,y=calculate(weapons[b],guards).displayed_dps;if(!std::isfinite(x))x=-1;if(!std::isfinite(y))y=-1;if(x!=y)return x>y;}
        return lower(wide_name(weapons[a].name))<lower(wide_name(weapons[b].name));
    });return result;
}
inline void open_picker(int column){picker=column;query.clear();picker_page=0;}
inline std::wstring status=L"Open the lab to read current game definitions.";
inline bool read_weapon(uintptr_t record,Weapon& w){
    using ssc_names::read;
    int damage=0,extra=0;float multiplier=0;
    if(!read(record+0x214,w.tier)||!read(record+0x218,w.category)||w.tier<0||w.tier>2||w.category<0||w.category>8)return false;
    w.name=ssc_rpc::native_string(record+0x48);
    if(!read(record+0x448,damage)||!read(record+0x44c,extra)||!read(record+0x3f0,multiplier)
        ||damage<0||damage>1000000||extra<0||extra>10000||!std::isfinite(multiplier)||multiplier<0||multiplier>10000)return false;
    w.hit=float(damage)*multiplier*float(extra+1);
    if(int64_t(damage)*(extra+1)>10000)return false;
    w.projectiles=damage*(extra+1);
    return read(record+0x458,w.rpm)&&read(record+0x4f0,w.magazine)&&read(record+0x454,w.reload)
        &&read(record+0x410,w.guard)&&read(record+0x480,w.burst_size)&&read(record+0x484,w.burst_delay)
        &&read(record+0x400,w.extra_dps)&&valid(w);
}
inline bool read_catalog(uintptr_t table,std::vector<Weapon>& out){
    using ssc_names::read;uintptr_t first=0,last=0;
    out.clear();
    if(!table||!read(table,first)||!read(table+8,last)||!first||last<first||(last-first)%0x748||(last-first)/0x748>512)return false;
    // Native browser displays the first 25 records with nonnegative grid coordinates.
    for(size_t i=0;i<std::min<size_t>(25,(last-first)/0x748);++i){
        auto record=first+i*0x748;int x=-1,y=-1;
        if(!read(record+0x2a8,x)||!read(record+0x2ac,y)){out.clear();return false;}
        if(x<0||y<0)continue;
        Weapon w;if(!read_weapon(record,w)){out.clear();return false;}out.push_back(std::move(w));
    }
    return !out.empty();
}
inline void refresh(){
    weapons.clear();
    shield_rules={};
    if(!ssc_compat::initialized||!ssc_compat::supports(8)){status=L"Unavailable on this game version.";return;}
    auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    // All consumer functions are fingerprinted, including field layout and formula.
    const uintptr_t dependencies[]={base+ssc_compat::resolve(0x9fda00),base+ssc_compat::resolve(0x1cd8f0),base+ssc_compat::resolve(0x5bdae0),base+ssc_compat::resolve(0x5ebef0),base+ssc_compat::resolve(0x3a7aa0),base+ssc_compat::resolve(0x415f80),base+ssc_compat::resolve(0x7fa090)};
    for(auto address:dependencies)if(address==base){status=L"Weapon definitions unavailable.";return;}
    uintptr_t table=0;
    if(!ssc_names::read(base+ssc_compat::resolve(0xfa1c08),table)||!read_catalog(table,weapons)){status=L"Weapon data not ready. Try Refresh at the main menu.";return;}
    // Read balance globals only when both the damage implementation and its
    // address bindings match. Never substitute guessed shield constants.
    if(ssc_compat::resolve(0x7fa090)&&ssc_names::read(base+ssc_compat::resolve(0xfa5eac),shield_rules.low)
        &&ssc_names::read(base+ssc_compat::resolve(0xfa5eb0),shield_rules.high))
        shield_rules.ready=std::isfinite(shield_rules.low)&&std::isfinite(shield_rules.high)&&shield_rules.low>=0&&shield_rules.high>=0&&shield_rules.low<=100&&shield_rules.high<=100;
    for(int i=0;i<4;++i)column(i)=std::min(column(i),weapons.size()-1);
    if(left==0&&right==1){auto a=std::find_if(weapons.begin(),weapons.end(),[](const Weapon& w){return w.name=="USP Tactical";});auto b=std::find_if(weapons.begin(),weapons.end(),[](const Weapon& w){return w.name=="Deagle";});if(a!=weapons.end())left=size_t(a-weapons.begin());if(b!=weapons.end())right=size_t(b-weapons.begin());}
    recompute();
    status=L"Base weapons / current game data";
}
inline void select(bool second,int direction){
    if(weapons.empty())return;
    auto& i=second?right:left;i=(i+weapons.size()+direction)%weapons.size();
}
}
