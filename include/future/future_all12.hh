// Created by lifd on 25-6-6.

#ifndef FUTURE_ALL12_HH
#define FUTURE_ALL12_HH

#include <dirent.h>
#include "file_desc.hh"
#include <sys/ioctl.h>
#include <linux/fs.h>   
#include <sys/statfs.h>
#include "../task/task.hh"
#include "future.hh"
#include "net.hh"
#include <sys/vfs.h>
#include <linux/magic.h>
#include <stdexcept>
#include <atomic>
#include <memory>
#include <utility>
#include <regex>
#include <tuple>
#include <type_traits>
#include "../util/bitops.hh"
#include <assert.h>
#include <cstdlib>
#include <chrono>
#include <functional>
#include <type_traits>
#include <setjmp.h>
#include <optional>
#include <sys/uio.h>
#include "do_with.hh"
#include <chrono>
#include <boost/intrusive/list.hpp>
#include <setjmp.h>
#include <ucontext.h>
#include <boost/range/adaptors.hpp>
#include <boost/range/numeric.hpp> // boost::accumulate
#include <list>
#include "../resource/resource.hh"
#include "bitset.h"
#include <chrono>
#include <limits>
#include <bitset>
#include <array>
#include <variant>
#include <atomic>
#include <list>
#include <deque>
#include <unordered_map>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <optional>
#include <iostream>
#include <time.h>
#include <signal.h>
#include <thread>
#include <iomanip>
#include <mutex>
#include <stdexcept>
#include <exception>
#include <deque>
#include <netinet/in.h>
#include <unordered_set>
#include <netinet/tcp.h>
#include <netinet/sctp.h>
#include <queue>
#include <libaio.h>
#include <sys/mman.h>
#include "../util/align.hh"
#include "../util/bool_class.hh"
#include <boost/iterator/counting_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/numeric.hpp> // boost::accumulate
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/range/irange.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/algorithm/string.hpp>
#include "timer.hh"
#include "semaphore.hh"
#include "io_queue.hh"
#include "stream.hh"
#include "signals.hh"
#include "network_stack.hh"
#include "memory.hh"


inline int alarm_signal() {
    // We don't want to use SIGALRM, because the boost unit test library
    // also plays with it.
    return SIGALRM;
    return SIGRTMIN;
}



namespace posix {
    template <typename Rep, typename Period>
    struct timespec to_timespec(std::chrono::duration<Rep, Period> d);

    template <typename Rep1, typename Period1, typename Rep2, typename Period2>
    struct itimerspec
    to_relative_itimerspec(std::chrono::duration<Rep1, Period1> base, std::chrono::duration<Rep2, Period2> interval);

    template <typename Clock, class Duration, class Rep, class Period>
    struct itimerspec
    to_absolute_itimerspec(std::chrono::time_point<Clock, Duration> base, std::chrono::duration<Rep, Period> interval);
}





extern __thread bool g_need_preempt;
inline bool need_preempt();

void schedule_normal(std::unique_ptr<task> t);

class reactor;
reactor& engine();

template <typename Func>
class deferred_action {
    Func _func;
    bool _cancelled = false;
public:
    static_assert(std::is_nothrow_move_constructible<Func>::value, "Func(Func&&) must be noexcept");
    deferred_action(Func&& func) noexcept : _func(std::move(func)) {}
    deferred_action(deferred_action&& o) noexcept : _func(std::move(o._func)), _cancelled(o._cancelled) {
        o._cancelled = true;
    }
    deferred_action& operator=(deferred_action&& o) noexcept {
        if (this != &o) {
            this->~deferred_action();
            new (this) deferred_action(std::move(o));
        }
        return *this;
    }
    deferred_action(const deferred_action&) = delete;
    ~deferred_action() { if (!_cancelled) { _func(); }; }
    void cancel() { _cancelled = true; }
};

template <typename Func>
deferred_action<Func>
defer(Func&& func) {
    return deferred_action<Func>(std::forward<Func>(func));
}

template<typename T>
class reference_wrapper {
    T* _pointer;
    explicit reference_wrapper(T& object) noexcept:_pointer(&object){}
    template<typename U>
    friend reference_wrapper<U> ref(U&) noexcept;
    template<typename U>
    friend reference_wrapper<const U> cref(const U&) noexcept;
public:
    using type = T;
    operator T&() const noexcept { return *_pointer; }
    T& get() const noexcept { return *_pointer; }

};





class posix_thread {
public:
    class attr;
private:
    // must allocate, since this class is moveable
    std::unique_ptr<std::function<void ()>> _func;
    pthread_t _pthread;
    bool _valid = true;
    mmap_area _stack;
private:
    static void* start_routine(void* arg) noexcept;
public:
    posix_thread(std::function<void ()> func);
    posix_thread(attr a, std::function<void ()> func);
    posix_thread(posix_thread&& x);
    ~posix_thread();
    void join();
public:
    class attr {
    public:
        struct stack_size { size_t size = 0; };
        attr() = default;
        template <typename... A>
        attr(A... a) {
            set(std::forward<A>(a)...);
        }
        void set() {}
        template <typename A, typename... Rest>
        void set(A a, Rest... rest) {
            set(std::forward<A>(a));
            set(std::forward<Rest>(rest)...);
        }
        void set(stack_size ss) { _stack_size = ss; }
    private:
        stack_size _stack_size;
        friend class posix_thread;
    };
};



/// Executes a callable in a seastar thread.
/// Runs a block of code in a threaded context,
/// which allows it to block (using \ref future::get()).  The
/// result of the callable is returned as a future.
/// \param func a callable to be executed in a thread
/// \param args a parameter pack to be forwarded to \c func.
/// \return whatever \c func returns, as a future.
/// Clock used for scheduling threads
using thread_clock = std::chrono::steady_clock;
class thread_scheduling_group {
public:
    std::chrono::nanoseconds _period;
    std::chrono::nanoseconds _quota;
    std::chrono::time_point<thread_clock> _this_period_ends = {};
    std::chrono::time_point<thread_clock> _this_run_start = {};
    std::chrono::nanoseconds _this_period_remain = {};
    /// \brief Constructs a \c thread_scheduling_group object
    /// \param period a duration representing the period
    /// \param usage which fraction of the \c period to assign for the scheduling group. Expected between 0 and 1.
    thread_scheduling_group(std::chrono::nanoseconds period, float usage);
    /// \brief changes the current maximum usage per period
    /// \param new_usage The new fraction of the \c period (Expected between 0 and 1) during which to run
    void update_usage(float new_usage) {
        _quota = std::chrono::duration_cast<std::chrono::nanoseconds>(new_usage * _period);
    }
    void account_start();
    void account_stop();
    std::chrono::steady_clock::time_point* next_scheduling_point() const;
};



struct thread_attributes {
        thread_scheduling_group* scheduling_group = nullptr;
};



class thread_context;
struct jmp_buf_link {
    jmp_buf jmpbuf;
    jmp_buf_link* link;
    thread_context* thread;
    bool has_yield_at = false;
    std::chrono::time_point<thread_clock> yield_at_value;
    void initial_switch_in(ucontext_t* initial_context, const void* stack_bottom, size_t stack_size);
    void switch_in();
    void switch_out();
    void initial_switch_in_completed();
    void final_switch_out();
    std::chrono::time_point<thread_clock>* get_yield_at() {
        return has_yield_at ? &yield_at_value : nullptr;
    }
    void set_yield_at(const std::chrono::time_point<thread_clock>& value) {
        yield_at_value = value;
        has_yield_at = true;
    }
    void clear_yield_at() {
        has_yield_at = false;
    }
};

extern thread_local jmp_buf_link g_unthreaded_context;  // 在 jmp_buf_link init_switch_in 的时候用来初始化 g_current_context
extern thread_local jmp_buf_link* g_current_context;

struct thread_context {
    struct stack_deleter {
        void operator()(char *ptr) const noexcept;
    };
    using stack_holder = std::unique_ptr<char[], stack_deleter>;
    thread_attributes _attr;
    static constexpr size_t _stack_size = 128*1024;
    stack_holder _stack{make_stack()};
    std::function<void ()> _func;
    jmp_buf_link _context;
    promise<> _done;
    bool _joined = false;
    timer<> _sched_timer{[this] { reschedule(); }};
    promise<>* _sched_promise_ptr = nullptr;
    promise<> _sched_promise_value;
    std::list<thread_context*>::iterator _preempted_it;
    std::list<thread_context*>::iterator _all_it;
    // Replace boost::intrusive::list with std::list
    static thread_local std::list<thread_context*> _preempted_threads;
    static thread_local std::list<thread_context*> _all_threads;
    static void s_main(unsigned int lo, unsigned int hi);
    void setup();
    void main();
    static stack_holder make_stack();
    thread_context(thread_attributes attr, std::function<void ()> func);
    ~thread_context();
    void switch_in();
    void switch_out();
    bool should_yield() const;
    void reschedule();
    void yield();
    promise<>* get_sched_promise() {
        return _sched_promise_ptr;
    }
    void set_sched_promise() {
        _sched_promise_ptr = &_sched_promise_value;
    }
    void clear_sched_promise() {
        _sched_promise_ptr = nullptr;
    }
};

