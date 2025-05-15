#include <benchmark/benchmark.h>

// 非虚函数测试
class NormalBase {
protected:
    int64_t cnt_{0};
};

class NormalDerived1 : public NormalBase {
public:
    __attribute__((noinline)) int64_t foo(int64_t n) { 
        cnt_ += n; 
        return cnt_; 
    }
};

// 虚函数测试
class VirtualBase {
public:
    virtual ~VirtualBase() = default;
    virtual int64_t foo(int64_t n) = 0;
protected:
    int64_t cnt_{0};
};

class VirtualDerived1 : public VirtualBase {
public:
    __attribute__((noinline)) int64_t foo(int64_t n) override final { 
        cnt_ += n;
        
        return cnt_; 
    }
};

// CRTP模式测试
template<typename T>
class CRTPBase {
public:
    __attribute__((noinline)) int64_t foo(int64_t n) {
        return static_cast<T*>(this)->internal_foo(n);
    }
protected:
    int64_t cnt_{0};
};

class CRTPDerived1 : public CRTPBase<CRTPDerived1> {
public:
    __attribute__((noinline)) int64_t internal_foo(int64_t n) {
        cnt_ += n;
        return cnt_;
    }
};

// 非虚函数测试用例
static void BM_NormalCall(benchmark::State& state) {
    NormalDerived1 obj;
    for (auto _ : state) {
        benchmark::DoNotOptimize(obj.foo(1));
    }
}
BENCHMARK(BM_NormalCall);

// 虚函数测试用例
static void BM_VirtualCall(benchmark::State& state) {
    VirtualDerived1 obj;
    VirtualBase* ptr = &obj;
    for (auto _ : state) {
        benchmark::DoNotOptimize(ptr->foo(1));
    }
}
BENCHMARK(BM_VirtualCall);

// CRTP测试用例
static void BM_CRTPCall(benchmark::State& state) {
    CRTPDerived1 obj;
    CRTPBase<CRTPDerived1>* ptr = &obj;
    for (auto _ : state) {
        benchmark::DoNotOptimize(ptr->foo(1));
    }
}
BENCHMARK(BM_CRTPCall);

BENCHMARK_MAIN();