#pragma once
#include "freecam_capture_geometry.h"
#include <cstdint>
namespace ssc_freecam::capture {
// Normal 3D flight with terrain clearance. Unknown terrain permits horizontal
// travel/ascent but never lowers the camera; it is not verified floor support.
template<class Sample> void floor_flight(Camera& camera,float side,float forward,float rise,float distance,float clearance,Sample sample){
 if(!camera.seeded||!std::isfinite(distance)||distance<0||!std::isfinite(clearance)||clearance<0)return;
 int steps=std::max(1,int(std::ceil(std::min(distance,6000.f)/2.f)));
 for(int i=0;i<steps;++i){float previous=camera.eye.z;camera.move(side,forward,rise,std::min(distance,6000.f)/steps);auto surface=sample(camera.eye.x,camera.eye.y);float floor=surface&&std::isfinite(surface->z)?surface->z+clearance:previous;camera.eye.z=std::max(camera.eye.z,floor);}
}
// Reviewed terrain sampler, build25847950, native580d00..580f28.
// Terrain only: raised structures are rejected rather than treating their roofs
// as terrain or silently placing the camera underneath them.
template<class Read> std::optional<Surface> terrain(uintptr_t map,float x,float y,Read read){
 if(!map||!std::isfinite(x)||!std::isfinite(y)||x<0||y<0)return std::nullopt;
 int width=0,height=0;uint32_t count=0;uintptr_t cells=0,corners=0;
 if(!read(map+0x98,width)||!read(map+0x9c,height)||width<=0||height<=0||width>10000||height>10000||x>=width*32.f||y>=height*32.f||!read(map+0x8261c,count)||!read(map+0x825b8,cells)||!cells)return std::nullopt;
 uint32_t ix=uint32_t(x/32),iy=uint32_t(y/32),index=iy*uint32_t(width)+ix;
 if(index>=count)return std::nullopt;
 uintptr_t cell=cells+uintptr_t(index)*0xb0;uint8_t slope=0;int16_t flat=0,structure=0;
 if(!read(cell,slope)||!read(cell+6,flat)||!read(cell+0x8a,structure)||structure>flat)return std::nullopt;
 if(!slope)return Surface{float(flat),{0,0,1}};
 if(!read(map+0x826b8,corners)||!corners)return std::nullopt;
 float a=0,b=0,c=0,d=0;uintptr_t row=corners+uintptr_t(index)*20;
 if(!read(row,a)||!read(row+4,b)||!read(row+8,c)||!read(row+12,d)||!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c)||!std::isfinite(d))return std::nullopt;
 // Continuous bilinear surface, matching native corner order. The gameplay
 // helper quantizes XY to integers; continuous interpolation avoids camera steps.
 float u=(x-ix*32)/32,v=(y-iy*32)/32;
 float z=a*(1-u)*(1-v)+b*u*(1-v)+c*(1-u)*v+d*u*v;
 float dx=((b-a)*(1-v)+(d-c)*v)/32,dy=((c-a)*(1-u)+(d-b)*u)/32;
 if(!std::isfinite(z)||!std::isfinite(dx)||!std::isfinite(dy))return std::nullopt;
 return Surface{z,normalized(Vec3{-dx,-dy,1})};
}
}