namespace thread_impl {
    inline thread_context* get();
    inline bool should_yield();
    void yield();
    void switch_in(thread_context* to);
    void switch_out(thread_context* from);
    void init();
}

class thread {
    std::unique_ptr<thread_context> _context;
    static thread_local thread* _current;
public:
    /// \brief Constructs a \c thread object that does not represent a thread
    /// of execution.
    thread() = default;
    /// \brief Constructs a \c thread object that represents a thread of execution
    ///
    /// \param func Callable object to execute in thread.  The callable is
    ///             called immediately.
    template <typename Func>
    thread(Func func);
    /// \brief Constructs a \c thread object that represents a thread of execution
    /// \param attr Attributes describing the new thread.
    /// \param func Callable object to execute in thread.  The callable is
    ///             called immediately.
    template <typename Func>
    thread(thread_attributes attr, Func func);
    /// \brief Moves a thread object.
    thread(thread&& x) noexcept = default;
    /// \brief Move-assigns a thread object.
    thread& operator=(thread&& x) noexcept = default;
    /// \brief Destroys a \c thread object.
    /// The thread must not represent a running thread of execution (see join()).
    ~thread();
    future<> join();
    /// \brief Voluntarily defer execution of current thread.
    /// Gives other threads/fibers a chance to run on current CPU.
    /// The current thread will resume execution promptly.
    static void yield();
    /// \brief Checks whether this thread ought to call yield() now
    /// Useful where we cannot call yield() immediately because we
    /// Need to take some cleanup action first.
    static bool should_yield();
    static bool try_run_one_yielded_thread();
};

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
    // 入队多个元素 - 返回成功入队的元素结束迭代器
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
    // 出队操作 - 消费者调用
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



template <typename... T>
void future_state<T...>::forward_to(promise<T...>& pr) noexcept{
    assert(_state != state::future);
    if (_state == state::exception) {
        pr.set_urgent_exception(std::move(_u.ex));
        _u.ex.~exception_ptr();
    } else {
        pr.set_urgent_value(std::move(_u.value));
        _u.value.~tuple();
    }
    _state = state::invalid;
}

inline
void future_state<>::forward_to(promise<>& pr) noexcept {
    assert(_u.st != state::future && _u.st != state::invalid);
    if (_u.st >= state::exception_min) {
        pr.set_urgent_exception(std::move(_u.ex));
        _u.ex.~exception_ptr();
    } else {
        pr.set_urgent_value(std::tuple<>());
    }
    _u.st = state::invalid;
}


template <typename... T>
inline
future<T...>
promise<T...>::get_future() noexcept {
    assert(!_future && _state && !_task);
    return future<T...>(this);
}

template <typename... T>
template<typename promise<T...>::urgent Urgent>
inline
void promise<T...>::make_ready() noexcept {
    if (_task) {
        _state = nullptr;
        if (Urgent == urgent::yes && !need_preempt()) {
            ::schedule_urgent(std::move(_task));
        } else {
            ::schedule_normal(std::move(_task));
        }
    }
}


template <typename... T>
inline
void promise<T...>::abandoned() noexcept {
    if (_future) {
        assert(_state);
        assert(_state->available() || !_task);
        _future->_local_state = std::move(*_state);
        _future->_promise = nullptr;
    } else if (_state && _state->failed()) {
        report_failed_future(_state->get_exception());
    }
}


/// \brief Waits for the future to become available
/// This method blocks the current thread until the future becomes available.
template <typename... T>
void future<T...>::wait() {
    std::cout<<"future wait"<<std::endl;
    auto thread = thread_impl::get();
    assert(thread);//这里报错.

    schedule([this, thread] (future_state<T...>&& new_state) {
        *state() = std::move(new_state);
        std::cout<<" future wait end"<<std::endl;
        thread_impl::switch_in(thread);
    });
    thread_impl::switch_out(thread);
}

template <typename... T>
[[gnu::always_inline]]
std::tuple<T...> future<T...>::get() {
    if (!state()->available()) {
        wait();
    } else if (thread_impl::get() && thread_impl::should_yield()) {
        thread_impl::yield();
    }
    return get_available_state().get();
}

template <typename T>
template <typename Arg>
inline
future<T>
futurize<T>::make_exception_future(Arg&& arg) {
    return ::make_exception_future<T>(std::forward<Arg>(arg));
}

template <typename... T>
template <typename Arg>
inline
future<T...>
futurize<future<T...>>::make_exception_future(Arg&& arg) {
    return ::make_exception_future<T...>(std::forward<Arg>(arg));
}

template <typename Arg>
inline
future<>
futurize<void>::make_exception_future(Arg&& arg) {
    return ::make_exception_future<>(std::forward<Arg>(arg));
}

template <typename T>
inline
future<T>
futurize<T>::from_tuple(std::tuple<T>&& value) {
    return make_ready_future<T>(std::move(value));
}

template <typename T>
inline
future<T>
futurize<T>::from_tuple(const std::tuple<T>& value) {
    return make_ready_future<T>(value);
}
inline future<> futurize<void>::from_tuple(std::tuple<>&& value) {
    return make_ready_future<>();
}

inline future<> futurize<void>::from_tuple(const std::tuple<>& value) {
    return make_ready_future<>();
}
/// \brief Creates a \ref future in an available, failed state.
///
/// Creates a \ref future object that is already resolved in a failed
/// state.  This no I/O needs to be performed to perform a computation
/// (for example, because the connection is closed and we cannot read
/// from it).
template <typename... T, typename Exception>
inline
future<T...> make_exception_future(Exception&& ex) noexcept {
    return make_exception_future<T...>(std::make_exception_ptr(std::forward<Exception>(ex)));
}

/// @}

/// \cond internal

template<typename T>
template<typename Func, typename... FuncArgs>
typename futurize<T>::type futurize<T>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        return convert(std::apply(std::forward<Func>(func), std::move(args)));
        //执行这个函数,并返回结果.然后对结果执行convert。
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename T>
template<typename Func, typename... FuncArgs>
typename futurize<T>::type futurize<T>::apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        return convert(func(std::forward<FuncArgs>(args)...));
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
inline
std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        func(std::forward<FuncArgs>(args)...);
        return make_ready_future<>();
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
inline
std::enable_if_t<is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
do_void_futurize_apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        return func(std::forward<FuncArgs>(args)...);
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
inline
std::enable_if_t<!is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        std::apply(std::forward<Func>(func), std::move(args));
        return make_ready_future<>();
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
inline
std::enable_if_t<is_future<std::result_of_t<Func(FuncArgs&&...)>>::value, future<>>
do_void_futurize_apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        return std::apply(std::forward<Func>(func), std::move(args));
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... FuncArgs>
typename futurize<void>::type futurize<void>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    return do_void_futurize_apply_tuple(std::forward<Func>(func), std::move(args));
}

template<typename Func, typename... FuncArgs>
typename futurize<void>::type futurize<void>::apply(Func&& func, FuncArgs&&... args) noexcept {
    return do_void_futurize_apply(std::forward<Func>(func), std::forward<FuncArgs>(args)...);
}

template<typename... Args>
template<typename Func, typename... FuncArgs>
typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        return std::apply(std::forward<Func>(func), std::move(args));
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename... Args>
template<typename Func, typename... FuncArgs>
typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        return func(std::forward<FuncArgs>(args)...);
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

