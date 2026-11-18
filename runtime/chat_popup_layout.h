#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
namespace ssc_commands {
struct ChatAnchor {float x=0,y=0,width=0,height=0;bool valid=false;};
struct PopupLayout {float x=0,y=0,width=0,height=0;size_t first=0,count=0;};
// The game draws chat in a logical canvas, with 0/1/2 = start/centre/end anchors.
inline ChatAnchor chat_anchor(int x,int y,int w,int h,int ax,int ay,int lw,int lh,int sw,int sh){
 if(lw<320||lh<200||lw>32768||lh>32768||sw<160||sh<120||sw>32768||sh>32768||ax<0||ax>2||ay<0||ay>2||w<=0||w>lw||h<=0||h>lh)return {};
 const double px=double(x)+(ax==1?lw/2:ax==2?lw:0),py=double(y)+(ay==1?lh/2:ay==2?lh:0);
 if(px<0||py<0||px+w>lw+1||py+h>lh+1)return {};
 return {float(px*sw/lw),float(py*sh/lh),float(double(w)*sw/lw),float(double(h)*sh/lh),true};
}
inline PopupLayout popup_layout(const ChatAnchor& anchor,int sw,int sh,float content_width,size_t total,size_t selected){
 if(!anchor.valid||!total||!std::isfinite(content_width)||sw<160||sh<120)return {};
 constexpr float margin=8,gap=4,row=26,padding=4;
 float room=anchor.y-gap-margin;
 size_t count=std::min({total,size_t(8),size_t(std::max(0.f,std::floor((room-padding*2)/row)))});
 if(!count)return {};
 selected=std::min(selected,total-1);size_t first=selected>=count?selected-count+1:0;
 float width=std::clamp(content_width+20.f,60.f,std::min(700.f,float(sw)-margin*2));
 float height=float(count)*row+padding*2;
 return {std::clamp(anchor.x+8.f,margin,float(sw)-margin-width),anchor.y-gap-height,width,height,first,count};
}
}
