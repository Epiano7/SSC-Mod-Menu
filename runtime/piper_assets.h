#pragma once
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>
#include <atomic>
#include <set>
#include "third_party/json.hpp"
#include "piper_process.h"
#include "piper_payload.h"
namespace ssc_piper {
using Json=nlohmann::json;
inline bool no_reparse(std::filesystem::path path){for(;;){DWORD flags=GetFileAttributesW(path.c_str());if(flags!=INVALID_FILE_ATTRIBUTES&&(flags&FILE_ATTRIBUTE_REPARSE_POINT))return false;auto parent=path.parent_path();if(parent.empty()||parent==path)break;path=parent;}return true;}
inline std::string file_hash(const std::filesystem::path& path){
 if(!no_reparse(path))return {};
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;DWORD size=0,got=0;
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)return {};
 bool ok=BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&got,0)>=0;std::vector<unsigned char> object(size);unsigned char digest[32]{};
 ok=ok&&BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0)>=0;std::ifstream in(path,std::ios::binary);ok=ok&&bool(in);char buffer[65536];
 while(ok&&in){in.read(buffer,sizeof(buffer));auto n=in.gcount();if(n)ok=BCryptHashData(hash,reinterpret_cast<PUCHAR>(buffer),ULONG(n),0)>=0;}
 ok=ok&&!in.bad()&&BCryptFinishHash(hash,digest,32,0)>=0;if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);if(!ok)return {};
 const char* digits="0123456789abcdef";std::string result;result.reserve(64);for(auto byte:digest){result+=digits[byte>>4];result+=digits[byte&15];}return result;
}
inline bool identifier(const std::string& id){if(id=="norman")return true;return id.size()==27&&id.rfind("custom-",0)==0&&std::all_of(id.begin()+7,id.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
inline std::wstring wide(const std::string& text){int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),nullptr,0);if(n<=0)return {};std::wstring out(size_t(n),0);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),int(text.size()),out.data(),n);return out;}
inline bool write_text(const std::filesystem::path& file,const char* bytes){if(!no_reparse(file))return false;std::ofstream out(file,std::ios::binary);out<<bytes;out.close();return bool(out);}
inline bool verify_runtime(const std::filesystem::path& root,bool bootstrap=false){
 try{const auto manifest=Json::parse(runtime_manifest);for(const auto& entry:manifest.at("files")){auto name=entry.at("path").get<std::string>();if(bootstrap&&name.rfind("python/",0)!=0)continue;if(name.empty()||name.find("..")!=std::string::npos||name[0]=='/'||name.find(':')!=std::string::npos||file_hash(root/wide(name))!=entry.at("sha256").get<std::string>())return false;}return bootstrap||file_hash(root/L"ready.json")==runtime_manifest_hash;}catch(...){return false;}
}
struct HttpHandle{HINTERNET value=nullptr;~HttpHandle(){if(value)WinHttpCloseHandle(value);}HttpHandle()=default;explicit HttpHandle(HINTERNET h):value(h){}};
inline bool bootstrap_download(const Json& asset,const std::filesystem::path& destination,const std::function<bool()>& cancel){
 auto url=wide(asset.at("url").get<std::string>());URL_COMPONENTS parsed{};parsed.dwStructSize=sizeof(parsed);parsed.dwHostNameLength=parsed.dwUrlPathLength=DWORD(-1);if(!WinHttpCrackUrl(url.c_str(),DWORD(url.size()),0,&parsed)||parsed.nScheme!=INTERNET_SCHEME_HTTPS)return false;
 std::wstring host(parsed.lpszHostName,parsed.dwHostNameLength),path(parsed.lpszUrlPath,parsed.dwUrlPathLength);if(host!=L"www.python.org")return false;
 HttpHandle session(WinHttpOpen(L"SSC-Mod-Menu-optional-TTS/1.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,nullptr,nullptr,0));if(!session.value)return false;WinHttpSetTimeouts(session.value,5000,5000,5000,5000);
 HttpHandle connection(WinHttpConnect(session.value,host.c_str(),parsed.nPort,0));if(!connection.value)return false;
 HttpHandle request(WinHttpOpenRequest(connection.value,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));if(!request.value)return false;
 DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER,disabled=WINHTTP_DISABLE_AUTHENTICATION|WINHTTP_DISABLE_COOKIES;WinHttpSetOption(request.value,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof(policy));WinHttpSetOption(request.value,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof(disabled));
 if(cancel()||!WinHttpSendRequest(request.value,nullptr,0,nullptr,0,0,0)||!WinHttpReceiveResponse(request.value,nullptr))return false;
 DWORD status=0,length=sizeof(status);if(!WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&status,&length,nullptr)||status!=200)return false;
 size_t expected=asset.at("size").get<size_t>(),total=0;if(!expected||expected>16*1024*1024||!no_reparse(destination))return false;std::ofstream out(destination,std::ios::binary);char buffer[65536];
 while(out&&!cancel()){DWORD got=0;if(!WinHttpReadData(request.value,buffer,sizeof(buffer),&got))return false;if(!got)break;total+=got;if(total>expected)return false;out.write(buffer,got);}out.close();return !cancel()&&out&&total==expected&&file_hash(destination)==asset.at("sha256").get<std::string>();
}
struct Model {std::string id,label;int speakers=1;};
struct AssetStatus {bool busy=false,ready=false;uint64_t revision=0,done=0,total=0;std::string message;std::vector<Model> models;};
class Assets {
 mutable std::mutex mutex_;std::thread worker_;std::atomic<bool> cancel_{false};AssetStatus status_;std::filesystem::path home_,runtime_,models_;
 void message(const std::string& value){std::lock_guard<std::mutex> lock(mutex_);status_.message=value.substr(0,150);++status_.revision;}
 bool child_task(const std::filesystem::path& root,const std::wstring& action,const std::filesystem::path& source={}){
  PipeProcess child;std::vector<std::wstring> args={L"-I",L"-S",L"-B",(root/L"piper_worker.py").wstring(),action,L"--root",root.wstring(),L"--models",models_.wstring()};if(!source.empty()){args.push_back(L"--source");args.push_back(source.wstring());}
  if(!child.start((root/L"python/python.exe").wstring(),args,root.wstring())){message("Could not start the optional speech helper");return false;}
  bool completed=false;std::string line;while(child.line(line,8192,180000,[&]{return cancel_.load();})){
   auto reply=Json::parse(line);auto type=reply.value("type",std::string{});
   if(type=="progress"){std::lock_guard<std::mutex> lock(mutex_);status_.done=reply.value("done",uint64_t(0));status_.total=reply.value("total",uint64_t(0));++status_.revision;}
   else if(type=="error"){message(reply.value("message",std::string("Speech download failed")));return false;}
   else if(type=="installed"||type=="downloaded"||type=="imported"){completed=true;break;}
  }
  if(!completed){message(cancel_?"Download cancelled":"Optional speech helper stopped before completion");return false;}
  return child.finish(5000,[&]{return cancel_.load();});
 }
 bool ensure_runtime(){
  if(verify_runtime(runtime_))return true;
  if(std::filesystem::exists(runtime_)){
   if(!no_reparse(runtime_)||runtime_.parent_path()!=home_){message("Linked speech storage is unsupported");return false;}
   auto backup=home_/(runtime_.filename().wstring()+L".rejected-"+std::to_wstring(GetTickCount64()));if(backup.parent_path()!=home_)return false;
   std::filesystem::rename(runtime_,backup);{std::lock_guard<std::mutex> lock(mutex_);status_.ready=false;++status_.revision;}
  }
  auto stage=home_/L"runtime.partial";if(!no_reparse(stage)||stage.parent_path()!=home_)return false;
  std::filesystem::remove_all(stage);std::filesystem::create_directories(stage/L"python");message("Downloading the optional speech engine");
  auto pins=Json::parse(dependency_pins);auto python=pins.at("assets").at(0);auto archive=stage/L"python.zip";
  if(!bootstrap_download(python,archive,[&]{return cancel_.load();})){message(cancel_?"Download cancelled":"Python runtime download failed verification");return false;}
  wchar_t system[32768]{};if(!GetSystemDirectoryW(system,32768))return false;PipeProcess extractor;
  if(!extractor.start((std::filesystem::path(system)/L"tar.exe").wstring(),{L"-xf",archive.wstring(),L"-C",(stage/L"python").wstring()},stage.wstring())||!extractor.finish(30000,[&]{return cancel_.load();})){message("Could not extract the optional runtime");return false;}
  std::filesystem::remove(archive);
  if(!write_text(stage/L"python/python312._pth","python312.zip\n.\n../packages\n")||!write_text(stage/L"piper_worker.py",worker_source)||!write_text(stage/L"piper-pins.json",dependency_pins)||!verify_runtime(stage,true)){message("Bootstrap runtime failed verification");return false;}
  if(!child_task(stage,L"install"))return false;
  if(!verify_runtime(stage)){message("Optional runtime installation failed verification");return false;}
  std::filesystem::rename(stage,runtime_);{std::lock_guard<std::mutex> lock(mutex_);status_.ready=true;++status_.revision;}return true;
 }
 void scan_models(){
  std::vector<Model> values;std::error_code ec;
  if(std::filesystem::exists(models_,ec))for(const auto& entry:std::filesystem::directory_iterator(models_)){
   auto id=entry.path().filename().string();if(!identifier(id)||!no_reparse(entry.path())||!entry.is_directory())continue;auto meta=entry.path()/L"voice.json";if(!std::filesystem::is_regular_file(meta)||std::filesystem::file_size(meta)>8192)continue;
   try{std::ifstream in(meta);Json j;in>>j;auto label=j.at("label").get<std::string>();int speakers=j.at("speakers").get<int>();if(j.at("id")!=id||label.empty()||label.size()>128||speakers<1||speakers>2048)continue;values.push_back({id,label,speakers});if(values.size()==16)break;}catch(...){}
  }
  std::sort(values.begin(),values.end(),[](const Model& a,const Model& b){return a.id<b.id;});std::lock_guard<std::mutex> lock(mutex_);status_.models=std::move(values);++status_.revision;
 }
 public:
 void configure(const std::filesystem::path& state){std::lock_guard<std::mutex> lock(mutex_);home_=state/L"tts/piper";runtime_=home_/wide(std::string("runtime-1.8.0-")+std::string(runtime_manifest_hash,12));models_=home_/L"models";status_.ready=file_hash(runtime_/L"ready.json")==runtime_manifest_hash;}
 void refresh(){scan_models();}
 AssetStatus snapshot()const{std::lock_guard<std::mutex> lock(mutex_);return status_;}
 std::filesystem::path runtime()const{return runtime_;}std::filesystem::path models()const{return models_;}
 bool start(int action,const std::filesystem::path& source={}){
  {std::lock_guard<std::mutex> lock(mutex_);if(status_.busy)return false;status_.busy=true;status_.done=status_.total=0;++status_.revision;}
  if(worker_.joinable())worker_.join();
  cancel_=false;
  worker_=std::thread([this,action,source]{try{
   if(home_.empty()||!no_reparse(home_))throw std::runtime_error("Invalid speech storage directory");
   std::filesystem::create_directories(home_);
   if(ensure_runtime()){
    bool ok=true;if(action==1){message("Downloading Piper Norman");if(!std::filesystem::exists(models_/L"norman"))ok=child_task(runtime_,L"norman");}
    else if(action==2){message("Checking and importing the Piper model");ok=child_task(runtime_,L"import",source);}
    if(ok){scan_models();message(action==1?"Norman is ready; select Use Norman to switch":action==2?"Model imported; select it to use it":"Local speech engine ready");}
   }
  }catch(const std::exception& e){message(e.what());}if(cancel_)message("Cancelled; Windows speech is unchanged");{std::lock_guard<std::mutex> lock(mutex_);status_.busy=false;++status_.revision;}});return true;
 }
 void cancel(){cancel_=true;}
 void stop(){cancel_=true;if(worker_.joinable())worker_.join();}
};
inline Assets& assets(){static auto* instance=new Assets;return *instance;}
}
