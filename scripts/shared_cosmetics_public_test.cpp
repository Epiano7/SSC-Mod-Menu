#include "../runtime/shared_cosmetics_public.h"
#include <cassert>
#include <iostream>
using namespace ssc_shared;
struct Reply {int status;std::string body;};
struct Net {
 int calls=0,fail=0;std::string wrong;bool remove=false;
 Reply request(const wchar_t* path,const std::string& body){++calls;if(fail)return {fail,"proxy error"};
  if(calls==1){assert(std::wstring(path)==L"/api/cosmetics/v1/status"&&body.empty());return {200,"{\"api_version\":1,\"app_id\":308600,\"lease_seconds\":120,\"ok\":true,\"service\":\"shared-cosmetics\"}"};}
  assert(std::wstring(path)==L"/api/cosmetics/v1/publish");auto j=parse(body);exact(j,{"account_key","operation"});assert(j.at("account_key")=="owner");
  assert(j.at("operation").at("action")== (remove?"remove":"publish"));
  return {200,Json{{"ok",true},{"account_key",wrong.empty()?"owner":wrong},{"lease_seconds",remove?0:120}}.dump()};
 }
};
int main(){Style style;Time time{1000,1000};auto clock=[&]{return time;};
 Net good;publish_public(good,"owner",&style,clock,[]{});assert(good.calls==2);
 Net remove;remove.remove=true;publish_public(remove,"owner",nullptr,clock,[]{});
 for(int failure:{403,429,500}){Net n;n.fail=failure;bool rejected=false;try{publish_public(n,"owner",&style,clock,[]{});}catch(...){rejected=true;}assert(rejected);}
 Net wrong;wrong.wrong="someone_else";bool rejected=false;try{publish_public(wrong,"owner",&style,clock,[]{});}catch(...){rejected=true;}assert(rejected);
 Net cancelled;int checks=0;rejected=false;try{publish_public(cancelled,"owner",&style,clock,[&]{if(++checks==2)throw 1;});}catch(...){rejected=true;}assert(rejected&&cancelled.calls==1);
 Net late;checks=0;rejected=false;try{publish_public(late,"owner",&style,clock,[&]{if(++checks==3)time.mono_ms+=21000;});}catch(...){rejected=true;}assert(rejected);
 std::cout<<"Public sharing: no keys/enrollment, publish/remove, outage, wrong account, stale and timeout passed\n";
}
