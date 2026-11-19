#include "../runtime/update_module.h"
#include <cassert>
#include <iostream>
#include <iterator>
static unsigned closes=0;
static LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM w,LPARAM l){if(message==WM_CLOSE){++closes;return 0;}return DefWindowProcW(window,message,w,l);}
static HWND make_window(HWND owner=nullptr,bool visible=true){return CreateWindowExW(WS_EX_NOACTIVATE,L"SSCFallbackUpdateFixture",L"Updater test",WS_POPUP|(visible?WS_VISIBLE:0),-32000,-32000,64,64,owner,nullptr,GetModuleHandleW(nullptr),nullptr);}
static void pump(){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}}
static void status(const std::string& text){std::ofstream out(ssc_update::job/L"status.txt",std::ios::binary|std::ios::trunc);out<<text;out.close();assert(out);}
int main(int argc,char** argv){
 WNDCLASSW wc{};wc.lpfnWndProc=window_proc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"SSCFallbackUpdateFixture";assert(RegisterClassW(&wc));
 if(argc==7&&std::string(argv[1])=="--update-download"){
  auto folder=std::filesystem::path(argv[5]);auto window=make_window();assert(window);
  // The real helper uses the supplied job folder; no network or installer is run here.
  {std::ofstream out(folder/L"status.txt");out<<"downloading Fixture helper is running";}
  {std::ofstream out(folder/L"captured.json");out<<nlohmann::json{{"root",argv[2]},{"pid",std::stoul(argv[3])},{"created",std::stoull(argv[4])},{"version",argv[6]},{"window",reinterpret_cast<uintptr_t>(window)}};}
  auto start=GetTickCount64();while(!std::filesystem::exists(folder/L"stop")&&GetTickCount64()-start<10000){pump();Sleep(10);}DestroyWindow(window);return 0;
 }
 assert(argc==3);using namespace ssc_update;const std::string target=argv[2];assert(valid_version(target));
 version.clear();assert(!notification_due(true));version=target;assert(!notification_due(false)&&notification_due(true));ignored_version=target;assert(!notification_due(true));ignored_version.clear();notified=true;assert(!notification_due(true));notified=false;applying=true;assert(!notification_due(true));applying=false;
 auto own=make_window();assert(own&&fallback_window()==own);auto hidden=make_window(nullptr,false);auto owned=make_window(own);assert(fallback_window()==own);
 auto second=make_window();assert(second&&!fallback_window());DestroyWindow(second);assert(fallback_window()==own);
 assert(fallback_direct_update());linux_install=true;assert(!fallback_direct_update());linux_desktop=true;assert(fallback_direct_update());linux_install=linux_desktop=false;
 job=std::filesystem::path(argv[1])/std::to_string(GetTickCount64());assert(std::filesystem::create_directories(job));wchar_t exe[32768]{};assert(GetModuleFileNameW(nullptr,exe,32768));helper=exe;version=target;
 begin_fallback_update();assert(process&&applying&&!closing);auto child=GetProcessId(process);assert(child&&child!=GetCurrentProcessId());
 auto start=GetTickCount64();while(!std::filesystem::exists(job/L"captured.json")&&GetTickCount64()-start<5000)Sleep(10);nlohmann::json captured;{std::ifstream in(job/L"captured.json");in>>captured;}
 assert(captured["version"]==target&&captured["pid"]==GetCurrentProcessId());assert(std::filesystem::path(captured["root"].get<std::string>())==std::filesystem::path(exe).parent_path());
 FILETIME created,exited,kernel,user;assert(GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user));ULARGE_INTEGER ticks;ticks.LowPart=created.dwLowDateTime;ticks.HighPart=created.dwHighDateTime;assert(captured["created"]==ticks.QuadPart+504911232000000000ULL);
 auto foreign=reinterpret_cast<HWND>(captured["window"].get<uintptr_t>());assert(IsWindow(foreign)&&fallback_window()==own&&!request_game_close(foreign));
 poll(own,true);pump();assert(!closes&&!closing);
 status("readyish not an installer-ready status");poll(own,true);pump();assert(!closes&&!closing);
 status("ready Fixture installer ready");poll(own,false);pump();assert(!closes&&!closing);poll(nullptr,true);pump();assert(!closes&&!closing);
 second=make_window();assert(!fallback_window());poll(fallback_window(),true);pump();assert(!closes&&!closing);DestroyWindow(second);
 poll(foreign,true);pump();assert(!closes&&!closing);poll(fallback_window(),true);pump();assert(closing&&closes==1);poll(own,true);pump();assert(closes==1);
 {std::ofstream out(job/L"stop");out<<"done";}assert(WaitForSingleObject(process,5000)==WAIT_OBJECT_0);poll(own,true);assert(!process);
 closing=false;applying=true;version=target;result.clear();process=CreateEventW(nullptr,TRUE,FALSE,nullptr);status("error Installer checksum rejected");poll(own,true);pump();assert(!applying&&version.empty()&&!closing&&closes==1);CloseHandle(process);process=nullptr;
 applying=true;version=target;result.clear();process=CreateEventW(nullptr,TRUE,TRUE,nullptr);status("downloading Interrupted fixture helper");poll(own,true);pump();assert(!process&&!applying&&version.empty()&&!closing&&closes==1);assert(message.find(L"game was left open")!=std::wstring::npos);
 DestroyWindow(owned);DestroyWindow(hidden);DestroyWindow(own);
 std::cout<<"PASS fallback uses normal update helper with exact root/PID/start time/version; unique current-process window only; downloading, malformed readiness, no restart permission, absent/ambiguous/foreign windows, errors and stopped helper leave game open; verified readiness closes once; Linux manual/native routes retained\n";
}
