#pragma once
// Native Linux launcher owns keyring storage; Proton only receives the key in memory.
namespace ssc_auth { namespace license {
inline bool proton(){return GetProcAddress(GetModuleHandleW(L"ntdll.dll"),"wine_get_version")!=nullptr;}
inline int proton_seed(uint64_t account,bool create,std::array<unsigned char,32>& seed){
 wchar_t folder[32768]{};auto n=GetEnvironmentVariableW(L"SSC_PRIVATE_RELAY",folder,32768);
 if(!n||n>=32768)return -1;
 struct SecretText{std::string text;~SecretText(){ssc_beta::wipe(text);}} raw,body;
 std::ifstream input(std::filesystem::path(folder)/L"endpoint.json",std::ios::binary);
 char buf[1025]{};input.read(buf,1025);auto count=input.gcount();if(count<=0||count>1024)return -1;
 raw.text.assign(buf,size_t(count));SecureZeroMemory(buf,sizeof buf);
 try{
  auto endpoint=parse(raw.text);auto port=integer(endpoint,"port");require(port>0&&port<=65535);
  auto& token=endpoint.at("token").get_ref<std::string&>();require(token.size()==64&&token.find_first_not_of("0123456789abcdef")==std::string::npos);
  struct Handle{HINTERNET h;~Handle(){if(h)WinHttpCloseHandle(h);}};
  Handle session{WinHttpOpen(L"SSCPrivateLinux/1",WINHTTP_ACCESS_TYPE_NO_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0)};require(session.h);
  require(WinHttpSetTimeouts(session.h,1000,1000,3000,30000));
  Handle conn{WinHttpConnect(session.h,L"127.0.0.1",INTERNET_PORT(port),0)};require(conn.h);
  Handle req{WinHttpOpenRequest(conn.h,L"POST",L"/device",nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,0)};require(req.h);
  DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER,disabled=WINHTTP_DISABLE_COOKIES|WINHTTP_DISABLE_AUTHENTICATION;
  require(WinHttpSetOption(req.h,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof redirects));
  require(WinHttpSetOption(req.h,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof disabled));
  std::wstring headers=L"Content-Type: application/json\r\nAuthorization: Bearer ";headers.append(token.begin(),token.end());headers+=L"\r\n";ssc_beta::wipe(token);
  unsigned char random[16];randombytes_buf(random,sizeof random);char nonce[33];sodium_bin2hex(nonce,sizeof nonce,random,sizeof random);
  auto request=Json{{"account",std::to_string(account)},{"create",create},{"nonce",nonce}}.dump();
  bool sent=WinHttpSendRequest(req.h,headers.c_str(),DWORD(headers.size()),request.data(),DWORD(request.size()),DWORD(request.size()),0)!=0;
  SecureZeroMemory(headers.data(),headers.size()*sizeof(wchar_t));require(sent&&WinHttpReceiveResponse(req.h,nullptr));
  DWORD status=0,size=sizeof status;require(WinHttpQueryHeaders(req.h,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX));require(status==200);
  DWORD got=0;while(WinHttpReadData(req.h,buf,512,&got)&&got){body.text.append(buf,got);SecureZeroMemory(buf,sizeof buf);require(body.text.size()<=1024);}
  auto reply=parse(body.text);require(string(reply,"nonce")==nonce);
  auto state=string(reply,"status");if(state=="missing")return create?-1:0;
  require(state=="ok");auto& hex=reply.at("seed").get_ref<std::string&>();
  bool ok=hex.size()==64&&hex.find_first_not_of("0123456789abcdef")==std::string::npos;
  size_t written=0;if(ok)ok=sodium_hex2bin(seed.data(),seed.size(),hex.data(),hex.size(),nullptr,&written,nullptr)==0&&written==seed.size();
  ssc_beta::wipe(hex);require(ok);ssc_auth_privacy::secrets_used=true;return 1;
 }catch(...){SecureZeroMemory(buf,sizeof buf);sodium_memzero(seed.data(),seed.size());return -1;}
}
}}
