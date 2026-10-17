#pragma once
#include "freecam_camera.h"
#include <optional>

namespace ssc_freecam::capture {
inline bool finite(Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
// Values are native world units until a reviewed game calibration supplies meters.
struct GroundPolicy {
 float eye_height=0,maximum_step=0,maximum_drop=0,minimum_normal_z=0;
 bool valid()const{return std::isfinite(eye_height)&&eye_height>0&&std::isfinite(maximum_step)&&maximum_step>=0&&std::isfinite(maximum_drop)&&maximum_drop>=0&&std::isfinite(minimum_normal_z)&&minimum_normal_z>0&&minimum_normal_z<=1;}
};
struct Surface {float z=0;Vec3 normal{0,0,1};};
struct GroundQuery {float x=0,y=0,minimum_z=0,maximum_z=0;};
// The native adapter must select a real supporting surface in this interval,
// including roofs/bridges, rather than returning a guessed flat map plane.
template<class Sample> bool ground_move(Camera& camera,float side,float forward,float distance,const GroundPolicy& policy,Sample sample){
 if(!camera.seeded||!finite(camera.eye)||!finite(camera.right)||!finite(camera.back)||!policy.valid()||!std::isfinite(side)||!std::isfinite(forward)||!std::isfinite(distance)||distance<0)return false;
 Vec3 right=normalized(Vec3{camera.right.x,camera.right.y,0});
 if(dot(right,right)<.99f)return false;
 // Derive horizontal heading from right even when looking straight up/down.
 Vec3 heading=cross(Vec3{0,0,1},right);
 Vec3 direction=right*(std::clamp(side,-1.f,1.f)*camera.screen_x)+heading*std::clamp(forward,-1.f,1.f);
 float length=std::sqrt(dot(direction,direction));if(length>1)direction=direction*(1/length);
 Vec3 next=camera.eye+direction*distance;float foot=camera.eye.z-policy.eye_height;
 GroundQuery query{next.x,next.y,foot-policy.maximum_drop,foot+policy.maximum_step};
 if(!finite(next)||!std::isfinite(query.minimum_z)||!std::isfinite(query.maximum_z))return false;
 std::optional<Surface> surface=sample(query);
 if(!surface||!std::isfinite(surface->z)||!finite(surface->normal)||surface->z<query.minimum_z||surface->z>query.maximum_z)return false;
 float n=dot(surface->normal,surface->normal);
 if(n<.99f||n>1.01f||surface->normal.z<policy.minimum_normal_z)return false;
 next.z=surface->z+policy.eye_height;if(!finite(next))return false;
 camera.eye=next;return true;
}
// Explicit face bases avoid relying on differing cubemap library conventions.
struct Face {const char* name;Vec3 right,up,back;};
inline constexpr std::array<Face,6> cube_faces{{
 {"positive_x",{0,-1,0},{0,0,1},{-1,0,0}},
 {"negative_x",{0,1,0},{0,0,1},{1,0,0}},
 {"positive_y",{1,0,0},{0,0,1},{0,-1,0}},
 {"negative_y",{-1,0,0},{0,0,1},{0,1,0}},
 {"positive_z",{1,0,0},{0,-1,0},{0,0,-1}},
 {"negative_z",{1,0,0},{0,1,0},{0,0,1}}
}};
inline std::optional<Camera> face_camera(Vec3 origin,unsigned face){
 if(!finite(origin)||face>=cube_faces.size())return std::nullopt;
 auto f=cube_faces[face];Camera result;result.seeded=true;result.eye=origin;result.right=f.right;result.up=f.up;result.back=f.back;return result;
}
inline std::optional<std::array<float,16>> perspective(float vertical_degrees,float aspect,float near_plane,float far_plane){
 if(!std::isfinite(vertical_degrees)||vertical_degrees<=0||vertical_degrees>=179||!std::isfinite(aspect)||aspect<=0||!std::isfinite(near_plane)||!std::isfinite(far_plane)||near_plane<=0||far_plane<=near_plane)return std::nullopt;
 float f=1/std::tan(vertical_degrees*3.14159265358979323846f/360);
 std::array<float,16> result{f/aspect,0,0,0,0,f,0,0,0,0,-(far_plane+near_plane)/(far_plane-near_plane),-1,0,0,-2*far_plane*near_plane/(far_plane-near_plane),0};
 for(float v:result)if(!std::isfinite(v))return std::nullopt;
 return result;
}
}
