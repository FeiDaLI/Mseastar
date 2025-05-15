//#include "../../include/task/reactor_demo.hh"
#include "../../include/timer/timer_all.hh"
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

int main() {
    reactor& reac = engine();
    bool timer_fired = false;
    // 创建定时器并设置回调函数
    timer<steady_clock_type> t([&timer_fired]{
        std::cout << "Timer expired! Callback executed!" << std::endl;
        timer_fired = true;
    });
    //  新开一个线程
    auto runFunc = [&](){ reac.run();};
    auto s = std::thread(runFunc);
    s.detach();
    // 设置定时器在2秒后触发
    std::cout << "Setting timer for 2 seconds from now..." << std::endl;
    t.arm(std::chrono::seconds(2));
    // 等待定时器触发
    std::cout << "Waiting for timer to expire..." << std::endl;
    while (!timer_fired) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Test completed." << std::endl;
    return 0;

    // timer_set<timer<steady_clock_type>>set;
    // std::cout<<set.max_timestamp<<" max_timestamp"<<std::endl;
    // std::cout<<set.timestamp_bits<<" timestamp_bits"<<std::endl;
    // timer<steady_clock_type>t;
    // t.arm(std::chrono::seconds(2));
    // set.insert(t);
    // std::cout<<"过期时间"<<set.get_timestamp(t);
    
}

/*

9223372036854775807 max_timestamp
63 timestamp_bits
*/