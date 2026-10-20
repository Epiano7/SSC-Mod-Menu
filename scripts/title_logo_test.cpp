#define WIN32_LEAN_AND_MEAN
#include "../runtime/title_logo.h"
#include <cassert>
#include <iostream>
int main(int argc,char** argv){
 {
  HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED);assert(SUCCEEDED(hr));
  ssc_title::Com<IWICImagingFactory> factory;
  assert(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,reinterpret_cast<void**>(factory.out()))));
  ssc_title::reference={8,4,std::vector<unsigned char>(8*4*4,255)};
  ssc_title::replacement={4,2,std::vector<unsigned char>(4*2*4)};
  for(size_t i=0;i<ssc_title::replacement.rgba.size();i+=4){auto* p=ssc_title::replacement.rgba.data()+i;p[0]=20;p[1]=100;p[2]=230;p[3]=255;}
  assert(ssc_title::align_artwork(factory.p));
  for(size_t i=0;i<ssc_title::replacement.rgba.size();i+=4){const auto* p=ssc_title::replacement.rgba.data()+i;assert(p[0]==20&&p[1]==100&&p[2]==230&&p[3]==255);}
  factory.p->Release();factory.p=nullptr;CoUninitialize();
  std::cout<<"PASS: scaling preserves RGBA channel order\n";
 }
 assert(argc==2);unsigned wins=0;
 for(uint32_t i=0;i<10000;++i)wins+=ssc_title::lottery(i)==1;
 assert(wins==100);assert(ssc_title::lottery(4294967199u)==0);
 for(uint32_t i=4294967200u;i!=0;++i)assert(ssc_title::lottery(i)==-1);
 std::string input=argv[1];std::wstring path(input.begin(),input.end());
 assert(ssc_title::load(path.c_str()));ssc_title::ready=true;
 assert(ssc_title::reference.w==2048&&ssc_title::reference.h==1024);
 auto before=ssc_title::solid_bounds(ssc_title::reference),after=ssc_title::solid_bounds(ssc_title::replacement);
 assert(after.left+2>=before.left&&after.top+2>=before.top&&after.right<=before.right+2&&after.bottom<=before.bottom+2);
 assert(std::abs(int(after.left+after.right)-int(before.left+before.right))<=3);
 assert(std::abs(int(after.top+after.bottom)-int(before.top+before.bottom))<=3);
 std::cout<<"PASS: replacement artwork fits original visible bounds and center\n";
 assert(ssc_title::reference.rgba!=ssc_title::replacement.rgba);
 auto native=ssc_title::reference.rgba;std::vector<unsigned char> result;
 assert(ssc_title::substitute(native.data(),GL_RGBA,result));
 assert(result==ssc_title::replacement.rgba);
 for(size_t i=0;i<native.size();i+=4)std::swap(native[i],native[i+2]);
 assert(ssc_title::substitute(native.data(),0x80E1,result));
 for(size_t i=0;i<result.size();i+=4)std::swap(result[i],result[i+2]);
 assert(result==ssc_title::replacement.rgba);
 native=ssc_title::reference.rgba;const auto row=ssc_title::reference.w*4;
 for(UINT y=0;y<ssc_title::reference.h/2;++y)for(UINT x=0;x<row;++x)std::swap(native[y*row+x],native[(ssc_title::reference.h-1-y)*row+x]);
 assert(ssc_title::substitute(native.data(),GL_RGBA,result));
 for(UINT y=0;y<ssc_title::reference.h/2;++y)for(UINT x=0;x<row;++x)std::swap(result[y*row+x],result[(ssc_title::reference.h-1-y)*row+x]);
 assert(result==ssc_title::replacement.rgba);
 native=ssc_title::reference.rgba;native[3]^=1;assert(!ssc_title::substitute(native.data(),GL_RGBA,result));assert(!ssc_title::substitute(nullptr,GL_RGBA,result));assert(!ssc_title::substitute(native.data(),GL_RGB,result));
 HWND window=CreateWindowExW(0,L"STATIC",L"Logo test",WS_POPUP,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);assert(window);
 HDC dc=GetDC(window);PIXELFORMATDESCRIPTOR pfd{};pfd.nSize=sizeof pfd;pfd.nVersion=1;pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL;pfd.iPixelType=PFD_TYPE_RGBA;pfd.cColorBits=32;
 int pixel=ChoosePixelFormat(dc,&pfd);assert(pixel&&SetPixelFormat(dc,pixel,&pfd));HGLRC context=wglCreateContext(dc);assert(context&&wglMakeCurrent(dc,context));
 auto test_state=std::filesystem::temp_directory_path()/(L"ssc-title-test-"+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directories(test_state);SetEnvironmentVariableW(L"SSCMODS_STATE_DIR",test_state.c_str());ssc_title::preview_marker=test_state/L"larpshot-once.flag";std::ofstream(ssc_title::preview_marker)<<"once";ssc_title::preview=true;ssc_title::reported=false;
 auto installed=test_state/L"installed";std::filesystem::create_directory(installed);
 assert(ssc_title::select_preview(installed,test_state));assert(ssc_title::preview_marker==test_state/L"larpshot-once.flag");
 std::ofstream(installed/L"larpshot-once.flag")<<"once";
 assert(ssc_title::select_preview(installed,test_state));assert(ssc_title::preview_marker==installed/L"larpshot-once.flag");
 std::filesystem::remove(test_state/L"larpshot-once.flag");
 std::cout<<"PASS: installed preview marker takes precedence; AppData fallback supported\n";
 GLuint texture=0;glGenTextures(1,&texture);glBindTexture(GL_TEXTURE_2D,texture);ssc_title::original=glTexImage2D;ssc_title::query=glGetIntegerv;
 ssc_title::upload(GL_TEXTURE_2D,0,GL_RGBA,2048,1024,0,GL_RGBA,GL_UNSIGNED_BYTE,ssc_title::reference.rgba.data());
 assert(!std::filesystem::exists(ssc_title::preview_marker));assert(ssc_title::reported);
 result.resize(native.size());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,result.data());
 assert(result==ssc_title::replacement.rgba);
 ssc_title::upload(GL_TEXTURE_2D,0,GL_RGBA,2048,1024,0,GL_RGBA,GL_UNSIGNED_BYTE,native.data());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,result.data());assert(result==native);
 ssc_title::ready=false;ssc_title::upload(GL_TEXTURE_2D,0,GL_RGBA,2048,1024,0,GL_RGBA,GL_UNSIGNED_BYTE,ssc_title::reference.rgba.data());glGetTexImage(GL_TEXTURE_2D,0,GL_RGBA,GL_UNSIGNED_BYTE,result.data());assert(result==ssc_title::reference.rgba);
 glDeleteTextures(1,&texture);wglMakeCurrent(nullptr,nullptr);wglDeleteContext(context);ReleaseDC(window,dc);DestroyWindow(window);
 std::cout<<"PASS: exact lottery, PNG decode, RGBA/BGRA/flipped matching, unrelated-image rejection, OpenGL replacement and original fallback\n";
}
