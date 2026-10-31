#pragma once
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <wincodec.h>
#include <objidl.h>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

namespace ssc_capture_upload {
inline std::atomic<bool> enabled{false};
inline std::atomic<int> status{0}; // 0 disconnected, 1 ready, 2 uploading, 3 queued, -1 failed
inline std::mutex credential_mutex;
inline std::string credential;
inline bool connect(std::string code){
 while(!code.empty()&&(code.back()=='\r'||code.back()=='\n'||code.back()==' '))code.pop_back();
 if(code.size()!=72||code.substr(0,8)!="ssc-cap-"||!std::all_of(code.begin()+8,code.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}))return false;
 std::lock_guard<std::mutex> lock(credential_mutex);credential=std::move(code);status=1;return true;
}
inline std::string token(){std::lock_guard<std::mutex> lock(credential_mutex);return credential;}
inline void disconnect(){enabled=false;std::lock_guard<std::mutex> lock(credential_mutex);std::fill(credential.begin(),credential.end(),'\0');credential.clear();status=0;}
inline const wchar_t* message(){switch(status.load()){case 1:return L"Upload code loaded for this session.";case 2:return L"Sending capture in background...";case 3:return L"Sent to review queue. Not added to the map pool yet.";case -1:return L"Upload failed. Local capture kept; check connection/code.";default:return L"Get an upload code from the website, then paste it here.";}}
template<class T>struct Com {T* p=nullptr;~Com(){if(p)p->Release();}T** put(){return &p;}T* operator->()const{return p;}};
struct Jpeg {std::vector<unsigned char> bytes;int width=0,height=0;};
inline Jpeg jpeg(int width,int height,const std::vector<unsigned char>& rgb,bool face){
 Jpeg out;if(width<32||height<32||rgb.size()!=size_t(width)*height*3)return out;
 double scale=std::min({1.,double(face?1024:2560)/width,double(face?1024:1441)/height});out.width=std::max(32,int(width*scale));out.height=std::max(32,int(height*scale));
 std::vector<BYTE> pixels(size_t(out.width)*out.height*3);
 // Resample on the writer thread; input is OpenGL bottom-up RGB.
 for(int y=0;y<out.height;++y)for(int x=0;x<out.width;++x){
  double sx=std::clamp((x+.5)*width/out.width-.5,0.,double(width-1));double sy=std::clamp((y+.5)*height/out.height-.5,0.,double(height-1));
  int x0=int(sx),y0=int(sy),x1=std::min(x0+1,width-1),y1=std::min(y0+1,height-1);double fx=sx-x0,fy=sy-y0;
  for(int k=0;k<3;++k){auto at=[&](int xx,int yy){return rgb[(size_t(height-1-yy)*width+xx)*3+2-k];};pixels[(size_t(y)*out.width+x)*3+k]=BYTE(std::lround((1-fy)*((1-fx)*at(x0,y0)+fx*at(x1,y0))+fy*((1-fx)*at(x0,y1)+fx*at(x1,y1))));}
 }
 Com<IWICImagingFactory> factory;Com<IStream> stream;Com<IWICBitmapEncoder> encoder;Com<IWICBitmapFrameEncode> frame;Com<IPropertyBag2> options;
 if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,reinterpret_cast<void**>(factory.put())))||FAILED(CreateStreamOnHGlobal(nullptr,TRUE,stream.put()))||FAILED(factory->CreateEncoder(GUID_ContainerFormatJpeg,nullptr,encoder.put()))||FAILED(encoder->Initialize(stream.p,WICBitmapEncoderNoCache))||FAILED(encoder->CreateNewFrame(frame.put(),options.put())))return {};
 PROPBAG2 property{};wchar_t quality[]=L"ImageQuality";property.pstrName=quality;VARIANT value{};value.vt=VT_R4;value.fltVal=.88f;
 if(!options.p||FAILED(options->Write(1,&property,&value))||FAILED(frame->Initialize(options.p))||FAILED(frame->SetSize(out.width,out.height)))return {};
 WICPixelFormatGUID format=GUID_WICPixelFormat24bppBGR;
 if(FAILED(frame->SetPixelFormat(&format))||!IsEqualGUID(format,GUID_WICPixelFormat24bppBGR)||FAILED(frame->WritePixels(out.height,out.width*3,DWORD(pixels.size()),pixels.data()))||FAILED(frame->Commit())||FAILED(encoder->Commit()))return {};
 STATSTG stat{};HGLOBAL global=nullptr;if(FAILED(stream->Stat(&stat,STATFLAG_NONAME))||stat.cbSize.QuadPart>3*1024*1024||FAILED(GetHGlobalFromStream(stream.p,&global)))return {};
 auto data=static_cast<const BYTE*>(GlobalLock(global));if(!data)return {};out.bytes.assign(data,data+size_t(stat.cbSize.QuadPart));GlobalUnlock(global);return out;
}
inline std::string base64(const std::vector<unsigned char>& data){DWORD size=0;if(!CryptBinaryToStringA(data.data(),DWORD(data.size()),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&size))return {};std::string out(size,'\0');if(!CryptBinaryToStringA(data.data(),DWORD(data.size()),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,out.data(),&size))return {};out.resize(size);while(!out.empty()&&out.back()=='\0')out.pop_back();return out;}
struct Internet {HINTERNET p=nullptr;~Internet(){if(p)WinHttpCloseHandle(p);}};
inline bool send(const std::string& body,const std::string& code){
 if(!enabled.load()||body.empty()||body.size()>12*1024*1024||code.empty())return false;
 status=2;
 Internet session{WinHttpOpen(L"SSC-Capture/1",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,nullptr,nullptr,0)};
 if(!session.p){status=-1;return false;}WinHttpSetTimeouts(session.p,10000,10000,90000,90000);
 Internet connection{WinHttpConnect(session.p,L"ssg.epiano7.dev",INTERNET_DEFAULT_HTTPS_PORT,0)};
 Internet request{connection.p?WinHttpOpenRequest(connection.p,L"POST",L"/api/capture-submit",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE):nullptr};
 if(!request.p){status=-1;return false;}
 DWORD disabled=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;WinHttpSetOption(request.p,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof disabled);DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;if(!WinHttpSetOption(request.p,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof redirects)){status=-1;return false;}
 std::wstring headers=L"Content-Type: application/json\r\nOrigin: https://ssg.epiano7.dev\r\nAuthorization: Bearer "+std::wstring(code.begin(),code.end())+L"\r\n";
 bool ok=WinHttpSendRequest(request.p,headers.c_str(),DWORD(-1),const_cast<char*>(body.data()),DWORD(body.size()),DWORD(body.size()),0)&&WinHttpReceiveResponse(request.p,nullptr);
 DWORD result=0,size=sizeof result;ok=ok&&WinHttpQueryHeaders(request.p,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&result,&size,WINHTTP_NO_HEADER_INDEX)&&result==201;
 status=ok?3:-1;return ok;
}
}
