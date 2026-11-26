#pragma once
#include "achievement_core.h"
#include "beta_auth.h"

namespace ssc_achievements::community {
enum class Action {none,load,enable,pause,start,check,publish,withdraw};

struct Request {
    Action action=Action::none;
    std::string token;
    Json body;
    uint64_t revision=0;
};
struct Reply {
    Action action=Action::none;
    unsigned status=0;
    Json body;
    std::string token,state,code;
    uint64_t revision=0;
    bool persisted=true,enabled=false,paused=false;
};

inline bool hex(const std::string& value,size_t size,const char* alphabet="0123456789abcdef"){
    return value.size()==size&&value.find_first_not_of(alphabet)==std::string::npos;
}
inline Json payload(const Ledger& ledger){
    return {{"schema",1},{"catalog",catalog_wire_version},{"achievements",ledger.json().at("achievements")}};
}
inline std::filesystem::path token_path(const std::filesystem::path& root,uint64_t account){
    return root/(L"achievement-sync-"+std::to_wstring(account)+L".bin");
}
inline Reply perform(const std::filesystem::path& root,const Request& request,uint64_t account){
    Reply reply;reply.action=request.action;reply.revision=request.revision;
    if(!account||root.empty())return reply;
    ssc_beta::Storage store{token_path(root,account),L"SSC achievement sync"};
    const auto pref_path=root/(L"achievement-sharing-"+std::to_wstring(account)+L".bin");
    ssc_beta::Storage preference{pref_path,L"SSC achievement sharing preference"};
    if(request.action==Action::load){
        auto saved=store.load();
        if(saved.account==account&&hex(saved.token,64))reply.token=std::move(saved.token);
        const auto setting=preference.load();
        std::error_code error;
        const bool has_preference=std::filesystem::exists(pref_path,error);
        reply.enabled=setting.account==account?setting.token=="on":!has_preference&&!error;
        return reply;
    }
    if(request.action==Action::enable||request.action==Action::pause){
        reply.enabled=request.action==Action::enable;
        reply.persisted=preference.save({account,reply.enabled?"on":"off"});
        return reply;
    }
    try{
        if((request.action==Action::check||request.action==Action::withdraw)&&!hex(request.token,64))return reply;
        if(request.action==Action::withdraw){
            reply.paused=preference.save({account,"off"});
            if(!reply.paused)return reply;
        }
        ssc_beta::Http http;
        const auto id=std::to_string(account);
        const wchar_t* path=nullptr;
        std::string body;
        switch(request.action){
         case Action::start:path=L"/v1/achievements/link/start";body=Json{{"account",id}}.dump();break;
         case Action::check:path=L"/v1/achievements/link/status";break;
         case Action::publish:path=L"/v1/achievements/client-sync-v3";body=Json{{"account",id},{"progress",request.body}}.dump();break;
         case Action::withdraw:path=L"/v1/achievements/sync/withdraw-v3";body="{}";break;
         default:return reply;
        }
        const auto response=http.request(path,body,request.action==Action::publish?"":request.token);
        reply.status=response.status;
        if(reply.status==200){
            reply.body=Json::parse(response.body);
            if(request.action==Action::start){
                reply.token=reply.body.at("token").get<std::string>();
                reply.state=reply.body.at("state").get<std::string>();
                reply.code=reply.body.at("code").get<std::string>();
                if(!hex(reply.token,64)||!hex(reply.state,32)||!hex(reply.code,8,"0123456789ABCDEF")){
                    reply.status=0;reply.token.clear();reply.state.clear();reply.code.clear();
                }
            }else if(request.action==Action::check){
                if(reply.body.at("account")!=id){reply.status=401;return reply;}
                reply.token=request.token;
                reply.persisted=store.save({account,request.token});
            }else if(request.action==Action::withdraw){
                store.remove();
            }
        }else if(reply.status==401&&(request.action==Action::check||request.action==Action::withdraw)){
            store.remove();
        }
    }catch(...){reply.status=0;}
    return reply;
}

inline const wchar_t* public_scope=L"Mod Menu users who sync achievements";
struct Model {
    std::string token,state,code;
    std::wstring message;
    uint64_t next=0,deadline=0,published=UINT64_MAX;
    bool busy=false,loaded=false,enabled=false;
    uint64_t revision=0;
    void reset(){
        if(!token.empty())SecureZeroMemory(token.data(),token.size());
        token.clear();state.clear();code.clear();message.clear();busy=loaded=enabled=false;
        next=deadline=0;published=UINT64_MAX;++revision;
    }
    void accept(Reply& reply,uint64_t now,CompletionRates& rates){
        busy=false;
        ++revision;
        if(reply.action==Action::load){
            loaded=true;
            token=std::move(reply.token);
            enabled=reply.enabled;
            return;
        }
        if(reply.action==Action::enable||reply.action==Action::pause){
            if(reply.persisted){
                enabled=reply.enabled;
                next=now;
                published=UINT64_MAX;
                message=enabled?L"Syncing saved achievement progress":L"Achievement sync paused";
            }else message=L"Could not save the sync setting. Try again.";
            return;
        }
        if(reply.action==Action::start){
            if(reply.status==200){
                token=std::move(reply.token);state=std::move(reply.state);code=std::move(reply.code);
                next=now+3500;deadline=now+600000;message=L"Open Steam sign-in and enter the pairing code";
            }else message=L"Could not start Steam sign-in. Try again.";
            return;
        }
        if(reply.paused)enabled=false;
        if(reply.action==Action::withdraw&&reply.status==200){
            ssc_beta::wipe(token);published=UINT64_MAX;
            rates.replace(reply.body);message=L"Shared achievement progress removed";return;
        }
        if(reply.status==401&&(reply.action==Action::check||reply.action==Action::withdraw)){
            clear_pairing();
            message=L"Steam confirmation expired. Confirm again to remove progress.";
            return;
        }
        if(reply.action==Action::check){
            next=now+4000;
            if(reply.status==200){
                token=std::move(reply.token);state.clear();code.clear();next=now;
                message=L"Account confirmed. You can now remove shared progress.";
            }
            return;
        }
        if(reply.action==Action::publish){
            next=now+30000;
            if(reply.status==200&&rates.replace(reply.body)){
                published=reply.revision;
                message=L"Achievement progress synced";
            }else message=L"Sync unavailable. Retrying automatically.";
        }else if(reply.action==Action::withdraw){
            message=L"Could not remove shared progress. Try again.";
        }
    }
    void clear_pairing(){
        ssc_beta::wipe(token);state.clear();code.clear();
        next=deadline=0;
        ++revision;
    }
};
}