template<typename Func, typename... Args>
auto futurize_apply(Func&& func, Args&&... args) {
    using futurator = futurize<std::result_of_t<Func(Args&&...)>>;
    return futurator::apply(std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename... T, typename... A>
inline
future<T...> make_ready_future(A&&... value) {
    return future<T...>(ready_future_marker(), std::forward<A>(value)...);
}

template <typename... T>
inline
future<T...> make_exception_future(std::exception_ptr ex) noexcept {
    return future<T...>(exception_future_marker(), std::move(ex));
}

/*-----------------------------------------------------------------------------------------------------------------------------*/

struct pollfn {
        virtual ~pollfn() {}
        // Returns true if work was done (false = idle)
        virtual bool poll() = 0;
        // Checks if work needs to be done, but without actually doing any
        // returns true if works needs to be done (false = idle)
        virtual bool pure_poll() = 0;
        // Tries to enter interrupt mode.
        //
        // If it returns true, then events from this poller will wake
        // a sleeping idle loop, and exit_interrupt_mode() must be called
        // to return to normal polling.
        //
        // If it returns false, the sleeping idle loop may not be entered.
        virtual bool try_enter_interrupt_mode() { return false; }
        virtual void exit_interrupt_mode() {}
};

class pollable_fd_state {
public:
    struct speculation {
        int events = 0;
        explicit speculation(int epoll_events_guessed = 0) : events(epoll_events_guessed) {}
    };
    ~pollable_fd_state();
    explicit pollable_fd_state(file_desc fd, speculation speculate = speculation())
        : fd(std::move(fd)), events_known(speculate.events) {}
    pollable_fd_state(const pollable_fd_state&) = delete;
    void operator=(const pollable_fd_state&) = delete;
    void speculate_epoll(int events) { events_known |= events; }
    file_desc fd;
    int events_requested = 0; // wanted by pollin/pollout promises
    int events_epoll = 0;     // installed in epoll
    int events_known = 0;     // returned from epoll
    promise<> pollin;
    promise<> pollout;
    friend class reactor;
    friend class pollable_fd;
};


class pollable_fd {
public:
    using speculation = pollable_fd_state::speculation;
    pollable_fd(file_desc fd, speculation speculate = speculation())
        : _s(std::make_unique<pollable_fd_state>(std::move(fd), speculate)) {}
public:
    pollable_fd(pollable_fd&&) = default;
    pollable_fd& operator=(pollable_fd&&) = default;
    future<size_t> read_some(char* buffer, size_t size);
    future<size_t> read_some(uint8_t* buffer, size_t size);
    future<size_t> read_some(const std::vector<iovec>& iov);
    future<> write_all(const char* buffer, size_t size);
    future<> write_all(const uint8_t* buffer, size_t size);
    future<size_t> write_some(net::packet& p);
    future<> write_all(net::packet& p);
    future<> readable();
    future<> writeable();
    void abort_reader(std::exception_ptr ex);
    void abort_writer(std::exception_ptr ex);
    future<pollable_fd, net::socket_address> accept();
    future<size_t> sendmsg(struct msghdr *msg);
    future<size_t> recvmsg(struct msghdr *msg);
    future<size_t> sendto(net::socket_address addr, const void* buf, size_t len);
    file_desc& get_file_desc() const { return _s->fd; }
    void shutdown(int how) { _s->fd.shutdown(how); }
    void close() { _s.reset(); }
protected:
    int get_fd() const { return _s->fd.get(); }
    friend class reactor;
    friend class readable_eventfd;
    friend class writeable_eventfd;
private:
    std::unique_ptr<pollable_fd_state> _s;
};
class readable_eventfd;
class writeable_eventfd {
public:
    file_desc _fd;
    explicit writeable_eventfd(size_t initial = 0) : _fd(try_create_eventfd(initial)) {}
    writeable_eventfd(writeable_eventfd&&) = default;
    readable_eventfd read_side();
    void signal(size_t nr);
    int get_read_fd() { return _fd.get(); }
    explicit writeable_eventfd(file_desc&& fd) : _fd(std::move(fd)) {}
    static file_desc try_create_eventfd(size_t initial);
};

class readable_eventfd {
public:
    pollable_fd _fd;
    explicit readable_eventfd(size_t initial = 0) : _fd(try_create_eventfd(initial)) {}
    readable_eventfd(readable_eventfd&&) = default;
    writeable_eventfd write_side();
    future<size_t> wait();
    int get_write_fd() { return _fd.get_fd(); }
    explicit readable_eventfd(file_desc&& fd) : _fd(std::move(fd)) {}
    static file_desc try_create_eventfd(size_t initial);
};



class reactor_notifier {
public:
    virtual future<> wait() = 0;
    virtual void signal() = 0;
    virtual ~reactor_notifier() {}
};



class syscall_work_queue {
public:
    static constexpr size_t queue_length = 128;
    struct work_item;
    using lf_queue = boost::lockfree::spsc_queue<work_item*,
                            boost::lockfree::capacity<queue_length>>;
    lf_queue _pending;
    lf_queue _completed;
    writeable_eventfd _start_eventfd;
    semaphore _queue_has_room = { queue_length };
    struct work_item {
        virtual ~work_item() {}
        virtual void process() = 0;
        virtual void complete() = 0;
    };
    template <typename T, typename Func>
    struct work_item_returning :  work_item {
        Func _func;
        promise<T> _promise;
        std::optional<T> _result;
        work_item_returning(Func&& func) : _func(std::move(func)) {}
        virtual void process() override { _result = this->_func(); }
        virtual void complete() override { _promise.set_value(std::move(*_result)); }
        future<T> get_future() { return _promise.get_future(); }
    };
public:
    syscall_work_queue();
    template <typename T, typename Func>
    future<T> submit(Func func) {
        auto wi = std::make_unique<work_item_returning<T, Func>>(std::move(func));
        auto fut = wi->get_future();
        submit_item(std::move(wi));
        return fut;
    }
    void work();
    // Scans the _completed queue, that contains the requests already handled by the syscall thread,
    // effectively opening up space for more requests to be submitted. One consequence of this is
    // that from the reactor's point of view, a request is not considered handled until it is
    // removed from the _completed queue.
    //
    // Returns the number of requests handled.
    unsigned complete();
    void submit_item(std::unique_ptr<syscall_work_queue::work_item> wi);
    friend class thread_pool;
};


/*-----------------------------file-----------------------------------------------------------------*/

/// \see file::list_directory()
enum class directory_entry_type {
    block_device,
    char_device,
    directory,
    fifo,
    link,
    regular,
    socket,
};

/// Enumeration describing the type of a particular filesystem
enum class fs_type {
    other,
    xfs,
    ext2,
    ext3,
    ext4,
    btrfs,
    hfs,
    tmpfs,
};

/// A directory entry being listed.
struct directory_entry {
    /// Name of the file in a directory entry.  Will never be "." or "..".  Only the last component is included.
    std::string name;
    /// Type of the directory entry, if known.
    std::optional<::directory_entry_type> type;
};

/// File open options
///
/// Options used to configure an open file.
///
/// \ref file
struct file_open_options {
    uint64_t extent_allocation_size_hint = 1 << 20; ///< Allocate this much disk space when extending the file
    bool sloppy_size = false; ///< Allow the file size not to track the amount of data written until a flush
    uint64_t sloppy_size_hint = 1 << 20; ///< Hint as to what the eventual file size will be
};


const io_priority_class& default_priority_class();



class file;
class file_impl;

namespace File {

class file_handle;

// A handle that can be transported across shards and used to
// create a dup(2)-like `file` object referring to the same underlying file
class file_handle_impl {
public:
    virtual ~file_handle_impl() = default;
    virtual std::unique_ptr<file_handle_impl> clone() const = 0;
    virtual std::shared_ptr<file_impl> to_file() && = 0;
};

}

class file_impl {
protected:
    static file_impl* get_file_impl(file& f);
public:
    unsigned _memory_dma_alignment = 4096;
    unsigned _disk_read_dma_alignment = 4096;
    unsigned _disk_write_dma_alignment = 4096;
public:
    virtual ~file_impl() {}

    virtual future<size_t> write_dma(uint64_t pos, const void* buffer, size_t len, const io_priority_class& pc) = 0;
    virtual future<size_t> write_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc) = 0;
    virtual future<size_t> read_dma(uint64_t pos, void* buffer, size_t len, const io_priority_class& pc) = 0;
    virtual future<size_t> read_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc) = 0;
    virtual future<> flush(void) = 0;
    virtual future<struct stat> stat(void) = 0;
    virtual future<> truncate(uint64_t length) = 0;
    virtual future<> discard(uint64_t offset, uint64_t length) = 0;
    virtual future<> allocate(uint64_t position, uint64_t length) = 0;
    virtual future<uint64_t> size(void) = 0;
    virtual future<> close() = 0;
    virtual std::unique_ptr<File::file_handle_impl> dup();
    virtual subscription<directory_entry> list_directory(std::function<future<> (directory_entry de)> next) = 0;
    virtual future<temporary_buffer<uint8_t>> dma_read_bulk(uint64_t offset, size_t range_size, const io_priority_class& pc) = 0;

    friend class reactor;
};

/// \endcond

/// A data file on persistent storage.
///
/// File objects represent uncached, unbuffered files.  As such great care
/// must be taken to cache data at the application layer; neither seastar
/// nor the OS will cache these file.
///
/// Data is transferred using direct memory access (DMA).  This imposes
/// restrictions on file offsets and data pointers.  The former must be aligned
/// on a 4096 byte boundary, while a 512 byte boundary suffices for the latter.
class file {
    std::shared_ptr<file_impl> _file_impl;
private:
    explicit file(int fd, file_open_options options);
public:
    /// Default constructor constructs an uninitialized file object.
    ///
    /// A default constructor is useful for the common practice of declaring
    /// a variable, and only assigning to it later. The uninitialized file
    /// must not be used, or undefined behavior will result (currently, a null
    /// pointer dereference).
    ///
    /// One can check whether a file object is in uninitialized state with
    /// \ref operator bool(); One can reset a file back to uninitialized state
    /// by assigning file() to it.
    file() : _file_impl(nullptr) {}

    file(std::shared_ptr<file_impl> impl)
            : _file_impl(std::move(impl)) {}

    /// Constructs a file object from a \ref file_handle obtained from another shard
    explicit file(File::file_handle&& handle);

