#pragma once
#include <algorithm>
#include <cmath>
#include <string>
namespace ssc_color {
struct HSV {
 float h=0,s=0,v=0;
 unsigned rgb() const {
  float hue=h-std::floor(h),sat=std::clamp(s,0.f,1.f),value=std::clamp(v,0.f,1.f);
  float c=value*sat,x=c*(1-std::abs(std::fmod(hue*6,2.f)-1)),m=value-c;
  float r=0,g=0,b=0;int sector=int(hue*6);
  switch(sector){case 0:r=c;g=x;break;case 1:r=x;g=c;break;case 2:g=c;b=x;break;case 3:g=x;b=c;break;case 4:r=x;b=c;break;default:r=c;b=x;}
  return (unsigned(std::lround((r+m)*255))<<16)|(unsigned(std::lround((g+m)*255))<<8)|unsigned(std::lround((b+m)*255));
 }
 void set(unsigned color){
  float r=float((color>>16)&255)/255,g=float((color>>8)&255)/255,b=float(color&255)/255;
  float hi=std::max({r,g,b}),lo=std::min({r,g,b}),d=hi-lo;v=hi;s=hi>0?d/hi:0;
  if(d>0){h=(hi==r?(g-b)/d:hi==g?(b-r)/d+2:(r-g)/d+4)/6;if(h<0)h+=1;}
 }
};
inline bool parse(const std::wstring& value,unsigned& rgb){
 const size_t first=!value.empty()&&value[0]==L'#'?1:0;if(value.size()-first!=6)return false;
 unsigned result=0;for(size_t i=first;i<value.size();++i){auto c=value[i];unsigned digit;
  if(c>=L'0'&&c<=L'9')digit=c-L'0';else if(c>=L'a'&&c<=L'f')digit=c-L'a'+10;else if(c>=L'A'&&c<=L'F')digit=c-L'A'+10;else return false;
  result=(result<<4)|digit;
 }rgb=result;return true;
}
}
