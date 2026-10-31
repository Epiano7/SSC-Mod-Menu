#pragma once
#include "freecam_capture_geometry.h"
#include "capture_submission.h"
#include "third_party/json.hpp"
#include <windows.h>
#include <wincodec.h>
#include <bcrypt.h>
#include <GL/gl.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <vector>

namespace ssc_freecam::capture {
using Json=nlohmann::json;
struct Image {int width=0,height=0;std::vector<unsigned char> rgb;Json metadata;std::string name;};
struct Package {Json manifest;std::vector<Image> images;bool contribute=false;std::string upload_code;};
inline Json vector_json(Vec3 p){return Json::array({p.x,p.y,p.z});}
inline Json pose(const Camera& c){return {{"position",vector_json(c.eye)},{"right",vector_json(c.right)},{"up",vector_json(c.up)},{"back",vector_json(c.back)}};}
template<class T> struct Com {T* p=nullptr;~Com(){if(p)p->Release();}T** put(){return &p;}T* operator->()const{return p;}};
inline bool write_png(const std::filesystem::path& path,const Image& image){
 if(image.width<=0||image.height<=0||image.rgb.size()!=size_t(image.width)*image.height*3)return false;
 Com<IWICImagingFactory> factory;Com<IWICStream> stream;Com<IWICBitmapEncoder> encoder;Com<IWICBitmapFrameEncode> frame;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,reinterpret_cast<void**>(factory.put())))||FAILED(factory->CreateStream(stream.put()))||FAILED(stream->InitializeFromFilename(path.c_str(),GENERIC_WRITE))||FAILED(factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.put()))||FAILED(encoder->Initialize(stream.p,WICBitmapEncoderNoCache))||FAILED(encoder->CreateNewFrame(frame.put(),nullptr))||FAILED(frame->Initialize(nullptr))||FAILED(frame->SetSize(image.width,image.height)))return false;
 WICPixelFormatGUID format=GUID_WICPixelFormat24bppBGR;
 if(FAILED(frame->SetPixelFormat(&format))||!IsEqualGUID(format,GUID_WICPixelFormat24bppBGR))return false;
 // OpenGL rows are bottom-up; PNG rows are top-down.
 std::vector<BYTE> row(size_t(image.width)*3);
 for(int y=image.height-1;y>=0;--y){const auto* source=image.rgb.data()+size_t(y)*image.width*3;for(int x=0;x<image.width;++x){row[x*3]=source[x*3+2];row[x*3+1]=source[x*3+1];row[x*3+2]=source[x*3];}if(FAILED(frame->WritePixels(1,image.width*3,image.width*3,row.data())))return false;}
 return SUCCEEDED(frame->Commit())&&SUCCEEDED(encoder->Commit());
}
inline std::string executable_digest(){
 wchar_t path[32768];if(!GetModuleFileNameW(nullptr,path,32768))return {};
 std::ifstream in(std::filesystem::path(path),std::ios::binary);if(!in)return {};
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;unsigned char digest[32]{};
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
 bool ok=BCryptCreateHash(alg,&hash,nullptr,0,nullptr,0,0)>=0;char buffer[65536];
 while(ok&&in){in.read(buffer,sizeof buffer);auto n=in.gcount();if(n)ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer),ULONG(n),0)>=0;}
 ok=ok&&!in.bad()&&BCryptFinishHash(hash,digest,sizeof digest,0)>=0;if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);
 std::string result;if(ok)for(unsigned char b:digest){result+="0123456789abcdef"[b>>4];result+="0123456789abcdef"[b&15];}return result;
}
struct Writer {
 std::atomic<bool> busy{false};std::atomic<int> result{0};
 std::mutex retry_mutex;std::string retry_body;
 bool retry(){
  if(!ssc_capture_upload::enabled.load()||busy.exchange(true))return false;
  std::string body;{std::lock_guard<std::mutex> lock(retry_mutex);body=retry_body;}
  auto code=ssc_capture_upload::token();if(body.empty()||code.empty()){busy=false;return false;}
  HMODULE module=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&write_png),&module)){busy=false;return false;}
  try{std::thread([this,body=std::move(body),code=std::move(code),module](){
   if(ssc_capture_upload::send(body,code)){std::lock_guard<std::mutex> lock(retry_mutex);retry_body.clear();}
   busy=false;FreeLibrary(module);
  }).detach();}catch(...){FreeLibrary(module);busy=false;return false;}return true;
 }
 bool submit(std::filesystem::path root,Package package){
  if(busy.exchange(true))return false;
  result=0;
  // Process-lifetime writer pins the runtime, matching the other background services.
  HMODULE module=nullptr;
  if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&write_png),&module)){busy=false;return false;}
  try{std::thread([this,root=std::move(root),package=std::move(package),module]()mutable{
   bool ok=false;HRESULT com=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
   try{
    GUID id{};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("id");wchar_t uuid[40]{};StringFromGUID2(id,uuid,40);
    std::wstring name(uuid+1,36);std::string identifier(name.begin(),name.end());std::filesystem::create_directories(root);
    auto staging=root/(name+L".partial"),destination=root/name;
    if(!std::filesystem::create_directory(staging))throw std::runtime_error("directory");
    if(FAILED(com))throw std::runtime_error("com");
    package.manifest["capture_id"]=identifier;package.manifest["game_executable_sha256"]=executable_digest();package.manifest["media"]=Json::array();if(package.manifest["game_executable_sha256"].get<std::string>().size()!=64)throw std::runtime_error("digest");
    Json upload_images=Json::array();
    for(const auto& image:package.images){
     Json media=image.metadata;
     if(package.contribute){
      auto encoded=ssc_capture_upload::jpeg(image.width,image.height,image.rgb,image.name!="still");if(encoded.bytes.empty())throw std::runtime_error("jpeg");
      std::ofstream out(staging/(image.name+".jpg"),std::ios::binary);out.write(reinterpret_cast<const char*>(encoded.bytes.data()),encoded.bytes.size());out.close();if(!out)throw std::runtime_error("jpeg write");
      media["file"]=image.name+".jpg";media["width"]=encoded.width;media["height"]=encoded.height;
      upload_images.push_back({{"name",image.name},{"data",ssc_capture_upload::base64(encoded.bytes)}});
     }else{if(!write_png(staging/(image.name+".png"),image))throw std::runtime_error("png");media["file"]=image.name+".png";media["width"]=image.width;media["height"]=image.height;}
     package.manifest["media"].push_back(media);
    }
    {std::ofstream out(staging/L"manifest.json",std::ios::binary);out<<package.manifest.dump(2);out.flush();if(!out)throw std::runtime_error("manifest");}
    std::filesystem::rename(staging,destination);ok=true;
    if(package.contribute){Json body={{"schema","ssc.capture-submission"},{"version",1},{"manifest",package.manifest},{"images",std::move(upload_images)}};auto serialized=body.dump();{std::lock_guard<std::mutex> lock(retry_mutex);retry_body=serialized;}if(ssc_capture_upload::send(serialized,package.upload_code)){std::lock_guard<std::mutex> lock(retry_mutex);retry_body.clear();}}
   }catch(...){/* Incomplete exports remain .partial and must not be imported. */}
   if(SUCCEEDED(com))CoUninitialize();
   if(!ok&&package.contribute)ssc_capture_upload::status=-1;
   result=ok?1:-1;busy=false;
   FreeLibrary(module);
  }).detach();}catch(...){FreeLibrary(module);busy=false;return false;}return true;
 }
};
// Process lifetime storage avoids teardown racing a writer during DLL shutdown.
inline Writer& writer(){static auto* value=new Writer;return *value;}
struct Session {
 int stage=-1,width=0,height=0,face_size=0,still_width=0,still_height=0;Camera origin;uintptr_t world=0,actor=0;uint64_t started=0;bool rendered=false;
 std::array<float,16> projection{};Package package;
 bool active()const{return stage>=0;}
 void cancel(){stage=-1;rendered=false;package={};}
 bool begin(const Camera& c,uintptr_t a,uintptr_t w,uint64_t now){
  if(active()||writer().busy||!c.seeded||!finite(c.eye)||!a||!w)return false;
  cancel();origin=c;actor=a;world=w;started=now;stage=0;width=height=face_size=0;package.contribute=ssc_capture_upload::enabled.load();if(package.contribute)package.upload_code=ssc_capture_upload::token();
  FILETIME ft;GetSystemTimeAsFileTime(&ft);ULARGE_INTEGER utc{};utc.LowPart=ft.dwLowDateTime;utc.HighPart=ft.dwHighDateTime;
  package.manifest={{"schema","ssc.location-capture"},{"version",1},{"captured_utc_ms",(utc.QuadPart-116444736000000000ULL)/10000},{"coordinates",{{"axes","Z-up; X/Y native map plane"},{"units","native_world_units"},{"meters_per_unit",nullptr}}},{"map_id",nullptr},{"map_build",nullptr},{"calibration_id",nullptr},{"world_to_map",nullptr},{"validation_state","unreviewed_uncalibrated"},{"capture_pose",pose(c)},{"ground_verified",false},{"scene_coherence","singleplayer_simulation_frozen; render effects may animate"}};
  return true;
 }
 bool view(Camera& rendered_camera,float* p){
  if(!active())return false;
  GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);
  if(!width){width=viewport[2];height=viewport[3];face_size=std::min({width,height,1024});}
  if(width<=0||height<=0||width>8192||height>8192||size_t(width)*height>16000000||width!=viewport[2]||height!=viewport[3]||viewport[0]||viewport[1]||p[11]!=-1||p[15]!=0){cancel();return false;}
  rendered_camera=origin;
  if(!stage){
   still_width=width;still_height=height;
   if(package.contribute){
    constexpr float aspect=1.776544066620f;
    if(float(width)/height>aspect)still_width=int(height*aspect);else still_height=int(width/aspect);
    float near_plane=p[14]/(p[10]-1),far_plane=p[14]/(p[10]+1);auto standard=perspective(50.033786773682f,float(still_width)/still_height,near_plane,far_plane);
    if(!standard){cancel();return false;}float sx=std::copysign(1.f,p[0]),sy=std::copysign(1.f,p[5]);std::copy(standard->begin(),standard->end(),p);p[0]=sx*std::abs(p[0])*still_width/width;p[5]=sy*std::abs(p[5])*still_height/height;
   }
  }
  if(stage){rendered_camera=*face_camera(origin.eye,unsigned(stage-1));float near_plane=p[14]/(p[10]-1),far_plane=p[14]/(p[10]+1);auto cube=perspective(90,1,near_plane,far_plane);if(!cube){cancel();return false;}float sx=std::copysign(1.f,p[0]),sy=std::copysign(1.f,p[5]);std::copy(cube->begin(),cube->end(),p);p[0]=sx*float(face_size)/width;p[5]=sy*float(face_size)/height;}
  std::copy(p,p+16,projection.begin());rendered=true;return true;
 }
 bool read_frame(const std::filesystem::path& root){
  if(!active()||!rendered)return false;
  rendered=false;
  using Bind=void(APIENTRY*)(GLenum,GLuint);auto buffer=reinterpret_cast<Bind>(wglGetProcAddress("glBindBuffer"));auto framebuffer=reinterpret_cast<Bind>(wglGetProcAddress("glBindFramebuffer"));
  auto valid=[](auto p){auto n=reinterpret_cast<uintptr_t>(p);return n>3&&n!=~uintptr_t(0);};if(!valid(buffer)||!valid(framebuffer)){cancel();return false;}
  if(glGetError()!=GL_NO_ERROR){cancel();return false;}
  GLint pack=0,read_fbo=0,read_buffer=0,default_read=0;glGetIntegerv(0x88ed,&pack);glGetIntegerv(0x8caa,&read_fbo);glGetIntegerv(GL_READ_BUFFER,&read_buffer);
  Image image;image.width=stage?face_size:still_width;image.height=stage?face_size:still_height;image.name=stage?cube_faces[size_t(stage-1)].name:"still";image.rgb.resize(size_t(image.width)*image.height*3);
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPushAttrib(GL_PIXEL_MODE_BIT);buffer(0x88eb,0);framebuffer(0x8ca8,0);glGetIntegerv(GL_READ_BUFFER,&default_read);glReadBuffer(GL_BACK);glPixelTransferf(GL_RED_SCALE,1);glPixelTransferf(GL_GREEN_SCALE,1);glPixelTransferf(GL_BLUE_SCALE,1);glPixelTransferf(GL_RED_BIAS,0);glPixelTransferf(GL_GREEN_BIAS,0);glPixelTransferf(GL_BLUE_BIAS,0);glPixelTransferi(GL_MAP_COLOR,GL_FALSE);glPixelStorei(GL_PACK_ALIGNMENT,1);glPixelStorei(GL_PACK_ROW_LENGTH,0);glPixelStorei(GL_PACK_SKIP_ROWS,0);glPixelStorei(GL_PACK_SKIP_PIXELS,0);glPixelStorei(GL_PACK_SWAP_BYTES,GL_FALSE);
  glReadPixels((width-image.width)/2,(height-image.height)/2,image.width,image.height,GL_RGB,GL_UNSIGNED_BYTE,image.rgb.data());
  GLenum error=glGetError();glReadBuffer(default_read);framebuffer(0x8ca8,read_fbo);glReadBuffer(read_buffer);buffer(0x88eb,pack);glPopAttrib();glPopClientAttrib();if(error!=GL_NO_ERROR){cancel();return false;}
  // Keep native projection handedness while rendering (and therefore native
  // front/back-face culling). Normalize pixels only after readback, so exported
  // cube faces retain their documented right/up basis and 90-degree coverage.
  if(stage){
   if(projection[0]<0)for(int y=0;y<image.height;++y)for(int x=0;x<image.width/2;++x)for(int k=0;k<3;++k)std::swap(image.rgb[(size_t(y)*image.width+x)*3+k],image.rgb[(size_t(y)*image.width+image.width-1-x)*3+k]);
   if(projection[5]<0)for(int y=0;y<image.height/2;++y)for(int x=0;x<image.width*3;++x)std::swap(image.rgb[size_t(y)*image.width*3+x],image.rgb[size_t(image.height-1-y)*image.width*3+x]);
  }
  Camera c=stage?*face_camera(origin.eye,unsigned(stage-1)):origin;
  image.metadata={{"kind",stage?"cubemap_face":"still"},{"pose",pose(c)},{"vertical_fov_degrees",stage?90.f:package.contribute?50.033786773682f:float(360/3.14159265358979323846*std::atan(1/std::abs(projection[5])))},{"rendered_viewport_projection_column_major",projection},{"source_viewport",{width,height}},{"crop_xy",{(width-image.width)/2,(height-image.height)/2}}};
  if(stage)image.metadata["pixel_orientation"]="normalized_to_pose_right_up";
  package.manifest["capture_renderer_revision"]=2;
  package.images.push_back(std::move(image));if(++stage<7)return true;
  stage=-1;return writer().submit(root,std::move(package));
 }
};
inline Session session;
}
