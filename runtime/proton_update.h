#pragma once
#include <windows.h>
#include <winhttp.h>
#include <array>
#include <memory>
#include <stdexcept>
#include "third_party/json.hpp"

namespace ssc_proton_update {
struct ApplyJob {std::filesystem::path job,root;std::string version;};
inline DWORD WINAPI apply(void* parameter){
 std::unique_ptr<ApplyJob> task(static_cast<ApplyJob*>(parameter));
 auto status=task->root/L"SSCMods/linux-update-status.txt";
 auto request=task->root/L"SSCMods/linux-update-request.json";
 auto publish=[&](const std::string& text){auto temporary=task->job/L"native-status.tmp";{std::ofstream out(temporary);out<<text<<"\n";if(!out)return false;}return MoveFileExW(temporary.c_str(),(task->job/L"status.txt").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;};
 try{
  std::error_code error;std::filesystem::remove(status,error);
  auto temporary=request;temporary+=L".tmp";
  {std::ofstream out(temporary);out<<nlohmann::json{{"version",task->version}}.dump();if(!out)throw std::runtime_error("Could not request the Linux update");}
  if(!MoveFileExW(temporary.c_str(),request.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Could not request the Linux update");
  auto started=GetTickCount64();bool responded=false;std::string last;
  while(GetTickCount64()-started<480000){
   std::ifstream in(status);std::string line;std::getline(in,line);in.close();
   if(!line.empty()&&line.size()<4096){responded=true;if(line!=last&&publish(line))last=line;if(last==line&&(line.rfind("error ",0)==0||line.rfind("complete ",0)==0))return 0;}
   if(!responded&&GetTickCount64()-started>20000)throw std::runtime_error("Linux update helper unavailable. Launch through Steam after running the native installer");
   Sleep(250);
  }
  throw std::runtime_error("Linux update timed out; open the installer and export logs");
 }catch(const std::exception& e){publish(std::string("error ")+e.what());}
 return 1;
}
inline std::array<unsigned,3> parse_version(const std::string& text){
 std::array<unsigned,3> out{};unsigned index=0,digits=0;
 if(text.empty()||text.size()>20)throw std::runtime_error("version");
 for(char c:text){if(c=='.'){if(!digits||++index>2)throw std::runtime_error("version");digits=0;}
 else if(c>='0'&&c<='9'){if(++digits>5)throw std::runtime_error("version");out[index]=out[index]*10+unsigned(c-'0');}
 else throw std::runtime_error("version");}if(index!=2||!digits)throw std::runtime_error("version");return out;
}
inline std::string select(const std::string& body,const std::string& current){
 if(body.size()>1048576)throw std::runtime_error("response");
 auto j=nlohmann::json::parse(body);
 if(j.at("draft").get<bool>()||j.at("prerelease").get<bool>())return "current";
 auto tag=j.at("tag_name").get<std::string>();if(tag.size()<2||tag[0]!='v')throw std::runtime_error("tag");
 auto next=tag.substr(1);if(parse_version(next)<=parse_version(current))return "current";
 unsigned matches=0;for(const auto& asset:j.at("assets")){
  if(asset.value("name",std::string())!="SSC-Mod-Menu-Linux.tar.gz")continue;
  ++matches;auto hash=asset.at("digest").get<std::string>();auto size=asset.at("size").get<int64_t>();
  if(asset.at("state")!="uploaded"||size<1024||size>134217728||hash.size()!=71||hash.substr(0,7)!="sha256:"||
     hash.find_first_not_of("0123456789abcdefABCDEF",7)!=std::string::npos||
     asset.at("browser_download_url")!="https://github.com/Epiano7/SSC-Mod-Menu/releases/download/"+tag+"/SSC-Mod-Menu-Linux.tar.gz")throw std::runtime_error("asset");
 }
 if(matches>1)throw std::runtime_error("duplicate");
 return matches==1?"available "+next:"unavailable The newer release has no Linux package yet.";
}
// Runs inside the game's existing Proton environment; no standalone Wine or .NET helper.
inline std::string fetch(){
 HMODULE module=LoadLibraryW(L"winhttp.dll");if(!module)throw std::runtime_error("http");
 struct Library {HMODULE h;~Library(){FreeLibrary(h);}} library{module};
 #define SSC_HTTP(name) auto name=reinterpret_cast<decltype(&::name)>(GetProcAddress(module,#name));if(!name)throw std::runtime_error("http API")
 SSC_HTTP(WinHttpOpen);SSC_HTTP(WinHttpConnect);SSC_HTTP(WinHttpOpenRequest);SSC_HTTP(WinHttpSendRequest);
 SSC_HTTP(WinHttpReceiveResponse);SSC_HTTP(WinHttpQueryHeaders);SSC_HTTP(WinHttpReadData);SSC_HTTP(WinHttpCloseHandle);SSC_HTTP(WinHttpSetTimeouts);SSC_HTTP(WinHttpSetOption);
 #undef SSC_HTTP
 struct Handle{HINTERNET h;decltype(WinHttpCloseHandle) close;~Handle(){if(h)close(h);}};
 Handle session{WinHttpOpen(L"SSC-Mod-Menu Linux",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,nullptr,nullptr,0),WinHttpCloseHandle};
 if(!session.h)throw std::runtime_error("session");
 if(!WinHttpSetTimeouts(session.h,10000,10000,10000,10000))throw std::runtime_error("timeout");
 Handle connection{WinHttpConnect(session.h,L"api.github.com",443,0),WinHttpCloseHandle};if(!connection.h)throw std::runtime_error("connect");
 Handle request{WinHttpOpenRequest(connection.h,L"GET",L"/repos/Epiano7/SSC-Mod-Menu/releases/latest",nullptr,nullptr,nullptr,WINHTTP_FLAG_SECURE),WinHttpCloseHandle};if(!request.h)throw std::runtime_error("request");
 DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;if(!WinHttpSetOption(request.h,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects)))throw std::runtime_error("redirect policy");
 if(!WinHttpSendRequest(request.h,L"Accept: application/vnd.github+json\r\n",DWORD(-1),nullptr,0,0,0)||!WinHttpReceiveResponse(request.h,nullptr))throw std::runtime_error("response");
 DWORD code=0,size=sizeof(code);if(!WinHttpQueryHeaders(request.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&code,&size,nullptr)||code!=200)throw std::runtime_error("status");
 std::string body;char buffer[8192];DWORD read=0;auto start=GetTickCount64();
 do{if(!WinHttpReadData(request.h,buffer,sizeof(buffer),&read)||body.size()+read>1048576||GetTickCount64()-start>30000)throw std::runtime_error("read");body.append(buffer,read);}while(read);
 return body;
}
struct Job{std::filesystem::path path;std::string current;};
inline DWORD WINAPI check(void* argument){
 std::unique_ptr<Job> job(static_cast<Job*>(argument));std::string result;
 try{result=select(fetch(),job->current);}catch(...){result="error Could not verify the Linux release. Try again later.";}
 try{auto temp=job->path/L"linux-status.tmp";std::ofstream out(temp,std::ios::binary);out<<result;out.close();if(out)MoveFileExW(temp.c_str(),(job->path/L"status.txt").c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}catch(...){}
 return 0;
}
}
