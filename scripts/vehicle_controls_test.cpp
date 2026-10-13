#include "../runtime/vehicle_controls.h"
#include <cassert>
#include <limits>
#include <iostream>
using namespace ssc_vehicle;
int main(){
 Mapper m;
 for(int q=0;q<4;++q){m.reset();auto o=m.map({0,1},float(q*90),0,1,2,true);const Input expected[]={{0,1},{-1,0},{0,-1},{1,0}};assert(o.right==expected[q].right&&o.forward==expected[q].forward);}
 for(int a=-1080;a<=1080;++a){m.reset();auto o=m.map({.25f,.75f},float(a),37,1,2,true);assert(std::abs(o.right*o.right+o.forward*o.forward-.625f)<.0001f);}
 m.reset();m.map({0,1},44,0,1,2,true);for(float a:{46.f,49.f,45.f,48.f}){auto o=m.map({0,1},a,0,1,2,true);assert(o.forward==1);}
 assert(m.map({0,1},51,0,1,2,true).right==-1);assert(m.map({0,1},41,0,1,2,true).right==-1);assert(m.map({0,1},39,0,1,2,true).forward==1);
 assert(m.map({0,1},49,0,1,3,true).right==-1); // New car discards old hysteresis.
 auto o=m.map({1,0},90,0,1,3,false);assert(o.right==1&&o.forward==0&&m.sector==-1);
 o=m.map({1,0},std::numeric_limits<float>::quiet_NaN(),0,1,3,true);assert(o.right==1&&m.sector==-1);
 m.reset();o=m.map({0,1},120,30,1,3,true);assert(o.right==-1&&o.forward==0);
 for(int q=0;q<4;++q)for(int mask=0;mask<16;++mask){m.reset();Buttons b{bool(mask&1),bool(mask&2),bool(mask&4),bool(mask&8)};auto r=m.buttons(b,float(q*90),1,2);assert(int(b.forward)+int(b.reverse)+int(b.left)+int(b.right)==int(r.forward)+int(r.reverse)+int(r.left)+int(r.right));m.reset();auto restored=m.buttons(r,float((4-q)%4*90),1,2);assert(b.forward==restored.forward&&b.reverse==restored.reverse&&b.left==restored.left&&b.right==restored.right);}
 auto huge=m.map({0,1},std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),1,3,true);assert(std::isfinite(huge.right)&&std::isfinite(huge.forward));
 std::cout<<"PASS: four directions, rotated camera, analog magnitude, boundary hysteresis, vehicle changes, disabled/invalid fallback\n";
}
