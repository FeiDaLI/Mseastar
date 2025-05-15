#include <benchmark/benchmark.h>
#include <atomic>
#include <thread>
#include <mutex>

std::mutex mutex_;
// 使用普通变量进行操作
void increment_non_atomic(int& value) {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++value;
    }
}

// 使用原子变量进行操作
void increment_atomic(std::atomic<int>& value) {
    for (int i = 0; i < 100000; ++i) {
        value.fetch_add(1, std::memory_order_relaxed);
    }
}

// 定义基准测试
static void BM_IncrementNonAtomic(benchmark::State& state) {
    int value = 0;
    for (auto _ : state) {
        increment_non_atomic(value);
    }
    benchmark::DoNotOptimize(value);
}
BENCHMARK(BM_IncrementNonAtomic);

static void BM_IncrementAtomic(benchmark::State& state) {
    std::atomic<int> value(0);
    for (auto _ : state) {
        increment_atomic(value);
    }
    benchmark::DoNotOptimize(value.load());
}
BENCHMARK(BM_IncrementAtomic);

// 主函数
BENCHMARK_MAIN();