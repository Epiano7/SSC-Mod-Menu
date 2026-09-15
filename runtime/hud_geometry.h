#pragma once
// Included inside ssc_hud. Read native immediate-mode geometry only while calibrating
// or editing; pointer slots are restored at the end of each outer HUD draw.
struct Bounds {float left=10,top=10,right=-10,bottom=-10;unsigned points=0;};
inline std::array<Bounds,9> measured_frame{};
inline std::array<bool,9> measured{};
inline std::array<ULONGLONG,9> last_seen{};
inline bool editing=false,freeze_bounds=false,capture_frame=true;
inline int active_item=-1,view_w=0,view_h=0,capture_depth=0;
inline ULONGLONG capture_at=0;
inline std::array<unsigned,9> primitive_count{};
using BeginProc=void(APIENTRY*)(GLenum);
using Vertex2Proc=void(APIENTRY*)(GLfloat,GLfloat);
using Vertex3Proc=void(APIENTRY*)(GLfloat,GLfloat,GLfloat);
inline BeginProc native_begin=nullptr;
inline Vertex2Proc native_vertex2=nullptr;
inline Vertex3Proc native_vertex3=nullptr;
inline bool primitive_capture=false;
inline GLfloat capture_model[16],capture_projection[16];
inline void accumulate(int index,float x,float y){
 if(index<0||index>=int(items.size())||!std::isfinite(x)||!std::isfinite(y))return;
 const auto& item=items[index];
 if(enabled){x=item.home_x+(x-item.x)/item.scale;y=item.home_y+(y-item.y)/item.scale;}
 if(x<-.5f||x>1.5f||y<-.5f||y>1.5f)return;
 auto& b=measured_frame[index];b.left=std::min(b.left,x);b.right=std::max(b.right,x);b.top=std::min(b.top,y);b.bottom=std::max(b.bottom,y);++b.points;
}
inline void capture_vertex(float x,float y,float z){
 if(!primitive_capture||active_item<0)return;
 const float in[]={x,y,z,1};float world[4]{},clip[4]{};
 for(int r=0;r<4;++r)for(int c=0;c<4;++c)world[r]+=capture_model[c*4+r]*in[c];
 for(int r=0;r<4;++r)for(int c=0;c<4;++c)clip[r]+=capture_projection[c*4+r]*world[c];
 if(std::abs(clip[3])>.00001f)accumulate(active_item,(clip[0]/clip[3]+1)*.5f,(1-clip[1]/clip[3])*.5f);
}
inline void APIENTRY capture_begin(GLenum mode){
 primitive_capture=active_item>=0&&(!measured[active_item]||editing)&&primitive_count[active_item]<256;
 if(primitive_capture){
  GLint stencil=GL_ALWAYS;glGetIntegerv(GL_STENCIL_FUNC,&stencil);
  // The map's stencil mask gives its visible bounds. Ignore the clipped world
  // geometry beneath it, which can extend far beyond the minimap rectangle.
  if(glIsEnabled(GL_STENCIL_TEST)&&stencil!=GL_ALWAYS)primitive_capture=false;
  else {++primitive_count[active_item];glGetFloatv(GL_MODELVIEW_MATRIX,capture_model);glGetFloatv(GL_PROJECTION_MATRIX,capture_projection);}
 }
 native_begin(mode);
}
inline void APIENTRY capture_v2(GLfloat x,GLfloat y){capture_vertex(x,y,0);native_vertex2(x,y);}
inline void APIENTRY capture_v3(GLfloat x,GLfloat y,GLfloat z){capture_vertex(x,y,z);native_vertex3(x,y,z);}
inline bool writable_slot(uintptr_t address){MEMORY_BASIC_INFORMATION info{};return VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info))&&info.State==MEM_COMMIT&&(info.Protect&(PAGE_READWRITE|PAGE_EXECUTE_READWRITE));}
inline bool start_capture(){
 if(!game_base||!capture_frame||game_base!=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)))return false;
 if(capture_depth){++capture_depth;return true;}
 auto b=reinterpret_cast<BeginProc*>(game_base+0xdde708);auto v2=reinterpret_cast<Vertex2Proc*>(game_base+0xdde798);auto v3=reinterpret_cast<Vertex3Proc*>(game_base+0xdde780);
 if(!writable_slot(reinterpret_cast<uintptr_t>(b))||!writable_slot(reinterpret_cast<uintptr_t>(v2))||!writable_slot(reinterpret_cast<uintptr_t>(v3))||!*b||!*v2||!*v3)return false;
 native_begin=*b;native_vertex2=*v2;native_vertex3=*v3;
 *b=capture_begin;*v2=capture_v2;*v3=capture_v3;capture_depth=1;return true;
}
inline void stop_capture(){
 if(!capture_depth||--capture_depth)return;
 *reinterpret_cast<BeginProc*>(game_base+0xdde708)=native_begin;
 *reinterpret_cast<Vertex2Proc*>(game_base+0xdde798)=native_vertex2;
 *reinterpret_cast<Vertex3Proc*>(game_base+0xdde780)=native_vertex3;
 primitive_capture=false;
}
inline void apply_bounds(int index,const Bounds& bounds){
 if(bounds.points<2)return;
 float w=bounds.right-bounds.left,h=bounds.bottom-bounds.top;
 if(w<.001f||h<.001f||w>1.1f||h>1.1f)return;
 // Expanded map keeps its native zoom behavior; do not replace the minimap box.
 if(index==2&&(w>.5f||h>.8f))return;
 auto& item=items[index];bool original=!changed(item);
 if(!original){item.x+=item.scale*(bounds.left-item.home_x);item.y+=item.scale*(bounds.top-item.home_y);}
 item.home_x=bounds.left;item.home_y=bounds.top;item.w=w;item.h=h;
 if(original){item.x=item.home_x;item.y=item.home_y;}
 measured[index]=true;
}
inline bool finish_frame(int width,int height,ULONGLONG now){
 bool changed_bounds=false;
 for(int i=0;i<int(items.size());++i){
  if(measured_frame[i].points){last_seen[i]=now;if(!freeze_bounds){apply_bounds(i,measured_frame[i]);changed_bounds=true;}}
  measured_frame[i]={};primitive_count[i]=0;
 }
 if(view_w&&view_h&&(width!=view_w||height!=view_h))measured.fill(false);
 view_w=width;view_h=height;
 capture_frame=now-capture_at>=(editing?150u:1000u);
 if(capture_frame)capture_at=now;
 return changed_bounds;
}
inline bool visible(int index,ULONGLONG now){return measured[index]&&last_seen[index]&&now-last_seen[index]<2000;}
