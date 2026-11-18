#pragma once
#include <algorithm>
#include <cmath>
namespace ssc_cosmetic_animation {
enum class Motion { forward, reverse, ping_pong };
struct Settings {
 Motion motion=Motion::forward;
 int speed_percent=100;
 bool breathing=false;
 int minimum_percent=55;
 int period_seconds=4;
};
inline bool valid(const Settings& s){return int(s.motion)>=0&&int(s.motion)<=2&&s.speed_percent>=25&&s.speed_percent<=200&&s.minimum_percent>=25&&s.minimum_percent<=100&&s.period_seconds>=2&&s.period_seconds<=12;}
inline float phase(double cycles,const Settings& s){
 if(!std::isfinite(cycles)||!valid(s))return 0;
 double t=cycles*double(s.speed_percent)/100.;
 if(s.motion==Motion::reverse)t=-t;
 if(s.motion==Motion::ping_pong){t=std::fmod(t,2.);if(t<0)t+=2.;t=t<=1?t:2-t;}
 else {t-=std::floor(t);}
 return float(t);
}
inline float brightness(double seconds,const Settings& s){
 if(!s.breathing||!valid(s)||!std::isfinite(seconds))return 1;
 constexpr double tau=6.2831853071795864769;
 const double wave=(1.+std::cos(std::remainder(seconds,double(s.period_seconds))*tau/s.period_seconds))*.5;
 return float(s.minimum_percent/100.+(1.-s.minimum_percent/100.)*wave);
}
}
