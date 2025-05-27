#include<functional>    //std::apply
#include<tuple>
#include<iostream>

int main(){
    auto lambda1 = [](int a){
        std::cout<<"a="<<a<<std::endl;
        return a+2;
    };
    std::cout<<std::apply(lambda1,std::make_tuple(1))<<std::endl;
}
