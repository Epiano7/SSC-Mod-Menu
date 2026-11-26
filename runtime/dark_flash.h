#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <atomic>
#include <array>
#include <string>
#include <map>
#include <set>
#include <mutex>
#include <cstring>
#include <cmath>
#include "flash_image.h"
namespace ssc_dark_flash {
inline std::atomic<bool> enabled{false};
inline void(*diagnostic)(const char*)=nullptr;inline std::atomic<bool> reported{false};
// Only the native flash uniform's sign is encoded. Magnitude and game state are untouched.
inline float encode(float value,bool dark){return dark&&std::isfinite(value)&&value>0?-value:value;}
inline bool rewrite(std::string& source){
 if(source.find("uniform float flashIntensity;")==std::string::npos)return false;
 const std::string a="baseColour.rgb += flashIntensity;",b="baseColour.rgb += flashIntensity*0.3;";
 auto copy=source;unsigned count=0;
 for(const auto& term:{a,b}){size_t pos=0;const std::string expression=term==a?"flashIntensity":"flashIntensity*0.3";
  // Image adds only a scene-independent signal to the dark flash. It does not
  // restore scene contrast or use an independent fade clock.
  const std::string replacement="baseColour.rgb = flashIntensity < 0.0 ? max(baseColour.rgb + "+expression+", vec3(0.0)) : baseColour.rgb + "+expression+";";
  while((pos=copy.find(term,pos))!=std::string::npos){copy.replace(pos,term.size(),replacement);pos+=replacement.size();++count;}
 }
 if(count<1||count>2)return false;
 // Reject new/unknown shader uses of this uniform instead of applying a partial rewrite.
 auto stripped=source;for(const auto& term:{std::string("uniform float flashIntensity;"),a,b}){size_t pos;while((pos=stripped.find(term))!=std::string::npos)stripped.erase(pos,term.size());}
 if(stripped.find("flashIntensity")!=std::string::npos)return false;
 // Custom images are composited once after all native scene and HUD draws.
 source=std::move(copy);return true;
}
using Source=void(APIENTRY*)(GLuint,GLsizei,const char* const*,const GLint*);
using Attach=void(APIENTRY*)(GLuint,GLuint);
using Use=void(APIENTRY*)(GLuint);
using Location=GLint(APIENTRY*)(GLuint,const char*);
using Uniform=void(APIENTRY*)(GLint,GLfloat);
using Delete=void(APIENTRY*)(GLuint);
inline Source original_source=nullptr;inline Attach original_attach=nullptr;inline Use original_use=nullptr;
inline Location original_location=nullptr;inline Uniform original_uniform=nullptr;
inline Delete original_delete_program=nullptr,original_delete_shader=nullptr;
inline std::mutex mutex;
inline std::set<GLuint> shaders,programs;
inline std::map<GLuint,GLint> locations;
inline std::atomic<uint64_t> program_revision{1};
inline thread_local GLuint current_program=0;
struct FlashBinding { GLuint program=0; uint64_t revision=0; GLint index=-1; GLint reported_index=-1; };
inline thread_local std::array<FlashBinding,64> bindings{};
inline GLint flash_index(GLuint program){
 if(!program)return -1;
 auto& entry=bindings[program%bindings.size()];
 const auto revision=program_revision.load(std::memory_order_acquire);
 if(entry.program==program&&entry.revision==revision)return entry.index;
 std::lock_guard<std::mutex> lock(mutex);
 const auto location=locations.find(program);
 const GLint observed=location==locations.end()?-1:location->second;
 entry={program,program_revision.load(std::memory_order_relaxed),
        programs.count(program)?observed:-1,observed};
 return entry.index;
}
inline void APIENTRY shader_source(GLuint shader,GLsizei count,const char* const* strings,const GLint* lengths){
 try{
  if(count>0&&count<=128&&strings){std::string joined;bool valid=true;
   for(int i=0;i<count;++i){if(!strings[i]){valid=false;break;}size_t n=lengths&&lengths[i]>=0?size_t(lengths[i]):strnlen(strings[i],1048577);if(n>1048576||joined.size()+n>2097152){valid=false;break;}joined.append(strings[i],n);}
   if(valid&&rewrite(joined)){const char* s=joined.data();GLint size=GLint(joined.size());original_source(shader,1,&s,&size);std::lock_guard<std::mutex> lock(mutex);shaders.insert(shader);if(!reported.exchange(true)&&diagnostic)diagnostic("Dark flash color shader prepared; native intensity and timing unchanged");return;}
  }
  {std::lock_guard<std::mutex> lock(mutex);shaders.erase(shader);}
 }catch(...){}
 original_source(shader,count,strings,lengths);
}
inline void APIENTRY attach_shader(GLuint program,GLuint shader){
 original_attach(program,shader);
 std::lock_guard<std::mutex> lock(mutex);
 if(shaders.count(shader)&&programs.insert(program).second)
  program_revision.fetch_add(1,std::memory_order_release);
}
inline void APIENTRY use_program(GLuint program){original_use(program);current_program=program;}
inline GLint APIENTRY location(GLuint program,const char* name){
 auto result=original_location(program,name);
 if(result>=0&&name&&!std::strcmp(name,"flashIntensity")){
  auto& entry=bindings[program%bindings.size()];
  auto revision=program_revision.load(std::memory_order_acquire);
  if(entry.program==program&&entry.revision==revision&&entry.reported_index==result)return result;
  std::lock_guard<std::mutex> lock(mutex);
  auto it=locations.find(program);
  revision=program_revision.load(std::memory_order_relaxed);
  if(it==locations.end()||it->second!=result){
   locations[program]=result;
   revision=program_revision.fetch_add(1,std::memory_order_release)+1;
  }
  entry={program,revision,programs.count(program)?result:-1,result};
 }
 return result;
}
// One frame-owned sample: no second fade timer and no visibility changes to game state.
inline thread_local HGLRC intensity_context=nullptr;
inline thread_local float frame_intensity=0;
inline void observe_intensity(HGLRC context,float value){if(intensity_context!=context){intensity_context=context;frame_intensity=0;}if(std::isfinite(value)&&value>=0)frame_intensity=std::max(frame_intensity,value);}
inline float take_intensity(HGLRC context){const float value=intensity_context==context?frame_intensity:0;frame_intensity=0;return value;}
inline void APIENTRY uniform(GLint index,GLfloat value){
 if(!enabled.load(std::memory_order_relaxed)){original_uniform(index,value);return;}
 if(index>=0&&flash_index(current_program)==index){
  observe_intensity(wglGetCurrentContext(),value);
  value=encode(value,enabled.load(std::memory_order_relaxed));
 }
 original_uniform(index,value);
}
inline void APIENTRY delete_program(GLuint program){
 original_delete_program(program);
 std::lock_guard<std::mutex> lock(mutex);
 const auto removed=programs.erase(program)+locations.erase(program);
 if(removed)program_revision.fetch_add(1,std::memory_order_release);
}
inline void APIENTRY delete_shader(GLuint shader){original_delete_shader(shader);std::lock_guard<std::mutex> lock(mutex);shaders.erase(shader);}
inline PROC lookup(const char* name,PROC original){
 if(!name||reinterpret_cast<INT_PTR>(original)<=3||reinterpret_cast<INT_PTR>(original)==-1)return original;
 #define SSC_FLASH_PROC(label,type,storage,hook) if(!std::strcmp(name,label)){storage=reinterpret_cast<type>(original);return reinterpret_cast<PROC>(hook);}
 SSC_FLASH_PROC("glShaderSource",Source,original_source,shader_source)
 SSC_FLASH_PROC("glAttachShader",Attach,original_attach,attach_shader)
 SSC_FLASH_PROC("glUseProgram",Use,original_use,use_program)
 SSC_FLASH_PROC("glGetUniformLocation",Location,original_location,location)
 SSC_FLASH_PROC("glUniform1f",Uniform,original_uniform,uniform)
 SSC_FLASH_PROC("glDeleteProgram",Delete,original_delete_program,delete_program)
 SSC_FLASH_PROC("glDeleteShader",Delete,original_delete_shader,delete_shader)
 #undef SSC_FLASH_PROC
 return original;
}
}
extern "C" __declspec(dllexport) PROC WINAPI SscModLookupGL(const char* name,PROC original){return ssc_dark_flash::lookup(name,original);}
