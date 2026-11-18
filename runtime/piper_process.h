#pragma once
#include <windows.h>
#include <algorithm>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
namespace ssc_piper {
inline std::wstring quote_argument(const std::wstring& value){
 std::wstring out=L"\"";size_t slashes=0;
 for(auto c:value){if(c==L'\\'){++slashes;continue;}if(c==L'\"')out.append(slashes*2+1,L'\\');else out.append(slashes,L'\\');slashes=0;out+=c;}
 out.append(slashes*2,L'\\');out+=L'\"';return out;
}
class PipeProcess {
 HANDLE process_=nullptr,job_=nullptr,input_=nullptr,output_=nullptr;
 std::string pending_;
 public:
 PipeProcess()=default;PipeProcess(const PipeProcess&)=delete;PipeProcess& operator=(const PipeProcess&)=delete;
 ~PipeProcess(){reset();}
 bool active()const{return process_&&WaitForSingleObject(process_,0)==WAIT_TIMEOUT;}
 void reset(){if(job_){CloseHandle(job_);job_=nullptr;}if(process_){WaitForSingleObject(process_,200);CloseHandle(process_);process_=nullptr;}if(input_){CloseHandle(input_);input_=nullptr;}if(output_){CloseHandle(output_);output_=nullptr;}pending_.clear();}
 bool start(const std::wstring& program,const std::vector<std::wstring>& arguments,const std::wstring& directory){
  reset();SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};HANDLE child_input=nullptr,child_output=nullptr,discard=nullptr;
  auto close_child=[&](){if(child_input)CloseHandle(child_input);if(child_output)CloseHandle(child_output);if(discard&&discard!=INVALID_HANDLE_VALUE)CloseHandle(discard);};
  if(!CreatePipe(&child_input,&input_,&security,4096)||!CreatePipe(&output_,&child_output,&security,4096)){close_child();reset();return false;}
  SetHandleInformation(input_,HANDLE_FLAG_INHERIT,0);SetHandleInformation(output_,HANDLE_FLAG_INHERIT,0);
  discard=CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr);if(discard==INVALID_HANDLE_VALUE){close_child();reset();return false;}
  SIZE_T bytes=0;InitializeProcThreadAttributeList(nullptr,1,0,&bytes);std::vector<unsigned char> storage(bytes);auto attributes=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
  if(!InitializeProcThreadAttributeList(attributes,1,0,&bytes)){close_child();reset();return false;}
  HANDLE handles[]={child_input,child_output,discard};bool attributes_ok=UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,handles,sizeof(handles),nullptr,nullptr)!=FALSE;
  job_=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE|JOB_OBJECT_LIMIT_ACTIVE_PROCESS|JOB_OBJECT_LIMIT_PROCESS_MEMORY;limits.BasicLimitInformation.ActiveProcessLimit=1;limits.ProcessMemoryLimit=SIZE_T(1536)*1024*1024;
  if(!attributes_ok||!job_||!SetInformationJobObject(job_,JobObjectExtendedLimitInformation,&limits,sizeof(limits))){DeleteProcThreadAttributeList(attributes);close_child();reset();return false;}
  std::wstring command=quote_argument(program);for(const auto& argument:arguments)command+=L" "+quote_argument(argument);
  STARTUPINFOEXW info{};info.StartupInfo.cb=sizeof(info);info.StartupInfo.dwFlags=STARTF_USESTDHANDLES;info.StartupInfo.hStdInput=child_input;info.StartupInfo.hStdOutput=child_output;info.StartupInfo.hStdError=discard;info.lpAttributeList=attributes;PROCESS_INFORMATION result{};
  bool created=CreateProcessW(program.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW|CREATE_SUSPENDED|EXTENDED_STARTUPINFO_PRESENT|BELOW_NORMAL_PRIORITY_CLASS,nullptr,directory.c_str(),&info.StartupInfo,&result)!=FALSE;
  DeleteProcThreadAttributeList(attributes);close_child();
  if(!created){reset();return false;}process_=result.hProcess;
  if(!AssignProcessToJobObject(job_,process_)){TerminateProcess(process_,1);CloseHandle(result.hThread);reset();return false;}
  ResumeThread(result.hThread);CloseHandle(result.hThread);return true;
 }
 bool send(const std::string& value){if(!active()||value.size()>8192)return false;const std::string line=value+"\n";DWORD written=0;return WriteFile(input_,line.data(),DWORD(line.size()),&written,nullptr)&&written==line.size();}
 bool line(std::string& value,size_t limit,DWORD milliseconds,const std::function<bool()>& cancel){
  const auto started=GetTickCount64();
  for(;;){
   auto end=pending_.find('\n');if(end!=std::string::npos){if(end>limit)return false;value=pending_.substr(0,end);pending_.erase(0,end+1);return true;}
   if(pending_.size()>limit||cancel()||GetTickCount64()-started>milliseconds){reset();return false;}
   DWORD available=0;if(!output_||!PeekNamedPipe(output_,nullptr,0,nullptr,&available,nullptr))return false;
   if(available){char buffer[8192];DWORD got=0;if(!ReadFile(output_,buffer,std::min<DWORD>(available,sizeof(buffer)),&got,nullptr)||!got)return false;pending_.append(buffer,got);continue;}
   if(!active())return false;
   Sleep(10);
  }
 }
 bool finish(DWORD milliseconds,const std::function<bool()>& cancel){auto started=GetTickCount64();while(active()){if(cancel()||GetTickCount64()-started>milliseconds){reset();return false;}Sleep(10);}DWORD code=1;return process_&&GetExitCodeProcess(process_,&code)&&code==0;}
};
}
