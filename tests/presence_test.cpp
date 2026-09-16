#define WIN32_LEAN_AND_MEAN
#define SSC_RPC_TEST
#include "../runtime/presence_source.h"
#include <cassert>
#include <thread>
#include <atomic>
#include <cstdio>
using namespace ssc_rpc;
static Json read_frame(HANDLE pipe,uint32_t expected){
    unsigned char header[8];DWORD n;assert(ReadFile(pipe,header,8,&n,nullptr)&&n==8);
    uint32_t op=0,length=0;for(int i=0;i<4;++i){op|=uint32_t(header[i])<<(8*i);length|=uint32_t(header[4+i])<<(8*i);}
    assert(op==expected&&length<=65536);std::string body(length,0);assert(ReadFile(pipe,body.data(),length,&n,nullptr)&&n==length);return parse(body);
}
static void send_frame(HANDLE pipe,uint32_t opcode,const std::string& body,bool fragmented=false){
    auto data=frame(opcode,body);DWORD n;
    if(fragmented){assert(WriteFile(pipe,data.data(),3,&n,nullptr)&&n==3);Sleep(30);assert(WriteFile(pipe,data.data()+3,DWORD(data.size()-3),&n,nullptr)&&n==data.size()-3);}
    else assert(WriteFile(pipe,data.data(),DWORD(data.size()),&n,nullptr)&&n==data.size());
}
int main(){
    assert(valid_id(bundled_application_id));
    assert(activity(Snapshot{},0,false)["assets"]["large_image"]=="ssc_mods");
    assert(valid_id("123456789012345678"));
    for(auto id:{"","token","00000000000000000","1234567890123456","18446744073709551616","12345678901234567\n"})assert(!valid_id(id));
    assert(utf8_limit("abc\nxyz")=="abc xyz");assert(utf8_limit(std::string(127,'a')+"\xc3\xa9").size()==127);assert(utf8_limit("\xff").empty());
    auto menu=from_steam("#Status_AtMainMenu","stale previous round level 80");assert(menu.details=="Main menu"&&menu.phase==1);
    auto game=from_steam("#Status_Server","BR Solos Round 2 - Level 7 / Marksman");assert(game.details.find("Level 7")!=std::string::npos&&game.phase==2);
    assert(from_steam("unknown","private data").details=="Skillshot City");
    assert(!activity(menu,123,false).contains("timestamps"));assert(activity(menu,123,true)["timestamps"]["start"]==123);
    bool rejected=false;try{parse(std::string(25,'[')+"0"+std::string(25,']'));}catch(...){rejected=true;}assert(rejected);
    rejected=false;try{parse("invalid");}catch(...){rejected=true;}assert(rejected);

    RankedFields eligibility;eligibility.mode=6;eligibility.active=1;eligibility.kind=0;eligibility.name_length=7;eligibility.account_level=481;eligibility.eligibility=2;eligibility.normal_threshold=25;eligibility.bonus_threshold=25;eligibility.round=2;eligibility.committed=1;
    assert(ranked(eligibility));eligibility.singleplayer=1;assert(!ranked(eligibility));eligibility.singleplayer=0;
    eligibility.committed=0;assert(!ranked(eligibility));eligibility.committed=1;
    eligibility.disabled=1;assert(!ranked(eligibility));eligibility.disabled=0;
    eligibility.eligibility=1;assert(!ranked(eligibility));eligibility.eligibility=2;
    eligibility.mode=5;assert(!ranked(eligibility));eligibility.mode=6;
    auto rated=game;add_rating(rated,true,2450.9f,12,5);assert(rated.state=="Ranked | SSC rating: 2450");
    rated=game;add_rating(rated,false,2450,12,5);assert(rated.state==game.state);
    rated=menu;add_rating(rated,true,2450,12,5);assert(rated.state==menu.state);
    rated=game;add_rating(rated,true,2450,2,5);assert(rated.state=="Ranked | Placement matches");
    rated=game;add_rating(rated,true,NAN,12,5);assert(rated.state=="Ranked");
    auto base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x1000000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto world=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0xc000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    auto actor=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x3460,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(base&&world&&actor);
    auto put=[](uintptr_t at,auto value){std::memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));};
    put(base+0xfa235c,1);assert(native_fallback(base).details=="Main menu");
    put(base+0xfa235c,2);put(base+0xdda5d0,world);put(world+0x128,6);put(world+0x480,1);put(world+0x3e0,3);
    put(world+0xa5b8,actor);put(world+0xa5c0,actor+0x3460);put(base+0xe11de8,actor);put(base+0xe0e380,0);
    put(actor+0x81,static_cast<unsigned char>(1));put(actor+0x858,7);put(actor+0x350,-1);
    auto sampled=native_fallback(base);assert(sampled.details=="BR Solos - Round 2"&&sampled.state=="Level 7");
    put(actor+0x6e8,uint64_t(7));put(actor+0x270,static_cast<unsigned char>(1));put(actor+0xa9c,481);put(actor+0x204,2);
    put(base+0xfa6684,25.f);put(base+0xfa6688,25.f);put(base+0xdf4040,1);put(base+0xdf48e4,4554.f);put(base+0xdf48e0,41.f);put(base+0xfa6658,5);
    sample_rating(base,sampled);assert(sampled.state=="Ranked | SSC rating: 4554 | Level 7");
    put(world+0x2f4,static_cast<unsigned char>(1));sampled=native_fallback(base);sample_rating(base,sampled);assert(sampled.state=="Level 7");
    put(actor+0x78,5);assert(native_fallback(base).state=="In game");put(actor+0x78,0);
    put(base+0xe0e380,5);assert(native_fallback(base).state=="In game");
    put(base+0xfa235c,1);sampled=native_fallback(base);sample_rating(base,sampled);assert(sampled.state=="Choosing a game");
    VirtualFree(reinterpret_cast<void*>(actor),0,MEM_RELEASE);VirtualFree(reinterpret_cast<void*>(world),0,MEM_RELEASE);VirtualFree(reinterpret_cast<void*>(base),0,MEM_RELEASE);
    // An isolated mock pipe: these tests never connect to Discord or publish an activity.
    Shared state;state.enabled=true;state.id="123456789012345678";state.snapshot=game;state.sampled=GetTickCount64();
    state.test_pipe=L"\\\\.\\pipe\\ssc-rpc-test-"+std::to_wstring(GetCurrentProcessId());
    HANDLE pipe=CreateNamedPipeW(state.test_pipe.c_str(),PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT,1,65536,65536,0,nullptr);assert(pipe!=INVALID_HANDLE_VALUE);
    std::atomic<int> stage{0};
    std::thread server([&](){
        assert(ConnectNamedPipe(pipe,nullptr)||GetLastError()==ERROR_PIPE_CONNECTED);
        auto handshake=read_frame(pipe,0);assert(handshake["v"]==1&&handshake["client_id"]==state.id);
        send_frame(pipe,1,Json{{"cmd","DISPATCH"},{"evt","READY"},{"data",Json::object()}}.dump(),true);
        auto update=read_frame(pipe,1);assert(update["cmd"]=="SET_ACTIVITY"&&update["args"]["pid"]==GetCurrentProcessId());
        assert(update["args"]["activity"]["details"]==game.details);stage=1;
        // Wrong nonce must not report a successful publication.
        send_frame(pipe,1,Json{{"cmd","SET_ACTIVITY"},{"nonce","wrong"}}.dump());Sleep(600);
        {std::lock_guard<std::mutex> lock(state.mutex);assert(state.status!=L"Activity accepted by Discord");}
        send_frame(pipe,3,"{\"ping\":1}",true);assert(read_frame(pipe,4)["ping"]==1);
        send_frame(pipe,1,Json{{"cmd","SET_ACTIVITY"},{"evt",nullptr},{"nonce",update["nonce"]}}.dump());stage=2;
        auto clear=read_frame(pipe,1);assert(clear["args"]["activity"].is_null());stage=3;
        DisconnectNamedPipe(pipe);
    });
    HANDLE thread=CreateThread(nullptr,0,worker,&state,0,nullptr);assert(thread);
    auto deadline=GetTickCount64()+10000;bool accepted=false;
    while(GetTickCount64()<deadline){Sleep(50);std::lock_guard<std::mutex> lock(state.mutex);if(state.status==L"Activity accepted by Discord"){accepted=true;state.enabled=false;break;}}
    assert(accepted);deadline=GetTickCount64()+3000;while(stage!=3&&GetTickCount64()<deadline)Sleep(50);assert(stage==3);
    {std::lock_guard<std::mutex> lock(state.mutex);state.stop=true;}
    assert(WaitForSingleObject(thread,3000)==WAIT_OBJECT_0);CloseHandle(thread);server.join();CloseHandle(pipe);

    // Oversized frames fail before allocation or JSON parsing.
    auto path=state.test_pipe+L"-bad";pipe=CreateNamedPipeW(path.c_str(),PIPE_ACCESS_DUPLEX,PIPE_TYPE_BYTE|PIPE_WAIT,1,65536,65536,0,nullptr);assert(pipe!=INVALID_HANDLE_VALUE);
    Channel client;assert(client.open(path.c_str()));assert(ConnectNamedPipe(pipe,nullptr)||GetLastError()==ERROR_PIPE_CONNECTED);
    unsigned char oversized[]={1,0,0,0,1,0,1,0};DWORD n;assert(WriteFile(pipe,oversized,8,&n,nullptr));uint32_t opcode;std::string body;assert(client.receive(opcode,body)==-1);client.close();CloseHandle(pipe);
    std::puts("PASS: IDs, UTF-8 boundaries, stale menu data, activity privacy, timers, JSON limits, fragmented handshake, nonce acknowledgement, ping/pong, disable clear, bounded frames");
}
