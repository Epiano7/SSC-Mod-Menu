#pragma once
#include <cmath>
#include <cstdint>
namespace ssc_vehicle {
// Input-only prototype. The native vehicle adapter must validate driver ownership
// and input context before calling this. Angles are clockwise from camera-up.
struct Input {float right=0,forward=0;};
struct Buttons {bool forward=false,reverse=false,left=false,right=false;};
struct Mapper {
 uintptr_t driver=0,vehicle=0;int sector=-1;
 void reset(){driver=vehicle=0;sector=-1;}
 Input map(Input in,float heading,float camera_heading,uintptr_t who,uintptr_t car,bool allowed){
  if(!allowed||!who||!car||!std::isfinite(heading)||!std::isfinite(camera_heading)||!std::isfinite(in.right)||!std::isfinite(in.forward)){reset();return in;}
  if(driver!=who||vehicle!=car){reset();driver=who;vehicle=car;}
  float angle=std::remainder(std::remainder(heading,360.f)-std::remainder(camera_heading,360.f),360.f);
  int nearest=(int(std::floor((angle+45.f)/90.f))+4)%4;
  // Five degrees of hysteresis beyond each nominal 45-degree boundary.
  if(sector<0||std::abs(std::remainder(angle-float(sector)*90.f,360.f))>50.f)sector=nearest;
  switch(sector){
   case 1:return{-in.forward,in.right};
   case 2:return{-in.right,-in.forward};
   case 3:return{in.forward,-in.right};
   default:return in;
  }
 }
 Buttons buttons(Buttons in,float heading,uintptr_t who,uintptr_t car){
  map({},heading,0,who,car,true);
  // Preserve simultaneous opposite presses; native braking/priority decides.
  switch(sector){
   case 1:return {in.right,in.left,in.forward,in.reverse};
   case 2:return {in.reverse,in.forward,in.right,in.left};
   case 3:return {in.left,in.right,in.reverse,in.forward};
   default:return in;
  }
 }
};
}
