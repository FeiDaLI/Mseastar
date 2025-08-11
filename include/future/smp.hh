#pragma once
// 基础标准库
#include <atomic>                    // std::atomic
#include <memory>                    // std::unique_ptr, std::shared_ptr
#include <functional>                // std::function
#include <optional>                  // std::optional (C++17)
#include <type_traits>               // std::is_same, std::result_of_t, std::enable_if
#include <utility>                   // std::move, std::forward
#include <deque>                     // std::deque in _tx
#include <vector>                    // std::vector
#include <array>                     // 如果用数组
#include <cstddef>                   // size_t
#include <cstdint>                   // uint32_t 等
#include <exception>                 // std::exception_ptr
#include <stdexcept>                 // std::runtime_error 等（可选）

// 多线程与同步
#include <thread>                    // std::thread::id
#include <mutex>                     // 可能需要（虽然你用无锁，但某些地方可能隐含使用）
#include <condition_variable>        // 如果用 barrier 或等待

// Boost 库
#include <boost/range/irange.hpp>    // boost::irange (用于 all_cpus())
#include <boost/thread/barrier.hpp>  // boost::barrier
#include <boost/program_options.hpp> // boost::program_options
class reactor;
reactor& engine();
template <typename T, size_t Capacity>
class spsc_queue {
private:
    // 使用缓存行对齐来避免伪共享
    alignas(64) std::atomic<size_t> _head{0}; //
    alignas(64) std::atomic<size_t> _tail{0}; //
    // 环形缓冲区
    T _buffer[Capacity];
    // 帮助函数，计算下一个索引位置
    size_t next_index(size_t current) const {
        return (current + 1) % Capacity;
    }
public:
    spsc_queue() = default;
    // 禁止复制和移动
    spsc_queue(const spsc_queue&) = delete;
    spsc_queue& operator=(const spsc_queue&) = delete;
    spsc_queue(spsc_queue&&) = delete;
    spsc_queue& operator=(spsc_queue&&) = delete;
    // 检查队列是否为空
    bool empty() const {
        return _head.load(std::memory_order_relaxed) == _tail.load(std::memory_order_relaxed);
    }
    // 检查队列是否已满
    bool full() const {
        size_t next_tail = next_index(_tail.load(std::memory_order_relaxed));
        return next_tail == _head.load(std::memory_order_relaxed);
    }
    // 入队操作 - 生产者调用
    bool push(T item) {
        size_t current_tail = _tail.load(std::memory_order_relaxed);
        size_t next_tail = next_index(current_tail);
        if (next_tail == _head.load(std::memory_order_acquire)) {
            // 队列已满
            return false;
        }
        _buffer[current_tail] = std::move(item);
        _tail.store(next_tail, std::memory_order_release);
        return true;
    }
    //入队多个元素,返回成功入队的元素结束迭代器
    template <typename Iterator>
    Iterator push(Iterator begin, Iterator end) {
        Iterator current = begin;
        while (current != end) {
            if (!push(*current)) {
                break;
            }
            ++current;
        }
        return current;
    }
    //出队操作,消费者调用
    bool pop(T& item) {
        size_t current_head = _head.load(std::memory_order_relaxed);
        if (current_head == _tail.load(std::memory_order_acquire)) {
            // 队列为空
            return false;
        }
        item = std::move(_buffer[current_head]);
        _head.store(next_index(current_head), std::memory_order_release);
        return true;
    }
    // 批量出队 - 返回成功出队的元素数量
    template <size_t ArraySize>
    size_t pop(T (&items)[ArraySize]) {
        size_t popped = 0;
        while (popped < ArraySize && pop(items[popped])) {
            ++popped;
        }
        return popped;
    }
};

