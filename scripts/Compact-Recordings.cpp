#include "../runtime/statistics_reader.h"
#include <iostream>
int main(int argc,char** argv){
 try{if(argc!=2)return 2;std::filesystem::path folder=argv[1];
 std::map<std::string,std::string> before,after;
 ssc_record_archive::visit(folder,[&](const auto& p,const auto& raw){before[p.string()]=ssc_stats::digest(raw);});
 ssc_record_archive::compact(folder);
 ssc_record_archive::visit(folder,[&](const auto& p,const auto& raw){after[p.string()]=ssc_stats::digest(raw);});
 if(before!=after)throw std::runtime_error("Post-migration content mismatch");
 std::cout<<"Verified "<<after.size()<<" recording entries; archive bytes "<<std::filesystem::file_size(folder/L"recordings.zip")<<"\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}return 0;
}
