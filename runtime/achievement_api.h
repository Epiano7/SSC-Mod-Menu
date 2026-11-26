#pragma once
#include "achievement_core.h"
namespace ssc_achievements {
struct ApiTotals {uint64_t kills=0,matches=0;};
inline bool natural(const Json& v,uint64_t& out,uint64_t limit=1000000000){
 if(!v.is_number_integer())return false;
 if(v.is_number_unsigned()){out=v.get<uint64_t>();return out<=limit;}
 auto n=v.get<int64_t>();if(n<0||uint64_t(n)>limit)return false;out=uint64_t(n);return true;
}
inline bool api_totals(const Json& data,int userid,ApiTotals& out){
 try{
  uint64_t owner=0;if(userid<=0||!data.is_object()||!data.at("player").is_object()||!natural(data.at("player").at("userid"),owner,INT_MAX)||owner!=uint64_t(userid)||data.at("player").value("bot",true))return false;
  const auto& filters=data.at("filters");if(!filters.is_object())return false;
  // A period, ranked-only filter or competitive cohort is not a lifetime baseline.
  for(const auto* key:{"gameVersion","minRating","maxRating","since","until","groupId","region","mode","classId","ranked","topPlayers"})
   if(!filters.contains(key)||!filters.at(key).is_null())return false;
  if(!filters.contains("competitive")||filters.at("competitive")!=false)return false;
  const auto& s=data.at("summary");return natural(s.at("kills"),out.kills)&&natural(s.at("matches"),out.matches);
 }catch(...){return false;}
}
struct ImportState {
 bool done=false;uint64_t at=0,kills=0;std::vector<std::string> receipts;
 Json json()const{return {{"done",done},{"at",at},{"kills",kills},{"receipts",receipts}};}
 static bool parse(const Json& j,ImportState& out){
  try{
   ImportState next;next.done=j.at("done").get<bool>();
   if(!natural(j.at("at"),next.at,4102444800ULL)||!natural(j.at("kills"),next.kills))return false;
   if(next.done&&!next.at)return false;
   if(!j.at("receipts").is_array()||j.at("receipts").size()>128)return false;
   for(const auto& v:j.at("receipts")){auto id=v.get<std::string>();if(id.empty()||id.size()>96||id.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-")!=std::string::npos)return false;next.receipts.push_back(id);}
   out=next;return true;
  }catch(...){return false;}
 }
 bool import(const ApiTotals& totals,uint64_t utc){if(done||!utc||utc>4102444800ULL)return false;done=true;at=utc;kills=totals.kills;return true;}
 bool add_match(const std::string& id,uint64_t started,uint64_t count){
  if((done&&started<at)||id.empty()||id.size()>96||std::find(receipts.begin(),receipts.end(),id)!=receipts.end()||count>100000||kills>1000000000-count)return false;
  kills+=count;receipts.push_back(id);if(receipts.size()>128)receipts.erase(receipts.begin());return true;
 }
};
struct MatchEvidence {std::string id;uint64_t started=0,ended=0,kills=0,deaths=0;int placement=0;std::array<bool,3> wins{};};
inline uint64_t iso_seconds(std::string s){
 if(s.size()>=6&&s.compare(s.size()-6,6,"+00:00")==0)s.replace(s.size()-6,6,"Z");
 // UTC ISO timestamp only, strict Gregorian validation; no locale/timezone dependency.
 if(s.size()<20||s.size()>30||s[4]!='-'||s[7]!='-'||s[10]!='T'||s[13]!=':'||s[16]!=':'||s.back()!='Z')return 0;
 auto n=[&](size_t p,size_t len)->int{int v=0;for(size_t i=p;i<p+len;++i){if(s[i]<'0'||s[i]>'9')return -1;v=v*10+s[i]-'0';}return v;};
 int y=n(0,4),m=n(5,2),d=n(8,2),h=n(11,2),min=n(14,2),sec=n(17,2);
 if(y<2000||y>2099||m<1||m>12||h<0||h>23||min<0||min>59||sec<0||sec>59)return 0;
 const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};int last=days[m-1]+(m==2&&y%4==0?1:0);if(d<1||d>last)return 0;
 if(s.size()>20){if(s[19]!='.'||s.size()<22||s.size()>27)return 0;for(size_t i=20;i+1<s.size();++i)if(s[i]<'0'||s[i]>'9')return 0;}
 int yy=y-(m<=2);int era=yy/400;unsigned yoe=unsigned(yy-era*400);unsigned doy=(153*unsigned(m+(m>2?-3:9))+2)/5+unsigned(d)-1;unsigned doe=yoe*365+yoe/4-yoe/100+doy;int64_t day=int64_t(era)*146097+doe-719468;
 return uint64_t(day)*86400+uint64_t(h)*3600+uint64_t(min)*60+unsigned(sec);
}
inline bool api_match(const Json& data,int userid,uint64_t start,uint64_t finish,MatchEvidence& out){
 ApiTotals check;if(!api_totals(data,userid,check)||!data.contains("recentMatches")||!data["recentMatches"].is_array()||data["recentMatches"].size()>30)return false;
 unsigned found=0;
 try{
  for(const auto& row:data["recentMatches"]){
   const auto& m=row.at("match");const auto& p=row.at("player");
   uint64_t owner=0;if(!natural(p.at("userid"),owner,INT_MAX)||owner!=uint64_t(userid))continue;
   uint64_t a=iso_seconds(m.at("startedAt").get<std::string>()),b=iso_seconds(m.at("endedAt").get<std::string>());
   if(!a||!b||b<a||!start||finish<start||a+120<start||a>start+20||b+30<finish||b>finish+120)continue;
   MatchEvidence next;next.id=m.at("id").get<std::string>();if(next.id.size()!=36||next.id.find_first_not_of("0123456789abcdef-")!=std::string::npos||next.id[8]!='-'||next.id[13]!='-'||next.id[18]!='-'||next.id[23]!='-'||std::count(next.id.begin(),next.id.end(),'-')!=4)continue;next.started=a;next.ended=b;
   if(!natural(p.at("kills"),next.kills,100000)||!natural(p.at("deaths"),next.deaths,100000))continue;
   uint64_t placement=0;if(!natural(p.at("placement"),placement,2048)||!placement)continue;next.placement=int(placement);
   const auto& wins=p.at("roundWins");if(!wins.is_array()||wins.size()!=3)continue;
   bool good=true;for(size_t i=0;i<3;++i){uint64_t n=0;if(!natural(wins[i],n,1)){good=false;break;}next.wins[i]=n==1;}
   if(!good)continue;
   out=next;++found;
  }
 }catch(...){return false;}
 return found==1;
}
}