    /// Checks whether the file object was initialized.
    ///
    /// \return false if the file object is uninitialized (default
    /// constructed), true if the file object refers to an actual file.
    explicit operator bool() const noexcept { return bool(_file_impl); }

    /// Copies a file object.  The new and old objects refer to the
    /// same underlying file.
    ///
    /// \param x file object to be copied
    file(const file& x) = default;
    /// Moves a file object.
    file(file&& x) noexcept : _file_impl(std::move(x._file_impl)) {}
    /// Assigns a file object.  After assignent, the destination and source refer
    /// to the same underlying file.
    ///
    /// \param x file object to assign to `this`.
    file& operator=(const file& x) noexcept = default;
    /// Moves assigns a file object.
    file& operator=(file&& x) noexcept = default;

    // O_DIRECT reading requires that buffer, offset, and read length, are
    // all aligned. Alignment of 4096 was necessary in the past, but no longer
    // is - 512 is usually enough; But we'll need to use BLKSSZGET ioctl to
    // be sure it is really enough on this filesystem. 4096 is always safe.
    // In addition, if we start reading in things outside page boundaries,
    // we will end up with various pages around, some of them with
    // overlapping ranges. Those would be very challenging to cache.

    /// Alignment requirement for file offsets (for reads)
    uint64_t disk_read_dma_alignment() const {
        return _file_impl->_disk_read_dma_alignment;
    }

    /// Alignment requirement for file offsets (for writes)
    uint64_t disk_write_dma_alignment() const {
        return _file_impl->_disk_write_dma_alignment;
    }

    /// Alignment requirement for data buffers
    uint64_t memory_dma_alignment() const {
        return _file_impl->_memory_dma_alignment;
    }


    /**
     * Perform a single DMA read operation.
     *
     * @param aligned_pos offset to begin reading at (should be aligned)
     * @param aligned_buffer output buffer (should be aligned)
     * @param aligned_len number of bytes to read (should be aligned)
     * @param pc the IO priority class under which to queue this operation
     *
     * Alignment is HW dependent but use 4KB alignment to be on the safe side as
     * explained above.
     *
     * @return number of bytes actually read
     * @throw exception in case of I/O error
     */
    template <typename CharType>
    future<size_t>
    dma_read(uint64_t aligned_pos, CharType* aligned_buffer, size_t aligned_len, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->read_dma(aligned_pos, aligned_buffer, aligned_len, pc);
    }

    /**
     * Read the requested amount of bytes starting from the given offset.
     *
     * @param pos offset to begin reading from
     * @param len number of bytes to read
     * @param pc the IO priority class under which to queue this operation
     *
     * @return temporary buffer containing the requested data.
     * @throw exception in case of I/O error
     *
     * This function doesn't require any alignment for both "pos" and "len"
     *
     * @note size of the returned buffer may be smaller than "len" if EOF is
     *       reached of in case of I/O error.
     */
    template <typename CharType>
    future<temporary_buffer<CharType>> dma_read(uint64_t pos, size_t len, const io_priority_class& pc = default_priority_class()) {
        return dma_read_bulk<CharType>(pos, len, pc).then(
                [len] (temporary_buffer<CharType> buf) {
            if (len < buf.size()) {
                buf.trim(len);
            }

            return std::move(buf);
        });
    }

    /// Error thrown when attempting to read past end-of-file
    /// with \ref dma_read_exactly().
    class eof_error : public std::exception {};

    /**
     * Read the exact amount of bytes.
     *
     * @param pos offset in a file to begin reading from
     * @param len number of bytes to read
     * @param pc the IO priority class under which to queue this operation
     *
     * @return temporary buffer containing the read data
     * @throw end_of_file_error if EOF is reached, file_io_error or
     *        std::system_error in case of I/O error.
     */
    template <typename CharType>
    future<temporary_buffer<CharType>>
    dma_read_exactly(uint64_t pos, size_t len, const io_priority_class& pc = default_priority_class()) {
        return dma_read<CharType>(pos, len, pc).then(
                [pos, len] (auto buf) {
            if (buf.size() < len) {
                throw eof_error();
            }

            return std::move(buf);
        });
    }

    /// Performs a DMA read into the specified iovec.
    ///
    /// \param pos offset to read from.  Must be aligned to \ref dma_alignment.
    /// \param iov vector of address/size pairs to read into.  Addresses must be
    ///            aligned.
    /// \param pc the IO priority class under which to queue this operation
    ///
    /// \return a future representing the number of bytes actually read.  A short
    ///         read may happen due to end-of-file or an I/O error.
    future<size_t> dma_read(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->read_dma(pos, std::move(iov), pc);
    }

    /// Performs a DMA write from the specified buffer.
    ///
    /// \param pos offset to write into.  Must be aligned to \ref dma_alignment.
    /// \param buffer aligned address of buffer to read from.  Buffer must exists
    ///               until the future is made ready.
    /// \param len number of bytes to write.  Must be aligned.
    /// \param pc the IO priority class under which to queue this operation
    ///
    /// \return a future representing the number of bytes actually written.  A short
    ///         write may happen due to an I/O error.
    template <typename CharType>
    future<size_t> dma_write(uint64_t pos, const CharType* buffer, size_t len, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->write_dma(pos, buffer, len, pc);
    }

    /// Performs a DMA write to the specified iovec.
    ///
    /// \param pos offset to write into.  Must be aligned to \ref dma_alignment.
    /// \param iov vector of address/size pairs to write from.  Addresses must be
    ///            aligned.
    /// \param pc the IO priority class under which to queue this operation
    ///
    /// \return a future representing the number of bytes actually written.  A short
    ///         write may happen due to an I/O error.
    future<size_t> dma_write(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->write_dma(pos, std::move(iov), pc);
    }

    /// Causes any previously written data to be made stable on persistent storage.
    ///
    /// Prior to a flush, written data may or may not survive a power failure.  After
    /// a flush, data is guaranteed to be on disk.
    future<> flush() {
        return _file_impl->flush();
    }

    /// Returns \c stat information about the file.
    future<struct stat> stat() {
        return _file_impl->stat();
    }

    /// Truncates the file to a specified length.
    future<> truncate(uint64_t length) {
        return _file_impl->truncate(length);
    }

    /// Preallocate disk blocks for a specified byte range.
    ///
    /// Requests the file system to allocate disk blocks to
    /// back the specified range (\c length bytes starting at
    /// \c position).  The range may be outside the current file
    /// size; the blocks can then be used when appending to the
    /// file.
    ///
    /// \param position beginning of the range at which to allocate
    ///                 blocks.
    /// \parm length length of range to allocate.
    /// \return future that becomes ready when the operation completes.
    future<> allocate(uint64_t position, uint64_t length) {
        return _file_impl->allocate(position, length);
    }

    /// Discard unneeded data from the file.
    ///
    /// The discard operation tells the file system that a range of offsets
    /// (which be aligned) is no longer needed and can be reused.
    future<> discard(uint64_t offset, uint64_t length) {
        return _file_impl->discard(offset, length);
    }

    /// Gets the file size.
    future<uint64_t> size() const {
        return _file_impl->size();
    }

    /// Closes the file.
    ///
    /// Flushes any pending operations and release any resources associated with
    /// the file (except for stable storage).
    ///
    /// \note
    /// to ensure file data reaches stable storage, you must call \ref flush()
    /// before calling \c close().
    future<> close() {
        return _file_impl->close();
    }

    /// Returns a directory listing, given that this file object is a directory.
    subscription<directory_entry> list_directory(std::function<future<> (directory_entry de)> next) {
        return _file_impl->list_directory(std::move(next));
    }

    /**
     * Read a data bulk containing the provided addresses range that starts at
     * the given offset and ends at either the address aligned to
     * dma_alignment (4KB) or at the file end.
     *
     * @param offset starting address of the range the read bulk should contain
     * @param range_size size of the addresses range
     * @param pc the IO priority class under which to queue this operation
     *
     * @return temporary buffer containing the read data bulk.
     * @throw system_error exception in case of I/O error or eof_error when
     *        "offset" is beyond EOF.
     */
    template <typename CharType>
    future<temporary_buffer<CharType>>
    dma_read_bulk(uint64_t offset, size_t range_size, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->dma_read_bulk(offset, range_size, pc).then([] (temporary_buffer<uint8_t> t) {
            return temporary_buffer<CharType>(reinterpret_cast<CharType*>(t.get_write()), t.size(), t.release());
        });
    }

    /// \brief Creates a handle that can be transported across shards.
    ///
    /// Creates a handle that can be transported across shards, and then
    /// used to create a new shard-local \ref file object that refers to
    /// the same on-disk file.
    ///
    /// \note Use on read-only files.
    ///
    File::file_handle dup();

    template <typename CharType>
    struct read_state;
private:
    friend class reactor;
    friend class file_impl;
};


/// \brief A shard-transportable handle to a file
/// If you need to access a file (for reads only) across multiple shards,
/// you can use the file::dup() method to create a `file_handle`, transport
/// this file handle to another shard, and use the handle to create \ref file
/// object on that shard.  This is more efficient than calling open_file_dma()
/// again.



