#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <shellapi.h>
#include "proton_update.h"
namespace ssc_update {
inline std::filesystem::path job,helper;
inline HANDLE process=nullptr;
inline bool checked=false,notified=false,applying=false,closing=false;
inline bool linux_install=false;
inline bool linux_desktop=false;
inline std::filesystem::path linux_root;
inline std::string linux_version;
inline const wchar_t* action_label(){return linux_install&&!linux_desktop?L"DOWNLOAD LINUX UPDATE":L"UPDATE AND RESTART";}
inline std::string result,version,ignored_version;
inline bool valid_version(const std::string& value){
 if(value.empty()||value.size()>20)return false;
 unsigned parts=0,digits=0;for(char c:value){if(c=='.'){if(!digits||++parts>2)return false;digits=0;}else if(c>='0'&&c<='9'){if(++digits>5)return false;}else return false;}return parts==2&&digits>0;
}
inline bool notification_due(bool main_menu){return main_menu&&!notified&&!version.empty()&&!applying&&version!=ignored_version;}
inline std::wstring message=L"Updates are checked once at the main menu.";
inline bool available(){return !version.empty()&&!applying;}
inline std::wstring quote(const std::wstring& value){return L"\""+value+L"\"";}
inline bool launch(const std::wstring& args){
 std::wstring command=quote(helper.wstring())+L" "+args;
 STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
 if(!CreateProcessW(helper.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,job.c_str(),&si,&pi))return false;
 CloseHandle(pi.hThread);process=pi.hProcess;return true;
}
inline bool prepare(const std::filesystem::path& state){
 try{
  wchar_t executable[32768];if(!GetModuleFileNameW(nullptr,executable,32768))return false;
  auto root=std::filesystem::path(executable).parent_path();
  job=state/L"updates"/(std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));
  if(!std::filesystem::create_directories(job))return false;
  helper=root/L"SSCMods"/L"Uninstall.exe";
  auto manifest=root/L"SSCMods"/L"linux-manifest.json";
  if(std::filesystem::exists(manifest)){
   if(std::filesystem::file_size(manifest)>65536)return false;
   std::ifstream in(manifest);nlohmann::json data;in>>data;
   if(data.at("product")!="SSCMods"||data.at("platform")!="linux-proton")return false;
   linux_version=data.at("version").get<std::string>();if(!valid_version(linux_version))return false;
   linux_install=true;linux_desktop=data.value("native_desktop",false);linux_root=root;return true;
  }
  return std::filesystem::is_regular_file(helper);
 }catch(...){return false;}
}
inline void check(const std::filesystem::path& state){
 if(process)return;
 checked=true;version.clear();result.clear();
 if(!prepare(state)){message=L"Update helper unavailable. Run the latest installer.";return;}
 if(linux_install){auto task=new ssc_proton_update::Job{job,linux_version};process=CreateThread(nullptr,0,ssc_proton_update::check,task,0,nullptr);if(!process)delete task;}
 else launch(L"--update-check "+quote(job.wstring()));
 if(!process){message=L"Could not start the update check.";return;}
 message=L"Checking GitHub...";
}
inline void apply(){
 if(process||!available())return;
 if(linux_install){
  if(!valid_version(version))return;
  if(linux_desktop){auto task=new ssc_proton_update::ApplyJob{job,linux_root,version};process=CreateThread(nullptr,0,ssc_proton_update::apply,task,0,nullptr);if(!process){delete task;message=L"Could not start the Linux updater.";}else{applying=true;message=L"Preparing Linux update...";}return;}
  auto url=L"https://github.com/Epiano7/SSC-Mod-Menu/releases/tag/v"+std::wstring(version.begin(),version.end());
  auto result=ShellExecuteW(nullptr,L"open",url.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
  message=reinterpret_cast<intptr_t>(result)>32?L"Extract the Linux package and open SSC-Mod-Menu-Setup.":L"Could not open the browser. Download the Linux package from GitHub.";return;
 }
 try{auto copy=job/L"UpdateHelper.exe";std::filesystem::copy_file(helper,copy);helper=copy;}
 catch(...){message=L"Could not prepare the update helper.";return;}
 wchar_t executable[32768];if(!GetModuleFileNameW(nullptr,executable,32768))return;
 FILETIME created,exited,kernel,user;if(!GetProcessTimes(GetCurrentProcess(),&created,&exited,&kernel,&user))return;
 ULARGE_INTEGER time;time.LowPart=created.dwLowDateTime;time.HighPart=created.dwHighDateTime;
 auto root=std::filesystem::path(executable).parent_path();
 std::wstring args=L"--update-download "+quote(root.wstring())+L" "+std::to_wstring(GetCurrentProcessId())+L" "+std::to_wstring(time.QuadPart+504911232000000000ULL)+L" "+quote(job.wstring())+L" "+std::wstring(version.begin(),version.end());
 if(launch(args)){applying=true;message=L"Preparing update...";}else message=L"Could not start the update helper.";
}
inline bool poll(HWND window,bool may_restart){
 bool changed=false;
 bool ended=process&&WaitForSingleObject(process,0)==WAIT_OBJECT_0;
 if(ended){CloseHandle(process);process=nullptr;changed=true;}
 if(job.empty())return changed;
 std::ifstream in(job/L"status.txt");std::string line;std::getline(in,line);if(line.size()>4096)return changed;
 if(!line.empty()&&line!=result){result=line;changed=true;
  if(line.rfind("available ",0)==0&&valid_version(line.substr(10))){version=line.substr(10);message=L"Update "+std::wstring(version.begin(),version.end())+L" is available.";}
  else if(line=="current")message=L"You're up to date.";
  else{auto pos=line.find(' ');auto value=pos==std::string::npos?line:line.substr(pos+1);int n=MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),nullptr,0);message.assign(n,0);MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),message.data(),n);}
  if(line.rfind("error ",0)==0){applying=false;version.clear();}
 }
 if(ended&&applying&&!closing){applying=false;version.clear();if(result.rfind("error ",0)!=0)message=L"Update helper stopped. The game was left open.";}
 if(process&&applying&&!closing&&may_restart&&result.rfind("ready ",0)==0){closing=true;PostMessageW(window,WM_CLOSE,0,0);}
 return changed;
}
}
