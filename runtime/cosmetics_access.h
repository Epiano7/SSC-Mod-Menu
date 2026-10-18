#pragma once
#include "cosmetics_access_core.h"
#include "cosmetic_adapter.h"
namespace ssc_cosmetic_access {
inline Ownership current;
inline uint64_t account_id=0;
inline ssc_beta::Identity steam;
inline std::string account_key;
inline uint64_t last_poll=0;
inline void refresh(const std::string& own,uint64_t now){
 if(now-last_poll<1000)return;
 last_poll=now;
 current={};account_key=own;
 if(!steam.attach()||!(account_id=steam.read())||own.empty()){account_id=0;return;}
 const auto dll=GetModuleHandleW(L"steam_api64.dll");
 auto apps=reinterpret_cast<void*(*)()>(GetProcAddress(dll,"SteamAPI_SteamApps_v008"));
 auto subscribed=reinterpret_cast<bool(*)(void*,uint32_t)>(GetProcAddress(dll,"SteamAPI_ISteamApps_BIsSubscribedApp"));
 if(!apps||!subscribed)return;
 auto api=apps();if(!api)return;
 // Explicit paid DLC catalogue; Acid Pass is a timed native-game entitlement.
 current.dlc_known=true;
 if(subscribed(api,4172540))current.dlcs.push_back(L"Starter Pack");
 if(subscribed(api,2194030))current.dlcs.push_back(L"Deluxe Edition");
 current.pass_known=ssc_names::attached&&ssc_names::native_pass_account==own&&ssc_names::native_pass_observed&&now>=ssc_names::native_pass_observed&&now-ssc_names::native_pass_observed<=60000;
 current.active_pass=current.pass_known&&ssc_names::native_pass_value;
}
inline const wchar_t* status(){switch(tier(current)){
 case Tier::gradient:return L"Active Acid Pass: solid, rainbow and gradient sharing";
 case Tier::solid:return L"DLC owned: solid sharing; gradients stay local";
 case Tier::local:return L"Local colors only: no qualifying DLC or active pass";
 default:return L"Ownership unverified: local colors only";
}}
}
