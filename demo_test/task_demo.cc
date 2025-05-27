#include<memory>
#include<functional>
#include<iostream>  
#include<vector>
using task = std::function<void()>;
std::vector<task>vec;
void run(){
    for(auto&&f:vec){
        f();
    }
}
int main(){
    auto lamba1 = [](){
        std::cout<<"lamdba  1"<<std::endl;
    };
    auto lamba2 = [](){
        std::cout<<"lamdba  2"<<std::endl;
    };
    vec.push_back(std::move(lamba1));
    vec.push_back(std::move(lamba2));
    run();
}
/*
正常输出：
lambda 1
lambda 2
*/