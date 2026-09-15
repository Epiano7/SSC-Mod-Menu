#pragma once
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
namespace ssc_update {
inline std::filesystem::path job,helper;
inline HANDLE process=nullptr;
inline bool checked=false,notified=false,applying=false,closing=false;
inline std::string result,version;
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
  return std::filesystem::is_regular_file(helper);
 }catch(...){return false;}
}
inline void check(const std::filesystem::path& state){
 if(process)return;
 checked=true;version.clear();result.clear();notified=false;
 if(!prepare(state)||!launch(L"--update-check "+quote(job.wstring()))){message=L"Update helper unavailable. Run the latest installer.";return;}
 message=L"Checking GitHub...";
}
inline void apply(){
 if(process||!available())return;
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
  if(line.rfind("available ",0)==0){version=line.substr(10);message=L"Update "+std::wstring(version.begin(),version.end())+L" is available.";}
  else if(line=="current")message=L"You're up to date.";
  else{auto pos=line.find(' ');auto value=pos==std::string::npos?line:line.substr(pos+1);int n=MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),nullptr,0);message.assign(n,0);MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),message.data(),n);}
  if(line.rfind("error ",0)==0){applying=false;version.clear();}
 }
 if(ended&&applying&&!closing){applying=false;version.clear();if(result.rfind("error ",0)!=0)message=L"Update helper stopped. The game was left open.";}
 if(process&&applying&&!closing&&may_restart&&result.rfind("ready ",0)==0){closing=true;PostMessageW(window,WM_CLOSE,0,0);}
 return changed;
}
}