namespace File{
class file_handle {
    std::unique_ptr<File::file_handle_impl> _impl;
private:
    explicit file_handle(std::unique_ptr<File::file_handle_impl> impl) : _impl(std::move(impl)) {}
public:
    /// Copies a file handle object
    file_handle(const file_handle&);
    /// Moves a file handle object
    file_handle(file_handle&&) noexcept;
    /// Assigns a file handle object
    file_handle& operator=(const file_handle&);
    /// Move-assigns a file handle object
    file_handle& operator=(file_handle&&) noexcept;
    /// Converts the file handle object to a \ref file.
    file to_file() const &;
    /// Converts the file handle object to a \ref file.
    file to_file() &&;
    friend class ::file;
};
}
template <typename CharType>
struct file::read_state {
    typedef temporary_buffer<CharType> tmp_buf_type;
    read_state(uint64_t offset, uint64_t front, size_t to_read,
            size_t memory_alignment, size_t disk_alignment)
    : buf(tmp_buf_type::aligned(memory_alignment, align_up(to_read, disk_alignment)))
    , _offset(offset)
    , _to_read(to_read)
    , _front(front) {}
    bool done() const {
        return eof || pos >= _to_read;
    }
    /**
     * Trim the buffer to the actual number of read bytes and cut the
     * bytes from offset 0 till "_front".
     * @note this function has to be called only if we read bytes beyond
     *       "_front".
     */
    void trim_buf_before_ret() {
        if (have_good_bytes()) {
            buf.trim(pos);
            buf.trim_front(_front);
        } else {
            buf.trim(0);
        }
    }
    uint64_t cur_offset() const {
        return _offset + pos;
    }
    size_t left_space() const {
        return buf.size() - pos;
    }
    size_t left_to_read() const {
        // positive as long as (done() == false)
        return _to_read - pos;
    }
    void append_new_data(tmp_buf_type& new_data) {
        auto to_copy = std::min(left_space(), new_data.size());
        std::memcpy(buf.get_write() + pos, new_data.get(), to_copy);
        pos += to_copy;
    }
    bool have_good_bytes() const {
        return pos > _front;
    }
public:
    bool         eof      = false;
    tmp_buf_type buf;
    size_t       pos      = 0;
private:
    uint64_t     _offset;
    size_t       _to_read;
    uint64_t     _front;
};

class posix_file_handle_impl : public File::file_handle_impl {
    int _fd;
    std::atomic<unsigned>* _refcount;
public:
    posix_file_handle_impl(int fd, std::atomic<unsigned>* refcount)
            : _fd(fd), _refcount(refcount) { }
    virtual ~posix_file_handle_impl();
    posix_file_handle_impl(const posix_file_handle_impl&) = delete;
    posix_file_handle_impl(posix_file_handle_impl&&) = delete;
    virtual std::shared_ptr<file_impl> to_file() && override;
    virtual std::unique_ptr<File::file_handle_impl> clone() const override;
};

class posix_file_impl : public file_impl {
    std::atomic<unsigned>* _refcount = nullptr;
public:
    int _fd;
    posix_file_impl(int fd, file_open_options options);
    posix_file_impl(int fd, std::atomic<unsigned>* refcount);
    virtual ~posix_file_impl() override;
    future<size_t> write_dma(uint64_t pos, const void* buffer, size_t len, const io_priority_class& pc);
    future<size_t> write_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc);
    future<size_t> read_dma(uint64_t pos, void* buffer, size_t len, const io_priority_class& pc);
    future<size_t> read_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc);
    future<> flush(void);
    future<struct stat> stat(void);
    future<> truncate(uint64_t length);
    future<> discard(uint64_t offset, uint64_t length);
    virtual future<> allocate(uint64_t position, uint64_t length) override;
    future<uint64_t> size();
    virtual future<> close() noexcept override;
    virtual std::unique_ptr<File::file_handle_impl> dup() override;
    virtual subscription<directory_entry> list_directory(std::function<future<> (directory_entry de)> next) override;
    virtual future<temporary_buffer<uint8_t>> dma_read_bulk(uint64_t offset, size_t range_size, const io_priority_class& pc);
private:
    void query_dma_alignment();
    /**
     * Try to read from the given position where the previous short read has
     * stopped. Check the EOF condition.
     *
     * The below code assumes the following: short reads due to I/O errors
     * always end at address aligned to HW block boundary. Therefore if we issue
     * a new read operation from the next position we are promised to get an
     * error (different from EINVAL). If we've got a short read because we have
     * reached EOF then the above read would either return a zero-length success
     * (if the file size is aligned to HW block size) or an EINVAL error (if
     * file length is not aligned to HW block size).
     * @param pos offset to read from
     * @param len number of bytes to read
     * @param pc the IO priority class under which to queue this operation
     *
     * @return temporary buffer with read data or zero-sized temporary buffer if
     *         pos is at or beyond EOF.
     * @throw appropriate exception in case of I/O error.
     */
    future<temporary_buffer<uint8_t>>
    read_maybe_eof(uint64_t pos, size_t len, const io_priority_class& pc);
};

// The Linux XFS implementation is challenged wrt. append: a write that changes
// eof will be blocked by any other concurrent AIO operation to the same file, whether
// it changes file size or not. Furthermore, ftruncate() will also block and be blocked
// by AIO, so attempts to game the system and call ftruncate() have to be done very carefully.
// Other Linux filesystems may have different locking rules, so this may need to be
// adjusted for them.
class append_challenged_posix_file_impl : public posix_file_impl {
    // File size as a result of completed kernel operations (writes and truncates)
    uint64_t _committed_size;
    // File size as a result of seastar API calls
    uint64_t _logical_size;
    // Pending operations
    enum class opcode {
        invalid,
        read,
        write,
        truncate,
        flush,
    };
    struct op {
        opcode type;
        uint64_t pos;
        size_t len;
        std::function<future<> ()> run;
    };
    // Queue of pending operations; processed from front to end to avoid
    // starvation, but can issue concurrent operations.
    std::deque<op> _q;
    unsigned _max_size_changing_ops = 0;
    unsigned _current_non_size_changing_ops = 0;
    unsigned _current_size_changing_ops = 0;
    // Set when the user closes the file
    bool _done = false;
    bool _sloppy_size = false;
    // Fulfiled when _done and I/O is complete
    promise<> _completed;
private:
    void commit_size(uint64_t size) noexcept;
    bool must_run_alone(const op& candidate) const noexcept;
    bool size_changing(const op& candidate) const noexcept;
    bool may_dispatch(const op& candidate) const noexcept;
    void dispatch(op& candidate) noexcept;
    void optimize_queue() noexcept;
    void process_queue() noexcept;
    bool may_quit() const noexcept;
    void enqueue(op&& op);
public:
    append_challenged_posix_file_impl(int fd, file_open_options options, unsigned max_size_changing_ops);
    ~append_challenged_posix_file_impl() override;
    future<size_t> read_dma(uint64_t pos, void* buffer, size_t len, const io_priority_class& pc) override;
    future<size_t> read_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc) override;
    future<size_t> write_dma(uint64_t pos, const void* buffer, size_t len, const io_priority_class& pc) override;
    future<size_t> write_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc) override;
    future<> flush() override;
    future<struct stat> stat() override;
    future<> truncate(uint64_t length) override;
    future<uint64_t> size() override;
    future<> close() noexcept override;
};

class blockdev_file_impl : public posix_file_impl {
public:
    blockdev_file_impl(int fd, file_open_options options);
    future<> truncate(uint64_t length) override;
    future<> discard(uint64_t offset, uint64_t length) override;
    future<uint64_t> size() override;
    virtual future<> allocate(uint64_t position, uint64_t length) override;
};


class file_input_stream_history {
    static constexpr uint64_t window_size = 4 * 1024 * 1024;
    struct window {
        uint64_t total_read = 0;
        uint64_t unused_read = 0;
    };
    window current_window;
    window previous_window;
    unsigned read_ahead = 1;

    friend class file_data_source_impl;
};

/// Data structure describing options for opening a file input stream
struct file_input_stream_options {
    size_t buffer_size = 8192;    ///< I/O buffer size
    unsigned read_ahead = 0;      ///< Maximum number of extra read-ahead operations
    ::io_priority_class io_priority_class = default_priority_class();
    std::shared_ptr<file_input_stream_history> dynamic_adjustments = { }; ///< Input stream history, if null dynamic adjustments are disabled
};

/// \brief Creates an input_stream to read a portion of a file.
///
/// \param file File to read; multiple streams for the same file may coexist
/// \param offset Starting offset to read from (no alignment restrictions)
/// \param len Maximum number of bytes to read; the stream will stop at end-of-file
///            even if `offset + len` is beyond end-of-file.
/// \param options A set of options controlling the stream.
///
/// \note Multiple input streams may exist concurrently for the same file.
input_stream<char> make_file_input_stream(
        file file, uint64_t offset, uint64_t len, file_input_stream_options options = {});

