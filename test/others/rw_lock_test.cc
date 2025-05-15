#include <benchmark/benchmark.h>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

int shared_data = 0;
std::mutex mtx;
std::shared_mutex rw_mtx;

// 普通互斥锁的读操作（单线程基准测试）
static void BM_mutex_read(benchmark::State& state) {
    for (auto _ : state) {
        std::lock_guard<std::mutex> lock(mtx);
        benchmark::DoNotOptimize(shared_data);
    }
}
BENCHMARK(BM_mutex_read)->Threads(1)->Threads(4)->Threads(8);

// 读写锁的读操作（单线程基准测试）
static void BM_rw_mutex_read(benchmark::State& state) {
    for (auto _ : state) {
        std::shared_lock<std::shared_mutex> lock(rw_mtx);
        benchmark::DoNotOptimize(shared_data);
    }
}
BENCHMARK(BM_rw_mutex_read)->Threads(1)->Threads(4)->Threads(8);

int main(int argc, char** argv) {
    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();
    return 0;
}
/* g++ -std=c++17 -O3  rw_lock_test.cc -o rw_lock_test -lbenchmark -lpthread*/