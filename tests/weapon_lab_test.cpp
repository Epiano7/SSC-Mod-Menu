#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../runtime/weapon_lab.h"
#include <array>
#include <cassert>
#include <iostream>
#include <random>
template<class T> void put(unsigned char* p,size_t at,T v){std::memcpy(p+at,&v,sizeof(v));}
extern "C" void lab_shield_call(void*,void*,float);
int main(int argc,char** argv){
 using namespace ssc_lab;
 Weapon w{"Test",600,10,20,2,2,0,0,0};
 assert(valid(w));auto m=calculate(w);assert(m.displayed_dps==200&&std::abs(m.empty-.9)<1e-9&&std::abs(m.sustained-200/2.9)<1e-9);
 assert(calculate(w,true).displayed_dps==400);
 w.magazine=1;assert(calculate(w).empty==0&&calculate(w).sustained==10);
 w.reload=0;assert(std::isnan(calculate(w).sustained));
 w.burst_size=1;w.burst_delay=.1f;assert(std::isnan(calculate(w).empty));
 w.burst_delay=0;w.extra_dps=10;assert(calculate(w).displayed_dps==210&&std::isnan(calculate(w).sustained));
 w.rpm=0;assert(std::isnan(calculate(w).displayed_dps));
 w.reload=std::numeric_limits<float>::quiet_NaN();assert(!valid(w));
 std::array<unsigned char,0x748*2> records{};
 for(int i=0;i<2;++i){auto p=records.data()+i*0x748;std::memcpy(p+0x48,"Fixture",7);put(p,0x58,size_t(7));put(p,0x60,size_t(15));put(p,0x448,20);put(p,0x3f0,1.f);put(p,0x458,600);put(p,0x4f0,10);put(p,0x454,2.f);put(p,0x410,2.f);}
 uintptr_t table[]={reinterpret_cast<uintptr_t>(records.data()),reinterpret_cast<uintptr_t>(records.data()+records.size())};
 std::vector<Weapon> catalog;assert(read_catalog(reinterpret_cast<uintptr_t>(table),catalog)&&catalog.size()==2&&catalog[0].name=="Fixture");
 --table[1];assert(!read_catalog(reinterpret_cast<uintptr_t>(table),catalog)&&catalog.empty());++table[1];
 put(records.data(),0x2a8,-1);assert(read_catalog(reinterpret_cast<uintptr_t>(table),catalog)&&catalog.size()==1);
 put(records.data()+0x748,0x454,std::numeric_limits<float>::infinity());assert(!read_catalog(reinterpret_cast<uintptr_t>(table),catalog)&&catalog.empty());
 assert(!read_catalog(0,catalog));
 weapons={Weapon{"A"},Weapon{"B"}};left=0;right=1;select(false,-1);assert(left==1);select(true,1);assert(right==0);
 open_picker(1);query=L"b";auto matches=filtered();assert(matches.size()==1&&matches[0]==1);query=L"missing";assert(filtered().empty());query.clear();sort_mode=0;assert(filtered()[0]==0);weapons[1].rpm=600;sort_mode=1;assert(filtered()[0]==1);picker=-1;
 ShieldRules rules{1,1,true};Target target_test{100,0,100};auto trial=simulate(target_test,20,600,3,2,true,rules);
 assert(trial.killed&&trial.shots.size()==5&&trial.shots.back().reloads==1&&std::abs(trial.shots.back().time-2.3)<1e-9);
 assert(!simulate(target_test,20,600,3,2,false,rules).killed);
 assert(simulate(target_test,100,600,3,2,true,rules).shots.back().time==0);
 assert(simulate({1,0,1},.000001f,600,3,2,true,rules).capped);
 assert(simulate({0,0,100},20,600,3,2,true,rules).shots.empty());
 assert(simulate({100,50,100},20,600,3,2,true,{}).shots.empty());
 target_test={100,100,100};auto hit=impact(target_test,20,rules);assert(hit.health==10&&hit.shield==10);
 target_test={100,20,100};hit=impact(target_test,20,rules);assert(hit.health==20&&hit.shield==0);
 auto fast=simulate({100,0,100},25,600,2,1,true,rules),slow=simulate({100,0,100},20,300,2,1,true,rules);
 auto race=duel(fast,slow,{100,0,100});assert(race.winner==0&&race.players[1].health==0&&race.players[0].health>0&&race.reloads[0]==1);
 race=duel(fast,fast,{100,0,100});assert(race.winner==2&&race.players[0].health==0&&race.players[1].health==0);
 auto misses_all=simulate({100,0,100},25,600,2,1,false,rules,100,123);assert(!misses_all.killed&&misses_all.shots.size()==2&&misses_all.shots[0].missed&&misses_all.shots.back().health==100);
 auto random_a=simulate({100,0,100},25,600,2,1,true,rules,50,123),random_b=simulate({100,0,100},25,600,2,1,true,rules,50,123);assert(random_a.shots.size()==random_b.shots.size());for(size_t i=0;i<random_a.shots.size();++i)assert(random_a.shots[i].missed==random_b.shots[i].missed);

 auto frame=duel_at(fast,slow,{100,0,100},.5);assert(frame.winner==-1&&frame.shots[0]==2&&frame.players[1].health==50&&frame.players[0].health==60);
 frame=duel_at(fast,slow,{100,0,100},100);assert(frame.winner==0&&frame.players[1].health==0);
 Weapon meta{"UZI"};meta.tier=0;meta.category=3;assert(label(meta)==L"UZI (T1 SMG)");weapons={meta};query=L"t1 smg";assert(filtered().size()==1);query.clear();
 auto pellets=simulate({100,100,100},30,600,3,2,false,rules,0,1,3);Target sequential{100,100,100};for(int i=0;i<3;++i)impact(sequential,10,rules);assert(pellets.shots[0].health==sequential.health&&pellets.shots[0].shield==sequential.shield);

 weapons={{"Minigun",720,60,10.5f,1.5f,1}};weapons[0].projectiles=3;left=right=0;target={};guards=false;recompute();assert(trials[0].killed&&!exclusion(weapons[0]));
 weapons[0].name="Unknown multi-impact weapon";assert(exclusion(weapons[0]));

 Trial visual_trial;visual_trial.shots={{1,0,0,80,0,20,0,false},{2,1,2,60,0,20,0,false}};
 auto initial=Target{100,0,100};assert(visual_target(visual_trial,initial,0,0).health==100);
 assert(std::abs(visual_target(visual_trial,initial,.06,.06).health-90)<.001);
 assert(visual_target(visual_trial,initial,1,1).health==80); // No damage during reload.
 assert(visual_target(visual_trial,initial,1,3).health==80); // Stop never reveals a future shot.
 assert(visual_target(visual_trial,initial,2,2.12).health==60);
 if(argc>1){
   HMODULE image=LoadLibraryExA(argv[1],nullptr,DONT_RESOLVE_DLL_REFERENCES);assert(image);
   auto base=reinterpret_cast<uintptr_t>(image);
   // Validate the complete helper fingerprint before executing a mapped native leaf.
   const ssc_compat::FunctionSpec* spec=nullptr;for(const auto& f:ssc_compat::function_specs)if(f.key==0x3a7aa0)spec=&f;
   assert(spec&&ssc_compat::fingerprint(reinterpret_cast<unsigned char*>(base+spec->key),*spec)==spec->hash);
   assert(*reinterpret_cast<float*>(base+0xc6f0ec)==60.f&&*reinterpret_cast<float*>(base+0xc6e108)==1.f);
   using Native=float(*)(const void*,unsigned char);auto native=reinterpret_cast<Native>(base+spec->key);
   std::mt19937 rng(12345);
   for(int i=0;i<10000;++i){
     std::array<unsigned char,0x748> record{};auto p=record.data();
     int damage=1+rng()%250,extra=rng()%5;float multiplier=(1+rng()%30)/10.f;
     w={"Fixture",int(60+rng()%2000),int(5+rng()%100),float(damage)*multiplier*float(extra+1),1.5f,(1+rng()%20)/10.f,0,0,float(rng()%20)};
     if(i%2){w.burst_size=3;w.burst_delay=.2f;}
     put(p,0x448,damage);put(p,0x44c,extra);put(p,0x3f0,multiplier);put(p,0x458,w.rpm);put(p,0x4f0,w.magazine);put(p,0x410,w.guard);put(p,0x480,w.burst_size);put(p,0x484,w.burst_delay);
     for(bool guard:{false,true})assert(calculate(w,guard).displayed_dps==std::floor(double(native(p,guard))+w.extra_dps+.5));
   }
   const ssc_compat::FunctionSpec* damage_spec=nullptr;for(const auto& f:ssc_compat::function_specs)if(f.key==0x7fa090)damage_spec=&f;
   assert(damage_spec&&ssc_compat::fingerprint(reinterpret_cast<unsigned char*>(base+damage_spec->key),*damage_spec)==damage_spec->hash);
   // Execute only the isolated arithmetic block in this private mapped image.
   // No game entry point, actor enumeration, native calls, or live process edits.
   auto stop=reinterpret_cast<unsigned char*>(base+0x7fd647);DWORD old=0;assert(VirtualProtect(stop,1,PAGE_EXECUTE_READWRITE,&old));auto saved=*stop;*stop=0xc3;FlushInstructionCache(GetCurrentProcess(),stop,1);
   for(int i=0;i<20000;++i){
     std::array<unsigned char,0x2500> actor{};auto p=actor.data();
     Target t{float(1+rng()%10000),float(rng()%20000),10000};float damage=float(rng()%20000)/10;
     ShieldRules r{float(rng()%300)/100,float(rng()%300)/100,true};
     put(reinterpret_cast<unsigned char*>(base),0xfa5eac,r.low);put(reinterpret_cast<unsigned char*>(base),0xfa5eb0,r.high);
     put(p,0x138c,t.health);put(p,0x1390,t.shield);put(p,0x840,t.max_health);
     impact(t,damage,r);lab_shield_call(reinterpret_cast<void*>(base+0x7fd534),p,damage);
     float hp=0,shield=0;std::memcpy(&hp,p+0x138c,4);std::memcpy(&shield,p+0x1390,4);
     if(!(std::abs(t.health-std::max(0.f,hp))<.003f&&std::abs(t.shield-shield)<.003f)){std::cerr<<i<<" rules "<<r.low<<" "<<r.high<<" damage "<<damage<<" expected "<<t.health<<" "<<t.shield<<" native "<<hp<<" "<<shield<<"\n";return 2;}
   }
   *stop=saved;DWORD ignored=0;VirtualProtect(stop,1,old,&ignored);
   FreeLibrary(image);
 }
 std::cout<<"Weapon Lab: record bounds, selection, invalid data, model assumptions and native formula passed\n";
}
