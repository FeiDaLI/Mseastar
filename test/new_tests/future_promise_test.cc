#include"../include/future/future_all10.hh"
#include<string>
#include<chrono>
#include<iostream>
void test1(){
        promise<int> p;
        future<int> f = p.get_future();
        if(f.available()){
            std::cout << "f is available" << std::endl;
        }else{
            std::cout << "f is not available" << std::endl;
        }
        p.set_value(std::make_tuple(42));
        std::cout<<f.available()<<std::endl;//1
        std::cout<<f.failed()<<std::endl;   //0
        auto result = std::get<0>(f.get());//42
        std::cout << "result is " << result << std::endl;
}


void test2(){
    promise<int, std::string> p;
    future<int, std::string> f = p.get_future();
    std::cout<<f.available()<<std::endl;//0
    p.set_value(std::make_tuple(42, "hello"));
    std::cout<<f.available()<<std::endl;//1
    std::cout<<f.failed()<<std::endl;    //0
    auto [i, s] = f.get();//hello
    std::cout<<i<<std::endl;//42
    std::cout<<s<<std::endl;//hello
}
void test3(){
    auto &eng = engine();
    promise<int> p;
    future<int> f = p.get_future();
    auto f2 = f.then([](std::tuple<int> val) {
        return std::get<0>(val) * 2;
    });
    p.set_value(std::make_tuple(21));
    auto runFunc = [&](){ eng.run();};
    auto s = std::thread(runFunc);
    s.detach();
    // // //这行代码出错
    auto result = std::get<0>(f2.get());
}


int main(){
    // Initialize the thread context before using futures
    thread_impl::init();
    test1();
    test2();
    test3();
    
}
