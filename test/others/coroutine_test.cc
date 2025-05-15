#include <benchmark/benchmark.h>
#include <coroutine>
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <array>
#include <algorithm>
#include <cmath>
#include <functional>

// Minimal coroutine implementation to measure pure context switching overhead
struct minimal_task {
    // The promise type with minimal overhead
    struct promise_type {
        minimal_task get_return_object() { 
            return minimal_task(std::coroutine_handle<promise_type>::from_promise(*this)); 
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    // Constructor, destructor, and basic operations
    explicit minimal_task(std::coroutine_handle<promise_type> h) : coro_(h) {}
    minimal_task(const minimal_task&) = delete;
    minimal_task& operator=(const minimal_task&) = delete;
    minimal_task(minimal_task&& other) noexcept : coro_(other.coro_) {
        other.coro_ = {};
    }
    minimal_task& operator=(minimal_task&& other) noexcept {
        if (this != &other) {
            if (coro_)
                coro_.destroy();
            coro_ = other.coro_;
            other.coro_ = {};
        }
        return *this;
    }
    
    ~minimal_task() {
        if (coro_)
            coro_.destroy();
    }

    // Core operations
    bool resume() {
        if (!coro_.done())
            coro_.resume();
        return !coro_.done();
    }

private:
    std::coroutine_handle<promise_type> coro_;
};

// Helper function to create a coroutine that just suspends and resumes
minimal_task ping_pong(uint64_t iterations) {
    for (uint64_t i = 0; i < iterations; ++i) {
        co_await std::suspend_always{};
    }
}

// Benchmark for measuring pure coroutine context switching overhead
static void BM_CoroutineContextSwitch(benchmark::State& state) {
    // The number of context switches to measure
    const uint64_t iterations = state.range(0);
    
    // Pin thread to CPU 0 to avoid CPU migrations
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    // Standard deviations calculation variables
    std::vector<double> samples;
    
    for (auto _ : state) {
        // Start timing
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create and resume the coroutine multiple times
        auto task = ping_pong(iterations);
        
        // Resume the coroutine for the specified number of iterations
        for (uint64_t i = 0; i < iterations; ++i) {
            task.resume();
        }
        
        // Stop timing
        auto end = std::chrono::high_resolution_clock::now();
        
        // Calculate elapsed time in nanoseconds
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        // Calculate time per context switch
        double ns_per_switch = static_cast<double>(elapsed) / iterations;
        
        // Record the sample
        samples.push_back(ns_per_switch);
        
        // Report the time per context switch
        state.SetIterationTime(ns_per_switch / 1e9);  // Convert to seconds for benchmark
    }
    
    // Calculate statistics
    if (!samples.empty()) {
        // Calculate mean
        double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
        
        // Calculate standard deviation
        double sq_sum = std::inner_product(samples.begin(), samples.end(), samples.begin(), 0.0,
                                          std::plus<>(), [mean](double x, double y) { 
                                              return (x - mean) * (y - mean); 
                                          });
        double std_dev = std::sqrt(sq_sum / samples.size());
        
        // Find min and max values
        auto [min_it, max_it] = std::minmax_element(samples.begin(), samples.end());
        
        // Report statistics
        state.counters["mean_ns"] = mean;
        state.counters["stddev_ns"] = std_dev;
        state.counters["min_ns"] = *min_it;
        state.counters["max_ns"] = *max_it;
        state.counters["cv_percent"] = 100.0 * std_dev / mean; // Coefficient of variation
    }
    
    // Set the number of iterations performed
    state.SetItemsProcessed(state.iterations() * iterations);
}

// More sophisticated benchmark for comparing coroutine vs. function call overhead
struct empty_awaitable {
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const noexcept {}
};

minimal_task empty_coroutine() {
    co_await empty_awaitable{};
}

// Function to be called in the non-coroutine benchmark
void empty_function() {
    // Do nothing - just measure function call overhead
}

// Function with a lambda capture to simulate suspend point
void function_with_callback(std::function<void()> callback) {
    callback();
}

// Benchmark coroutine creation and single suspend/resume
static void BM_CoroutineVsFunction(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        switch (state.range(0)) {
            case 0: {
                // Measure empty function call
                state.ResumeTiming();
                empty_function();
                break;
            }
            case 1: {
                // Measure function with callback
                auto callback = [](){};
                state.ResumeTiming();
                function_with_callback(callback);
                break;
            }
            case 2: {
                // Measure coroutine creation and single suspend/resume
                state.ResumeTiming();
                auto task = empty_coroutine();
                task.resume();
                break;
            }
        }
    }
}

// Define benchmarks with various parameters
BENCHMARK(BM_CoroutineContextSwitch)
    ->RangeMultiplier(10)
    ->Range(10, 10)  // From 10 to 100,000 iterations
    ->UseManualTime()
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_CoroutineVsFunction)
    ->Arg(0)  // Regular function
    ->Arg(1)  // Function with callback
    ->Arg(2)  // Coroutine
    ->Unit(benchmark::kNanosecond);

