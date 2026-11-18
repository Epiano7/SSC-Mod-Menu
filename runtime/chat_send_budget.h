#pragma once
#include <cmath>

namespace ssc_chat_limit {
inline constexpr float stock_cost=2.f,custom_cost=3.f,threshold=7.f,lockout_seconds=8.f;
struct Meter {float points=0,cooldown=0;};
inline bool valid(const Meter& value){
 return std::isfinite(value.points)&&std::isfinite(value.cooldown)&&
        value.points>=0&&value.points<=1000000&&value.cooldown>=0&&value.cooldown<=1000000;
}
inline bool available(const Meter& value){return valid(value)&&value.cooldown<=0;}
inline Meter charge(Meter value,bool wheel){
 // The native wheel adds its own two points immediately after our callback.
 value.points+=wheel?custom_cost-stock_cost:custom_cost;
 if(!wheel&&value.points>=threshold)value.cooldown=lockout_seconds;
 return value;
}
}
