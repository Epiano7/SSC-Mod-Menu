#pragma once
// Included inside ssc_hud. Read native immediate-mode geometry only while calibrating
// or editing; pointer slots are restored at the end of each outer HUD draw.
struct Bounds {float left=10,top=10,right=-10,bottom=-10;unsigned points=0;};
inline std::array<Bounds,9> measured_frame{},queued_frame{};
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
using Color4Proc=void(APIENTRY*)(GLfloat,GLfloat,GLfloat,GLfloat);
using Color3Proc=void(APIENTRY*)(GLfloat,GLfloat,GLfloat);
using EndProc=void(APIENTRY*)();
inline Color4Proc native_color4=nullptr;
inline Color3Proc native_color3=nullptr;
inline EndProc native_end=nullptr;
using CallListProc=void(APIENTRY*)(GLuint);
inline CallListProc native_call_list=nullptr;
inline float capture_alpha=1;
inline Bounds primitive_bounds{};
inline bool primitive_visible=false;
inline unsigned primitive_vertices=0,primitive_size=0;
inline void commit_primitive(){
 if(primitive_visible&&primitive_bounds.points&&primitive_bounds.right>primitive_bounds.left&&primitive_bounds.bottom>primitive_bounds.top&&active_item>=0){auto& out=measured_frame[active_item];const auto& b=primitive_bounds;out.left=std::min(out.left,b.left);out.top=std::min(out.top,b.top);out.right=std::max(out.right,b.right);out.bottom=std::max(out.bottom,b.bottom);out.points+=b.points;}
 primitive_bounds={};primitive_visible=false;primitive_vertices=0;
}
inline bool primitive_capture=false;
inline GLfloat capture_model[16],capture_projection[16];
inline void accumulate(int index,float x,float y){
 if(index<0||index>=int(items.size())||!std::isfinite(x)||!std::isfinite(y))return;
 const auto& item=items[index];
 if(enabled){x=item.home_x+(x-item.x)/item.scale;y=item.home_y+(y-item.y)/item.scale;}
 if(x<-.5f||x>1.5f||y<-.5f||y>1.5f)return;
 auto& b=primitive_bounds;b.left=std::min(b.left,x);b.right=std::max(b.right,x);b.top=std::min(b.top,y);b.bottom=std::max(b.bottom,y);++b.points;
}
inline void capture_vertex(float x,float y,float z){
 if(!primitive_capture||active_item<0)return;
 const float in[]={x,y,z,1};float world[4]{},clip[4]{};
 for(int r=0;r<4;++r)for(int c=0;c<4;++c)world[r]+=capture_model[c*4+r]*in[c];
 for(int r=0;r<4;++r)for(int c=0;c<4;++c)clip[r]+=capture_projection[c*4+r]*world[c];
 if(capture_alpha>0)primitive_visible=true;
 if(std::abs(clip[3])>.00001f)accumulate(active_item,(clip[0]/clip[3]+1)*.5f,(1-clip[1]/clip[3])*.5f);
 if(++primitive_vertices==primitive_size)commit_primitive();
}
inline void APIENTRY capture_begin(GLenum mode){
 primitive_bounds={};primitive_visible=false;primitive_vertices=0;primitive_size=mode==GL_QUADS?4:mode==GL_TRIANGLES?3:mode==GL_LINES?2:0;
 primitive_capture=active_item>=0&&(!measured[active_item]||editing||enabled)&&primitive_count[active_item]<256;
 if(primitive_capture){
  GLfloat color[4];glGetFloatv(GL_CURRENT_COLOR,color);capture_alpha=color[3];
  GLint stencil=GL_ALWAYS;glGetIntegerv(GL_STENCIL_FUNC,&stencil);
  // The map's stencil mask gives its visible bounds. Ignore the clipped world
  // geometry beneath it, which can extend far beyond the minimap rectangle.
  if(active_item==2&&glIsEnabled(GL_STENCIL_TEST)&&stencil!=GL_ALWAYS)primitive_capture=false;
  else if(primitive_capture){++primitive_count[active_item];glGetFloatv(GL_MODELVIEW_MATRIX,capture_model);glGetFloatv(GL_PROJECTION_MATRIX,capture_projection);}
 }
 native_begin(mode);
}
inline void APIENTRY capture_color4(GLfloat r,GLfloat g,GLfloat b,GLfloat a){capture_alpha=a;native_color4(r,g,b,a);}
inline void APIENTRY capture_color3(GLfloat r,GLfloat g,GLfloat b){capture_alpha=1;native_color3(r,g,b);}
inline void APIENTRY capture_end(){
 if(primitive_capture&&!primitive_size)commit_primitive();
 primitive_capture=false;native_end();
}
inline void APIENTRY capture_v2(GLfloat x,GLfloat y){capture_vertex(x,y,0);native_vertex2(x,y);}
inline void APIENTRY capture_v3(GLfloat x,GLfloat y,GLfloat z){capture_vertex(x,y,z);native_vertex3(x,y,z);}
// The native sprite cache replays unit quads through display lists. Their
// vertices bypass our glBegin/glVertex hooks after the first frame.
inline void capture_cached_quad(){
 // The map stencil and immediate caption already define its visible region.
 if(active_item<0||active_item==2||!capture_frame||freeze_bounds)return;
 GLint stencil=GL_ALWAYS;glGetIntegerv(GL_STENCIL_FUNC,&stencil);
 if(active_item==2&&glIsEnabled(GL_STENCIL_TEST)&&stencil!=GL_ALWAYS)return;
 GLfloat color[4];glGetFloatv(GL_CURRENT_COLOR,color);if(color[3]<=0)return;
 glGetFloatv(GL_MODELVIEW_MATRIX,capture_model);glGetFloatv(GL_PROJECTION_MATRIX,capture_projection);
 primitive_bounds={};primitive_visible=false;primitive_vertices=0;primitive_size=4;primitive_capture=true;capture_alpha=color[3];
 capture_vertex(0,0,0);capture_vertex(1,0,0);capture_vertex(1,1,0);capture_vertex(0,1,0);primitive_capture=false;
}
inline __attribute__((noinline)) void APIENTRY capture_call_list(GLuint list){
 if(reinterpret_cast<uintptr_t>(__builtin_return_address(0))==game_base+ssc_compat::resolve(0x47bc17))capture_cached_quad();
 native_call_list(list);
}
inline bool writable_slot(uintptr_t address){MEMORY_BASIC_INFORMATION info{};return VirtualQuery(reinterpret_cast<void*>(address),&info,sizeof(info))&&info.State==MEM_COMMIT&&(info.Protect&(PAGE_READWRITE|PAGE_EXECUTE_READWRITE));}
inline bool start_capture(){
 if(!game_base||!capture_frame||game_base!=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)))return false;
 if(capture_depth){++capture_depth;return true;}
 auto b=reinterpret_cast<BeginProc*>(game_base+ssc_compat::resolve(0xde1710));auto v2=reinterpret_cast<Vertex2Proc*>(game_base+ssc_compat::resolve(0xde17a0));auto v3=reinterpret_cast<Vertex3Proc*>(game_base+ssc_compat::resolve(0xde1788));
 auto c4=reinterpret_cast<Color4Proc*>(game_base+ssc_compat::resolve(0xde1690));auto c3=reinterpret_cast<Color3Proc*>(game_base+ssc_compat::resolve(0xde1838));auto end=reinterpret_cast<EndProc*>(game_base+ssc_compat::resolve(0xde16b8));
 if(!writable_slot(reinterpret_cast<uintptr_t>(c4))||!writable_slot(reinterpret_cast<uintptr_t>(c3))||!writable_slot(reinterpret_cast<uintptr_t>(end))||!*c4||!*c3||!*end)return false;
 if(!writable_slot(reinterpret_cast<uintptr_t>(b))||!writable_slot(reinterpret_cast<uintptr_t>(v2))||!writable_slot(reinterpret_cast<uintptr_t>(v3))||!*b||!*v2||!*v3)return false;
 auto list=reinterpret_cast<CallListProc*>(game_base+ssc_compat::resolve(0xde1670));
 if(!writable_slot(reinterpret_cast<uintptr_t>(list))||!*list)return false;
 native_call_list=*list;*list=capture_call_list;
 native_color4=*c4;native_color3=*c3;native_end=*end;*c4=capture_color4;*c3=capture_color3;*end=capture_end;
 native_begin=*b;native_vertex2=*v2;native_vertex3=*v3;
 *b=capture_begin;*v2=capture_v2;*v3=capture_v3;capture_depth=1;return true;
}
inline void stop_capture(){
 if(!capture_depth||--capture_depth)return;
 *reinterpret_cast<CallListProc*>(game_base+ssc_compat::resolve(0xde1670))=native_call_list;
 *reinterpret_cast<Color4Proc*>(game_base+ssc_compat::resolve(0xde1690))=native_color4;
 *reinterpret_cast<Color3Proc*>(game_base+ssc_compat::resolve(0xde1838))=native_color3;
 *reinterpret_cast<EndProc*>(game_base+ssc_compat::resolve(0xde16b8))=native_end;
 *reinterpret_cast<BeginProc*>(game_base+ssc_compat::resolve(0xde1710))=native_begin;
 *reinterpret_cast<Vertex2Proc*>(game_base+ssc_compat::resolve(0xde17a0))=native_vertex2;
 *reinterpret_cast<Vertex3Proc*>(game_base+ssc_compat::resolve(0xde1788))=native_vertex3;
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
// Native batched sprite/glyph rectangles, before any GL batching or flushing.
inline void capture_queued_rectangle(const unsigned char* stack){
 if(active_item<0||!capture_frame||freeze_bounds||!stack)return;
 int width=view_w,height=view_h;
 if(game_base==reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))){width=*reinterpret_cast<const int*>(game_base+ssc_compat::resolve(0xfa2750));height=*reinterpret_cast<const int*>(game_base+ssc_compat::resolve(0xfa2754));}
 if(width<=0||height<=0)return;
 if(active_item==2){GLint stencil=GL_ALWAYS;glGetIntegerv(GL_STENCIL_FUNC,&stencil);if(glIsEnabled(GL_STENCIL_TEST)&&stencil!=GL_ALWAYS)return;}
 auto value=[&](int argument){float v;std::memcpy(&v,stack+8*(argument-1),4);return v;};
 // stack points at argument 5 (the first stack-passed argument).
 float x=value(2),y=value(3),w=value(4),h=value(5),alpha=value(13),alpha2=value(14);
 if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(w)||!std::isfinite(h)||!(alpha>0||alpha2>0)||w==0||h==0)return;
 auto& b=queued_frame[active_item];float left=std::min(x,x+w)/width,right=std::max(x,x+w)/width,top=std::min(y,y+h)/height,bottom=std::max(y,y+h)/height;
 if(left<-.5f||right>1.5f||top<-.5f||bottom>1.5f)return;
 b.left=std::min(b.left,left);b.top=std::min(b.top,top);b.right=std::max(b.right,right);b.bottom=std::max(b.bottom,bottom);b.points+=4;
}
inline bool finish_frame(int width,int height,ULONGLONG now){
 bool changed_bounds=false;
 for(int i=0;i<int(items.size());++i){
  if(queued_frame[i].points)measured_frame[i]=queued_frame[i];
  queued_frame[i]={};
  if(measured_frame[i].points){last_seen[i]=now;if(!freeze_bounds){apply_bounds(i,measured_frame[i]);changed_bounds=true;}}
  measured_frame[i]={};primitive_count[i]=0;
 }
 if(view_w&&view_h&&(width!=view_w||height!=view_h))measured.fill(false);
 view_w=width;view_h=height;
 capture_frame=now-capture_at>=(editing?150u:250u);
 if(capture_frame)capture_at=now;
 return changed_bounds;
}
inline bool visible(int index,ULONGLONG now){return measured[index]&&last_seen[index]&&now-last_seen[index]<2000;}
