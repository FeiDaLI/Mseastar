// #include<memory>
// #include<iostream>
// #include<functional>
// #include<unistd.h>
// struct state{
//     void print(){
//         std::cout<<"print"<<std::endl;
//     }
//     state(){std::cout<<"construcor"<<std::endl;}
//     ~state(){
//         std::cout<<"destructor"<<std::endl;
//     }
// };

// std::function<void()> f1;

// void f(){
//     auto p =  std::make_unique<state>();
//     auto lambda = [p = p.get()](){
//         sleep(1);
//         p->print();
//     };
//     f1 = lambda;
//     std::cout<<"执行return"<<std::endl;
//     return;
// }


// int main(){
//     f();
//     f1();
// }


//上面的代码是不安全的

// #include <memory>
// #include <iostream>
// #include <unistd.h>

// struct state {
//     void print() { std::cout << "print" << std::endl; }
//     state() { std::cout << "construcor" << std::endl; }
//     ~state() { std::cout << "destructor" << std::endl; }
// };

// auto f1 = []() {}; // 初始化为空 lambda，避免未初始化

// void f() {
//     auto p = std::make_unique<state>();
//     auto lambda = [p = std::move(p)]() {
//         sleep(1);
//         p->print();
//     };
//     f1 = std::move(lambda); // 移动 lambda 
//     /*
//     This compilation error occurs because lambda expressions are not assignable by default in C++. Each lambda has its own unique type, 
//     and you cannot assign one lambda to another lambda variable directly.
//     */
//     std::cout << "执行return" << std::endl;
// }

// int main() {
//     f();
//     f1();
// }

#include <memory>
#include <iostream>
#include <unistd.h>
#include <functional>

struct state {
    void print() { std::cout << "print" << std::endl; }
    state() { std::cout << "constructor" << std::endl; }
    ~state() { std::cout << "destructor" << std::endl; }
};

// Solution 1: Use std::function to store lambdas
std::function<void()> f1;

void f() {
    auto p = std::make_unique<state>();
    auto lambda = [p = std::move(p)]() {
        sleep(1);
        p->print();
    };
    f1 = std::move(lambda); // Now this works
    std::cout << "执行return" << std::endl;
}

int main() {
    f();
    if (f1) { // Check if f1 is not empty
        f1();
    }
    return 0;
}
