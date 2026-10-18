#include "../runtime/cosmetics_access_core.h"
#include <cassert>
#include <iostream>
int main(){
 using namespace ssc_cosmetic_access;
 Ownership o;ssc_shared::Style chosen,out;chosen.mode=ssc_shared::Mode::gradient;chosen.count=2;chosen.colors={0x112233,0x445566,0};
 assert(tier(o)==Tier::checking&&!publication(o,chosen,0xabcdef,out));
 o.dlc_known=o.pass_known=true;assert(tier(o)==Tier::local&&!publication(o,chosen,0xabcdef,out));
 o.dlcs={L"Starter Pack"};assert(tier(o)==Tier::solid&&publication(o,chosen,0xabcdef,out));assert(out.mode==ssc_shared::Mode::solid&&out.colors[0]==0xabcdef&&chosen.mode==ssc_shared::Mode::gradient);
 o.active_pass=true;assert(tier(o)==Tier::gradient&&publication(o,chosen,0xabcdef,out));assert(out.mode==ssc_shared::Mode::gradient&&out.count==2);
 o.dlcs.clear();assert(tier(o)==Tier::gradient);o.active_pass=false;assert(tier(o)==Tier::local);
 auto no=notice(o,false);bool restart=false;for(auto& l:no)if(l.find(L"restart")!=std::wstring::npos)restart=true;assert(restart);
 o.dlcs={L"Starter Pack",L"Deluxe Edition"};o.active_pass=true;auto all=notice(o,true);int listed=0;for(auto& l:all){if(l==L"- Starter Pack"||l==L"- Deluxe Edition"||l==L"- Acid Pass (active)")++listed;assert(l.find(L"restart")==std::wstring::npos);}assert(listed==3);
 o.pass_known=false;assert(tier(o)==Tier::checking&&!publication(o,chosen,0xabcdef,out));
 std::cout<<"PASS: ownership tiers, expiry, local gradient preservation, solid fallback, notice lists and conditional purchase text\n";
}