// Ping-pong benchmark between two coroutines
struct PingPongState {
    std::coroutine_handle<> ping_handle = nullptr;
    std::coroutine_handle<> pong_handle = nullptr;
    uint64_t counter = 0;
    uint64_t limit = 0;
    bool done = false;
};

struct PingPongAwaitable {
    PingPongState* state;
    bool is_ping;

    bool await_ready() const noexcept { 
        return state->done; 
    }
    
    void await_suspend(std::coroutine_handle<> h) noexcept {
        if (is_ping) {
            state->ping_handle = h;
            if (state->pong_handle) {
                state->pong_handle.resume();
            }
        } else {
            state->pong_handle = h;
            state->counter++;
            if (state->counter >= state->limit) {
                state->done = true;
                return;
            }
            if (state->ping_handle) {
                state->ping_handle.resume();
            }
        }
    }
    
    void await_resume() const noexcept {}
};

minimal_task ping_coroutine(PingPongState* state) {
    while (!state->done) {
        co_await PingPongAwaitable{state, true};
    }
}

minimal_task pong_coroutine(PingPongState* state) {
    while (!state->done) {
        co_await PingPongAwaitable{state, false};
    }
}

// Benchmark for measuring ping-pong context switching between two coroutines
static void BM_CoroutinePingPong(benchmark::State& state) {
    const uint64_t iterations = state.range(0);
    
    // Pin thread to CPU 0 to avoid CPU migrations
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    
    std::vector<double> samples;
    
    for (auto _ : state) {
        state.PauseTiming();
        
        // Create shared state
        PingPongState ping_pong_state;
        ping_pong_state.limit = iterations;
        
        // Create tasks with shared state
        auto ping_task = ping_coroutine(&ping_pong_state);
        auto pong_task = pong_coroutine(&ping_pong_state);
        
        // Start both coroutines
        ping_task.resume();
        pong_task.resume();
        
        state.ResumeTiming();
        
        // Start timing
        auto start = std::chrono::high_resolution_clock::now();
        
        // Start the ping-pong by resuming ping handle
        if (ping_pong_state.ping_handle) {
            ping_pong_state.ping_handle.resume();
        }
        
        // Stop timing when done
        while (!ping_pong_state.done) {
            // Continue running until the benchmark is complete
            // This is just a safety loop in case the coroutines don't automatically
            // stop after reaching the iteration limit
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        
        // Calculate elapsed time in nanoseconds
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        // Calculate time per context switch
        double ns_per_switch = static_cast<double>(elapsed) / (iterations * 2); // Each ping-pong is 2 switches
        
        // Record the sample
        samples.push_back(ns_per_switch);
        
        // Report the time per context switch
        state.SetIterationTime(ns_per_switch / 1e9);  // Convert to seconds for benchmark
    }
    
    // Calculate statistics
    if (!samples.empty()) {
        double mean = std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size();
        
        double sq_sum = std::inner_product(samples.begin(), samples.end(), samples.begin(), 0.0,
                                          std::plus<>(), [mean](double x, double y) { 
                                              return (x - mean) * (y - mean); 
                                          });
        double std_dev = std::sqrt(sq_sum / samples.size());
        
        auto [min_it, max_it] = std::minmax_element(samples.begin(), samples.end());
        
        state.counters["mean_ns"] = mean;
        state.counters["stddev_ns"] = std_dev;
        state.counters["min_ns"] = *min_it;
        state.counters["max_ns"] = *max_it;
        state.counters["cv_percent"] = 100.0 * std_dev / mean;
    }
    
    state.SetItemsProcessed(state.iterations() * iterations * 2); // Each iteration does iterations*2 switches
}

BENCHMARK(BM_CoroutinePingPong)
    ->RangeMultiplier(10)
    ->Range(10, 10)
    ->UseManualTime()
    ->Unit(benchmark::kNanosecond);

// Run the benchmark
BENCHMARK_MAIN();
