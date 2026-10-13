#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace ssc_freecam {
struct Vec3 {float x=0,y=0,z=0;Vec3 operator+(Vec3 b)const{return{x+b.x,y+b.y,z+b.z};}Vec3 operator-(Vec3 b)const{return{x-b.x,y-b.y,z-b.z};}Vec3 operator*(float s)const{return{x*s,y*s,z*s};}};
inline float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec3 normalized(Vec3 v){float n=std::sqrt(dot(v,v));return n>0?v*(1/n):Vec3{};}
inline Vec3 rotated(Vec3 v,Vec3 axis,float a){float c=std::cos(a),s=std::sin(a);return v*c+cross(axis,v)*s+axis*(dot(axis,v)*(1-c));}
struct Camera {
 bool seeded=false;float zoom=1,screen_x=1,screen_y=1;Vec3 eye,right{1,0,0},up{0,1,0},back{0,0,1};
 void reset(){seeded=false;zoom=1;}
 bool seed(const float* m){
  for(int i=0;i<16;++i)if(!std::isfinite(m[i]))return false;
  Vec3 r{m[0],m[4],m[8]},u{m[1],m[5],m[9]},b{m[2],m[6],m[10]};
  if(std::abs(dot(r,r)-1)>.01f||std::abs(dot(u,u)-1)>.01f||std::abs(dot(b,b)-1)>.01f||std::abs(dot(r,u))>.01f||std::abs(dot(r,b))>.01f||std::abs(dot(u,b))>.01f)return false;
  eye=r*(-m[12])+u*(-m[13])+b*(-m[14]);
  // Keep the screen's horizontal axis parallel to the ground. Native roll,
  // or successive local-axis yaw/pitch rotations, must not bank the camera.
  Vec3 level=cross(Vec3{0,0,1},b);
  if(dot(level,level)<.000001f)level={r.x,r.y,0};
  level=normalized(level);if(dot(level,r)<0)level=level*(-1);
  right=level;back=b;up=normalized(cross(back,right));seeded=true;return true;
 }
 void projection(float x,float y){screen_x=x<0?-1.f:1.f;screen_y=y<0?-1.f:1.f;}
 void look(float dx,float dy){
  if(!seeded||!std::isfinite(dx)||!std::isfinite(dy))return;
  // World-vertical yaw prevents accumulated roll. Reverse yaw when upside
  // down so horizontal mouse movement stays screen-relative. Pitch remains
  // unrestricted; at a pole, yaw changes the heading for the next pitch.
  float yaw=std::remainder(-dx*.0025f*screen_x,6.28318530718f);
  float pitch=std::remainder(-dy*.0025f*screen_y,6.28318530718f);
  Vec3 vertical{0,0,up.z<0?-1.f:1.f};
  right=rotated(right,vertical,yaw);back=rotated(back,vertical,yaw);
  back=normalized(rotated(back,right,pitch));right=normalized(right);
  up=normalized(cross(back,right));right=normalized(cross(up,back));
 }
 void move(float side,float forward,float rise,float distance){
  if(!seeded)return;
  Vec3 direction=right*(side*screen_x)-back*forward+Vec3{0,0,rise};float n=std::sqrt(dot(direction,direction));if(n>1)direction=direction*(1/n);eye=eye+direction*distance;bound();
 }
 void bound(){eye.x=std::clamp(eye.x,-100000.f,100000.f);eye.y=std::clamp(eye.y,-100000.f,100000.f);eye.z=std::clamp(eye.z,-100000.f,100000.f);}
 void wheel(float steps,bool fine,bool fly){
  if(!std::isfinite(steps))return;
  steps=std::clamp(steps,-100.f,100.f)*(fine?.1f:1.f);
  if(fly){if(seeded){eye=eye-back*(steps*std::max(20.f,std::abs(eye.z)*.15f));bound();}}
  else zoom=std::clamp(zoom*std::exp(-steps*.18f),.025f,32.f);
 }
 std::array<float,16> matrix()const{return{right.x,up.x,back.x,0,right.y,up.y,back.y,0,right.z,up.z,back.z,0,-dot(right,eye),-dot(up,eye),-dot(back,eye),1};}
 // Conservative XY bounds of the complete view volume, including elevated
 // geometry. No guessed ground/roof plane: that would drop tall objects.
 bool visible_bounds(const float* p,float width,float height,float* bounds)const{
  if(!seeded||width<=0||height<=0)return false;
  for(int i=0;i<16;++i)if(!std::isfinite(p[i]))return false;
  if(p[11]!=-1||p[15]!=0||p[0]==0||p[5]==0||p[10]==-1||p[10]==1)return false;
  float near_plane=p[14]/(p[10]-1),far_plane=p[14]/(p[10]+1);
  if(!(near_plane>0&&far_plane>near_plane))return false;
  float min_x=INFINITY,min_y=INFINITY,max_x=-INFINITY,max_y=-INFINITY;
  for(float depth:{near_plane,far_plane})for(float x:{-1.f,1.f})for(float y:{-1.f,1.f}){
   Vec3 v=eye+right*((x+p[8])*depth/p[0])+up*((y+p[9])*depth/p[5])-back*depth;
   if(!std::isfinite(v.x)||!std::isfinite(v.y))return false;
   min_x=std::min(min_x,v.x);max_x=std::max(max_x,v.x);min_y=std::min(min_y,v.y);max_y=std::max(max_y,v.y);
  }
  // Include the native tile/object margin, even for views outside the map.
  min_x=std::clamp(min_x-256,-256.f,width+256);max_x=std::clamp(max_x+256,min_x,width+256);
  min_y=std::clamp(min_y-256,-256.f,height+256);max_y=std::clamp(max_y+256,min_y,height+256);
  bounds[0]=min_x;bounds[1]=min_y;bounds[2]=std::max(1.f,max_x-min_x);bounds[3]=std::max(1.f,max_y-min_y);return true;
 }
};
}