class reactor;
class posix_thread;
class smp_message_queue {
    static constexpr size_t queue_length = 128;
    static constexpr size_t batch_size = 16;
    static constexpr size_t prefetch_cnt = 2;
    struct work_item;
    struct lf_queue_remote {
        reactor* remote;
    };
    // 使用自定义的无锁队列替换boost::lockfree::spsc_queue
    using lf_queue_base = spsc_queue<work_item*, queue_length>;
    // 使用继承来控制布局顺序(?)
    struct lf_queue : lf_queue_remote, lf_queue_base {
        lf_queue(reactor* remote) : lf_queue_remote{remote} {}
        void maybe_wakeup();
    };
    lf_queue _pending;
    lf_queue _completed;
    struct alignas(64) {
        size_t _sent = 0;
        size_t _compl = 0;
        size_t _last_snt_batch = 0;
        size_t _last_cmpl_batch = 0;
        size_t _current_queue_length = 0;
    };
    // 在两个带有统计信息的结构体之间保持这个字段
    // 这确保它们之间至少有一个缓存行
    // 以便硬件预取器不会意外地预取另一个CPU使用的缓存行(硬件预取器是什么?)
    // metrics::metric_groups _metrics;
    struct alignas(64) {
        size_t _received = 0;
        size_t _last_rcv_batch = 0;
    };
    struct work_item {
        virtual ~work_item() {}
        virtual future<> process() = 0;
        virtual void complete() = 0;
    };
    template <typename Func>
    struct async_work_item : work_item {
        Func _func;
        using futurator = futurize<std::result_of_t<Func()>>;
        using future_type = typename futurator::type;
        using value_type = typename future_type::value_type;
        std::optional<value_type> _result;
        std::exception_ptr _ex; // if !_result
        typename futurator::promise_type _promise; // 在本地端使用
        async_work_item(Func&& func) : _func(std::move(func)) {}
        virtual future<> process() override {
            try {
                return futurator::apply(this->_func).then_wrapped([this] (auto&& f) {
                    try {
                        _result = f.get();
                    } catch (...) {
                        _ex = std::current_exception();
                    }
                });
            } catch (...) {
                _ex = std::current_exception();
                return make_ready_future<void>();
            }
        }
        virtual void complete() override {
            if (_result) {
                _promise.set_value(std::move(*_result));
            } else {
                // FIXME: _ex was allocated on another cpu
                _promise.set_exception(std::move(_ex));
            }
        }
        future_type get_future() { return _promise.get_future(); }
    };
    union tx_side {
        tx_side() {}
        ~tx_side() {}
        void init() { new (&a) aa; }
        struct aa {
            std::deque<work_item*> pending_fifo;
        } a;
    } _tx;
    std::vector<work_item*> _completed_fifo;
public:
    smp_message_queue(reactor* from, reactor* to); // 使用reactor from 和reactor to初始化smp_message_queue.
    template <typename Func>
    futurize_t<std::result_of_t<Func()>> submit(Func&& func) {
        auto wi = std::make_unique<async_work_item<Func>>(std::forward<Func>(func));
        auto fut = wi->get_future();
        submit_item(std::move(wi));
        return fut;
    }
    void start(unsigned cpuid);
    template<size_t PrefetchCnt, typename Func>
    size_t process_queue(lf_queue& q, Func process);
    size_t process_incoming();
    size_t process_completions();
    void stop();

private:
    void work();
    void submit_item(std::unique_ptr<work_item> wi);
    void respond(work_item* wi);
    void move_pending();
    void flush_request_batch();
    void flush_response_batch();
    bool has_unflushed_responses() const;
    bool pure_poll_rx() const;
    bool pure_poll_tx() const;

    friend class smp;
};


class smp {
public:
    static std::vector<posix_thread> _threads;
    static std::vector<std::function<void ()>> _thread_loops; // for dpdk
    static std::optional<boost::barrier> _all_event_loops_done;
    static std::vector<reactor*> _reactors;
    static smp_message_queue** _qs;
    static std::thread::id _tmain;
    static bool _using_dpdk;

    template <typename Func>
    using returns_future = is_future<std::result_of_t<Func()>>;

    template <typename Func>
    using returns_void = std::is_same<std::result_of_t<Func()>, void>;

    static boost::program_options::options_description get_options_description();
    static void configure(boost::program_options::variables_map vm);
    static void cleanup();
    static void cleanup_cpu();
    static void arrive_at_event_loop_end();
    static void join_all();
    static bool main_thread() { return std::this_thread::get_id() == _tmain; }
    template <typename Func>
    static futurize_t<std::result_of_t<Func()>> submit_to(unsigned t, Func&& func);
    static bool poll_queues();
    static bool pure_poll_queues();
    static boost::integer_range<unsigned> all_cpus() {
        return boost::irange(0u, count);
    }

    template<typename Func>
    static future<> invoke_on_all(Func&& func);

    static void start_all_queues();
    static void pin(unsigned cpu_id);
    static void allocate_reactor(unsigned id);
    static void create_thread(std::function<void ()> thread_loop);
public:
    static unsigned count;
};

template<typename Func>
future<> smp::invoke_on_all(Func&& func) {
        static_assert(std::is_same<future<>, typename futurize<std::result_of_t<Func()>>::type>::value, "bad Func signature");
        return parallel_for_each(all_cpus(), [&func] (unsigned id) {
            return smp::submit_to(id, Func(func));
        });
}

template <typename Func>
futurize_t<std::result_of_t<Func()>> smp::submit_to(unsigned t, Func&& func) {

        using ret_type = std::result_of_t<Func()>;
        if (t == engine().cpu_id()) {
            try {
                if (!is_future<ret_type>::value) {
                    // Non-deferring function, so don't worry about func lifetime
                    return futurize<ret_type>::apply(std::forward<Func>(func));
                } else if (std::is_lvalue_reference<Func>::value) {
                    // func is an lvalue, so caller worries about its lifetime
                    return futurize<ret_type>::apply(func);
                } else {
                    // Deferring call on rvalue function, make sure to preserve it across call
                    auto w = std::make_unique<std::decay_t<Func>>(std::move(func));
                    auto ret = futurize<ret_type>::apply(*w);
                    return ret.finally([w = std::move(w)] {});
                }
            } catch (...) {
                // Consistently return a failed future rather than throwing, to simplify callers
                return futurize<std::result_of_t<Func()>>::make_exception_future(std::current_exception());
            }
        } else {
            if (_qs != nullptr) {
                return _qs[t][engine().cpu_id()].submit(std::forward<Func>(func));
            } else {
                return futurize<std::result_of_t<Func()>>::make_exception_future(std::make_exception_ptr(std::runtime_error("smp::_qs is null")));
            }
        }
}