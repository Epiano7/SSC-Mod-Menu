#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <GL/gl.h>
#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
namespace ssc_challenge {
struct RandomSettings {bool enabled=false;bool stims=true;bool blind=false;};
struct ChaosSettings {bool enabled=false;std::array<bool,9> effects{};bool visual_confirmed=false;bool scramble_controls=false;unsigned control_interval_ms=60000;};
inline RandomSettings random;inline ChaosSettings chaos;inline bool attached=false,render_supported=false;
inline bool view_supported=false,input_supported=false;
// Fixtures use the known baseline; runtime attachment replaces this after exact feature guards.
inline bool blind_supported=true;
inline bool paused=false,in_game=false,blind_locked=false;inline uintptr_t session=0;inline int last_round=0;inline uint64_t frame_ms=0;
inline bool draft_achievement_eligible=true;
inline void draft_settings_changed(){if(in_game&&last_round>=1)draft_achievement_eligible=false;}
inline std::wstring status=L"Off";
inline constexpr std::array<int,9> effect_weights={2,1,1,1,2,2,1,1,2};
inline int severity(const ChaosSettings& s){if(!s.enabled)return 0;int score=0;for(size_t i=0;i<s.effects.size();++i)if(s.effects[i])score+=effect_weights[i];return score;}
inline bool visuals(const ChaosSettings& s){return s.effects[3]||s.effects[4];}
inline bool active(){return chaos.enabled&&in_game&&severity(chaos)>0;}
inline bool projection_effect(std::array<float,16>& p){
 if(!active()||!view_supported||(!chaos.effects[5]&&!chaos.effects[6]))return false;
 for(float v:p)if(!std::isfinite(v))return false;
 const float scale=(chaos.effects[5]?-1.f:1.f)*(chaos.effects[6]?1.8f:1.f);
 for(int col=0;col<4;++col){p[size_t(col)*4]*=scale;p[size_t(col)*4+1]*=scale;}return true;
}
inline thread_local std::array<float,16> view_last_output{};
inline thread_local bool view_output_valid=false;
inline void world_view(){
 if(!active()||!view_supported||(!chaos.effects[5]&&!chaos.effects[6]))return;
 std::array<float,16> p{};glGetFloatv(GL_PROJECTION_MATRIX,p.data());if(view_output_valid&&p==view_last_output)return;if(!projection_effect(p))return;view_last_output=p;view_output_valid=true;
 GLint mode=GL_MODELVIEW;glGetIntegerv(GL_MATRIX_MODE,&mode);glMatrixMode(GL_PROJECTION);glLoadMatrixf(p.data());glMatrixMode(mode);
}
inline bool blind(){return attached&&render_supported&&blind_supported&&random.blind&&blind_locked;}
inline void stop(){random.enabled=false;chaos.enabled=false;blind_locked=false;status=L"Challenges stopped";}
inline void observe(uintptr_t world,bool valid,bool playing,bool ending,int round,int final,uint64_t now,bool menu){frame_ms=now;paused=menu;in_game=valid&&playing;
 if(!valid)return;
 if(world!=session||round<last_round){session=world;blind_locked=false;draft_achievement_eligible=true;}last_round=round;
 if(random.enabled&&round>=1)blind_locked=true;
 if(ending&&round==final&&final>0)blind_locked=false;
}
template<class Next> bool uniform(size_t count,Next next,size_t& choice){if(!count||count>54)return false;uint32_t n=uint32_t(count),threshold=uint32_t(0-n)%n;for(int i=0;i<16;++i){uint32_t word=0;if(!next(word))return false;if(word>=threshold){choice=word%n;return true;}}return false;}
inline bool random_word(uint32_t& word){return BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&word),sizeof(word),BCRYPT_USE_SYSTEM_PREFERRED_RNG)>=0;}
struct Offer {bool offline=false;uintptr_t actor=0,world=0;int round=0,points=0,syringes=0,generation=0;bool reroll_known=false,reroll_available=false;std::vector<int> skills;std::vector<uintptr_t> stims;std::array<int,54> acquired{};std::array<bool,54> mutated{};bool owned_known=false;int class_id=-1;std::array<bool,54> present{};std::array<int,54> allocated{};};
struct Picker {bool pending=false,rejected=false;uint64_t sent=0;Offer before;bool last_skill=false;uintptr_t last_choice=0;void reset(){pending=false;rejected=false;before={};sent=0;last_skill=false;last_choice=0;}
 bool settled(const Offer& o,uint64_t now){
  if(pending){if(o.actor!=before.actor||o.world!=before.world||o.round!=before.round||o.class_id!=before.class_id||(last_skill?o.points<before.points:o.syringes<before.syringes)||(last_choice<54&&o.owned_known&&before.owned_known&&(last_skill?(!before.present[last_choice]&&o.present[last_choice])||o.acquired[last_choice]>before.acquired[last_choice]||o.allocated[last_choice]>before.allocated[last_choice]:o.mutated[last_choice]&&!before.mutated[last_choice])))pending=false;else {if(now>=sent&&now-sent>15000){status=L"Waiting for game confirmation; Random Drafts is still on";}return false;}}
  return true;
 }
 template<class Choose,class Send>void tick(const Offer& o,uint64_t now,Choose choose,Send send){
  if(!settled(o,now))return;
  if(sent&&now>=sent&&now-sent<(rejected?1000:(last_skill?150:1000)))return;
  const bool skill=o.points>0&&!o.skills.empty();const bool stim=!skill&&random.stims&&o.syringes>0&&!o.stims.empty();if(!skill&&!stim)return;
  size_t index=0;if(!choose(skill?o.skills.size():o.stims.size(),index)){status=L"Random source unavailable; selection paused";return;}
  if(index>=(skill?o.skills.size():o.stims.size()))return;
  before=o;last_skill=skill;last_choice=skill?uintptr_t(o.skills[index]):o.stims[index];sent=now;if(send(skill,skill?uintptr_t(o.skills[index]):o.stims[index])){pending=true;rejected=false;status=skill?L"Waiting for the selected skill to apply":L"Waiting for the selected stim to apply";}else {status=L"Selection unavailable; waiting to retry";pending=false;rejected=true;}
 }
};inline Picker picker;
struct Placement {float x,y,scale;};
inline Placement placement(int id,float x,float y,float scale,float w,float h,double seconds){
 if(!active()||id<1||id>8)return{x,y,scale};
 if(chaos.effects[0]){static const float spots[][2]={{.07f,.14f},{.60f,.66f},{.10f,.67f},{.65f,.13f},{.38f,.48f},{.43f,.14f},{.13f,.44f},{.72f,.43f}};int offset=int(seconds/12)%8;int slot=(id-1+offset)%8;x=spots[slot][0];y=spots[slot][1];}
 if(chaos.effects[1]){x+=.045f*float(std::sin(seconds*.7+id));y+=.035f*float(std::sin(seconds*.53+id*2));}
 if(chaos.effects[2])scale*=.82f+.36f*float(.5+.5*std::sin(seconds*.65+id));
 scale=std::clamp(scale,.25f,3.f);x=std::clamp(x,0.f,std::max(0.f,1-w*scale));y=std::clamp(y,0.f,std::max(0.f,1-h*scale));return{x,y,scale};
}
// Called only inside the overlay's saved GL state and pixel-space projection.
inline void draw_chaos(int width,int height){
 if(!active()||!chaos.visual_confirmed||width<=0||height<=0)return;
 const double t=double(frame_ms)*.001;
 glDisable(GL_TEXTURE_2D);
 if(chaos.effects[3]){
  glBlendFunc(GL_ZERO,GL_SRC_COLOR);
  glColor4f(.65f+.3f*float(std::sin(t*.31)),.65f+.3f*float(std::sin(t*.27+2)),.65f+.3f*float(std::sin(t*.23+4)),1);
  glBegin(GL_QUADS);glVertex2i(0,0);glVertex2i(width,0);glVertex2i(width,height);glVertex2i(0,height);glEnd();
 }
 glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
 if(chaos.effects[4])for(int i=0;i<3;++i){
  float y=float(height)*(.1f+.32f*i+.07f*float(std::sin(t*.29+i)));
  glColor4f(.03f,.015f,.08f,.62f);glBegin(GL_QUADS);glVertex2f(0,y);glVertex2f(float(width),y);glVertex2f(float(width),y+height*.12f);glVertex2f(0,y+height*.12f);glEnd();
 }
 glColor4f(1,1,1,1);
}
}
