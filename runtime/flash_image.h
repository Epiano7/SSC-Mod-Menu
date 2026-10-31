#pragma once
#include <wincodec.h>
#include <commdlg.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <array>
#include <algorithm>
namespace ssc_flash_image {
inline std::atomic<int> mode{0}; // 0 black, 1 opaque image
inline std::mutex mutex;
inline std::vector<unsigned char> pixels;
inline unsigned revision=0;
inline std::wstring status;
inline constexpr unsigned edge=512,bytes=edge*edge*4;
inline bool opaque(const std::vector<unsigned char>& data){if(data.empty()||data.size()%4)return false;for(size_t i=3;i<data.size();i+=4)if(data[i]!=255)return false;return true;}
inline bool ready(){std::lock_guard<std::mutex> guard(mutex);return pixels.size()==bytes;}
template<class T>struct Com{T* p=nullptr;~Com(){if(p)p->Release();}T** out(){return &p;}T* operator->(){return p;}};
inline bool import_file(const std::filesystem::path& file,const std::filesystem::path& dir){
 std::error_code ec;auto size=std::filesystem::file_size(file,ec);if(ec||size>16*1024*1024){status=L"Choose an image smaller than 16 MB";return false;}
 HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);struct Cleanup{bool on;~Cleanup(){if(on)CoUninitialize();}}cleanup{SUCCEEDED(init)};
 Com<IWICImagingFactory> factory;Com<IWICBitmapDecoder> decoder;Com<IWICBitmapFrameDecode> frame;Com<IWICFormatConverter> original,scaled;Com<IWICBitmapScaler> scaler;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.out())))||FAILED(factory->CreateDecoderFromFilename(file.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,decoder.out())))return false;
 UINT count=0,w=0,h=0;if(FAILED(decoder->GetFrameCount(&count))||count!=1||FAILED(decoder->GetFrame(0,frame.out()))||FAILED(frame->GetSize(&w,&h))||!w||!h||w>4096||h>4096){status=L"Use a still image up to 4096 x 4096";return false;}
 if(FAILED(factory->CreateFormatConverter(original.out()))||FAILED(original->Initialize(frame.p,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
 // Check every source pixel before scaling; a tiny transparent hole must not
 // disappear during resampling and bypass the importer.
 std::vector<unsigned char> source(size_t(w)*h*4);if(FAILED(original->CopyPixels(nullptr,w*4,UINT(source.size()),source.data())))return false;
 if(!opaque(source)){status=L"Transparent images are not allowed";return false;}
 if(FAILED(factory->CreateBitmapScaler(scaler.out()))||FAILED(scaler->Initialize(original.p,edge,edge,WICBitmapInterpolationModeFant))||FAILED(factory->CreateFormatConverter(scaled.out()))||FAILED(scaled->Initialize(scaler.p,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
 std::vector<unsigned char> out(bytes);if(FAILED(scaled->CopyPixels(nullptr,edge*4,bytes,out.data()))||!opaque(out))return false;
 std::filesystem::create_directories(dir,ec);if(ec)return false;auto dest=dir/L"flash-image.rgba",temp=dir/L"flash-image.rgba.tmp";
 std::ofstream f(temp,std::ios::binary);f.write(reinterpret_cast<const char*>(out.data()),out.size());f.close();if(!f||!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;
 {std::lock_guard<std::mutex> guard(mutex);pixels=std::move(out);++revision;}mode=1;status=L"Image imported";return true;
}
inline void load(const std::filesystem::path& dir){
 std::vector<unsigned char> data;std::error_code ec;auto file=dir/L"flash-image.rgba";
 if(std::filesystem::file_size(file,ec)==bytes&&!ec){data.resize(bytes);std::ifstream in(file,std::ios::binary);in.read(reinterpret_cast<char*>(data.data()),bytes);if(!in||!opaque(data))data.clear();}
 std::lock_guard<std::mutex> guard(mutex);pixels=std::move(data);++revision;
}
inline bool choose(HWND window,const std::filesystem::path& dir){
 wchar_t file[32768]{};OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=window;ofn.lpstrFile=file;ofn.nMaxFile=32768;ofn.lpstrFilter=L"Images (PNG, JPG, BMP)\0*.png;*.jpg;*.jpeg;*.bmp\0\0";ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
 if(!GetOpenFileNameW(&ofn)){return false;}status.clear();try{if(import_file(file,dir))return true;}catch(...){}if(status.empty())status=L"Could not import image";return false;
}
// The reserved texture binding lasts only for the affected native program.
// Restore the prior binding before any program switch, including the mod UI.
struct GLState{HGLRC context=nullptr;GLuint texture=0;unsigned uploaded=0;GLint previous=0;bool bound=false;};
inline thread_local GLState gl;
using Active=void(APIENTRY*)(GLenum);
inline void restore(){if(!gl.bound)return;auto active=reinterpret_cast<Active>(wglGetProcAddress("glActiveTexture"));if(active&&gl.context==wglGetCurrentContext()){GLint old=0;glGetIntegerv(0x84e0,&old);active(0x84c0+15);glBindTexture(GL_TEXTURE_2D,gl.previous);active(old);}gl.bound=false;}
inline bool bind(){
 if(!wglGetCurrentContext()){return false;}auto active=reinterpret_cast<Active>(wglGetProcAddress("glActiveTexture"));if(!active)return false;
 if(gl.context!=wglGetCurrentContext()){gl=GLState{};gl.context=wglGetCurrentContext();}
 GLint units=0,old=0;glGetIntegerv(0x8872,&units);if(units<16)return false;
 std::lock_guard<std::mutex> guard(mutex);if(pixels.size()!=bytes)return false;
 glGetIntegerv(0x84e0,&old);active(0x84c0+15);if(!gl.bound){glGetIntegerv(GL_TEXTURE_BINDING_2D,&gl.previous);gl.bound=true;}
 if(!gl.texture){glGenTextures(1,&gl.texture);}glBindTexture(GL_TEXTURE_2D,gl.texture);
 if(gl.uploaded!=revision){
  using Buffer=void(APIENTRY*)(GLenum,GLuint);auto buffer=reinterpret_cast<Buffer>(wglGetProcAddress("glBindBuffer"));GLint unpack=0;if(buffer){glGetIntegerv(0x88ef,&unpack);buffer(0x88ec,0);}
  glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,0x812f);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,0x812f);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,edge,edge,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());glPopClientAttrib();if(buffer)buffer(0x88ec,unpack);gl.uploaded=revision;
 }
 active(old);return true;
}
}