// Create an input_stream for a given file, with the specified options.
// Multiple fibers of execution (continuations) may safely open
// multiple input streams concurrently for the same file.
input_stream<char> make_file_input_stream(
        file file, uint64_t offset, file_input_stream_options = {});

// Create an input_stream for reading starting at a given position of the
// given file. Multiple fibers of execution (continuations) may safely open
// multiple input streams concurrently for the same file.
input_stream<char> make_file_input_stream(
        file file, file_input_stream_options = {});

struct file_output_stream_options {
    unsigned buffer_size = 8192;
    unsigned preallocation_size = 1024*1024; // 1MB
    unsigned write_behind = 1; ///< Number of buffers to write in parallel
    ::io_priority_class io_priority_class = default_priority_class();
};

// Create an output_stream for writing starting at the position zero of a
// newly created file.
// NOTE: flush() should be the last thing to be called on a file output stream.
output_stream<char> make_file_output_stream(file file,uint64_t buffer_size = 8192);

/// Create an output_stream for writing starting at the position zero of a
/// newly created file.
/// NOTE: flush() should be the last thing to be called on a file output stream.

output_stream<char> make_file_output_stream(file file, file_output_stream_options options);

class thread_pool {
    uint64_t _aio_threaded_fallbacks = 0;
    // FIXME: implement using reactor_notifier abstraction we used for SMP
    syscall_work_queue inter_thread_wq;
    posix_thread _worker_thread;
    std::atomic<bool> _stopped = { false };
    std::atomic<bool> _main_thread_idle = { false };
    pthread_t _notify;
public:
    explicit thread_pool(std::string thread_name);
    ~thread_pool();
    template <typename T, typename Func>
    future<T> submit(Func func) {
        ++_aio_threaded_fallbacks;
        return inter_thread_wq.submit<T>(std::move(func));
    }
    uint64_t operation_count() const { return _aio_threaded_fallbacks; }
    unsigned complete() { return inter_thread_wq.complete(); }
    // Before we enter interrupt mode, we must make sure that the syscall thread will properly
    // generate signals to wake us up. This means we need to make sure that all modifications to
    // the pending and completed fields in the inter_thread_wq are visible to all threads.
    // Simple release-acquire won't do because we also need to serialize all writes that happens
    // before the syscall thread loads this value, so we'll need full seq_cst.
    void enter_interrupt_mode() { _main_thread_idle.store(true, std::memory_order_seq_cst); }
    // When we exit interrupt mode, however, we can safely used relaxed order. If any reordering
    // takes place, we'll get an extra signal and complete will be called one extra time, which is
    // harmless.
    void exit_interrupt_mode() { _main_thread_idle.store(false, std::memory_order_relaxed); }
    void work(std::string thread_name);
};



class reactor_backend {
public:
    virtual ~reactor_backend() {};
    // wait_and_process() waits for some events to become available, and
    // processes one or more of them. If block==false, it doesn't wait,
    // and just processes events that have already happened, if any.
    // After the optional wait, just before processing the events, the
    // pre_process() function is called.
    virtual bool wait_and_process(int timeout = -1, const sigset_t* active_sigmask = nullptr) = 0;
    // Methods that allow polling on file descriptors. This will only work on
    // reactor_backend_epoll. Other reactor_backend will probably abort if
    // they are called (which is fine if no file descriptors are waited on):
    virtual future<> readable(pollable_fd_state& fd) = 0;
    virtual future<> writeable(pollable_fd_state& fd) = 0;
    virtual void forget(pollable_fd_state& fd) = 0;
    // Methods that allow polling on a reactor_notifier. This is currently
    // used only for reactor_backend_osv, but in the future it should really
    // replace the above functions.
    virtual future<> notified(reactor_notifier *n) = 0;
    // Methods for allowing sending notifications events between threads.
    virtual std::unique_ptr<reactor_notifier> make_reactor_notifier() = 0;
};

class reactor_backend_epoll : public reactor_backend {
private:
    file_desc _epollfd;
    future<> get_epoll_future(pollable_fd_state& fd,
            promise<> pollable_fd_state::* pr, int event);
    void complete_epoll_event(pollable_fd_state& fd,
            promise<> pollable_fd_state::* pr, int events, int event);
    void abort_fd(pollable_fd_state& fd, std::exception_ptr ex,
            promise<> pollable_fd_state::* pr, int event);
public:
    reactor_backend_epoll();
    virtual ~reactor_backend_epoll() override { }
    virtual bool wait_and_process(int timeout, const sigset_t* active_sigmask) override;
    virtual future<> readable(pollable_fd_state& fd) override;
    virtual future<> writeable(pollable_fd_state& fd) override;
    virtual void forget(pollable_fd_state& fd) override;
    virtual future<> notified(reactor_notifier *n) override;
    virtual std::unique_ptr<reactor_notifier> make_reactor_notifier() override;
    void abort_reader(pollable_fd_state& fd, std::exception_ptr ex);
    void abort_writer(pollable_fd_state& fd, std::exception_ptr ex);
};

class reactor_notifier_epoll : public reactor_notifier {
    writeable_eventfd _write;
    readable_eventfd _read;
public:
    reactor_notifier_epoll()
        : _write()
        , _read(_write.read_side()) {
    }
    virtual future<> wait() override {
        // convert _read.wait(), a future<size_t>, to a future<>:
        return _read.wait().then([] (size_t ignore) {
            return make_ready_future<>();
        });
    }
    virtual void signal() override {
        _write.signal(1);
    }
};
enum class open_flags {
    rw = O_RDWR,
    ro = O_RDONLY,
    wo = O_WRONLY,
    create = O_CREAT,
    truncate = O_TRUNC,
    exclusive = O_EXCL,
};

