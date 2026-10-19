#pragma once
#include "advisor_core.h"
#include "performance_metrics.h"
#include "advisor_api.h"
#include "private_license.h"
#include "freecam.h"
#include "local_recorder.h"
namespace ssc_advisor {
inline bool enabled=true,attached=false,use_api=true;
// Entitlement is independent of the Bot Highlight module's enabled preference.
inline bool (*private_access)()=+[](){return ssc_auth::draft_available();};
inline bool allowed(){return enabled&&private_access();}
inline uintptr_t cached_actor=0,cached_world=0;
inline const wchar_t* advice_status=L"Waiting for your build";
inline int recommended=-1,recommended_icon=-1;
inline uintptr_t game=0;
inline bool (*selection_held)()=+[](){return (GetAsyncKeyState(VK_CONTROL)&0x8000)!=0;};
inline Build last_build;inline bool cached=false;inline std::array<Score,54> cached_scores{},cached_mutation_scores{};
struct AuditState {Build build;std::array<int,54> offers{};std::array<Score,54> scores{};unsigned count=0,segment=0;int best=-1;bool valid=false,api_valid=false,api_used=false;std::string match;};
inline AuditState skill_audit,stim_audit;
inline void audit(bool mutation,const Build& build,const std::array<int,54>& offers,unsigned count,int best,const std::array<Score,54>& scores,bool api_valid=false,bool api_used=false){
 auto& record=ssc_record::state();if(!allowed()||!record.enabled||!record.match_open||count>54)return;
 auto& old=mutation?stim_audit:skill_audit;
 bool changed=!old.valid||old.match!=record.match_id||old.segment!=record.segment||!same_build(old.build,build)||old.count!=count||old.best!=best||old.api_valid!=api_valid||old.api_used!=api_used;
 for(unsigned i=0;i<count;++i){int id=offers[i];if(id<0||id>=54)return;if(old.offers[i]!=id||old.scores[size_t(id)].value!=scores[size_t(id)].value)changed=true;}
 if(!changed)return;
 old={build,offers,scores,count,record.segment,best,true,api_valid,api_used,record.match_id};
 ssc_record::Json candidates=ssc_record::Json::array();for(unsigned i=0;i<count;++i){int id=offers[i];candidates.push_back({{"skill_id",id},{"score",scores[size_t(id)].value},{"evidence",scores[size_t(id)].evidence}});}
 ssc_record::diagnostic({{"event","advisor_recommendation"},{"kind",mutation?"mutation":"skill"},{"class_id",build.player_class+1},{"skill_levels",build.levels},{"mutations",build.mutated},{"candidates",candidates},{"recommended_skill_id",best},{"api_enabled",use_api},{"api_response_available",api_valid},{"api_contributed",api_used},{"score_model","local-priors-v1"}});
}
using Draw=void(*)(uintptr_t,uintptr_t,float,float,char,uint64_t,char,char,char);
using Icon=void(*)(uintptr_t,uintptr_t,float,float,float,float,int,char,int,float,uint32_t,uint32_t,uint32_t,char);
inline Draw original_draw=nullptr;inline Icon original_icon=nullptr;
template<class T> bool read(uintptr_t p,T& v){return ssc_freecam::read(p,v);}

inline bool build_snapshot(uintptr_t context,Build& b,uintptr_t& actor,uintptr_t& world){
 b=Build{};
 int slot=-1,local=-2,kind=-1;unsigned char active=0;
 if(!read(context,actor)||!actor||!read(game+0xde85d0,world)||!world||!read(actor+0x78,slot)||!read(world+0x6b98,local)||slot!=local||slot<0||!read(actor+0x7dc,kind)||kind!=0||!read(actor+0x81,active)||!active||!read(actor+0x350,b.player_class)||b.player_class<1||b.player_class>8)return false;
 --b.player_class;
 uintptr_t acquired=0,end=0;if(!read(actor+0x10b8,acquired)||!read(actor+0x10c0,end)||end<acquired||(end-acquired)%0x240||(end-acquired)/0x240>54||(!acquired&&end))return false;
 for(uintptr_t p=acquired;p<end;p+=0x240){int id=-1,level=-1;unsigned char excluded=0;if(!read(p,id)||!read(p+0x1f8,level)||!read(p+0x1dc,excluded)||id<0||id>=54||level<0||level>300||excluded>1)return false;if(!excluded)b.levels[size_t(id)]=level;}
 uintptr_t first=0,last=0;if(!read(actor+0x3c0,first)||!read(actor+0x3c8,last)||last<first||(last-first)%4||(last-first)/4>54||(!first&&last))return false;
 for(auto p=first;p<last;p+=4){int id=-1;if(!read(p,id)||id<0||id>=54||b.mutated[size_t(id)])return false;b.mutated[size_t(id)]=true;}
 return true;
}
inline bool snapshot(uintptr_t context,Build& b,std::array<int,54>& offers,unsigned& count,std::array<unsigned,54>* slots=nullptr){
 uintptr_t actor=0,world=0,first=0,last=0;int points=0;
 if(!build_snapshot(context,b,actor,world)||!read(actor+0x1240,points)||points<=0)return false;
 float animation=0;if(!read(context+0x3148,animation)||!std::isfinite(animation)||animation>0)return false;
 if(!read(game+0xe03350,first)||!read(game+0xe03358,last)||!first||last<first||(last-first)%12||(last-first)/12>3)return false;
 const unsigned native_count=unsigned((last-first)/12);if(native_count<2)return false;count=0;
 // The native panel copies this displayed list at 0x3e0951. The actor's
 // separate offer copy can lag a queued pick until Ctrl is reopened.
 // build_snapshot already proves this is the local player's panel.
 for(unsigned i=0;i<native_count;++i){int id=-1;if(!read(first+i*12,id))return false;
  // Native drafts retain an empty (-1) slot when only two choices are offered.
  if(id==-1)continue;
  if(id<0||id>=54)return false;
  for(unsigned j=0;j<count;++j)if(offers[j]==id)return false;
  if(slots)(*slots)[count]=i;
  offers[count++]=id;
 }
 return count>=2;
}
inline void prepare(const Build& b,uintptr_t actor,uintptr_t world){
 if(!cached||actor!=cached_actor||world!=cached_world||!same_build(last_build,b)){cached_scores=evaluate(b);cached_mutation_scores=evaluate_mutations(b);last_build=b;cached=true;cached_actor=actor;cached_world=world;}
}
inline void tick(){
 static uint64_t checked=0;auto now=GetTickCount64();if(now-checked<100)return;checked=now;
 if(!attached||!allowed()){cached=false;recommended=recommended_icon=-1;advice_status=L"Private access required";return;}Build b;uintptr_t actor=0,world=0;
 if(!build_snapshot(ssc_freecam::observed_context,b,actor,world)){cached=false;advice_status=L"Waiting for your build";return;}
 prepare(b,actor,world);advice_status=L"Build ready";
}
struct StimBorder {std::array<std::array<float,2>,64> points{};bool pending=false;};
inline StimBorder stim_border;
inline bool stim_ready(){return stim_border.pending&&allowed()&&!ssc_geri::enabled.load()&&!ssc_freecam::clean_view();}
inline void render_stim(int width,int height){
 bool show=stim_ready();stim_border.pending=false;if(!show)return;
 GLint mode=0;glGetIntegerv(GL_MATRIX_MODE,&mode);glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_LINE_BIT|GL_COLOR_BUFFER_BIT);
 glDisable(GL_SCISSOR_TEST);glDisable(GL_TEXTURE_2D);glDisable(GL_DEPTH_TEST);glDisable(GL_ALPHA_TEST);glColor4f(.25f,1.f,.65f,1);glLineWidth(3);
 glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
 glBegin(GL_LINE_LOOP);for(const auto& p:stim_border.points)glVertex2f(p[0]*width,p[1]*height);glEnd();
 glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(mode);glPopAttrib();
}
inline void draw(uintptr_t context,uintptr_t out,float a,float b,char c,uint64_t d,char e,char f,char g){
 recommended_icon=-1;stim_border.pending=false;
 if(allowed()&&!ssc_freecam::clean_view()&&!ssc_geri::enabled.load()){ssc_perf::Scope timing(ssc_perf::advisor);Build build;uintptr_t actor=0,world=0;int syringes=0;
  if(build_snapshot(context,build,actor,world)&&read(actor+0x8d0,syringes)&&syringes>0){prepare(build,actor,world);const auto& scores=cached_mutation_scores;std::array<int,54> choices{};unsigned count=0;
   for(int i=0;i<54;++i)if(build.levels[size_t(i)]>0&&!build.mutated[size_t(i)])choices[count++]=i;
   int best=count==1?choices[0]:choose(scores,choices,count);uintptr_t table=0;if(best>=0&&read(game+0xfaef98,table)&&table)read(table+uintptr_t(best)*0x240+0x10,recommended_icon);
   audit(true,build,choices,count,best,scores);
  }
 }
 original_draw(context,out,a,b,c,d,e,f,g);recommended_icon=-1;
}
using Panel=void(*)(uintptr_t,uintptr_t,float,uint64_t,char,char);
inline Panel original_panel=nullptr;
struct DraftBorder {int x=0,y=0,w=0,h=0,width=0,height=0;bool pending=false;};
inline DraftBorder border;
inline bool border_ready(){return border.pending&&allowed()&&selection_held()&&!ssc_geri::enabled.load()&&!ssc_freecam::clean_view();}
inline void render_border(){
 const bool show=border_ready();auto box=border;border.pending=false;if(!show)return;
 const auto [x,y,w,h,width,height,pending]=box;(void)pending;
  GLint mode=0;glGetIntegerv(GL_MATRIX_MODE,&mode);glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_LINE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glDisable(GL_SCISSOR_TEST);glDisable(GL_TEXTURE_2D);glDisable(GL_DEPTH_TEST);glDisable(GL_ALPHA_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glColor4f(.25f,1.f,.65f,1);glLineWidth(3);
  glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,width,height,0,-1,1);glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();glBegin(GL_LINE_LOOP);glVertex2i(x-3,y-3);glVertex2i(x+w+3,y-3);glVertex2i(x+w+3,y+h+3);glVertex2i(x-3,y+h+3);glEnd();glPopMatrix();glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(mode);glPopAttrib();
}
inline void panel(uintptr_t context,uintptr_t output,float a,uint64_t b,char c,char d){
 original_panel(context,output,a,b,c,d);
 recommended=-1;border.pending=false;
 if(!allowed()||!selection_held()||ssc_geri::enabled.load()||ssc_freecam::clean_view())return;
 ssc_perf::Scope timing(ssc_perf::advisor);Build build;std::array<int,54> offers{};std::array<unsigned,54> slots{};unsigned count=0;if(!snapshot(context,build,offers,count,&slots))return;
 uintptr_t actor=0,world=0;if(!read(context,actor)||!read(game+0xde85d0,world))return;prepare(build,actor,world);
 auto scores=cached_scores;bool api_valid=false,api_used=false;
 // Highlight only the interactive Ctrl draft, never the collapsed preview.
 if(use_api){auto key=api_key(build,offers,count);if(!key.empty()){auto answer=api().request(key,GetTickCount64());api_valid=answer.valid;if(answer.valid){for(unsigned i=0;i<count;++i){auto id=size_t(offers[i]);if(answer.samples[id]>=5){scores[id].value+=2.f*std::tanh(answer.edge[id]/10.f);api_used=true;}}advice_status=L"Local research + API";}}}
 recommended=choose(scores,offers,count);if(recommended<0)return;
 for(unsigned i=0;i<count;++i)if(offers[i]==recommended){uintptr_t button=context+0x310+slots[i]*0x170;int x=0,y=0,w=0,h=0,ax=0,ay=0,width=0,height=0;
  // +0x44 gates native input after a click; it is not panel visibility.
  // Held Ctrl, pending points, current displayed offers and the transition
  // guard above determine whether this recommendation is visible.
  if(!read(button+0x14,x)||!read(button+0x18,y)||!read(button+0x1c,w)||!read(button+0x20,h)||!read(button+0x70,ax)||!read(button+0x74,ay)||!read(game+0xfafa40,width)||!read(game+0xfafa44,height)||w<=0||h<=0||width<=0||height<=0)return;
  x+=ax==1?width/2:ax==2?width:0;y+=ay==1?height/2:ay==2?height:0;
  border={x,y,w,h,width,height,true};
  audit(false,build,offers,count,recommended,scores,api_valid,api_used);
 }
}
inline void icon(uintptr_t renderer,uintptr_t sheet,float x,float y,float w,float h,int index,char flip,int blend,float alpha,uint32_t r,uint32_t g,uint32_t b,char state){
 original_icon(renderer,sheet,x,y,w,h,index,flip,blend,alpha,r,g,b,state);
 if(index!=recommended_icon||recommended_icon<0||!allowed()||ssc_freecam::clean_view()||!std::isfinite(w)||!std::isfinite(h)||w<=0||h<=0||alpha<.9f)return;
 // Project the native icon outline now, then draw it after game HUD compositing.
 GLfloat model[16]{},projection[16]{};glGetFloatv(GL_MODELVIEW_MATRIX,model);glGetFloatv(GL_PROJECTION_MATRIX,projection);
 for(int i=0;i<64;++i){float t=float(i)*6.28318530718f/64;float point[4]={x+std::cos(t)*w*.62f,y+std::sin(t)*h*.62f,0,1},eye[4]{},clip[4]{};
  for(int row=0;row<4;++row)for(int col=0;col<4;++col)eye[row]+=model[col*4+row]*point[col];
  for(int row=0;row<4;++row)for(int col=0;col<4;++col)clip[row]+=projection[col*4+row]*eye[col];
  if(!std::isfinite(clip[3])||clip[3]<=.00001f){stim_border.pending=false;return;}
  stim_border.points[size_t(i)]={(clip[0]/clip[3]+1)*.5f,(1-clip[1]/clip[3])*.5f};
 }
 stim_border.pending=true;
}
inline bool attach(){
 game=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));auto bytes=reinterpret_cast<unsigned char*>(game);
 if(ssc_creator::hash(bytes+0x3e0290,14780)!=0x5da8b1450cda7607ULL||!ssc_compat::supports(128)||!ssc_record::validate_build_code(game,[](uintptr_t p,auto& v){return read(p,v);})||ssc_creator::hash(bytes+0x9511d0,1212)!=0x1dd1b583ce4ca2b8ULL||ssc_creator::hash(bytes+0x3db8d0,14692)!=0x4a4f120bf159e587ULL)return false;
 struct Site{uint32_t call,target;uintptr_t hook;};const Site sites[]={{0x3d9504,0x3e0290,reinterpret_cast<uintptr_t>(panel)},{0x3e0693,0x3db8d0,reinterpret_cast<uintptr_t>(draw)},{0x3dea60,0x488040,reinterpret_cast<uintptr_t>(icon)}};
 for(auto s:sites){int32_t rel=0;std::memcpy(&rel,bytes+s.call+1,4);if(bytes[s.call]!=0xe8||int64_t(s.call)+5+rel!=s.target)return false;}
 unsigned char* bridge=nullptr;for(uintptr_t n=0x10000;n<0x60000000&&!bridge;n+=0x10000)bridge=static_cast<unsigned char*>(VirtualAlloc(reinterpret_cast<void*>((game+n)&~uintptr_t(0xffff)),4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!bridge)return false;
 const unsigned char jump[]={0xff,0x25,0,0,0,0};for(unsigned i=0;i<3;++i){std::memcpy(bridge+i*16,jump,6);std::memcpy(bridge+i*16+6,&sites[i].hook,8);}DWORD old=0;if(!VirtualProtect(bridge,4096,PAGE_EXECUTE_READ,&old)){VirtualFree(bridge,0,MEM_RELEASE);return false;}DWORD protections[3]{};
 for(unsigned i=0;i<3;++i)if(!VirtualProtect(bytes+sites[i].call,5,PAGE_EXECUTE_READWRITE,&protections[i])){for(unsigned j=0;j<i;++j){DWORD ignored;VirtualProtect(bytes+sites[j].call,5,protections[j],&ignored);}VirtualFree(bridge,0,MEM_RELEASE);return false;}
 original_draw=reinterpret_cast<Draw>(game+sites[1].target);original_icon=reinterpret_cast<Icon>(game+sites[2].target);original_panel=reinterpret_cast<Panel>(game+sites[0].target);
 for(unsigned i=0;i<3;++i){int32_t rel=int32_t(reinterpret_cast<uintptr_t>(bridge+i*16)-(game+sites[i].call+5));std::memcpy(bytes+sites[i].call+1,&rel,4);FlushInstructionCache(GetCurrentProcess(),bytes+sites[i].call,5);DWORD ignored;VirtualProtect(bytes+sites[i].call,5,protections[i],&ignored);}FlushInstructionCache(GetCurrentProcess(),bridge,48);return true;
}
}

