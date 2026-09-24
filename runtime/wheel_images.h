#pragma once
#include <windows.h>
#include <commdlg.h>
#include <wincodec.h>
#include <GL/gl.h>
#include <array>
#include <vector>
#include <filesystem>
#include <fstream>
namespace ssc_wheel_images {
inline constexpr int tile=64,atlas_size=2048,tag=0x700000;
inline std::array<std::vector<unsigned char>,9> images;
inline std::array<int,9> fallback{};
inline unsigned revision=1,uploaded=0;inline GLuint texture=0;inline HGLRC context=nullptr;
inline std::wstring status;
template<class T>struct Com {T* p=nullptr;~Com(){if(p)p->Release();}T** out(){return &p;}T* operator->(){return p;}};
inline std::filesystem::path path(const std::filesystem::path& dir,int slot){return dir/L"wheel-icons"/(L"slot-"+std::to_wstring(slot)+L".rgba");}
inline void load(const std::filesystem::path& dir){for(int i=0;i<9;++i){images[i].clear();auto file=path(dir,i);std::error_code ec;if(std::filesystem::file_size(file,ec)!=tile*tile*4||ec)continue;std::ifstream in(file,std::ios::binary);std::vector<unsigned char> bytes(tile*tile*4);in.read(reinterpret_cast<char*>(bytes.data()),bytes.size());if(in)images[i]=std::move(bytes);}++revision;}
inline bool import_file(const std::filesystem::path& input,const std::filesystem::path& dir,int slot){
 if(slot<0||slot>8){return false;}
 std::error_code ec;auto size=std::filesystem::file_size(input,ec);if(ec||size>16*1024*1024){status=L"Choose an image smaller than 16 MB";return false;}
 HRESULT init=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);struct Cleanup{bool on;~Cleanup(){if(on)CoUninitialize();}}cleanup{SUCCEEDED(init)};
 Com<IWICImagingFactory> factory;Com<IWICBitmapDecoder> decoder;Com<IWICBitmapFrameDecode> frame;Com<IWICBitmapScaler> scaler;Com<IWICFormatConverter> converter;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.out())))||FAILED(factory->CreateDecoderFromFilename(input.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnLoad,decoder.out()))||FAILED(decoder->GetFrame(0,frame.out())))return false;
 UINT w=0,h=0;frame->GetSize(&w,&h);if(!w||!h||w>4096||h>4096){status=L"Image dimensions must be 1-4096 pixels";return false;}
 UINT dw=w>=h?tile:std::max(1u,UINT(tile*w/h)),dh=h>=w?tile:std::max(1u,UINT(tile*h/w));
 if(FAILED(factory->CreateBitmapScaler(scaler.out()))||FAILED(scaler->Initialize(frame.p,dw,dh,WICBitmapInterpolationModeFant))||FAILED(factory->CreateFormatConverter(converter.out()))||FAILED(converter->Initialize(scaler.p,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return false;
 std::vector<unsigned char> scaled(dw*dh*4),bytes(tile*tile*4);if(FAILED(converter->CopyPixels(nullptr,dw*4,UINT(scaled.size()),scaled.data())))return false;
 for(UINT y=0;y<dh;++y)std::memcpy(bytes.data()+((y+(tile-dh)/2)*tile+(tile-dw)/2)*4,scaled.data()+y*dw*4,dw*4);
 std::filesystem::create_directories(dir/L"wheel-icons",ec);if(ec)return false;auto dest=path(dir,slot),temp=dest;temp+=L".tmp";std::ofstream out(temp,std::ios::binary);out.write(reinterpret_cast<char*>(bytes.data()),bytes.size());out.close();if(!out||!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;images[slot]=std::move(bytes);++revision;status=L"Image imported";return true;
}
inline bool choose(HWND window,const std::filesystem::path& dir,int slot){wchar_t file[32768]{};OPENFILENAMEW ofn{};ofn.lStructSize=sizeof(ofn);ofn.hwndOwner=window;ofn.lpstrFile=file;ofn.nMaxFile=32768;ofn.lpstrFilter=L"Images (PNG, JPG, BMP)\0*.png;*.jpg;*.jpeg;*.bmp\0\0";ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(!GetOpenFileNameW(&ofn))return false;try{if(import_file(file,dir,slot))return true;}catch(...){}status=L"Could not import this image";return false;}
inline bool upload(){if(!wglGetCurrentContext())return false;if(context!=wglGetCurrentContext()){texture=0;uploaded=0;context=wglGetCurrentContext();}if(texture&&uploaded==revision)return true;
 GLint bound=0,alignment=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);glGetIntegerv(GL_UNPACK_ALIGNMENT,&alignment);
 using BindBuffer=void(APIENTRY*)(GLenum,GLuint);auto bind=reinterpret_cast<BindBuffer>(wglGetProcAddress("glBindBuffer"));GLint unpack=0;if(bind){glGetIntegerv(0x88ef,&unpack);bind(0x88ec,0);}
 std::vector<unsigned char> atlas(atlas_size*atlas_size*4);for(int i=0;i<9;++i)if(images[i].size()==tile*tile*4)for(int y=0;y<tile;++y)std::memcpy(atlas.data()+(y*atlas_size+i*tile)*4,images[i].data()+y*tile*4,tile*4);
 if(!texture){glGenTextures(1,&texture);}
 glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);glPixelStorei(GL_UNPACK_SKIP_ROWS,0);glBindTexture(GL_TEXTURE_2D,texture);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,atlas_size,atlas_size,0,GL_RGBA,GL_UNSIGNED_BYTE,atlas.data());glBindTexture(GL_TEXTURE_2D,bound);glPixelStorei(GL_UNPACK_ALIGNMENT,alignment);glPopClientAttrib();if(bind)bind(0x88ec,unpack);uploaded=revision;return texture!=0;
}
using Draw=void(*)(uintptr_t,uintptr_t,float,float,float,float,int,char,int,float,uint32_t,uint32_t,uint32_t,char);
inline Draw original=nullptr;
inline void draw(uintptr_t object,uintptr_t atlas,float x,float y,float w,float h,int icon,char flip,int blend,float alpha,uint32_t r,uint32_t g,uint32_t b,char depth){
 if(icon<tag||icon>=tag+9){original(object,atlas,x,y,w,h,icon,flip,blend,alpha,r,g,b,depth);return;}
 int slot=icon-tag;std::array<unsigned char,0x38> copy{};
 if(images[slot].size()==tile*tile*4&&upload()){
  std::memcpy(copy.data(),reinterpret_cast<void*>(atlas),copy.size());auto put=[&](int at,int value){std::memcpy(copy.data()+at,&value,4);};put(0x20,int(texture));put(0x24,atlas_size);put(0x28,atlas_size);put(0x2c,32);put(0x30,32);put(0x34,tile);atlas=reinterpret_cast<uintptr_t>(copy.data());icon=slot;
 }else icon=0x200+fallback[slot];
 original(object,atlas,x,y,w,h,icon,flip,blend,alpha,r,g,b,depth);
}
}
