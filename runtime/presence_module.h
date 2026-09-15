#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <ctime>
#include <cmath>
#include "third_party/json.hpp"
#include "presence_app.h"

// Discord transport owns no Steam interfaces. Only copied presentation data crosses threads.
namespace ssc_rpc {
using Json=nlohmann::json;
inline bool valid_id(const std::string& s) {
    if(s.size()<17||s.size()>20||s[0]=='0')return false;
    for(char c:s)if(c<'0'||c>'9')return false;
    try {std::stoull(s);return true;}catch(...){return false;}
}
inline std::string utf8_limit(std::string s,size_t limit=128) {
    // Validate UTF-8 before placing it in JSON; never split a multibyte character.
    if(!s.empty()&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s.data(),int(s.size()),nullptr,0))return {};
    for(char& c:s)if(static_cast<unsigned char>(c)<32)c=' ';
    if(s.size()>limit){size_t n=limit;while(n&&(static_cast<unsigned char>(s[n])&0xc0)==0x80)--n;s.resize(n);}
    return s;
}
struct Snapshot {std::string details="Skillshot City",state="Game running";int phase=0;};
inline Snapshot from_steam(const std::string& display,const std::string& status) {
    Snapshot s;
    if(display=="#Status_AtMainMenu"){s.details="Main menu";s.state="Choosing a game";s.phase=1;}
    else if(display=="#Status_Server"){
        s.phase=2;s.details=utf8_limit(status);if(s.details.empty())s.details="In game";
        s.state="In a round";
        // Native status already includes mode, round (where applicable), level and class.
        // Use two Discord lines when it exceeds one line's byte limit.
        if(status.size()>128){size_t split=status.rfind(' ',120);if(split!=std::string::npos&&split>32){s.details=utf8_limit(status.substr(0,split));s.state=utf8_limit(status.substr(split+1));}}
    }
    return s;
}
inline Json activity(const Snapshot& s,std::time_t start,bool timer) {
    Json a={{"details",utf8_limit(s.details)},{"state",utf8_limit(s.state)},{"instance",false}};
    a["assets"]={{"large_image",large_image_key},{"large_text","Skillshot City - SSC Mods"}};
    if(timer)a["timestamps"]={{"start",start}};
    return a;
}
inline std::vector<unsigned char> frame(uint32_t opcode,const std::string& body) {
    std::vector<unsigned char> b(8+body.size());uint32_t n=uint32_t(body.size());
    for(int i=0;i<4;++i){b[i]=static_cast<unsigned char>(opcode>>(8*i));b[4+i]=static_cast<unsigned char>(n>>(8*i));}
    std::copy(body.begin(),body.end(),b.begin()+8);return b;
}
inline Json parse(const std::string& body) {
    return Json::parse(body,[](int depth,Json::parse_event_t,Json&){if(depth>16)throw std::runtime_error("RPC nesting limit");return true;});
}
class Channel {
    HANDLE pipe=INVALID_HANDLE_VALUE;
    std::vector<unsigned char> incoming;
public:
    Channel()=default;Channel(const Channel&)=delete;Channel& operator=(const Channel&)=delete;
    ~Channel(){close();}
    bool connected()const{return pipe!=INVALID_HANDLE_VALUE;}
    void close(){if(connected())CloseHandle(pipe);pipe=INVALID_HANDLE_VALUE;incoming.clear();}
    bool open(const wchar_t* path){close();pipe=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);return connected();}
    bool io(bool writing,void* data,DWORD size,DWORD& count){
        OVERLAPPED ov{};ov.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!ov.hEvent)return false;
        BOOL ok=writing?WriteFile(pipe,data,size,&count,&ov):ReadFile(pipe,data,size,&count,&ov);
        if(!ok&&GetLastError()==ERROR_IO_PENDING){
            if(WaitForSingleObject(ov.hEvent,500)!=WAIT_OBJECT_0){CancelIoEx(pipe,&ov);GetOverlappedResult(pipe,&ov,&count,TRUE);CloseHandle(ov.hEvent);return false;}
            ok=GetOverlappedResult(pipe,&ov,&count,FALSE);
        }
        CloseHandle(ov.hEvent);return ok!=FALSE;
    }
    bool send(uint32_t opcode,const std::string& body){
        if(!connected()||body.size()>65536)return false;
        auto b=frame(opcode,body);DWORD n=0;return io(true,b.data(),DWORD(b.size()),n)&&n==b.size();
    }
    // 0: partial/no frame, 1: complete frame, -1: disconnect or invalid size/opcode.
    int receive(uint32_t& opcode,std::string& body){
        if(!connected())return -1;
        DWORD available=0;if(!PeekNamedPipe(pipe,nullptr,0,nullptr,&available,nullptr))return -1;
        if(available&&incoming.size()<65544){
            unsigned char b[4096];DWORD n=0,request=std::min<DWORD>(available,std::min<DWORD>(sizeof(b),DWORD(65544-incoming.size())));
            if(!io(false,b,request,n)||!n)return -1;
            incoming.insert(incoming.end(),b,b+n);
        }
        if(incoming.size()<8)return 0;
        uint32_t length=0;opcode=0;for(int i=0;i<4;++i){opcode|=uint32_t(incoming[i])<<(8*i);length|=uint32_t(incoming[4+i])<<(8*i);}
        if(length>65536||opcode>4)return -1;
        if(incoming.size()<8+length)return 0;
        body.assign(incoming.begin()+8,incoming.begin()+8+length);incoming.erase(incoming.begin(),incoming.begin()+8+length);return 1;
    }
};
struct Shared {
    std::mutex mutex;bool enabled=false,timer=true,started=false,stop=false;
    std::string id;Snapshot snapshot;ULONGLONG sampled=0;
    std::wstring status=L"Disabled";
#ifdef SSC_RPC_TEST
    std::wstring test_pipe;
#endif
};
// Intentionally process-lifetime: the runtime does not hot-unload, and DllMain never waits on IPC.
inline Shared& shared(){static Shared* s=new Shared;return *s;}
inline void status(Shared& s,const wchar_t* value){std::lock_guard<std::mutex> lock(s.mutex);s.status=value;}
inline std::wstring status(){auto& s=shared();std::lock_guard<std::mutex> lock(s.mutex);return s.status;}
inline DWORD WINAPI worker(void* context){
    auto& s=*static_cast<Shared*>(context);Channel channel;std::string id,last_activity,pending;
    bool ready=false;ULONGLONG retry=0,last_send=0,deadline=0;unsigned nonce=0;int phase=-1;std::time_t start=0;
    auto disconnect=[&](){channel.close();ready=false;pending.clear();last_activity.clear();retry=GetTickCount64()+5000;};
    for(;;){
        bool enabled,timer,stop;std::string requested;Snapshot snapshot;ULONGLONG sampled;
        {std::lock_guard<std::mutex> lock(s.mutex);enabled=s.enabled;timer=s.timer;stop=s.stop;requested=s.id;snapshot=s.snapshot;sampled=s.sampled;}
        auto now=GetTickCount64();bool stale=!sampled||now-sampled>10000;
        if(stop||!enabled||!valid_id(requested)||requested!=id||stale){
            if(channel.connected()){
                if(ready)channel.send(1,Json{{"cmd","SET_ACTIVITY"},{"args",{{"pid",GetCurrentProcessId()},{"activity",nullptr}}},{"nonce",std::to_string(++nonce)}}.dump());
                disconnect();
            }
            if(requested!=id){id=requested;retry=0;}
            if(stop)break;
            if(!enabled||!valid_id(id)||stale){status(s,!enabled?L"Disabled":!valid_id(id)?L"Application ID needed":L"Waiting for fresh game status");Sleep(250);continue;}
        }
        try {
            if(!channel.connected()&&now>=retry){
#ifdef SSC_RPC_TEST
                if(!s.test_pipe.empty())channel.open(s.test_pipe.c_str());else
#endif
                for(int i=0;i<10&&!channel.connected();++i){std::wstring path=L"\\\\?\\pipe\\discord-ipc-"+std::to_wstring(i);channel.open(path.c_str());}
                if(channel.connected()&&channel.send(0,Json{{"v",1},{"client_id",id}}.dump())){status(s,L"Connecting to Discord");deadline=now+5000;}
                else {disconnect();status(s,L"Discord desktop unavailable; retrying");}
            }
            if(channel.connected()){
                for(int i=0;i<32;++i){uint32_t opcode;std::string body;int result=channel.receive(opcode,body);if(result<0)throw std::runtime_error("Disconnected");if(!result)break;
                    if(opcode==3){if(!channel.send(4,body))throw std::runtime_error("Pong failed");continue;}
                    if(opcode==4)continue;
                    if(opcode!=1)throw std::runtime_error("Discord closed connection");
                    auto message=parse(body);if(!message.is_object())throw std::runtime_error("Invalid response");
                    auto evt=message.find("evt");auto cmd=message.find("cmd");auto token=message.find("nonce");
                    if(evt!=message.end()&&*evt=="ERROR")throw std::runtime_error("Discord rejected request");
                    if(!ready&&evt!=message.end()&&*evt=="READY"){ready=true;last_send=0;deadline=0;status(s,L"Connected; sending activity");}
                    if(ready&&!pending.empty()&&cmd!=message.end()&&*cmd=="SET_ACTIVITY"&&token!=message.end()&&*token==pending){pending.clear();deadline=0;status(s,L"Activity accepted by Discord");}
                }
                if(deadline&&now>deadline)throw std::runtime_error("Discord response timeout");
                if(ready&&pending.empty()){
                    if(phase!=snapshot.phase){phase=snapshot.phase;start=std::time(nullptr);}
                    auto a=activity(snapshot,start,timer);auto serialized=a.dump();
                    if(serialized!=last_activity&&(!last_send||now-last_send>=15000)){
                        pending=std::to_string(++nonce);
                        if(!channel.send(1,Json{{"cmd","SET_ACTIVITY"},{"args",{{"pid",GetCurrentProcessId()},{"activity",a}}},{"nonce",pending}}.dump()))throw std::runtime_error("Write failed");
                        last_activity=serialized;last_send=now;deadline=now+5000;
                    }
                }
            }
        }catch(const std::exception&){disconnect();status(s,L"Discord connection rejected/lost; retrying");}
        Sleep(250);
    }
    return 0;
}
inline void submit(bool enabled,const std::string& id,bool timer,const Snapshot& snapshot){
    auto& s=shared();std::lock_guard<std::mutex> lock(s.mutex);s.enabled=enabled;s.id=id;s.timer=timer;s.snapshot=snapshot;s.sampled=GetTickCount64();
    if(!s.started&&enabled&&valid_id(id)){HANDLE thread=CreateThread(nullptr,0,worker,&s,0,nullptr);if(thread){s.started=true;CloseHandle(thread);}else s.status=L"Could not start Discord worker";}
    if(!s.started)s.status=enabled?L"Application ID needed":L"Disabled";
}
}
