#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <algorithm>
#include <cstdint>
namespace ssc_render {
struct Dispatch {
 using Use=void(APIENTRY*)(GLuint);using Active=void(APIENTRY*)(GLenum);using Bind=void(APIENTRY*)(GLenum,GLuint);using Equation=void(APIENTRY*)(GLenum);
 using Gen=void(APIENTRY*)(GLsizei,GLuint*);using Delete=void(APIENTRY*)(GLsizei,const GLuint*);
 using Texture=void(APIENTRY*)(GLenum,GLenum,GLenum,GLuint,GLint);using Check=GLenum(APIENTRY*)(GLenum);
 using Blit=void(APIENTRY*)(GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum);
 HGLRC context=nullptr;int texture_units=1;Use use=nullptr;Active active=nullptr;Bind framebuffer=nullptr,buffer=nullptr;Equation equation=nullptr;
 Gen gen_framebuffers=nullptr;Delete delete_framebuffers=nullptr;Texture texture=nullptr;Check check=nullptr;Blit blit=nullptr;
 template<class T,class Lookup>static T load(const char* name,Lookup lookup){auto p=lookup(name);auto v=reinterpret_cast<intptr_t>(p);return v>3&&v!=-1?reinterpret_cast<T>(p):nullptr;}
 template<class Lookup,class Query>void update(HGLRC next,Lookup lookup,Query query){
  if(context==next)return;
  *this={};context=next;if(!next)return;
  use=load<Use>("glUseProgram",lookup);active=load<Active>("glActiveTexture",lookup);framebuffer=load<Bind>("glBindFramebuffer",lookup);buffer=load<Bind>("glBindBuffer",lookup);equation=load<Equation>("glBlendEquation",lookup);
  gen_framebuffers=load<Gen>("glGenFramebuffers",lookup);delete_framebuffers=load<Delete>("glDeleteFramebuffers",lookup);texture=load<Texture>("glFramebufferTexture2D",lookup);check=load<Check>("glCheckFramebufferStatus",lookup);blit=load<Blit>("glBlitFramebuffer",lookup);
  GLint units=1;query(0x84e2,&units);texture_units=std::clamp(int(units),1,32);
 }
 bool pixel_supported()const{return framebuffer&&gen_framebuffers&&delete_framebuffers&&texture&&check&&blit;}
};
inline Dispatch dispatch;
struct Pixelation {
 HGLRC context=nullptr;GLuint fbo=0,texture=0;int width=0,height=0;bool failed=false,blocked_msaa=false;
 bool apply(const Dispatch& d,int w,int h){
  if(!d.pixel_supported()||w<4||h<4||w>16384||h>16384)return false;
  if(context!=d.context){context=d.context;fbo=texture=0;width=height=0;failed=false;blocked_msaa=false;}
  if(failed)return false;
  const int small_w=std::min(2048,(w+3)/4),small_h=std::min(2048,(h+3)/4);
  GLint read=0,draw=0,bound=0;glGetIntegerv(0x8caa,&read);glGetIntegerv(0x8ca6,&draw);glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);
  struct Restore {const Dispatch& d;GLint read,draw,bound;~Restore(){d.framebuffer(0x8ca8,GLuint(read));d.framebuffer(0x8ca9,GLuint(draw));glBindTexture(GL_TEXTURE_2D,GLuint(bound));}} restore{d,read,draw,bound};
  GLint samples=0;d.framebuffer(0x8ca9,0);glGetIntegerv(0x80a9,&samples);blocked_msaa=samples>1;if(blocked_msaa)return false;
  if(!fbo)d.gen_framebuffers(1,&fbo);
  if(!texture)glGenTextures(1,&texture);
  if(!fbo||!texture){failed=true;return false;}
  if(width!=small_w||height!=small_h){
   glBindTexture(GL_TEXTURE_2D,texture);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,small_w,small_h,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);
   d.framebuffer(0x8d40,fbo);d.texture(0x8d40,0x8ce0,GL_TEXTURE_2D,texture,0);
   if(d.check(0x8d40)!=0x8cd5){failed=true;return false;}width=small_w;height=small_h;
  }
  d.framebuffer(0x8ca8,0);d.framebuffer(0x8ca9,fbo);d.blit(0,0,w,h,0,0,width,height,GL_COLOR_BUFFER_BIT,GL_NEAREST);
  d.framebuffer(0x8ca8,fbo);d.framebuffer(0x8ca9,0);d.blit(0,0,width,height,0,0,w,h,GL_COLOR_BUFFER_BIT,GL_NEAREST);return true;
 }
};inline Pixelation pixelation;
}
