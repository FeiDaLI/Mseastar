#include "../../include/future/future_all3.hh"
#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <cassert>

/*
cd /home/lifd/Mseastar2 && g++ -std=c++17 -I. test/new_tests/thread_test.cc -o thread_test
*/

// Helper function to print with thread ID
void print(const std::string& message) {
    
    std::cout << "Thread " << (thread::running_in_thread() ? "worker" : "main") 
              << ": " << message << std::endl;
}

// Test basic thread creation and execution
void test_basic_thread() {
    thread_impl::init();
    std::cout << "\n=== Testing Basic Thread Creation and Execution ===" << std::endl;
    bool thread_executed = false;
    thread t([&thread_executed]() {
        print("Thread function executed");
        thread_executed = true;
    });
    // Wait for thread to complete
    t.join().get();//t.join返回一个future<>.
    
    assert(thread_executed);
    print("Basic thread test passed");
}

// Test thread yielding
void test_thread_yield() {
    std::cout << "\n=== Testing Thread Yielding ===" << std::endl;
    std::atomic<bool> thread1_yielded(false);
    std::atomic<bool> thread2_executed(false);
    thread t1([&thread1_yielded,&thread2_executed](){
        print("Thread 1 started");
        // Simulate some work
        for (int i = 0;i < 5;i++) {
            if (thread::should_yield()) {
                // 这里一直是true.
                print("Thread 1 yielding");
                thread1_yielded = true; // 正确:operator=的隐式转换(需C++20支持)
                thread::yield();
            }
            // Busy wait to simulate work
            for (volatile int j = 0; j < 1000000; j++) {}
        }
        print("Thread 1 completed");
    });
    thread t2([&thread2_executed]() {
        print("Thread 2 started");
        thread2_executed = true;
        print("Thread 2 completed");
    });
    // Wait for both threads to complete
    t1.join().get();
    t2.join().get();
    assert(thread1_yielded);
    assert(thread2_executed);
    print("Thread yielding test passed") ;
}






int main() {
    std::cout << "Starting thread module tests..." << std::endl;
    auto &reac = engine();
    auto runFunc = [&](){ reac.run();};
    auto s = std::thread(runFunc);

    // Initialize thread implementation
    thread_impl::init();
    
    // Run all tests
    // test_basic_thread();
    test_thread_yield();
    // test_thread_scheduling();
    // test_thread_gate();
    // test_thread_exception();
    // test_thread_futures();
    
    std::cout << "\nAll thread module tests completed successfully!" << std::endl;
    return 0;
}
