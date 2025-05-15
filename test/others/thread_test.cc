#include <benchmark/benchmark.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <sched.h>

// Futex系统调用封装
int futex_wait(std::atomic<int>* futex_addr, int expected) {
    return syscall(SYS_futex, futex_addr, FUTEX_WAIT, expected, nullptr, nullptr, 0);
}

int futex_wake(std::atomic<int>* futex_addr, int n) {
    return syscall(SYS_futex, futex_addr, FUTEX_WAKE, n, nullptr, nullptr, 0);
}
// 绑定线程到指定CPU核心
void pin_thread_to_cpu(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}

static void BM_ThreadSwitch(benchmark::State& state) {
    constexpr int kIterations = 100000; // 每次测试10万次交换
    static std::atomic<int> flag1, flag2;
    for (auto _ : state) {
        flag1.store(1, std::memory_order_relaxed);
        flag2.store(0, std::memory_order_relaxed);
        auto thread_a = [] {
            pin_thread_to_cpu(0); // 绑定到CPU0
            for (int i = 0; i < kIterations; ++i) {
                // 等待flag1变为1
                while (true) {
                    int expected = 1;
                    if (flag1.compare_exchange_strong(expected, 0)) break;
                    futex_wait(&flag1, expected);
                }
                // 触发线程B
                flag2.store(1, std::memory_order_relaxed);
                futex_wake(&flag2, 1);
            }
        };

        auto thread_b = [] {
            pin_thread_to_cpu(0); // 绑定到CPU0
            for (int i = 0; i < kIterations; ++i) {
                // 等待flag2变为1
                while (true) {
                    int expected = 1;
                    if (flag2.compare_exchange_strong(expected, 0)) break;
                    futex_wait(&flag2, expected);
                }
                // 触发线程A
                flag1.store(1, std::memory_order_relaxed);
                futex_wake(&flag1, 1);
            }
        };

        auto start = std::chrono::high_resolution_clock::now();
        
        std::thread t1(thread_a);
        std::thread t2(thread_b);
        t1.join();
        t2.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        // 每次循环产生2次上下文切换
        state.counters["ctx_switch_ns"] = static_cast<double>(elapsed) / (2 * kIterations);
    }
}

BENCHMARK(BM_ThreadSwitch)->Unit(benchmark::kNanosecond)->UseRealTime();

BENCHMARK_MAIN();