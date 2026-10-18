#pragma once
#include "shared_cosmetics_core.h"
#include <string>
#include <vector>
namespace ssc_cosmetic_access {
enum class Tier {checking,local,solid,gradient};
struct Ownership {
 bool dlc_known=false,pass_known=false,active_pass=false;
 std::vector<std::wstring> dlcs;
};
inline Tier tier(const Ownership& v){
 if(!v.dlc_known||!v.pass_known)return Tier::checking;
 if(v.active_pass)return Tier::gradient;
 return v.dlcs.empty()?Tier::local:Tier::solid;
}
inline bool publication(const Ownership& v,const ssc_shared::Style& local,unsigned solid,ssc_shared::Style& output){
 const auto access=tier(v);if(access==Tier::checking||access==Tier::local)return false;
 output=local;
 if(access==Tier::solid){output.mode=ssc_shared::Mode::solid;output.count=1;output.colors={solid,0,0};}
 return true;
}
inline std::vector<std::wstring> notice(const Ownership& v,bool owns_everything){
 std::vector<std::wstring> lines={L"Hi! A quick update for the Cosmetics module.",L"Sharing colors with other mod users now depends on your DLCs and active Acid Pass."};
 if(tier(v)==Tier::checking){lines.push_back(L"Checking your purchases and pass status...");lines.push_back(L"Your local colors still work. Sharing will start once your access is confirmed.");return lines;}
 auto owned=v.dlcs;if(v.active_pass)owned.push_back(L"Acid Pass (active)");
 if(owned.empty())lines.push_back(L"You have no qualifying DLCs or active passes.");
 else if(owned.size()<=2){std::wstring s=L"You have: "+owned[0];if(owned.size()==2)s+=L" and "+owned[1];lines.push_back(s+L".");}
 else {lines.push_back(L"You have:");for(const auto& name:owned)lines.push_back(L"- "+name);}
 lines.push_back(L"Your benefits:");
 lines.push_back(L"- Custom solid colors and gradients visible to you.");
 switch(tier(v)){
 case Tier::gradient:lines.push_back(L"- Share solid colors, rainbow names, and custom gradients with other mod users.");break;
 case Tier::solid:lines.push_back(L"- Share your selected solid color with other mod users.");lines.push_back(L"- Gradients and rainbow names remain visible only to you.");break;
 default:lines.push_back(L"- Your custom colors are visible only to you.");break;
 }
 if(!owns_everything){lines.push_back(L"Any non-pass DLC unlocks solid-color sharing.");lines.push_back(L"An active Acid Pass unlocks gradient and rainbow sharing.");lines.push_back(L"After a purchase, restart the game to receive your mod menu benefits.");}
 return lines;
}
}
