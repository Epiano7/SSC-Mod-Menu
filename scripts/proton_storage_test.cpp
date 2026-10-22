#include "../runtime/private_auth.h"
#include <iostream>
int main(){
 if(sodium_init()<0)return 2;
 std::array<unsigned char,32> first{},second{},other{};
 auto a=ssc_auth::license::proton_seed(76561198025298853ULL,true,first);
 auto b=ssc_auth::license::proton_seed(76561198025298853ULL,false,second);
 auto c=ssc_auth::license::proton_seed(76561198029717676ULL,true,other);
 bool ok=a==1&&b==1&&c==1&&first==second&&first!=other;
 sodium_memzero(first.data(),32);sodium_memzero(second.data(),32);sodium_memzero(other.data(),32);
 std::cout<<(ok?"PASS Proton/native keyring IPC and persistence":"FAIL Proton/native keyring IPC")<<std::endl;return ok?0:1;
}
