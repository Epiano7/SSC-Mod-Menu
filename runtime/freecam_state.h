#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace ssc_freecam {
struct State {
 bool active=false,ready=false;uintptr_t actor=0,world=0;uint64_t seen=0,last=0;
 float x=0,y=0,max_x=0,max_y=0,speed=120;
 void stop(){active=false;last=0;}
 void observe(uintptr_t a,uintptr_t w,float native_x,float native_y,float width,float height,uint64_t now){
  if(!a||!w||!std::isfinite(native_x)||!std::isfinite(native_y)||!std::isfinite(width)||!std::isfinite(height)||width<=0||height<=0){stop();ready=false;return;}
  if(a!=actor||w!=world||now<seen||(seen&&now-seen>500))stop();
  actor=a;world=w;seen=now;ready=true;max_x=width;max_y=height;
  if(!active){x=native_x;y=native_y;}
 }
 bool start(uint64_t now){if(!ready||now<seen||now-seen>500)return false;active=true;last=now;return true;}
 void tick(uint64_t now,float horizontal,float vertical,bool paused,float precise_dt=-1){
  if(now<seen||now-seen>500){stop();ready=false;return;}
  if(!active)return;
  if(now<last||now-last>500){stop();return;}
  float dt=precise_dt>=0&&std::isfinite(precise_dt)?std::clamp(precise_dt,0.f,.5f):float(now-last)/1000;last=now;if(paused)return;
  float length=std::sqrt(horizontal*horizontal+vertical*vertical);if(length>1){horizontal/=length;vertical/=length;}
  x=std::clamp(x+horizontal*speed*dt,0.f,max_x);y=std::clamp(y+vertical*speed*dt,0.f,max_y);
 }
};
}
