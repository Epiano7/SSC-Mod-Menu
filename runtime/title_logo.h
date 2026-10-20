#pragma once
#include <windows.h>
#include <GL/gl.h>
#include <bcrypt.h>
#include <wincodec.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <mutex>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include "assets/larpshot_logo_data.h"

// Texture substitution only: never modify the game's logo or other assets on disk.
namespace ssc_title {
using Upload=void(APIENTRY*)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
template<class T> struct Com {T* p=nullptr;~Com(){if(p)p->Release();} T** out(){return &p;}};
struct Image {UINT w=0,h=0;std::vector<unsigned char> rgba;};
inline Upload original=nullptr;
using Query=void(APIENTRY*)(GLenum,GLint*);
inline Query query=nullptr;
inline Image reference,replacement;
inline std::once_flag selection;
inline bool selected=false,ready=false;
inline std::filesystem::path preview_marker;
inline bool preview=false,reported=false;
inline DWORD preview_attributes=0,preview_error=0;
inline std::filesystem::path state_path(){
 wchar_t value[32768];DWORD n=GetEnvironmentVariableW(L"SSCMODS_STATE_DIR",value,32768);
 if(n&&n<32768)return value;
 n=GetEnvironmentVariableW(L"LOCALAPPDATA",value,32768);
 return n&&n<32768?std::filesystem::path(value)/L"SkillshotCityMod":std::filesystem::path{};
}
inline std::filesystem::path installed_state(){
 wchar_t exe[32768];DWORD n=GetModuleFileNameW(nullptr,exe,32768);
 if(!n||n>=32768)return {};
 auto path=std::filesystem::path(std::wstring(exe,n));
 if(_wcsicmp(path.filename().c_str(),L"SkillshotCity.exe"))return {};
 return path.parent_path()/L"SSCMods";
}
inline void diagnostic(const char* message){try{
 auto path=state_path();if(!path.empty())std::ofstream(path/L"title-logo.log",std::ios::app)<<message<<'\n';
 auto installed=installed_state();if(!installed.empty())std::ofstream(installed/L"title-logo.log",std::ios::app)<<message<<'\n';
}catch(...) {}}
inline bool select_preview(const std::filesystem::path& installed,const std::filesystem::path& state){
 for(const auto& directory:{installed,state}){
  if(directory.empty())continue;
  auto marker=directory/L"larpshot-once.flag";DWORD attributes=GetFileAttributesW(marker.c_str());
  preview_attributes=attributes;preview_error=attributes==INVALID_FILE_ATTRIBUTES?GetLastError():0;
  if(attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY)){preview_marker=marker;return true;}
 }
 return false;
}
// Reject the incomplete final bucket instead of introducing modulo bias.
inline int lottery(uint32_t value){return value>=4294967200u?-1:value%100==0?1:0;}
inline bool roll(){uint32_t value=0;for(unsigned i=0;i<8;++i){if(BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&value),sizeof value,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)return false;int result=lottery(value);if(result>=0)return result==1;}return false;}
inline bool decode(IWICImagingFactory* factory,IWICBitmapDecoder* decoder,Image& image,UINT width=0,UINT height=0){
 Com<IWICBitmapFrameDecode> frame;Com<IWICFormatConverter> converter;Com<IWICBitmapScaler> scaler;
 if(FAILED(decoder->GetFrame(0,frame.out())))return false;
 if(FAILED(frame.p->GetSize(&image.w,&image.h))||!image.w||!image.h||image.w>4096||image.h>4096)return false;
 IWICBitmapSource* source=frame.p;
 if(width&&height&&(width!=image.w||height!=image.h)){
  if(FAILED(factory->CreateBitmapScaler(scaler.out()))||FAILED(scaler.p->Initialize(source,width,height,WICBitmapInterpolationModeFant)))return false;
  source=scaler.p;image.w=width;image.h=height;
 }
 if(FAILED(factory->CreateFormatConverter(converter.out()))||FAILED(converter.p->Initialize(source,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
 image.rgba.resize(size_t(image.w)*image.h*4);
 return SUCCEEDED(converter.p->CopyPixels(nullptr,image.w*4,UINT(image.rgba.size()),image.rgba.data()));
}
struct Bounds {UINT left=0,top=0,right=0,bottom=0;};
inline Bounds solid_bounds(const Image& image){
 Bounds b{image.w,image.h,0,0};
 for(UINT y=0;y<image.h;++y)for(UINT x=0;x<image.w;++x)if(image.rgba[(size_t(y)*image.w+x)*4+3]>=200){
  b.left=std::min(b.left,x);b.top=std::min(b.top,y);b.right=std::max(b.right,x+1);b.bottom=std::max(b.bottom,y+1);
 }return b;
}
inline bool align_artwork(IWICImagingFactory* factory){
 const auto to=solid_bounds(reference),from=solid_bounds(replacement);
 if(to.right<=to.left||to.bottom<=to.top||from.right<=from.left||from.bottom<=from.top)return false;
 const double scale=std::min(double(to.right-to.left)/(from.right-from.left),double(to.bottom-to.top)/(from.bottom-from.top));
 const UINT w=std::max(1u,UINT(replacement.w*scale+0.5)),h=std::max(1u,UINT(replacement.h*scale+0.5));
 Com<IWICBitmap> bitmap;Com<IWICBitmapScaler> scaler;Com<IWICFormatConverter> converter;
 if(FAILED(factory->CreateBitmapFromMemory(replacement.w,replacement.h,GUID_WICPixelFormat32bppRGBA,replacement.w*4,UINT(replacement.rgba.size()),replacement.rgba.data(),bitmap.out()))||
    FAILED(factory->CreateBitmapScaler(scaler.out()))||FAILED(scaler.p->Initialize(bitmap.p,w,h,WICBitmapInterpolationModeFant)))return false;
 // WIC scaling may return a different channel order/premultiplied format.
 // Explicitly restore straight RGBA before treating the result as RGBA bytes.
 if(FAILED(factory->CreateFormatConverter(converter.out()))||FAILED(converter.p->Initialize(scaler.p,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
 std::vector<unsigned char> pixels(size_t(w)*h*4);
 if(FAILED(converter.p->CopyPixels(nullptr,w*4,UINT(pixels.size()),pixels.data())))return false;
 const int left=int((to.left+to.right)*0.5-(from.left+from.right)*0.5*w/replacement.w+0.5);
 const int top=int((to.top+to.bottom)*0.5-(from.top+from.bottom)*0.5*h/replacement.h+0.5);
 replacement.rgba.assign(size_t(reference.w)*reference.h*4,0);replacement.w=reference.w;replacement.h=reference.h;
 for(UINT y=0;y<h;++y)for(UINT x=0;x<w;++x){int dx=left+int(x),dy=top+int(y);if(dx>=0&&dy>=0&&dx<int(reference.w)&&dy<int(reference.h))
  std::copy_n(pixels.data()+(size_t(y)*w+x)*4,4,replacement.rgba.data()+(size_t(dy)*reference.w+dx)*4);
 }return true;
}
inline bool load(const wchar_t* logo){
 HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
 bool success=false;
 {Com<IWICImagingFactory> factory;Com<IWICBitmapDecoder> native,alternate;Com<IWICStream> stream;
  if(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,reinterpret_cast<void**>(factory.out())))&&
     SUCCEEDED(factory.p->CreateDecoderFromFilename(logo,nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,native.out()))&&decode(factory.p,native.p,reference)&&
     SUCCEEDED(factory.p->CreateStream(stream.out()))&&SUCCEEDED(stream.p->InitializeFromMemory(const_cast<BYTE*>(larpshot_png),sizeof larpshot_png))&&
     SUCCEEDED(factory.p->CreateDecoderFromStream(stream.p,nullptr,WICDecodeMetadataCacheOnLoad,alternate.out())))
    success=decode(factory.p,alternate.p,replacement)&&align_artwork(factory.p);
 }
 if(SUCCEEDED(initialized))CoUninitialize();
 if(!success){reference={};replacement={};}
 return success;
}
inline bool matches(const unsigned char* data,bool bgra,bool flipped){
 for(UINT y=0;y<reference.h;++y)for(UINT x=0;x<reference.w;++x){
  size_t a=(size_t(y)*reference.w+x)*4,b=(size_t(flipped?reference.h-1-y:y)*reference.w+x)*4;
  const auto* r=reference.rgba.data()+b;
  if(data[a+3]!=r[3])return false;
  // Decoders may clear invisible RGB. Alpha and every visible pixel must match.
  if(r[3]&&(data[a+(bgra?2:0)]!=r[0]||data[a+1]!=r[1]||data[a+(bgra?0:2)]!=r[2]))return false;
 }
 return true;
}
inline bool substitute(const void* pixels,GLenum format,std::vector<unsigned char>& output){
 if(!pixels||!ready||(format!=GL_RGBA&&format!=0x80E1))return false;
 bool bgra=format==0x80E1,flipped=false;
 if(!matches(static_cast<const unsigned char*>(pixels),bgra,false)){flipped=true;if(!matches(static_cast<const unsigned char*>(pixels),bgra,true))return false;}
 output.resize(replacement.rgba.size());
 for(UINT y=0;y<reference.h;++y)for(UINT x=0;x<reference.w;++x){
  size_t a=(size_t(y)*reference.w+x)*4,b=(size_t(flipped?reference.h-1-y:y)*reference.w+x)*4;
  const auto* r=replacement.rgba.data()+b;output[a]=r[bgra?2:0];output[a+1]=r[1];output[a+2]=r[bgra?0:2];output[a+3]=r[3];
 }
 return true;
}
inline void APIENTRY upload(GLenum target,GLint level,GLint internal,GLsizei width,GLsizei height,GLint border,GLenum format,GLenum type,const void* pixels){
 if(ready&&query&&pixels&&target==GL_TEXTURE_2D&&level==0&&border==0&&width==int(reference.w)&&height==int(reference.h)&&type==GL_UNSIGNED_BYTE&&(format==GL_RGBA||format==0x80E1)){
  GLint buffer=0,row=0,skip_rows=0,skip_pixels=0;
  query(0x88EF,&buffer);query(GL_UNPACK_ROW_LENGTH,&row);query(GL_UNPACK_SKIP_ROWS,&skip_rows);query(GL_UNPACK_SKIP_PIXELS,&skip_pixels);
  if(!buffer&&!row&&!skip_rows&&!skip_pixels){try{std::vector<unsigned char> changed;if(substitute(pixels,format,changed)){original(target,level,internal,width,height,border,format,type,changed.data());if(!reported){reported=true;diagnostic("Larpshot logo uploaded");if(preview&&!preview_marker.empty()){if(DeleteFileW(preview_marker.c_str()))diagnostic("One-time preview consumed; later launches use 1/100 chance");else diagnostic("Preview flag could not be removed");}}return;}}catch(...) {}}
 }
 original(target,level,internal,width,height,border,format,type,pixels);
}
inline Upload configure(Upload next,Query get_integer){
 std::call_once(selection,[]{preview=select_preview(installed_state(),state_path());selected=preview||roll();});
 diagnostic((std::string("Logo selection: preview=")+std::to_string(preview)+" selected="+std::to_string(selected)+" flag_attributes="+std::to_string(preview_attributes)+" flag_error="+std::to_string(preview_error)).c_str());
 if(!selected||!next)return nullptr;
 original=next;query=get_integer;
 try{wchar_t exe[32768];DWORD n=GetModuleFileNameW(nullptr,exe,32768);if(!n||n>=32768)return nullptr;std::wstring path(exe,n);auto end=path.find_last_of(L"\\/");if(end==std::wstring::npos)return nullptr;path.resize(end);path+=L"\\data\\textures\\logo.png";ready=load(path.c_str());}catch(...){ready=false;}
 diagnostic(ready?"Larpshot selected; texture replacement ready":"Larpshot selected; image loading failed");
 return ready?upload:nullptr;
}
}
