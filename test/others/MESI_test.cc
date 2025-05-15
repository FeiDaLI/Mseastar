#include <benchmark/benchmark.h>
#include <thread>
#include <vector>
#include <atomic>

// 全局共享数据
std::vector<int> shared_data(1024 * 1024, 0); // 4MB 数据
std::atomic<bool> start_flag(false);
// 单线程访问
static void SingleThreadAccess(benchmark::State& state) {
    for (auto _ : state) {
        for (size_t i = 0; i < shared_data.size(); ++i) {
            shared_data[i] += 1;
        }
    }
}


std::vector<std::atomic<int>> shared_data_atomic(1024 * 1024);

static void MultiThreadSameData(benchmark::State& state) {
    int num_threads = state.range(0);
    std::vector<std::thread> threads;

    for (auto _ : state) {
        start_flag.store(false);
        threads.clear();

        // 启动线程
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&]() {
                while (!start_flag.load()) {} // 等待所有线程启动
                for (size_t j = 0; j < shared_data_atomic.size(); ++j) {
                    shared_data_atomic[j].fetch_add(1, std::memory_order_relaxed);
                }
            });
        }

        // 开始测试
        start_flag.store(true);
        for (auto& t : threads) {
            t.join();
        }
    }
}

BENCHMARK(MultiThreadSameData)->Arg(2)->Arg(4)->Arg(8);
static void MultiThreadDifferentData(benchmark::State& state) {
    int num_threads = state.range(0);
    std::vector<std::thread> threads;
    std::vector<std::vector<int>> local_data(num_threads, std::vector<int>(1024 * 1024 / num_threads, 0));
    for (auto _ : state) {
        start_flag.store(false);
        threads.clear();
        // 启动线程
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&, i]() {
                while (!start_flag.load()) {} // 等待所有线程启动
                for (size_t j = 0; j < local_data[i].size(); ++j) {
                    local_data[i][j] += 1;
                }
            });
        }
        // 开始测试
        start_flag.store(true);
        for (auto& t : threads) {
            t.join();
        }
    }
}
BENCHMARK(MultiThreadDifferentData)->Arg(2)->Arg(4)->Arg(8);

// 主函数
BENCHMARK_MAIN();