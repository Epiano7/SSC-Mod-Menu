#pragma once
#include "shared_cosmetics_core.h"
namespace ssc_shared {
template<class Net,class Now,class Current>
void publish_public(Net& net,const std::string& account,const Style* appearance,Now now,Current current){
 require(account_valid(account));current();const auto start=now();
 auto status=net.request(L"/api/cosmetics/v1/status","");require(status.status==200);current();
 auto meta=parse(status.body);exact(meta,{"ok","api_version","app_id","service","lease_seconds"});
 require(meta.at("ok")==true&&meta.at("api_version")==1&&meta.at("app_id")==308600&&meta.at("service")=="shared-cosmetics"&&meta.at("lease_seconds")==120);
 Json op={{"action",appearance?"publish":"remove"},{"style",appearance?encode(*appearance):Json(nullptr)}};
 auto response=net.request(L"/api/cosmetics/v1/publish",Json{{"account_key",account},{"operation",op}}.dump());
 current();require(response.status==200&&fresh(start,now(),20000));auto result=parse(response.body);exact(result,{"ok","account_key","lease_seconds"});
 require(result.at("ok")==true&&result.at("account_key")==account&&result.at("lease_seconds")==(appearance?120:0));
}
}