inline open_flags operator|(open_flags a, open_flags b) {
    return open_flags(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

extern __thread reactor* local_engine;
reactor& engine();


struct reactor {
    struct poller {
        std::unique_ptr<pollfn> _pollfn;
        class registration_task;
        class deregistration_task;
        registration_task* _registration_task;
    public:
        template <typename Func> // signature: bool ()
        static poller simple(Func&& poll) {
            return poller(make_pollfn(std::forward<Func>(poll)));
        }
        poller(std::unique_ptr<pollfn> fn)
                : _pollfn(std::move(fn)) {
            do_register();
        }
        ~poller();
        poller(poller&& x);
        poller& operator=(poller&& x);
        void do_register();
        friend class reactor;
    };

    struct io_stats {
        uint64_t aio_reads = 0;
        uint64_t aio_read_bytes = 0;
        uint64_t aio_writes = 0;
        uint64_t aio_write_bytes = 0;
        uint64_t fstream_reads = 0;
        uint64_t fstream_read_bytes = 0;
        uint64_t fstream_reads_blocked = 0;
        uint64_t fstream_read_bytes_blocked = 0;
        uint64_t fstream_read_aheads_discarded = 0;
        uint64_t fstream_read_ahead_discarded_bytes = 0;
    };
    io_stats _io_stats;
    uint64_t _fsyncs = 0;
    enum class idle_cpu_handler_result {
        no_more_work,
        interrupted_by_higher_priority_task
    };
/*---------构造函数和析构函数----------------*/
    // reactor();
    reactor(unsigned int id);
    ~reactor();
    reactor(const reactor&) = delete;
    reactor& operator=(const reactor&) = delete;
    unsigned _id = 0;
    std::deque<double> _loads;
    double _load = 0;
/*----------定时器相关------------------------*/
    steady_clock_type::duration _total_idle;
    std::unique_ptr<lowres_clock> _lowres_clock;
    lowres_clock::time_point _lowres_next_timeout;
    timer_t _steady_clock_timer = {};
    timer_set<timer<steady_clock_type>> _timers;
    typename timer_set<timer<steady_clock_type>>::timer_list_t _expired_timers;
    timer_set<timer<lowres_clock>> _lowres_timers;
    typename timer_set<timer<lowres_clock>>::timer_list_t _expired_lowres_timers;
    using steady_timer = timer<steady_clock_type>;
    using lowres_timer = timer<lowres_clock>;
    file_desc _task_quota_timer;
    std::optional<::pollable_fd> _aio_eventfd;
    std::optional<reactor::poller> _epoll_poller;
/*---------------------------------------------*/
    void add_timer(steady_timer* tmr);
    bool queue_timer(steady_timer* tmr);
    void del_timer(steady_timer* tmr);
    void add_timer(lowres_timer* tmr);
    bool queue_timer(lowres_timer* tmr);
    void del_timer(lowres_timer* tmr);
    void enable_timer(steady_clock_type::time_point when);
    bool do_expire_lowres_timers();
    template <typename T, typename E, typename EnableFunc>
    void complete_timers(T&, E&, EnableFunc&& enable_fn);
    bool do_check_lowres_timers() const;
/*---------------信号处理相关------------------------*/
    signals _signals;
    bool _handle_sigint = true;
    static void block_notifier(int);
/*----------任务相关-------------------*/
    bool _stopping = false;
    bool _stopped = false;
    int _return = 0;
    unsigned _tasks_processed_report_threshold;
    std::chrono::duration<double> _task_quota;
    condition_variable _stop_requested;
    std::atomic<bool> _sleeping alignas(64);
    std::atomic<uint64_t> _tasks_processed = { 0 };
    std::atomic<uint64_t> _polls = { 0 };
    std::atomic<unsigned> _tasks_processed_stalled = { 0 };
    std::deque<std::unique_ptr<task>> _pending_tasks;
    std::deque<std::unique_ptr<task>> _at_destroy_tasks;
    void run_tasks(std::deque<std::unique_ptr<task>>& tasks);
    std::vector<std::function<future<> ()>> _exit_funcs;//为什么不用引用?
    void add_task(std::unique_ptr<task>&& t) { _pending_tasks.push_back(std::move(t)); }
    void add_urgent_task(std::unique_ptr<task>&& t) { _pending_tasks.push_front(std::move(t)); }
    void add_high_priority_task(std::unique_ptr<task>&& t){
            _pending_tasks.push_front(std::move(t));
            // break .then() chains
            g_need_preempt = true;
    }
    void sleep();
    void force_poll() {
        g_need_preempt = true;
    }
    template <typename Rep, typename Period>
    future<> wait_for_stop(std::chrono::duration<Rep, Period> timeout) {
        return _stop_requested.wait(timeout, [this] { return _stopping; });
    }
    void at_exit(std::function<future<> ()> func);
    void exit(int ret);
    void stop();
    future<> run_exit_tasks();
    semaphore _io_context_available;
    static constexpr size_t max_aio = 128;
    semaphore _cpu_started;
    promise<> _start_promise;
    future<> when_started() {
        std::cout<<"when_started" << std::endl;//这里被执行.
        return _start_promise.get_future();
    }
    /*---------- 全局--------------*/
    int run();
    /*----------配置相关----------------*/
    static boost::program_options::options_description get_options_description();
    void configure(boost::program_options::variables_map config);
   /*----------------------资源分配相关-----------------------*/
    shard_id _io_coordinator;
    io_queue* _io_queue;
    std::unique_ptr<io_queue> my_io_queue = {};
    pthread_t _thread_id alignas(64) = pthread_self();
    shard_id cpu_id() const { return _id; }
    void wakeup() { pthread_kill(_thread_id, alarm_signal());}
    std::chrono::nanoseconds calculate_poll_time();
    /*-----------其他---------------*/
    sigset_t _active_sigmask; // holds sigmask while sleeping with sig disabled
    unsigned _max_task_backlog = 1000;
    std::chrono::nanoseconds _max_poll_time = calculate_poll_time();
    bool _strict_o_direct = true;
    using work_waiting_on_reactor = const std::function<bool()>&;
    using idle_cpu_handler = std::function<idle_cpu_handler_result(work_waiting_on_reactor)>;
    idle_cpu_handler _idle_cpu_handler{ [] (work_waiting_on_reactor) {return idle_cpu_handler_result::no_more_work;} };
    /*----------------------poller相关----------------------------------------------------*/
    std::vector<pollfn*> _pollers;
    thread_pool _thread_pool;
    reactor_backend_epoll _backend;
    template <typename Func> // signature: bool ()
    static std::unique_ptr<pollfn> make_pollfn(Func&& func);
    void unregister_poller(pollfn* p);
    void register_poller(pollfn* p);
    void replace_poller(pollfn* old, pollfn* neww);
    class io_pollfn;
    class signal_pollfn;
    class aio_batch_submit_pollfn;
    class batch_flush_pollfn;
    class smp_pollfn;
    class drain_cross_cpu_freelist_pollfn;
    class lowres_timer_pollfn;
    class manual_timer_pollfn;
    class epoll_pollfn;
    class syscall_pollfn;
    class execution_stage_pollfn;
    bool poll_once();
    bool pure_poll_once();
    bool flush_tcp_batches();
    bool flush_pending_aio();
    io_priority_class register_one_priority_class(std::string name, uint32_t shares) {
        return io_queue::register_one_priority_class(std::move(name), shares);
    }
    /*----------------------------------IO相关--------------------------------------------*/
    std::deque<output_stream<char>* > _flush_batching;
    io_context_t _io_context;
    std::vector<struct ::iocb> _pending_aio;
    bool process_io();
    void start_epoll();
    void start_aio_eventfd_loop();
    server_socket listen(net::socket_address sa, listen_options opts = {});
    future<connected_socket> connect(net::socket_address sa);
    future<connected_socket> connect(net::socket_address, net::socket_address, transport proto = transport::TCP);
    pollable_fd posix_listen(net::socket_address sa, listen_options opts = {});
    bool posix_reuseport_available() const { return _reuseport; }
    std::shared_ptr<pollable_fd> make_pollable_fd(net::socket_address sa, transport proto = transport::TCP);
    future<> posix_connect(std::shared_ptr<pollable_fd> pfd, net::socket_address sa, net::socket_address local);
    future<pollable_fd, net::socket_address> accept(pollable_fd_state& listen_fd);
    future<size_t> read_some(pollable_fd_state& fd, void* buffer, size_t size);
    future<size_t> read_some(pollable_fd_state& fd, const std::vector<iovec>& iov);
    future<size_t> write_some(pollable_fd_state& fd, const void* buffer, size_t size);
    future<> write_all(pollable_fd_state& fd, const void* buffer, size_t size);
    future<file> open_file_dma(std::string name, open_flags flags, file_open_options options = {});
    future<file> open_directory(std::string name);
    future<> make_directory(std::string name);
    future<> touch_directory(std::string name);
    future<std::optional<directory_entry_type>>  file_type(std::string name);
    future<uint64_t> file_size(std::string pathname);
    future<bool> file_exists(std::string pathname);
    future<fs_type> file_system_at(std::string pathname);
    future<> remove_file(std::string pathname);
    future<> rename_file(std::string old_pathname, std::string new_pathname);
    future<> link_file(std::string oldpath, std::string newpath);
    // In the following three methods, prepare_io is not guaranteed to execute in the same processor
    // in which it was generated. Therefore, care must be taken to avoid the use of objects that could
    // be destroyed within or at exit of prepare_io.
    template <typename Func>
    future<io_event> submit_io(Func prepare_io);
    template <typename Func>
    future<io_event> submit_io_read(const io_priority_class& priority_class, size_t len, Func prepare_io);
    template <typename Func>
    future<io_event> submit_io_write(const io_priority_class& priority_class, size_t len, Func prepare_io);
    bool wait_and_process(int timeout = 0, const sigset_t* active_sigmask = nullptr) {
        return _backend.wait_and_process(timeout, active_sigmask);
    }
    future<> readable(pollable_fd_state& fd) {
        return _backend.readable(fd);
    }
    future<> writeable(pollable_fd_state& fd) {
        return _backend.writeable(fd);
    }
    void forget(pollable_fd_state& fd) {
        _backend.forget(fd);
    }
    future<> notified(reactor_notifier *n) {
        return _backend.notified(n);
    }
    void abort_reader(pollable_fd_state& fd, std::exception_ptr ex) {
        return _backend.abort_reader(fd, std::move(ex));
    }
    void abort_writer(pollable_fd_state& fd, std::exception_ptr ex) {
        return _backend.abort_writer(fd, std::move(ex));
    }
    std::unique_ptr<reactor_notifier> make_reactor_notifier() {
        return _backend.make_reactor_notifier();
    }
    /// Sets the "Strict DMA" flag.
    /// When true (default), file I/O operations must use DMA.  This is
    /// the most performant option, but does not work on some file systems
    /// such as tmpfs or aufs (used in some Docker setups)
    /// When false, file I/O operations can fall back to buffered I/O if
    /// DMA is not available.  This can result in dramatic reducation in
    /// performance and an increase in memory consumption.
    void set_strict_dma(bool value) {
        _strict_o_direct = value;
    }
    future<> write_all_part(pollable_fd_state& fd, const void* buffer, size_t size, size_t completed);
    /*-------------------------------------网络相关--------------------------------------------------------------------*/
    const bool _reuseport;
    bool posix_reuseport_detect();
    std::unique_ptr<network_stack> _network_stack;
    promise<std::unique_ptr<network_stack>> _network_stack_ready_promise;
    network_stack& net() { return *_network_stack; }
};




class file_data_source_impl : public data_source_impl {
    struct issued_read {
        uint64_t _pos;
        uint64_t _size;
        future<temporary_buffer<char>> _ready;

        issued_read(uint64_t pos, uint64_t size, future<temporary_buffer<char>> f)
            : _pos(pos), _size(size), _ready(std::move(f)) { }
    };

    reactor& _reactor = engine();
    file _file;
    file_input_stream_options _options;
    uint64_t _pos;
    uint64_t _remain;
    std::deque<issued_read> _read_buffers;
    unsigned _reads_in_progress = 0;
    unsigned _current_read_ahead;
    future<> _dropped_reads = make_ready_future<>();
    std::optional<promise<>> _done;
    size_t _current_buffer_size;
    bool _in_slow_start = false;
    using unused_ratio_target = std::ratio<25, 100>;
private:
    size_t minimal_buffer_size() const {
        return std::min(std::max(_options.buffer_size / 4, size_t(8192)), _options.buffer_size);
    }

    void try_increase_read_ahead() {
        // Read-ahead can be increased up to user-specified limit if the
        // consumer has to wait for a buffer and we are not in a slow start
        // phase.
        if (_current_read_ahead < _options.read_ahead && !_in_slow_start) {
            _current_read_ahead++;
            if (_options.dynamic_adjustments) {
                auto& h = *_options.dynamic_adjustments;
                h.read_ahead = std::max(h.read_ahead, _current_read_ahead);
            }
        }
    }
    unsigned get_initial_read_ahead() const {
        return _options.dynamic_adjustments
               ? std::min(_options.dynamic_adjustments->read_ahead, _options.read_ahead)
               : !!_options.read_ahead;
    }

    void update_history(uint64_t unused, uint64_t total) {
        // We are maintaining two windows each no larger than window_size.
        // Dynamic adjustment logic uses data from both of them, which
        // essentially means that the actual window size is variable and
        // in the range [window_size, 2*window_size].
        auto& h = *_options.dynamic_adjustments;
        h.current_window.total_read += total;
        h.current_window.unused_read += unused;
        if (h.current_window.total_read >= h.window_size) {
            h.previous_window = h.current_window;
            h.current_window = { };
        }
    }
    static bool below_target(uint64_t unused, uint64_t total) {
        return unused * unused_ratio_target::den < total * unused_ratio_target::num;
    }
    void update_history_consumed(uint64_t bytes) {
        if (!_options.dynamic_adjustments) {
            return;
        }
        update_history(0, bytes);
        if (!_in_slow_start) {
            return;
        }
        unsigned new_size = std::min(_current_buffer_size * 2, _options.buffer_size);
        auto& h = *_options.dynamic_adjustments;
        auto total = h.current_window.total_read + h.previous_window.total_read + new_size;
        auto unused = h.current_window.unused_read + h.previous_window.unused_read + new_size;
        // Check whether we can safely increase the buffer size to new_size
        // and still be below unused_ratio_target even if it is entirely
        // dropped.
        if (below_target(unused, total)) {
            _current_buffer_size = new_size;
            _in_slow_start = _current_buffer_size < _options.buffer_size;
        }
    }

    using after_skip = bool_class<class after_skip_tag>;
    void set_new_buffer_size(after_skip skip) {
        if (!_options.dynamic_adjustments) {
            return;
        }
        auto& h = *_options.dynamic_adjustments;
        int64_t total = h.current_window.total_read + h.previous_window.total_read;
        int64_t unused = h.current_window.unused_read + h.previous_window.unused_read;
        if (skip == after_skip::yes && below_target(unused, total)) {
            // Do not attempt to shrink buffer size if we are still below the
            // target. Otherwise, we could get a bad interaction with
            // update_history_consumed() which tries to increase the buffer
            // size as much as possible so that after a single drop we are
            // still below the target.
            return;
        }
        // Calculate the maximum buffer size that would guarantee that we are
        // still below unused_ratio_target even if the subsequent reads are
        // dropped. If it is larger than or equal to the current buffer size do
        // nothing. If it is smaller then we are back in the slow start phase.
        auto new_target = (unused_ratio_target::num * total - unused_ratio_target::den * unused) / (unused_ratio_target::den - unused_ratio_target::num);
        uint64_t new_size = std::max(new_target, int64_t(minimal_buffer_size()));
        new_size = std::max(uint64_t(1) << log2floor(new_size), uint64_t(minimal_buffer_size()));
        if (new_size >= _current_buffer_size) {
            return;
        }
        _in_slow_start = true;
        _current_read_ahead = std::min(_current_read_ahead, 1u);
        _current_buffer_size = new_size;
    }
    void update_history_unused(uint64_t bytes) {
        if (!_options.dynamic_adjustments) {
            return;
        }
        update_history(bytes, bytes);
        set_new_buffer_size(after_skip::yes);
    }
public:
    file_data_source_impl(file f, uint64_t offset, uint64_t len, file_input_stream_options options)
            : _file(std::move(f)), _options(options), _pos(offset), _remain(len), _current_read_ahead(get_initial_read_ahead())
            , _current_buffer_size(_options.buffer_size) {
        // prevent wraparounds
        set_new_buffer_size(after_skip::no);
        _remain = std::min(std::numeric_limits<uint64_t>::max() - _pos, _remain);
    }
    virtual future<temporary_buffer<char>> get() override {
        if (!_read_buffers.empty() && !_read_buffers.front()._ready.available()) {
            try_increase_read_ahead();
        }
        issue_read_aheads(1);
        auto ret = std::move(_read_buffers.front());
        _read_buffers.pop_front();
        update_history_consumed(ret._size);
        _reactor._io_stats.fstream_reads += 1;
        _reactor._io_stats.fstream_read_bytes += ret._size;
        if (!ret._ready.available()) {
            _reactor._io_stats.fstream_reads_blocked += 1;
            _reactor._io_stats.fstream_read_bytes_blocked += ret._size;
        }
        return std::move(ret._ready);
    }
    virtual future<temporary_buffer<char>> skip(uint64_t n) override {
        uint64_t dropped = 0;
        while (n) {
            if (_read_buffers.empty()) {
                assert(n <= _remain);
                _pos += n;
                _remain -= n;
                break;
            }
            auto& front = _read_buffers.front();
            if (n < front._size) {
                front._size -= n;
                front._pos += n;
                front._ready = front._ready.then([n] (temporary_buffer<char> buf) {
                    buf.trim_front(n);
                    return buf;
                });
                break;
            } else {
                auto f = front._ready.then_wrapped([] (auto f) { f.ignore_ready_future(); });
                _dropped_reads = _dropped_reads.then([f = std::move(f)] () mutable { return std::move(f); });
                n -= front._size;
                dropped += front._size;
                _reactor._io_stats.fstream_read_aheads_discarded += 1;
                _reactor._io_stats.fstream_read_ahead_discarded_bytes += front._size;
                _read_buffers.pop_front();
            }
        }
        update_history_unused(dropped);
        return make_ready_future<temporary_buffer<char>>();
    }
    virtual future<> close() {
        _done.emplace();
        if (!_reads_in_progress) {
            _done->set_value();
        }
        return _done->get_future().then([this] {
            uint64_t dropped = 0;
            for (auto&& c : _read_buffers) {
                _reactor._io_stats.fstream_read_aheads_discarded += 1;
                _reactor._io_stats.fstream_read_ahead_discarded_bytes += c._size;
                dropped += c._size;
                c._ready.ignore_ready_future();
            }
            update_history_unused(dropped);
            return std::move(_dropped_reads);
        });
    }
private:
    void issue_read_aheads(unsigned additional = 0) {
        if (_done) {
            return;
        }
        auto ra = _current_read_ahead + additional;
        // _read_buffers.reserve(ra); // prevent push_back() failure
        while (_read_buffers.size() < ra) {
            if (!_remain) {
                if (_read_buffers.size() >= additional) {
                    return;
                }
                _read_buffers.emplace_back(_pos, 0, make_ready_future<temporary_buffer<char>>());
                continue;
            }
            ++_reads_in_progress;
            // if _pos is not dma-aligned, we'll get a short read.  Account for that.
            // Also avoid reading beyond _remain.
            uint64_t align = _file.disk_read_dma_alignment();
            auto start = align_down(_pos, align);
            auto end = std::min(align_up(start + _current_buffer_size, align), _pos + _remain);
            auto len = end - start;
            auto actual_size = std::min(end - _pos, _remain);
            _read_buffers.emplace_back(_pos, actual_size, futurize<future<temporary_buffer<char>>>::apply([&] {
                    return _file.dma_read_bulk<char>(start, len, _options.io_priority_class);
            }).then_wrapped(
                    [this, start, pos = _pos, remain = _remain] (future<temporary_buffer<char>> ret) {
                --_reads_in_progress;
                if (_done && !_reads_in_progress) {
                    _done->set_value();
                }
                if (ret.failed()) {
                    // no games needed
                    return ret;
                } else {
                    // first or last buffer, need trimming
                    auto tmp = ret.get0();
                    auto real_end = start + tmp.size();
                    if (real_end <= pos) {
                        return make_ready_future<temporary_buffer<char>>();
                    }
                    if (real_end > pos + remain) {
                        tmp.trim(pos + remain - start);
                    }
                    if (start < pos) {
                        tmp.trim_front(pos - start);
                    }
                    return make_ready_future<temporary_buffer<char>>(std::move(tmp));
                }
            }));
            _remain -= end - _pos;
            _pos = end;
        };
    }
};

class file_data_source : public data_source {
public:
    file_data_source(file f, uint64_t offset, uint64_t len, file_input_stream_options options)
        : data_source(std::make_unique<file_data_source_impl>(
                std::move(f), offset, len, options)) {}
};







#endif //FUTURE_ALL12_HH
