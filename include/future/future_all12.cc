#pragma once

#include "future_all12.hh"
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/fs.h>   // 或者适当的头文件来定义BLKGETSIZE
#include <sys/statfs.h> // 为fstatfs提供声明
#include "../task/task.hh"
#include <sys/vfs.h>
#include <linux/magic.h>
#include <stdexcept>
#include <atomic>
#include <memory>
#include <utility>
#include <regex>
#include <tuple>
#include <type_traits>
#include "../util/shared_ptr.hh"
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

#include <setjmp.h>
#include <ucontext.h>


#include <list>
#include "../resource/resource.hh"
#include <chrono>
#include <limits>
#include <bitset>
#include <array>
#include <variant>
#include <atomic>
#include <list>
#include <deque>
#include <unordered_map>

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
#include <atomic>
#include "../util/align.hh"
#include "../util/spinlock.hh"
#include "distributed.hh"
#include "stream.hh"
#include "queue_.hh"
#include "net.hh"
#include "smp.hh"




inline size_t iovec_len(const iovec* begin, size_t len){
    size_t ret = 0;
    auto end = begin + len;
    while (begin != end) {
        ret += begin++->iov_len;
    }
    return ret;
}

/// Wraps reference in a reference_wrapper
template<typename T>
inline reference_wrapper<T> ref(T& object) noexcept {
    return reference_wrapper<T>(object);
}
/// Wraps constant reference in a reference_wrapper
template<typename T>
inline reference_wrapper<const T> cref(const T& object) noexcept {
    return reference_wrapper<const T>(object);
}

__thread bool g_need_preempt;
inline bool need_preempt() {
    return true;
    // prevent compiler from eliminating loads in a loop
    std::atomic_signal_fence(std::memory_order_seq_cst);
    return g_need_preempt;
}

void systemwide_memory_barrier() {
    // FIXME: use sys_membarrier() when available
    static thread_local char* mem = [] {
       void* mem = mmap(nullptr, getpagesize(),
               PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_ANONYMOUS,
               -1, 0) ;
       assert(mem != MAP_FAILED);
       return reinterpret_cast<char*>(mem);
    }();
    int r1 = mprotect(mem, getpagesize(), PROT_READ | PROT_WRITE);
    assert(r1 == 0);
    // Force page into memory to avoid next mprotect() attempting to be clever
    *mem = 3;
    // Force page into memory
    // lower permissions to force kernel to send IPI to all threads, with
    // a side effect of executing a memory barrier on those threads
    // FIXME: does this work on ARM?
    int r2 = mprotect(mem, getpagesize(), PROT_READ);
    assert(r2 == 0);
}





using namespace std::chrono_literals;
std::ostream& operator<<(std::ostream& os, const std::chrono::steady_clock::time_point& tp) {
    auto duration = tp.time_since_epoch();
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    os << hours.count() << "h " << minutes.count() << "m " << seconds.count() << "s";
    return os;
}

inline int block_notifier_signal() {
    return SIGRTMIN + 1;
}






static constexpr size_t  cacheline_size = 64;
template <size_t N, int RW, int LOC>
struct prefetcher;

template<int RW, int LOC>
struct prefetcher<0, RW, LOC> {
    prefetcher(uintptr_t ptr) {}
};

template <size_t N, int RW, int LOC>
struct prefetcher {
    prefetcher(uintptr_t ptr) {
        __builtin_prefetch(reinterpret_cast<void*>(ptr), RW, LOC);
        std::atomic_signal_fence(std::memory_order_seq_cst);
        prefetcher<N-64, RW, LOC>(ptr + 64);
    }
};
template<typename T, int LOC = 3>
void prefetch(T* ptr) {
    prefetcher<align_up(sizeof(T), cacheline_size), 0, LOC>(reinterpret_cast<uintptr_t>(ptr));
}

template<typename Iterator, int LOC = 3>
void prefetch(Iterator begin, Iterator end) {
    std::for_each(begin, end, [] (auto v) { prefetch<decltype(*v), LOC>(v); });
}

template<size_t C, typename T, int LOC = 3>
void prefetch_n(T** pptr) {
    boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetch<T, LOC>(*(pptr + x)); } );
}

template<size_t L, int LOC = 3>
void prefetch(void* ptr) {
    prefetcher<L*cacheline_size, 0, LOC>(reinterpret_cast<uintptr_t>(ptr));
}

template<size_t L, typename Iterator, int LOC = 3>
void prefetch_n(Iterator begin, Iterator end) {
    std::for_each(begin, end, [] (auto v) { prefetch<L, LOC>(v); });
}

template<size_t L, size_t C, typename T, int LOC = 3>
void prefetch_n(T** pptr) {
    boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetch<L, LOC>(*(pptr + x)); } );
}

template<typename T, int LOC = 3>
void prefetchw(T* ptr) {
    prefetcher<align_up(sizeof(T), cacheline_size), 1, LOC>(reinterpret_cast<uintptr_t>(ptr));
}
template<typename Iterator, int LOC = 3>
void prefetchw_n(Iterator begin, Iterator end) {
    std::for_each(begin, end, [] (auto v) { prefetchw<decltype(*v), LOC>(v); });
}
template<size_t C, typename T, int LOC = 3>
void prefetchw_n(T** pptr) {
    boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetchw<T, LOC>(*(pptr + x)); } );
}
template<size_t L, int LOC = 3>
void prefetchw(void* ptr) {
    prefetcher<L*cacheline_size, 1, LOC>(reinterpret_cast<uintptr_t>(ptr));
}
template<size_t L, typename Iterator, int LOC = 3>
void prefetchw_n(Iterator begin, Iterator end) {
   std::for_each(begin, end, [] (auto v) { prefetchw<L, LOC>(v); });
}

template<size_t L, size_t C, typename T, int LOC = 3>
void prefetchw_n(T** pptr) {
    boost::mpl::for_each< boost::mpl::range_c<size_t,0,C> >( [pptr] (size_t x) { prefetchw<L, LOC>(*(pptr + x)); } );
}



template <typename T>
struct syscall_result {
    T result;
    int error;
    void throw_if_error() {
        if (long(result) == -1) {
            throw std::system_error(error, std::system_category());
        }
    }
};

// Wrapper for a system call result containing the return value,
// an output parameter that was returned from the syscall, and errno.
template <typename Extra>
struct syscall_result_extra {
    int result;
    Extra extra;
    int error;
    void throw_if_error() {
        if (result == -1) {
            throw std::system_error(error, std::system_category());
        }
    }
};

template <typename T>
syscall_result<T>
wrap_syscall(T result) {
    syscall_result<T> sr;
    sr.result = result;
    sr.error = errno;
    return sr;
}

template <typename Extra>
syscall_result_extra<Extra>
wrap_syscall(int result, const Extra& extra) {
    return {result, extra, errno};
}













inline
sigset_t make_empty_sigset_mask() {
    sigset_t set;
    sigemptyset(&set);
    return set;
}


inline
sigset_t make_sigset_mask(int signo) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signo);
    return set;
}

template <typename T>
inline
void throw_pthread_error(T r) {
    if (r != 0) {
        throw std::system_error(r, std::system_category());
    }
}















class reactor;
reactor& engine();
template <typename Clock> class timer;
using steady_clock_type = std::chrono::steady_clock;
void schedule_normal(std::unique_ptr<task> t);
void schedule_urgent(std::unique_ptr<task> t);

/*---------------------------------------------------posix相关-------------------------------------------------------*/
namespace posix {

template <typename Rep, typename Period>
struct timespec to_timespec(std::chrono::duration<Rep, Period> d) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    struct timespec ts {};
    ts.tv_sec = ns / 1000000000;
    ts.tv_nsec = ns % 1000000000;
    return ts;
}

template <typename Rep1, typename Period1, typename Rep2, typename Period2>
struct itimerspec
to_relative_itimerspec(std::chrono::duration<Rep1, Period1> base, std::chrono::duration<Rep2, Period2> interval) {
    struct itimerspec its {};
    its.it_interval = to_timespec(interval);
    its.it_value = to_timespec(base);
    return its;
}

template <typename Clock, class Duration, class Rep, class Period>
struct itimerspec
to_absolute_itimerspec(std::chrono::time_point<Clock, Duration> base, std::chrono::duration<Rep, Period> interval) {
    return to_relative_itimerspec(base.time_since_epoch(), interval);
}
}






namespace internal {

// Execution wraps lreferences in reference_wrapper so that the caller is forced
// to use seastar::ref(). Then when the function is actually called the
// reference is unwrapped. However, we need to distinguish between functions
// which argument is lvalue reference and functions that take
// reference_wrapper<> as an argument and not unwrap the latter. To solve this
// issue reference_wrapper_for_es type is used for wrappings done automatically
// by execution stage.
template<typename T>
struct reference_wrapper_for_es : reference_wrapper<T> {
    reference_wrapper_for_es(reference_wrapper <T> rw) noexcept
        : reference_wrapper<T>(std::move(rw)) {}
};

template<typename T>
struct wrap_for_es {
    using type = T;
};

template<typename T>
struct wrap_for_es<T&> {
    using type = reference_wrapper_for_es<T>;
};

template<typename T>
struct wrap_for_es<T&&> {
    using type = T;
};

template<typename T>
decltype(auto) unwrap_for_es(T&& object) {
    return std::forward<T>(object);
}

template<typename T>
std::reference_wrapper<T> unwrap_for_es(reference_wrapper_for_es<T> ref) {
    return std::reference_wrapper<T>(ref.get());
}

}
/// \endcond

/// Base execution stage class
class execution_stage {
public:
    struct stats {
        uint64_t tasks_scheduled = 0;
        uint64_t tasks_preempted = 0;
        uint64_t function_calls_enqueued = 0;
        uint64_t function_calls_executed = 0;
    };
protected:
    bool _empty = true;
    bool _flush_scheduled = false;
    stats _stats;
    std::string _name;
    // metrics::metric_group _metric_group;
protected:
    virtual void do_flush() noexcept = 0;
public:
    explicit execution_stage(const sstring& name);
    virtual ~execution_stage();

    execution_stage(const execution_stage&) = delete;

    /// Move constructor
    ///
    /// \warning It is illegal to move execution_stage after any operation has
    /// been pushed to it. The only reason why the move constructor is not
    /// deleted is the fact that C++14 does not guarantee return value
    /// optimisation which is required by make_execution_stage().
    execution_stage(execution_stage&&);

    /// Returns execution stage name
    const sstring& name() const noexcept { return _name; }

    /// Returns execution stage usage statistics
    const stats& get_stats() const noexcept { return _stats; }

    /// Flushes execution stage
    ///
    /// Ensures that a task which would execute all queued operations is
    /// scheduled. Does not schedule a new task if there is one already pending
    /// or the queue is empty.
    ///
    /// \return true if a new task has been scheduled
    bool flush() noexcept {
        if (_empty || _flush_scheduled) {
            return false;
        }
        _stats.tasks_scheduled++;
        schedule_normal(make_task([this] {
            do_flush();
            _flush_scheduled = false;
        }));
        _flush_scheduled = true;
        return true;
    };

    /// Checks whether there are pending operations.
    ///
    /// \return true if there is at least one queued operation
    bool poll() const noexcept {
        return !_empty;
    }
};
/*----------------------------------execution_statge-----------------------------------------------------------------------------*/

/// \cond internal
namespace internal {

class execution_stage_manager {
    std::vector<execution_stage*> _execution_stages;
    std::unordered_map<sstring, execution_stage*> _stages_by_name;
private:
    execution_stage_manager() = default;
    execution_stage_manager(const execution_stage_manager&) = delete;
    execution_stage_manager(execution_stage_manager&&) = delete;
public:
    void register_execution_stage(execution_stage& stage) {
        auto ret = _stages_by_name.emplace(stage.name(), &stage);
        if (!ret.second) {
            throw std::runtime_error("error registering execution stage: name already in use");
        }
        try {
            _execution_stages.push_back(&stage);
        } catch (...) {
            _stages_by_name.erase(stage.name());
            throw;
        }
    }
    void unregister_execution_stage(execution_stage& stage) noexcept {
        auto it = std::find(_execution_stages.begin(), _execution_stages.end(), &stage);
        _execution_stages.erase(it);
        _stages_by_name.erase(stage.name());
    }
    void update_execution_stage_registration(execution_stage& old_es, execution_stage& new_es) noexcept {
        auto it = std::find(_execution_stages.begin(), _execution_stages.end(), &old_es);
        *it = &new_es;
        _stages_by_name.find(new_es.name())->second = &new_es;
    }

    execution_stage* get_stage(const sstring& name) {
        return _stages_by_name[name];
    }

    bool flush() noexcept {
        bool did_work = false;
        for (auto&& stage : _execution_stages) {
            did_work |= stage->flush();
        }
        return did_work;
    }
    bool poll() const noexcept {
        for (auto&& stage : _execution_stages) {
            if (stage->poll()) {
                return true;
            }
        }
        return false;
    }
public:
    static execution_stage_manager& get() noexcept {
        static thread_local execution_stage_manager instance;
        return instance;
    }
};

}



/*----------------------------------tuple_utils-------------------------------------------------------*/



#include <tuple>
#include <utility>

/// \cond internal
namespace internal {

template<typename Tuple>
Tuple untuple(Tuple t) {
    return std::move(t);
}

template<typename T>
T untuple(std::tuple<T> t) {
    return std::get<0>(std::move(t));
}

template<typename Tuple, typename Function, size_t... I>
void tuple_for_each_helper(Tuple&& t, Function&& f, std::index_sequence<I...>&&) {
    auto ignore_me = { (f(std::get<I>(std::forward<Tuple>(t))), 1)... };
    (void)ignore_me;
}

template<typename Tuple, typename MapFunction, size_t... I>
auto tuple_map_helper(Tuple&& t, MapFunction&& f, std::index_sequence<I...>&&) {
    return std::make_tuple(f(std::get<I>(std::forward<Tuple>(t)))...);
}

template<size_t I, typename IndexSequence>
struct prepend;

template<size_t I, size_t... Is>
struct prepend<I, std::index_sequence<Is...>> {
    using type = std::index_sequence<I, Is...>;
};

template<template<typename> class Filter, typename Tuple, typename IndexSequence>
struct tuple_filter;

template<template<typename> class Filter, typename T, typename... Ts, size_t I, size_t... Is>
struct tuple_filter<Filter, std::tuple<T, Ts...>, std::index_sequence<I, Is...>> {
    using tail = typename tuple_filter<Filter, std::tuple<Ts...>, std::index_sequence<Is...>>::type;
    using type = std::conditional_t<Filter<T>::value, typename prepend<I, tail>::type, tail>;
};

template<template<typename> class Filter>
struct tuple_filter<Filter, std::tuple<>, std::index_sequence<>> {
    using type = std::index_sequence<>;
};
template<typename Tuple, size_t... I>
auto tuple_filter_helper(Tuple&& t, std::index_sequence<I...>&&) {
    return std::make_tuple(std::get<I>(std::forward<Tuple>(t))...);
}
/// \addtogroup utilities
/// @{

/// Applies type transformation to all types in tuple
///
/// Member type `type` is set to a tuple type which is a result of applying
/// transformation `MapClass<T>::type` to each element `T` of the input tuple
/// type.
///
/// \tparam MapClass class template defining type transformation
/// \tparam Tuple input tuple type
template<template<typename> class MapClass, typename Tuple>
struct tuple_map_types;

/// @}

template<template<typename> class MapClass, typename... Elements>
struct tuple_map_types<MapClass, std::tuple<Elements...>> {
    using type = std::tuple<typename MapClass<Elements>::type...>;
};

/// \addtogroup utilities
/// @{

/// Filters elements in tuple by their type
///
/// Returns a tuple containing only those elements which type `T` caused
/// expression `FilterClass<T>::value` to be true.
///
/// \tparam FilterClass class template having an element value set to true for elements that
///                     should be present in the result
/// \param t tuple to filter
/// \return a tuple contaning elements which type passed the test
template<template<typename> class FilterClass, typename... Elements>
auto tuple_filter_by_type(const std::tuple<Elements...>& t) {
    using sequence = typename internal::tuple_filter<FilterClass, std::tuple<Elements...>,
                                                     std::index_sequence_for<Elements...>>::type;
    return internal::tuple_filter_helper(t, sequence());
}
template<template<typename> class FilterClass, typename... Elements>
auto tuple_filter_by_type(std::tuple<Elements...>&& t) {
    using sequence = typename internal::tuple_filter<FilterClass, std::tuple<Elements...>,
                                                     std::index_sequence_for<Elements...>>::type;
    return internal::tuple_filter_helper(std::move(t), sequence());
}

/// Applies function to all elements in tuple
///
/// Applies given function to all elements in the tuple and returns a tuple
/// of results.
///
/// \param t original tuple
/// \param f function to apply
/// \return tuple of results returned by f for each element in t
template<typename Function, typename... Elements>
auto tuple_map(const std::tuple<Elements...>& t, Function&& f) {
    return internal::tuple_map_helper(t, std::forward<Function>(f),
                                      std::index_sequence_for<Elements...>());
}
template<typename Function, typename... Elements>
auto tuple_map(std::tuple<Elements...>&& t, Function&& f) {
    return internal::tuple_map_helper(std::move(t), std::forward<Function>(f),
                                      std::index_sequence_for<Elements...>());
}
/// Iterate over all elements in tuple
///
/// Iterates over given tuple and calls the specified function for each of
/// it elements.
///
/// \param t a tuple to iterate over
/// \param f function to call for each tuple element
template<typename Function, typename... Elements>
void tuple_for_each(const std::tuple<Elements...>& t, Function&& f) {
    return internal::tuple_for_each_helper(t, std::forward<Function>(f),
                                           std::index_sequence_for<Elements...>());
}
template<typename Function, typename... Elements>
void tuple_for_each(std::tuple<Elements...>& t, Function&& f) {
    return internal::tuple_for_each_helper(t, std::forward<Function>(f),
                                           std::index_sequence_for<Elements...>());
}
template<typename Function, typename... Elements>
void tuple_for_each(std::tuple<Elements...>&& t, Function&& f) {
    return internal::tuple_for_each_helper(std::move(t), std::forward<Function>(f),
                                           std::index_sequence_for<Elements...>());
}
}

/*--------------------------------------------------------------------------------------------------*/




template<typename Function, typename ReturnType, typename ArgsTuple>
GCC6_CONCEPT(requires std::is_nothrow_move_constructible<ArgsTuple>::value)
class concrete_execution_stage final : public execution_stage {
    static_assert(std::is_nothrow_move_constructible<ArgsTuple>::value,
                  "Function arguments need to be nothrow move constructible");

    static constexpr size_t flush_threshold = 128;

    using return_type = futurize_t<ReturnType>;
    using promise_type = typename return_type::promise_type;
    using input_type = typename internal::tuple_map_types<internal::wrap_for_es, ArgsTuple>::type;

    struct work_item {
        input_type _in;
        promise_type _ready;

        template<typename... Args>
        work_item(Args&&... args) : _in(std::forward<Args>(args)...) { }

        work_item(work_item&& other) = delete;
        work_item(const work_item&) = delete;
        work_item(work_item&) = delete;
    };
    std::deque<work_item> _queue;

    Function _function;
private:
    auto unwrap(input_type&& in) {
        return tuple_map(std::move(in), [] (auto&& obj) {
            return internal::unwrap_for_es(std::forward<decltype(obj)>(obj));
        });
    }

    virtual void do_flush() noexcept override {
        while (!_queue.empty()) {
            auto& wi = _queue.front();
            futurize<ReturnType>::apply(_function, unwrap(std::move(wi._in))).forward_to(std::move(wi._ready));
            _queue.pop_front();
            _stats.function_calls_executed++;

            if (need_preempt()) {
                _stats.tasks_preempted++;
                break;
            }
        }
        _empty = _queue.empty();
    }
public:
    explicit concrete_execution_stage(const sstring& name, Function f)
        : execution_stage(name)
        , _function(std::move(f)){
        _queue.reserve(flush_threshold);
    }
    template<typename... Args>
    GCC6_CONCEPT(requires std::is_constructible<input_type, Args...>::value)
    return_type operator()(Args&&... args) {
        _queue.emplace_back(std::forward<Args>(args)...);
        _empty = false;
        _stats.function_calls_enqueued++;
        auto f = _queue.back()._ready.get_future();
        if (_queue.size() > flush_threshold) {
            flush();
        }
        return f;
    }
};


template<typename Function>
auto make_execution_stage(const sstring& name, Function&& fn) {
    using traits = function_traits<Function>;
    return concrete_execution_stage<std::decay_t<Function>, typename traits::return_type,
                                    typename traits::args_as_tuple>(name, std::forward<Function>(fn));
}
template<typename Ret, typename Object, typename... Args>
auto make_execution_stage(const sstring& name, Ret (Object::*fn)(Args...)) {
    return concrete_execution_stage<decltype(std::mem_fn(fn)), Ret, std::tuple<Object*, Args...>>(name, std::mem_fn(fn));
}

template<typename Ret, typename Object, typename... Args>
auto make_execution_stage(const sstring& name, Ret (Object::*fn)(Args...) const) {
    return concrete_execution_stage<decltype(std::mem_fn(fn)), Ret, std::tuple<const Object*, Args...>>(name, std::mem_fn(fn));
}

inline execution_stage::execution_stage(const sstring& name):_name(name){
    internal::execution_stage_manager::get().register_execution_stage(*this);
    auto undo = defer([&] { internal::execution_stage_manager::get().unregister_execution_stage(*this); });
    undo.cancel();
}

inline execution_stage::~execution_stage()
{
    internal::execution_stage_manager::get().unregister_execution_stage(*this);
}

inline execution_stage::execution_stage(execution_stage&& other)
    : _stats(other._stats)
    , _name(std::move(other._name)){
        internal::execution_stage_manager::get().update_execution_stage_registration(other, *this);
}














/*--------------------------------------------------------------------------------------------------------------------------------*/






#include <iosfwd>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <system_error>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <cstring>
#include <utility>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <signal.h>
#include <optional>
#include <pthread.h>
#include <signal.h>
#include <memory>
#include <chrono>
#include <string>
#include <sys/uio.h>
#include "../util/unaligned.hh"














file_desc
file_desc::temporary(std::string directory) {
    // FIXME: add O_TMPFILE support one day
    directory += "/XXXXXX";
    std::vector<char> templat(directory.c_str(), directory.c_str() + directory.size() + 1);
    int fd = ::mkstemp(templat.data());

    int r = ::unlink(templat.data());
    // throw_system_error_on(r == -1); // leaks created file, but what can we do?
    return file_desc(fd);
}


/*---------------------------------------------------------------------------------------------------------------*/


#include <algorithm>
#include <memory>
#include <cassert>
#include <experimental/optional>
// Support classes for Ragel parsers
// Builds an std::string that can be scattered across multiple packets.
// Use a std::string_build::guard variable to designate each scattered
// char array, and call mark_start() and mark_end() at the start
// and end points, respectively.  std::string_builder will collect data
// from intervening segments, if needed.
// After mark_end() has been called, use the get() method to obtain
// the built string.
// FIXME: switch to string_view.
class string_builder {
    std::string _value;
    const char* _start = nullptr;
public:
    class guard;
public:
    std::string get() && {
        return std::move(_value);
    }
    void reset() {
        // _value.reset();
        _start = nullptr;
    }
    friend class guard;
};

class string_builder::guard {
    string_builder& _builder;
    const char* _block_end;
public:
    guard(string_builder& builder, const char* block_start, const char* block_end)
        : _builder(builder), _block_end(block_end) {
        if (!_builder._value.empty()) {
            mark_start(block_start);
        }
    }
    ~guard() {
        if (_builder._start) {
            mark_end(_block_end);
        }
    }
    void mark_start(const char* p) {
        _builder._start = p;
    }
    void mark_end(const char* p) {
        if (_builder._value.empty()) {
            // avoid an allocation in the common case
            _builder._value = std::string(_builder._start, p);
        } else {
            _builder._value += std::string(_builder._start, p);
        }
        _builder._start = nullptr;
    }
};















// CRTP
template <typename ConcreteParser>
class ragel_parser_base {
protected:
    int _fsm_cs;
    std::unique_ptr<int[]> _fsm_stack = nullptr;
    int _fsm_stack_size = 0;
    int _fsm_top;
    int _fsm_act;
    char* _fsm_ts;
    char* _fsm_te;
    string_builder _builder;
protected:
    void init_base() {
        _builder.reset();
    }
    void prepush() {
        if (_fsm_top == _fsm_stack_size) {
            auto old = _fsm_stack_size;
            _fsm_stack_size = std::max(_fsm_stack_size * 2, 16);
            assert(_fsm_stack_size > old);
            std::unique_ptr<int[]> new_stack{new int[_fsm_stack_size]};
            std::copy(_fsm_stack.get(), _fsm_stack.get() + _fsm_top, new_stack.get());
            std::swap(_fsm_stack, new_stack);
        }
    }
    void postpop() {}
    std::string get_str() {
        auto s = std::move(_builder).get();
        return std::move(s);
    }
public:
    using unconsumed_remainder = std::optional<temporary_buffer<char>>;
    future<unconsumed_remainder> operator()(temporary_buffer<char> buf) {
        char* p = buf.get_write();
        char* pe = p + buf.size();
        char* eof = buf.empty() ? pe : nullptr;
        char* parsed = static_cast<ConcreteParser*>(this)->parse(p, pe, eof);
        if (parsed) {
            buf.trim_front(parsed - p);
            return make_ready_future<std::optional<temporary_buffer<char>>>(std::move(buf));
        }
        return make_ready_future<std::optional<temporary_buffer<char>>>();
    }
};









/*---------------------------------------------------socket相关----------------------------------------------------------------*/



namespace net { class packet; }
















static inline
bool is_ip_unspecified(ipv4_addr &addr) {
    return addr.ip == 0;
}

static inline
bool is_port_unspecified(ipv4_addr &addr) {
    return addr.port == 0;
}

static inline
std::ostream& operator<<(std::ostream &os, ipv4_addr addr) {
}

class network_stack_registry {
public:
    using options = boost::program_options::variables_map;
private:
    static std::unordered_map<sstring,
            std::function<future<std::unique_ptr<network_stack>> (options opts)>>& _map() {
        static std::unordered_map<sstring,
                std::function<future<std::unique_ptr<network_stack>> (options opts)>> map;
        return map;
    }
    static std::string& _default() {
        static std::string def;
        return def;
    }
public:
    static boost::program_options::options_description& options_description() {
        static boost::program_options::options_description opts;
        return opts;
    }
    static void register_stack(std::string name,
            boost::program_options::options_description opts,
            std::function<future<std::unique_ptr<network_stack>> (options opts)> create,
            bool make_default = false);
    static std::string default_stack();
    static std::vector<std::string> list();
    static future<std::unique_ptr<network_stack>> create(options opts);
    static future<std::unique_ptr<network_stack>> create(std::string name, options opts);
};



class network_stack_registrator {
public:
    using options = boost::program_options::variables_map;
    explicit network_stack_registrator(std::string name,
            boost::program_options::options_description opts,
            std::function<future<std::unique_ptr<network_stack>> (options opts)> factory,
            bool make_default = false);
};





/*-------------------------------------------------reactor类定义----------------------------------------------------------------*/
#include "../resource/resource.hh"
#include <sys/syscall.h>


/*posix_stack*/

namespace net {

class posix_data_source_impl final : public data_source_impl {
    std::shared_ptr<pollable_fd> _fd;
    temporary_buffer<char> _buf;
    size_t _buf_size;
public:
    explicit posix_data_source_impl(std::shared_ptr<pollable_fd> fd, size_t buf_size = 8192)
        : _fd(std::move(fd)), _buf(buf_size), _buf_size(buf_size) {}
    future<temporary_buffer<char>> get() override;
    future<> close() override;
};

class posix_data_sink_impl : public data_sink_impl {
    std::shared_ptr<pollable_fd> _fd;
    packet _p;
public:
    explicit posix_data_sink_impl(std::shared_ptr<pollable_fd> fd) : _fd(std::move(fd)) {}
    future<> put(packet p) override;
    future<> put(temporary_buffer<char> buf) override;
    future<> close() override;
};

/*----------------------------------------------------------------------------------------*/
struct sockaddr_in_hash {
    std::size_t operator()(const ::sockaddr_in& addr) const {
        // Create a hash from the address and port
        // You may want to adjust this based on your specific needs
        return std::hash<uint32_t>()(addr.sin_addr.s_addr) ^
               std::hash<uint16_t>()(addr.sin_port);
    }
};
struct sockaddr_in_equal {
    bool operator()(const ::sockaddr_in& a, const ::sockaddr_in& b) const {
        // Compare the relevant fields of sockaddr_in
        return a.sin_addr.s_addr == b.sin_addr.s_addr &&
               a.sin_port == b.sin_port;
    }
};
/*----------------------------新增hash和equeal代码------------------------*/


template <transport Transport>
class posix_ap_server_socket_impl : public server_socket_impl {
    struct connection {
        pollable_fd fd;
        net::socket_address addr;
        connection(pollable_fd xfd, net::socket_address xaddr) : fd(std::move(xfd)), addr(xaddr) {}
    };
    static auto &get_sockets() {
        static thread_local std::unordered_map<::sockaddr_in, promise<connected_socket, net::socket_address>,sockaddr_in_hash,sockaddr_in_equal> sockets;
        return sockets;
    }
  //  static thread_local std::unordered_map<::sockaddr_in, promise<connected_socket, socket_address>,sockaddr_in_hash,sockaddr_in_equal> sockets;
    static auto &get_conn_q() {
        static thread_local std::unordered_multimap<::sockaddr_in, connection,sockaddr_in_hash,sockaddr_in_equal> conn_q;
        return conn_q;
    }
    // static thread_local std::unordered_map<::sockaddr_in, promise<connected_socket, socket_address>,sockaddr_in_hash,sockaddr_in_equal> sockets;
    // static thread_local std::unordered_multimap<::sockaddr_in, connection,sockaddr_in_hash,sockaddr_in_equal> conn_q;
    net::socket_address _sa;
public:
    explicit posix_ap_server_socket_impl(net::socket_address sa) : _sa(sa) {}
    virtual future<connected_socket, net::socket_address> accept() override;
    virtual void abort_accept() override;
    static void move_connected_socket(net::socket_address sa, pollable_fd fd, net::socket_address addr);
};
using posix_tcp_ap_server_socket_impl = posix_ap_server_socket_impl<transport::TCP>;
using posix_sctp_ap_server_socket_impl = posix_ap_server_socket_impl<transport::SCTP>;



template <transport Transport>
class posix_server_socket_impl : public server_socket_impl {
    net::socket_address _sa;
    pollable_fd _lfd;
public:
    explicit posix_server_socket_impl(net::socket_address sa, pollable_fd lfd) : _sa(sa), _lfd(std::move(lfd)) {}
    virtual future<connected_socket, net::socket_address> accept();
    virtual void abort_accept() override;
};
using posix_server_tcp_socket_impl = posix_server_socket_impl<transport::TCP>;
using posix_server_sctp_socket_impl = posix_server_socket_impl<transport::SCTP>;

template <transport Transport>
class posix_reuseport_server_socket_impl : public server_socket_impl {
    net::socket_address _sa;
    pollable_fd _lfd;
public:
    explicit posix_reuseport_server_socket_impl(net::socket_address sa, pollable_fd lfd) : _sa(sa), _lfd(std::move(lfd)) {}
    virtual future<connected_socket, net::socket_address> accept();
    virtual void abort_accept() override;
};
using posix_reuseport_server_tcp_socket_impl = posix_reuseport_server_socket_impl<transport::TCP>;
using posix_reuseport_server_sctp_socket_impl = posix_reuseport_server_socket_impl<transport::SCTP>;

class posix_network_stack : public network_stack {
private:
    const bool _reuseport;
public:
    void print_stack()
    {
        std::cout<<"posix_network_statck"<<std::endl;
    }
    explicit posix_network_stack(boost::program_options::variables_map opts) : _reuseport(engine().posix_reuseport_available()) {}
    virtual server_socket listen(net::socket_address sa, listen_options opts) override;
    virtual net::socket socket() override;
    virtual ::net::udp_channel make_udp_channel(ipv4_addr addr) override;
    posix_udp_channel make_posix_udp_channel(ipv4_addr addr);
    static future<std::unique_ptr<network_stack>> create(boost::program_options::variables_map opts) {
        return make_ready_future<std::unique_ptr<network_stack>>(std::unique_ptr<network_stack>(new posix_network_stack(opts)));
    }
    virtual bool has_per_core_namespace() override { return _reuseport; };
};

class posix_ap_network_stack : public posix_network_stack {
private:
    const bool _reuseport;
public:
    void print_stack()
    {
        std::cout<<"访问到了posix_network_statck"<<std::endl;
    }
    posix_ap_network_stack(boost::program_options::variables_map opts) : posix_network_stack(std::move(opts)), _reuseport(engine().posix_reuseport_available()) {}
    virtual server_socket listen(net::socket_address sa, listen_options opts) override;
    static future<std::unique_ptr<network_stack>> create(boost::program_options::variables_map opts) {
        return make_ready_future<std::unique_ptr<network_stack>>(std::unique_ptr<network_stack>(new posix_ap_network_stack(opts)));
    }
};

}





bool
reactor::pure_poll_once() {
    for (auto c : _pollers) {
        if (c->pure_poll()) {
            return true;
        }
    }
    return false;
}

template <typename Func>
inline
std::unique_ptr<pollfn>
reactor::make_pollfn(Func&& func) {
    struct the_pollfn : pollfn {
        the_pollfn(Func&& func) : func(std::forward<Func>(func)) {}
        Func func;
        virtual bool poll() override final {
            return func();
        }
        virtual bool pure_poll() override final {
            return poll(); // dubious, but compatible
        }
    };
    return std::make_unique<the_pollfn>(std::forward<Func>(func));
}

bool reactor::flush_pending_aio() {
    bool did_work = false;
    while (!_pending_aio.empty()) {
        auto nr = _pending_aio.size();
        struct iocb* iocbs[max_aio];
        for (size_t i = 0; i < nr; ++i) {
            iocbs[i] = &_pending_aio[i];
        }
        auto r = ::io_submit(_io_context, nr, iocbs);
        size_t nr_consumed;
        if (r < 0) {
            auto ec = -r;
            switch (ec) {
                case EAGAIN:
                    return did_work;
                case EBADF: {
                    auto pr = reinterpret_cast<promise<io_event>*>(iocbs[0]->data);
                    try {
                        // throw_kernel_error(r);
                        std::cout<<"error"<<std::endl;
                        throw std::system_error(ec, std::system_category());
                    } catch (...) {
                        pr->set_exception(std::current_exception());
                    }
                    delete pr;
                    _io_context_available.signal(1);
                    // if EBADF, it means that the first request has a bad fd, so
                    // we will only remove it from _pending_aio and try again.
                    nr_consumed = 1;
                    break;
                }
                default:
                    throw std::system_error(ec, std::system_category());
                    abort();
            }
        } else {
            nr_consumed = size_t(r);
        }

        did_work = true;
        if (nr_consumed == nr) {
            _pending_aio.clear();
        } else {
            _pending_aio.erase(_pending_aio.begin(), _pending_aio.begin() + nr_consumed);
        }
    }
    return did_work;
}



bool reactor::process_io()
{
    io_event ev[max_aio];
    struct timespec timeout = {0, 0};
    auto n = ::io_getevents(_io_context, 1, max_aio, ev, &timeout);
    assert(n >= 0);
    for (size_t i = 0; i < size_t(n); ++i) {
        auto pr = reinterpret_cast<promise<io_event>*>(ev[i].data);
        pr->set_value(ev[i]);
        delete pr;
    }
    _io_context_available.signal(n);
    return n;
}

void reactor::register_poller(pollfn* p) {
    _pollers.push_back(p);
}

void reactor::unregister_poller(pollfn* p) {
    _pollers.erase(std::find(_pollers.begin(), _pollers.end(), p));
}



bool
reactor::flush_tcp_batches() {
    bool work = _flush_batching.size();
    while (!_flush_batching.empty()) {
        auto os = std::move(_flush_batching.front());
        _flush_batching.pop_front();
        os->poll_flush();
    }
    return work;
}


class reactor::poller::registration_task : public task {
private:
    poller* _p;
public:
    explicit registration_task(poller* p) : _p(p) {}
    virtual void run() noexcept override {
        if (_p) {
            engine().register_poller(_p->_pollfn.get());
            _p->_registration_task = nullptr;
        }
    }
    void cancel() {
        _p = nullptr;
    }
    void moved(poller* p) {
        _p = p;
    }
};



reactor::poller::poller(poller&& x)
        : _pollfn(std::move(x._pollfn)), _registration_task(x._registration_task) {
    if (_pollfn && _registration_task) {
        _registration_task->moved(this);
    }
}

reactor::poller&
reactor::poller::operator=(poller&& x) {
    if (this != &x) {
        this->~poller();
        new (this) poller(std::move(x));
    }
    return *this;
}
reactor_backend_epoll::reactor_backend_epoll()
    : _epollfd(file_desc::epoll_create(EPOLL_CLOEXEC)) {
}


future<> reactor_backend_epoll::get_epoll_future(pollable_fd_state& pfd,
        promise<> pollable_fd_state::*pr, int event) {
    if (pfd.events_known & event) {
        pfd.events_known &= ~event;
        return make_ready_future();
    }
    pfd.events_requested |= event;
    if (!(pfd.events_epoll & event)) {
        auto ctl = pfd.events_epoll ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        pfd.events_epoll |= event;
        ::epoll_event eevt;
        eevt.events = pfd.events_epoll;
        eevt.data.ptr = &pfd;
        int r = ::epoll_ctl(_epollfd.get(), ctl, pfd.fd.get(), &eevt);
        assert(r == 0);
        engine().start_epoll();
    }
    pfd.*pr = promise<>();
    return (pfd.*pr).get_future();
}

void reactor_backend_epoll::abort_fd(pollable_fd_state& pfd, std::exception_ptr ex,
                                     promise<> pollable_fd_state::* pr, int event) {
    if (pfd.events_epoll & event) {
        pfd.events_epoll &= ~event;
        auto ctl = pfd.events_epoll ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        ::epoll_event eevt;
        eevt.events = pfd.events_epoll;
        eevt.data.ptr = &pfd;
        int r = ::epoll_ctl(_epollfd.get(), ctl, pfd.fd.get(), &eevt);
        assert(r == 0);
    }
    if (pfd.events_requested & event) {
        pfd.events_requested &= ~event;
        (pfd.*pr).set_exception(std::move(ex));
    }
    pfd.events_known &= ~event;
}

future<> reactor_backend_epoll::readable(pollable_fd_state& fd) {
    return get_epoll_future(fd, &pollable_fd_state::pollin, EPOLLIN);
}

future<> reactor_backend_epoll::writeable(pollable_fd_state& fd) {
    return get_epoll_future(fd, &pollable_fd_state::pollout, EPOLLOUT);
}

void reactor_backend_epoll::abort_reader(pollable_fd_state& fd, std::exception_ptr ex) {
    abort_fd(fd, std::move(ex), &pollable_fd_state::pollin, EPOLLIN);
}

void reactor_backend_epoll::abort_writer(pollable_fd_state& fd, std::exception_ptr ex) {
    abort_fd(fd, std::move(ex), &pollable_fd_state::pollout, EPOLLOUT);
}

void reactor_backend_epoll::forget(pollable_fd_state& fd) {
    if (fd.events_epoll) {
        ::epoll_ctl(_epollfd.get(), EPOLL_CTL_DEL, fd.fd.get(), nullptr);
    }
}

future<> reactor_backend_epoll::notified(reactor_notifier *n) {
    // Currently reactor_backend_epoll doesn't need to support notifiers,
    // because we add to it file descriptors instead. But this can be fixed
    // later.
    std::cout << "reactor_backend_epoll does not yet support notifiers!\n";
    abort();
}

void reactor_backend_epoll::complete_epoll_event(pollable_fd_state& pfd, promise<> pollable_fd_state::*pr,
        int events, int event) {
    if (pfd.events_requested & events & event) {
        pfd.events_requested &= ~event;
        pfd.events_known &= ~event;
        (pfd.*pr).set_value();
        pfd.*pr = promise<>();
    }
}




bool
reactor_backend_epoll::wait_and_process(int timeout, const sigset_t* active_sigmask) {
    std::array<epoll_event, 128> eevt;
    int nr = ::epoll_pwait(_epollfd.get(), eevt.data(), eevt.size(), timeout, active_sigmask);
    if (nr == -1 && errno == EINTR) {
        return false; // gdb can cause this
    }
    assert(nr != -1);
    for (int i = 0; i < nr; ++i) {
        auto& evt = eevt[i];
        auto pfd = reinterpret_cast<pollable_fd_state*>(evt.data.ptr);
        auto events = evt.events & (EPOLLIN | EPOLLOUT);
        auto events_to_remove = events & ~pfd->events_requested;
        complete_epoll_event(*pfd, &pollable_fd_state::pollin, events, EPOLLIN);
        complete_epoll_event(*pfd, &pollable_fd_state::pollout, events, EPOLLOUT);
        if (events_to_remove) {
            pfd->events_epoll &= ~events_to_remove;
            evt.events = pfd->events_epoll;
            auto op = evt.events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            ::epoll_ctl(_epollfd.get(), op, pfd->fd.get(), &evt);
        }
    }
    return nr;
}

std::unique_ptr<reactor_notifier> reactor_backend_epoll::make_reactor_notifier() {
    return std::make_unique<reactor_notifier_epoll>();
}


class reactor::poller::deregistration_task : public task {
private:
    std::unique_ptr<pollfn> _p;
public:
    explicit deregistration_task(std::unique_ptr<pollfn>&& p) : _p(std::move(p)) {}
    virtual void run() noexcept override {
        engine().unregister_poller(_p.get());
    }
};


class reactor::io_pollfn final : public pollfn {
    reactor& _r;
public:
    io_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() override final {
        return _r.process_io();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // aio cannot generate events if there are no inflight aios;
        // but if we enabled _aio_eventfd, we can always enter
        return _r._io_context_available.current() == reactor::max_aio
                || _r._aio_eventfd;
    }
    virtual void exit_interrupt_mode() override {
        // nothing to do
    }
};


class reactor::signal_pollfn final : public pollfn {
    reactor& _r;
public:
    signal_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r._signals.poll_signal();
    }
    virtual bool pure_poll() override final {
        return _r._signals.pure_poll_signal();
    }
    virtual bool try_enter_interrupt_mode() override {
        // Signals will interrupt our epoll_pwait() call, but
        // disable them now to avoid a signal between this point
        // and epoll_pwait()
        sigset_t block_all;
        sigfillset(&block_all);
        ::pthread_sigmask(SIG_SETMASK, &block_all, &_r._active_sigmask);
        if (poll()) {
            // raced already, and lost
            exit_interrupt_mode();
            return false;
        }
        return true;
    }
    virtual void exit_interrupt_mode() override final {
        ::pthread_sigmask(SIG_SETMASK, &_r._active_sigmask, nullptr);
    }
};

class reactor::batch_flush_pollfn final : public pollfn {
    reactor& _r;
public:
    batch_flush_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r.flush_tcp_batches();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // This is a passive poller, so if a previous poll
        // returned false (idle), there's no more work to do.
        return true;
    }
    virtual void exit_interrupt_mode() override final {

    }
};

class reactor::aio_batch_submit_pollfn final : public pollfn {
    reactor& _r;
public:
    aio_batch_submit_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r.flush_pending_aio();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // This is a passive poller, so if a previous poll
        // returned false (idle), there's no more work to do.
        return true;
    }
    virtual void exit_interrupt_mode() override final {
    }
};

class reactor::drain_cross_cpu_freelist_pollfn final : public pollfn {
public:
    virtual bool poll() final override {
        return memory::drain_cross_cpu_freelist();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // Other cpus can queue items for us to free; and they won't notify
        // us about them.  But it's okay to ignore those items, freeing them
        // doesn't have any side effects.
        //
        // We'll take care of those items when we wake up for another reason.
        return true;
    }
    virtual void exit_interrupt_mode() override final {
    }
};



class reactor::lowres_timer_pollfn final : public pollfn {
    reactor& _r;
    // A highres timer is implemented as a waking  signal; so
    // we arm one when we have a lowres timer during sleep, so
    // it can wake us up.
    timer<> _nearest_wakeup { [this] { _armed = false; } };
    bool _armed = false;
public:
    lowres_timer_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r.do_expire_lowres_timers();
    }
    virtual bool pure_poll() final override {
        return _r.do_check_lowres_timers();
    }
    virtual bool try_enter_interrupt_mode() override {
        // arm our highres timer so a signal will wake us up
        auto next = _r._lowres_next_timeout;
        if (next == lowres_clock::time_point()) {
            // no pending timers
            return true;
        }
        auto now = lowres_clock::now();
        if (next <= now) {
            // whoops, go back
            return false;
        }
        _nearest_wakeup.arm(next - now);
        _armed = true;
        return true;
    }
    virtual void exit_interrupt_mode() override final {
        if (_armed) {
            _nearest_wakeup.cancel();
            _armed = false;
        }
    }
};



void
reactor::block_notifier(int) {
    auto steps = engine()._tasks_processed_stalled.load(std::memory_order_relaxed);
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(engine()._task_quota * steps);

    // backtrace_buffer buf;
    // buf.append("Reactor stalled for ");
    // buf.append_decimal(uint64_t(delta.count()));
    // buf.append(" ms");
    // print_with_backtrace(buf);
}



std::chrono::nanoseconds
reactor::calculate_poll_time() {
    // In a non-virtualized environment, select a poll time
    // that is competitive with halt/unhalt.
    // In a virutalized environment, IPIs are slow and dominate
    // sleep/wake (mprotect/tgkill), so increase poll time to reduce
    // so we don't sleep in a request/reply workload
    return 200us; //200us是怎么得到的?
}

struct reactor_deleter {
    void operator()(reactor* p) {
        p->~reactor();
        free(p);
    }
};
void schedule_normal(std::unique_ptr<task> t) {
    // std::cout<<"调用schedule normal"<<std::endl;
    engine().add_task(std::move(t));
}
void schedule_urgent(std::unique_ptr<task> t) {
    // std::cout<<"调用schedule urgent"<<std::endl;
    engine().add_urgent_task(std::move(t));
}






timespec to_timespec(steady_clock_type::time_point t) {
    using ns = std::chrono::nanoseconds;
    auto n = std::chrono::duration_cast<ns>(t.time_since_epoch()).count();
    return { n / 1'000'000'000, n % 1'000'000'000 };
}



bool queue_timer(timer<steady_clock_type>* tmr) {
    return engine().queue_timer(tmr);
}

void add_timer(timer<steady_clock_type>* tmr) {
    engine().add_timer(tmr);
}

void add_timer(timer<lowres_clock>* tmr) {
    engine().add_timer(tmr);
}




bool queue_timer(timer<lowres_clock>* tmr) {
    return engine().queue_timer(tmr);
}



void del_timer(timer<lowres_clock>* tmr) {
    engine().del_timer(tmr);
}

void del_timer(timer<steady_clock_type>* tmr) {
    engine().del_timer(tmr);
}


template<int Signal, void(*Func)()>
void install_oneshot_signal_handler() {
    static bool handled = false;
    static util::spinlock lock;
    struct sigaction sa;
    sa.sa_sigaction = [](int sig, siginfo_t *info, void *p) {
        std::lock_guard<util::spinlock> g(lock);
        if (!handled) {
            handled = true;
            Func();
            signal(sig, SIG_DFL);
        }
    };
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    if (Signal == SIGSEGV) {
        sa.sa_flags |= SA_ONSTACK;
    }
    auto r = ::sigaction(Signal, &sa, nullptr);
    // throw_system_error_on(false);//这是我改的.
}

static void sigsegv_action() noexcept {
    std::cout<<"Segmentation fault";
}

static void sigabrt_action() noexcept {
    std::cout<<"Aborting";
}

template<typename Clock>
struct with_clock {};
template <typename... T>
struct future_option_traits;
template <typename Clock, typename... T>
struct future_option_traits<with_clock<Clock>, T...> {
    using clock_type = Clock;
    template<template <typename...> class Class>
    struct parametrize {
        using type = Class<T...>;
    };
};

template <typename... T>
struct future_option_traits {
    using clock_type = lowres_clock;
    template<template <typename...> class Class>
    struct parametrize {
        using type = Class<T...>;
    };
};





inline
void pin_this_thread(unsigned cpu_id) {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu_id, &cs);
    auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
    assert(r == 0);
}

namespace bi = boost::intrusive;
template <typename... T>
class promise;

template <typename... T>
class future;
class thread;
class thread_attributes;
class thread_scheduling_group;
struct jmp_buf_link;




template<typename... T>
class shared_future {
    template <typename... U> friend class shared_promise;
    using options = future_option_traits<T...>;
public:
    using clock = typename options::clock_type;
    using time_point = typename clock::time_point;
    using future_type = typename future_option_traits<T...>::template parametrize<future>::type;
    using promise_type = typename future_option_traits<T...>::template parametrize<promise>::type;
    using value_tuple_type = typename future_option_traits<T...>::template parametrize<std::tuple>::type;
private:
    using future_state_type = typename future_option_traits<T...>::template parametrize<future_state>::type;
    using promise_expiry = typename future_option_traits<T...>::template parametrize<promise_expiry>::type;

    class shared_state {
        future_state_type _future_state;
        expiring_fifo<promise_type, promise_expiry, clock> _peers;
    public:
        void resolve(future_type&& f) noexcept {
            _future_state = f.get_available_state();
            if (_future_state.failed()) {
                while (_peers) {
                    _peers.front().set_exception(_future_state.get_exception());
                    _peers.pop_front();
                }
            } else {
                while (_peers) {
                    auto& p = _peers.front();
                    try {
                        p.set_value(_future_state.get_value());
                    } catch (...) {
                        p.set_exception(std::current_exception());
                    }
                    _peers.pop_front();
                }
            }
        }

        future_type get_future(time_point timeout = time_point::max()) {
            if (!_future_state.available()) {
                promise_type p;
                auto f = p.get_future();
                _peers.push_back(std::move(p), timeout);
                return f;
            } else if (_future_state.failed()) {
                return future_type(exception_future_marker(), _future_state.get_exception());
            } else {
                try {
                    return future_type(ready_future_marker(), _future_state.get_value());
                } catch (...) {
                    return future_type(exception_future_marker(), std::current_exception());
                }
            }
        }
    };
    std::shared_ptr<shared_state> _state;
public:
    shared_future(future_type&& f)
        : _state(std::make_shared<shared_state>())
    {
        f.then_wrapped([s = _state] (future_type&& f) mutable {
            s->resolve(std::move(f));
        });
    }

    shared_future() = default;
    shared_future(const shared_future&) = default;
    shared_future& operator=(const shared_future&) = default;
    shared_future(shared_future&&) = default;
    shared_future& operator=(shared_future&&) = default;
    future_type get_future(time_point timeout = time_point::max()) const {
        return _state->get_future(timeout);
    }
    operator future_type() const {
        return get_future();
    }
    bool valid() const {
        return bool(_state);
    }
};
template <typename... T>
class shared_promise {
public:
    using shared_future_type = shared_future<T...>;
    using future_type = typename shared_future_type::future_type;
    using promise_type = typename shared_future_type::promise_type;
    using clock = typename shared_future_type::clock;
    using time_point = typename shared_future_type::time_point;
    using value_tuple_type = typename shared_future_type::value_tuple_type;
    using future_state_type = typename shared_future_type::future_state_type;
private:
    promise_type _promise;
    shared_future_type _shared_future;
    static constexpr bool copy_noexcept = future_state_type::copy_noexcept;
public:
    shared_promise(const shared_promise&) = delete;
    shared_promise(shared_promise&&) = default;
    shared_promise& operator=(shared_promise&&) = default;
    shared_promise() : _promise(), _shared_future(_promise.get_future()) {
    }
    /// \brief Gets new future associated with this promise.
    /// If the promise is not resolved before timeout the returned future will resolve with \ref timed_out_error.
    /// This instance doesn't have to be kept alive until the returned future resolves.
    future_type get_shared_future(time_point timeout = time_point::max()) {
        return _shared_future.get_future(timeout);
    }
    /// \brief Sets the shared_promise's value (as tuple; by copying), same as normal promise
    void set_value(const value_tuple_type& result) noexcept(copy_noexcept) {
        _promise.set_value(result);
    }
    /// \brief Sets the shared_promise's value (as tuple; by moving), same as normal promise
    void set_value(value_tuple_type&& result) noexcept {
        _promise.set_value(std::move(result));
    }
    /// \brief Sets the shared_promise's value (variadic), same as normal promise
    template <typename... A>
    void set_value(A&&... a) noexcept {
        _promise.set_value(std::forward<A>(a)...);
    }
    /// \brief Marks the shared_promise as failed, same as normal promise
    void set_exception(std::exception_ptr ex) noexcept {
        _promise.set_exception(std::move(ex));
    }
    /// \brief Marks the shared_promise as failed, same as normal promise
    template<typename Exception>
    void set_exception(Exception&& e) noexcept {
        set_exception(make_exception_ptr(std::forward<Exception>(e)));
    }
};




// static std::unique_ptr<reactor, reactor_deleter> & reactor_holder(){
//     return static thread_local  std::unique_ptr<reactor, reactor_deleter> reactor_holder;
// }


struct reactor_holder {
    static auto & get() {
        static thread_local  std::unique_ptr<reactor, reactor_deleter> reactor_holder;
        return reactor_holder;
    }
};


// namespace reactor_ns {
//     thread_local std::unique_ptr<reactor, reactor_deleter> reactor_holder;
// }
// using reactor_ns::reactor_holder;


std::vector<posix_thread> smp::_threads;
std::vector<std::function<void ()>> smp::_thread_loops;
std::optional<boost::barrier> smp::_all_event_loops_done;
std::vector<reactor*> smp::_reactors;
smp_message_queue** smp::_qs;//为什么是二级指针？
std::thread::id smp::_tmain;



class reactor::smp_pollfn final : public pollfn {
    reactor& _r;
    struct aligned_flag {
        std::atomic<bool> flag;
        char pad[63];
        bool try_lock() {
            return !flag.exchange(true, std::memory_order_relaxed);
        }
        void unlock() {
            flag.store(false, std::memory_order_relaxed);
        }
    };
    static aligned_flag _membarrier_lock;
public:
    smp_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return smp::poll_queues();
    }
    virtual bool pure_poll() final override {
        return smp::pure_poll_queues();
    }
    virtual bool try_enter_interrupt_mode() override {
        // systemwide_memory_barrier() is very slow if run concurrently,
        // so don't go to sleep if it is running now.
        if (!_membarrier_lock.try_lock()) {
            return false;
        }
        _r._sleeping.store(true, std::memory_order_relaxed);
        systemwide_memory_barrier();
        _membarrier_lock.unlock();
        if (poll()) {
            // raced
            _r._sleeping.store(false, std::memory_order_relaxed);
            return false;
        }
        return true;
    }
    virtual void exit_interrupt_mode() override final {
        _r._sleeping.store(false, std::memory_order_relaxed);
    }
};

class reactor::execution_stage_pollfn final : public pollfn {
    internal::execution_stage_manager& _esm;
public:
    execution_stage_pollfn() : _esm(internal::execution_stage_manager::get()) { }

    virtual bool poll() override {
        return _esm.flush();
    }
    virtual bool pure_poll() override {
        return _esm.poll();
    }
    virtual bool try_enter_interrupt_mode() override {
        // This is a passive poller, so if a previous poll
        // returned false (idle), there's no more work to do.
        return true;
    }
    virtual void exit_interrupt_mode() override { }
};


class reactor::syscall_pollfn final : public pollfn {
    reactor& _r;
public:
    syscall_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r._thread_pool.complete();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        _r._thread_pool.enter_interrupt_mode();
        if (poll()) {
            // raced
            _r._thread_pool.exit_interrupt_mode();
            return false;
        }
        return true;
    }
    virtual void exit_interrupt_mode() override final {
        _r._thread_pool.exit_interrupt_mode();
    }
};


// alignas(64) reactor::smp_pollfn::aligned_flag reactor::smp_pollfn::_membarrier_lock;

class reactor::epoll_pollfn final : public pollfn {
    reactor& _r;
public:
    epoll_pollfn(reactor& r) : _r(r) {}
    virtual bool poll() final override {
        return _r.wait_and_process();
    }
    virtual bool pure_poll() override final {
        return poll(); // actually performs work, but triggers no user continuations, so okay
    }
    virtual bool try_enter_interrupt_mode() override {
        // Since we'll be sleeping in epoll, no need to do anything
        // for interrupt mode.
        return true;
    }
    virtual void exit_interrupt_mode() override final {
    }
};



inline
future<size_t> pollable_fd::read_some(char* buffer, size_t size) {
    return engine().read_some(*_s, buffer, size);
}

inline
future<size_t> pollable_fd::read_some(uint8_t* buffer, size_t size) {
    return engine().read_some(*_s, buffer, size);
}

inline
future<size_t> pollable_fd::read_some(const std::vector<iovec>& iov) {
    return engine().read_some(*_s, iov);
}

inline
future<> pollable_fd::write_all(const char* buffer, size_t size) {
    return engine().write_all(*_s, buffer, size);
}

inline
future<> pollable_fd::write_all(const uint8_t* buffer, size_t size) {
    return engine().write_all(*_s, buffer, size);
}
inline
size_t iovec_len(const std::vector<iovec>& iov)
{
    size_t ret = 0;
    for (auto&& e : iov) {
        ret += e.iov_len;
    }
    return ret;
}
inline
future<size_t> pollable_fd::write_some(net::packet& p) {
    return engine().writeable(*_s).then([this, &p] () mutable {
        static_assert(offsetof(iovec, iov_base) == offsetof(net::fragment, base) &&
            sizeof(iovec::iov_base) == sizeof(net::fragment::base) &&
            offsetof(iovec, iov_len) == offsetof(net::fragment, size) &&
            sizeof(iovec::iov_len) == sizeof(net::fragment::size) &&
            alignof(iovec) == alignof(net::fragment) &&
            sizeof(iovec) == sizeof(net::fragment)
            , "net::fragment and iovec should be equivalent");

        iovec* iov = reinterpret_cast<iovec*>(p.fragment_array());
        msghdr mh = {};
        mh.msg_iov = iov;
        mh.msg_iovlen = p.nr_frags();
        auto r = get_file_desc().sendmsg(&mh, MSG_NOSIGNAL);
        if (!r) {
            return write_some(p);
        }
        if (size_t(*r) == p.len()) {
            _s->speculate_epoll(EPOLLOUT);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<> pollable_fd::write_all(net::packet& p) {
    return write_some(p).then([this, &p] (size_t size) {
        if (p.len() == size) {
            return make_ready_future<>();
        }
        p.trim_front(size);
        return write_all(p);
    });
}

inline
future<> pollable_fd::readable() {
    return engine().readable(*_s);
}

inline
future<> pollable_fd::writeable() {
    return engine().writeable(*_s);
}

inline
void
pollable_fd::abort_reader(std::exception_ptr ex) {
    engine().abort_reader(*_s, std::move(ex));
}

inline
void
pollable_fd::abort_writer(std::exception_ptr ex) {
    engine().abort_writer(*_s, std::move(ex));
}

inline
future<pollable_fd, net::socket_address> pollable_fd::accept() {
    return engine().accept(*_s);
}

inline
future<size_t> pollable_fd::recvmsg(struct msghdr *msg) {
    return engine().readable(*_s).then([this, msg] {
        auto r = get_file_desc().recvmsg(msg, 0);
        if (!r) {
            return recvmsg(msg);
        }
        // We always speculate here to optimize for throughput in a workload
        // with multiple outstanding requests. This way the caller can consume
        // all messages without resorting to epoll. However this adds extra
        // recvmsg() call when we hit the empty queue condition, so it may
        // hurt request-response workload in which the queue is empty when we
        // initially enter recvmsg(). If that turns out to be a problem, we can
        // improve speculation by using recvmmsg().
        _s->speculate_epoll(EPOLLIN);
        return make_ready_future<size_t>(*r);
    });
};

inline
future<size_t> pollable_fd::sendmsg(struct msghdr* msg) {
    return engine().writeable(*_s).then([this, msg] () mutable {
        auto r = get_file_desc().sendmsg(msg, 0);
        if (!r) {
            return sendmsg(msg);
        }
        // For UDP this will always speculate. We can't know if there's room
        // or not, but most of the time there should be so the cost of mis-
        // speculation is amortized.
        if (size_t(*r) == iovec_len(msg->msg_iov, msg->msg_iovlen)) {
            _s->speculate_epoll(EPOLLOUT);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<size_t> pollable_fd::sendto(net::socket_address addr, const void* buf, size_t len) {
    return engine().writeable(*_s).then([this, buf, len, addr] () mutable {
        auto r = get_file_desc().sendto(addr, buf, len, 0);
        if (!r) {
            return sendto(std::move(addr), buf, len);
        }
        // See the comment about speculation in sendmsg().
        if (size_t(*r) == len) {
            _s->speculate_epoll(EPOLLOUT);
        }
        return make_ready_future<size_t>(*r);
    });
}



/* not yet implemented for OSv. TODO: do the notification like we do class smp. */


readable_eventfd writeable_eventfd::read_side() {
    return readable_eventfd(_fd.dup());
}

file_desc writeable_eventfd::try_create_eventfd(size_t initial) {
    assert(size_t(int(initial)) == initial);
    return file_desc::eventfd(initial, EFD_CLOEXEC);
}

void writeable_eventfd::signal(size_t count) {
    uint64_t c = count;
    auto r = _fd.write(&c, sizeof(c));
    assert(r == sizeof(c));
}

writeable_eventfd readable_eventfd::write_side() {
    return writeable_eventfd(_fd.get_file_desc().dup());
}

file_desc readable_eventfd::try_create_eventfd(size_t initial) {
    assert(size_t(int(initial)) == initial);
    return file_desc::eventfd(initial, EFD_CLOEXEC | EFD_NONBLOCK);
}

future<size_t> readable_eventfd::wait() {
    return engine().readable(*_fd._s).then([this] {
        uint64_t count;
        int r = ::read(_fd.get_fd(), &count, sizeof(count));
        assert(r == sizeof(count));
        return make_ready_future<size_t>(count);
    });
}



/// Executes a callable in a seastar thread.
/// Runs a block of code in a threaded context,
/// which allows it to block (using \ref future::get()).  The
/// result of the callable is returned as a future.
/// \param func a callable to be executed in a thread
/// \param args a parameter pack to be forwarded to \c func.
/// \return whatever \c func returns, as a future.
/// Clock used for scheduling threads



thread_local jmp_buf_link g_unthreaded_context;  // 在 jmp_buf_link init_switch_in 的时候用来初始化 g_current_context
thread_local jmp_buf_link* g_current_context;

namespace thread_impl {
    inline thread_context* get() {
        // std::cout<<"thread_impl::get "<<std::endl;
        // if(g_current_context->thread==nullptr){
        //     std::cout<<"thread_impl::get() return nullptr."<<std::endl;
        // }
        // else{
        //     std::cout<<" thread_impl::get() not NULL."<<std::endl;
        // }
        //每次都是nullptr.
        return g_current_context->thread;
    }
    inline bool should_yield() {
        std::cout<<"thread_impl::should yield"<<std::endl;
        if (need_preempt()) {
            return true;
        } else if (g_current_context->get_yield_at()) {
            return std::chrono::steady_clock::now() >= *(g_current_context->get_yield_at());
        } else {
            return false;
        }
    }
    void yield(){
        std::cout<<"thread_impl::yield"<<std::endl;
        g_current_context->thread->yield();
    }
    void switch_in(thread_context* to){
        std::cout<<"thread_impl::swtich in"<<std::endl;
        to->switch_in();
    }
    void switch_out(thread_context* from){
         std::cout<<"thread_impl::swtich out"<<std::endl;
        from->switch_out();
    }
    void init(){
        std::cout<<"thread_impl::init()"<<std::endl;
        g_unthreaded_context.link = nullptr;
        g_unthreaded_context.thread = nullptr;
        g_current_context = &g_unthreaded_context;
    }
}


// Define the static member
thread_local thread* thread::_current = nullptr;


class gate {
    size_t _count = 0;
    promise<>* _stopped_ptr = nullptr;
    promise<> _stopped_value;
public:
    void enter() {
        if (_stopped_ptr) {
            throw 1;
        }
        ++_count;
    }
    void leave() {
        --_count;
        if (!_count && _stopped_ptr) {
            _stopped_ptr->set_value();
        }
    }
    void check() {
        if (_stopped_ptr) {
            throw 1;
        }
    }
    future<> close() {
        assert(!_stopped_ptr && "gate::close() cannot be called more than once");
        _stopped_ptr = &_stopped_value;
        if (!_count) {
            _stopped_ptr->set_value();
        }
        return _stopped_ptr->get_future();
    }
    size_t get_count() const {
        return _count;
    }
};
template <typename Func>
inline
auto
with_gate(gate& g, Func&& func) {
    g.enter();
    return func().finally([&g] { g.leave(); });
}


// future<> later() {
//     promise<> p;
//     auto f = p.get_future();
//     engine().force_poll(); //把need_preempted改为true(这句是没有意义的)
//     ::schedule_normal(make_task([p = std::move(p)]() mutable {
//         p.set_value(); // 这段代码把一个p.set_value封装为一个task加到调度器中.
//     }));
//     return f;
// }


template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
future<semaphore_units<ExceptionFactory, Clock>>
get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
    return sem.wait(units).then([&sem, units] {
        return semaphore_units<ExceptionFactory, Clock>{ sem, units };
    });
}

template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
future<semaphore_units<ExceptionFactory, Clock>>
get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, typename basic_semaphore<ExceptionFactory, Clock>::time_point timeout) {
    return sem.wait(timeout, units).then([&sem, units] {
        return semaphore_units<ExceptionFactory, Clock>{ sem, units };
    });
}
template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
semaphore_units<ExceptionFactory, Clock>
consume_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
    sem.consume(units);
    return semaphore_units<ExceptionFactory, Clock>{ sem, units };
}

template <typename ExceptionFactory, typename Func, typename Clock = typename timer<>::clock>
inline
futurize_t<std::result_of_t<Func()>>
with_semaphore(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, Func&& func) {
    return get_units(sem, units).then([func = std::forward<Func>(func)] (auto units) mutable {
        return futurize_apply(std::forward<Func>(func)).finally([units = std::move(units)] {});
    });
}



template <typename Func, typename... Args>
inline futurize_t<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>>
async(Func&& func, Args&&... args) {
    return async(thread_attributes{}, std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename Func, typename... Args>
inline
futurize_t<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>>
async(thread_attributes attr, Func&& func, Args&&... args) {
    using return_type = std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>;
    struct work {
        thread_attributes attr;
        Func func;
        std::tuple<Args...> args;
        promise<return_type> pr;
        thread th;
    };
    return do_with(work{std::move(attr), std::forward<Func>(func), std::forward_as_tuple(std::forward<Args>(args)...)}, [] (work& w) mutable {
        auto ret = w.pr.get_future();
        w.th = thread(std::move(w.attr), [&w] {
            futurize<return_type>::apply(std::move(w.func), std::move(w.args)).forward_to(std::move(w.pr));
        });
        return w.th.join().then([ret = std::move(ret)] () mutable {
            return std::move(ret);
        });
    });
}


void report_failed_future(std::exception_ptr eptr) {
    std::cout<<"fail future";
   std::cout<<"####"<<std::endl;
}


// Define the static members
thread_local std::list<thread_context*> thread_context::_preempted_threads;
thread_local std::list<thread_context*> thread_context::_all_threads;








// #include "../util/shared_ptr.hh"



/// Smart pointer wrapper which makes it safe to move across CPUs.
/// \c foreign_ptr<> is a smart pointer wrapper which, unlike
/// \ref shared_ptr and \ref lw_shared_ptr, is safe to move to a
/// different core.
/// As seastar avoids locking, any but the most trivial objects must
/// be destroyed on the same core they were created on, so that,
/// for example, their destructors can unlink references to the
/// object from various containers.  In addition, for performance
/// reasons, the shared pointer types do not use atomic operations
/// to manage their reference counts.  As a result they cannot be
/// used on multiple cores in parallel.
/// \c foreign_ptr<> provides a solution to that problem.
/// \c foreign_ptr<> wraps any pointer type -- raw pointer,
/// \ref shared_ptr<>, or similar, and remembers on what core this
/// happened.  When the \c foreign_ptr<> object is destroyed, it
/// sends a message to the original core so that the wrapped object
/// can be safely destroyed.
/// \c foreign_ptr<> is a move-only object; it cannot be copied.

template <typename PtrType>
class foreign_ptr {
private:
    PtrType _value;
    unsigned _cpu;
private:
    bool on_origin() {
        return engine().cpu_id() == _cpu;
    }
public:
    using element_type = typename std::pointer_traits<PtrType>::element_type;

    /// Constructs a null \c foreign_ptr<>.
    foreign_ptr()
        : _value(PtrType())
        , _cpu(engine().cpu_id()) {
    }
    /// Constructs a null \c foreign_ptr<>.
    foreign_ptr(std::nullptr_t) : foreign_ptr() {}
    /// Wraps a pointer object and remembers the current core.
    foreign_ptr(PtrType value)
        : _value(std::move(value))
        , _cpu(engine().cpu_id()) {
    }
    // The type is intentionally non-copyable because copies
    // are expensive because each copy requires across-CPU call.
    foreign_ptr(const foreign_ptr&) = delete;
    /// Moves a \c foreign_ptr<> to another object.
    foreign_ptr(foreign_ptr&& other) = default;
    /// Destroys the wrapped object on its original cpu.
    ~foreign_ptr() {
        if (_value && !on_origin()) {
            smp::submit_to(_cpu, [v = std::move(_value)] () mutable {
                auto local(std::move(v));
            });
        }
    }
    /// Creates a copy of this foreign ptr. Only works if the stored ptr is copyable.
    future<foreign_ptr> copy() const {
        return smp::submit_to(_cpu, [this] () mutable {
            auto v = _value;
            return make_foreign(std::move(v));
        });
    }
    /// Accesses the wrapped object.
    element_type& operator*() const { return *_value; }
    /// Accesses the wrapped object.
    element_type* operator->() const { return &*_value; }
    /// Checks whether the wrapped pointer is non-null.
    operator bool() const { return static_cast<bool>(_value); }
    /// Move-assigns a \c foreign_ptr<>.
    foreign_ptr& operator=(foreign_ptr&& other) = default;
};

/// Wraps a raw or smart pointer object in a \ref foreign_ptr<>.
///
/// \relates foreign_ptr
template <typename T>
foreign_ptr<T> make_foreign(T ptr) {
    return foreign_ptr<T>(std::move(ptr));
}

#include <memory> // for std::unique_ptr

template<typename T>
struct is_smart_ptr : std::false_type {};

template<typename T>
struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};


template<typename T>
struct is_smart_ptr<::foreign_ptr<T>> : std::true_type {};

/// @}













GCC6_CONCEPT(
namespace impl {
// Want: folds
template <typename T>
struct is_tuple_of_futures : std::false_type {};

template <> struct is_tuple_of_futures<std::tuple<>> : std::true_type {};
template <typename... T, typename... Rest>
    struct is_tuple_of_futures<std::tuple<future<T...>, Rest...>> : is_tuple_of_futures<std::tuple<Rest...>> {
};
}
template <typename... Futs>
concept AllAreFutures = impl::is_tuple_of_futures<std::tuple<Futs...>>::value;
)




/// \cond internal
namespace internal {

// template <typename Iterator, typename IteratorCategory>
// inline
// size_t
// when_all_estimate_vector_capacity(Iterator begin, Iterator end, IteratorCategory category) {
//     // For InputIterators we can't estimate needed capacity
//     return 0;
// }

// template <typename Iterator>
// inline
// size_t
// when_all_estimate_vector_capacity(Iterator begin, Iterator end, std::forward_iterator_tag category) {
//     // May be linear time below random_access_iterator_tag, but still better than reallocation
//     return std::distance(begin, end);
// }

// template<typename Future>
// struct identity_futures_vector {
//     using future_type = future<std::vector<Future>>;
//     static future_type run(std::vector<Future> futures) {
//         return make_ready_future<std::vector<Future>>(std::move(futures));
//     }
// };

// // Internal function for when_all().
// template <typename ResolvedVectorTransform, typename Future>
// inline
// typename ResolvedVectorTransform::future_type
// complete_when_all(std::vector<Future>&& futures, typename std::vector<Future>::iterator pos) {
//     // If any futures are already ready, skip them.
//     while (pos != futures.end() && pos->available()) {
//         ++pos;
//     }
//     // Done?
//     if (pos == futures.end()) {
//         return ResolvedVectorTransform::run(std::move(futures));
//     }
//     // Wait for unready future, store, and continue.
//     return pos->then_wrapped([futures = std::move(futures), pos] (auto fut) mutable {
//         *pos++ = std::move(fut);
//         return complete_when_all<ResolvedVectorTransform>(std::move(futures), pos);
//     });
// }

// template<typename ResolvedVectorTransform, typename FutureIterator>
// inline auto
// do_when_all(FutureIterator begin, FutureIterator end) {
//     using itraits = std::iterator_traits<FutureIterator>;
//     std::vector<typename itraits::value_type> ret;
//     ret.reserve(when_all_estimate_vector_capacity(begin, end, typename itraits::iterator_category()));
//     // Important to invoke the *begin here, in case it's a function iterator,
//     // so we launch all computation in parallel.
//     std::move(begin, end, std::back_inserter(ret));
//     return complete_when_all<ResolvedVectorTransform>(std::move(ret), ret.begin());
// }

}


template <typename FutureIterator>
GCC6_CONCEPT( requires requires (FutureIterator i) { { *i++ }; requires is_future<std::remove_reference_t<decltype(*i)>>::value; } )





// inline
// future<std::vector<typename std::iterator_traits<FutureIterator>::value_type>>
// when_all(FutureIterator begin, FutureIterator end) {
//     namespace si = internal;
//     using itraits = std::iterator_traits<FutureIterator>;
//     using result_transform = si::identity_futures_vector<typename itraits::value_type>;
//     return si::do_when_all<result_transform>(std::move(begin), std::move(end));
// }

// template <typename T, bool IsFuture>
// struct reducer_with_get_traits;

// template <typename T>
// struct reducer_with_get_traits<T, false> {
//     using result_type = decltype(std::declval<T>().get());
//     using future_type = future<result_type>;
//     static future_type maybe_call_get(future<> f, std::shared_ptr<T> r) {
//         return f.then([r = std::move(r)] () mutable {
//             return make_ready_future<result_type>(std::move(*r).get());
//         });
//     }
// };

// template <typename T>
// struct reducer_with_get_traits<T, true> {
//     using future_type = decltype(std::declval<T>().get());
//     static future_type maybe_call_get(future<> f, std::shared_ptr<T> r) {
//         return f.then([r = std::move(r)] () mutable {
//             return r->get();
//         }).then_wrapped([r] (future_type f) {
//             return f;
//         });
//     }
// };

// template <typename T, typename V = void>
// struct reducer_traits {
//     using future_type = future<>;
//     static future_type maybe_call_get(future<> f, std::shared_ptr<T> r) {
//         return f.then([r = std::move(r)] {});
//     }
// // };

// template <typename T>
// struct reducer_traits<T, decltype(std::declval<T>().get(), void())> : public reducer_with_get_traits<T, is_future<std::result_of_t<decltype(&T::get)(T)>>::value> {};


// template <typename Iterator, typename Mapper, typename Reducer>
// inline
// auto
// map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
//     -> typename reducer_traits<Reducer>::future_type
// {
//     auto r_ptr = std::make_shared<Reducer>(std::forward<Reducer>(r));
//     future<> ret = make_ready_future<>();
//     using futurator = futurize<decltype(mapper(*begin))>;
//     while (begin != end) {
//         ret = futurator::apply(mapper, *begin++).then_wrapped([ret = std::move(ret), r_ptr] (auto f) mutable {
//             return ret.then_wrapped([f = std::move(f), r_ptr] (auto rf) mutable {
//                 if (rf.failed()) {
//                     f.ignore_ready_future();
//                     return std::move(rf);
//                 } else {
//                     return futurize<void>::apply(*r_ptr, std::move(f.get()));
//                 }
//             });
//         });
//     }
//     return reducer_traits<Reducer>::maybe_call_get(std::move(ret), r_ptr);
// }


// template <typename Iterator, typename Mapper, typename Initial, typename Reduce>
// GCC6_CONCEPT( requires requires (Iterator i, Mapper mapper, Initial initial, Reduce reduce) {
//     *i++;
//     { i != i } -> std::same_as<bool>;//为什么?
//     mapper(*i);
//     requires is_future<decltype(mapper(*i))>::value;
//     { reduce(std::move(initial), mapper(*i).get0()) } -> std::same_as<Initial>;
// } )
// inline
// future<Initial>
// map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Initial initial, Reduce reduce) {
//     struct state {
//         Initial result;
//         Reduce reduce;
//     };
//     auto s = std::make_shared(state{std::move(initial), std::move(reduce)});
//     future<> ret = make_ready_future<>();
//     using futurator = futurize<decltype(mapper(*begin))>;
//     while (begin != end) {
//         ret = futurator::apply(mapper, *begin++).then_wrapped([s = s.get(), ret = std::move(ret)] (auto f) mutable {
//             try {
//                 s->result = s->reduce(std::move(s->result), std::move(f.get0()));
//                 return std::move(ret);
//             } catch (...) {
//                 return std::move(ret).then_wrapped([ex = std::current_exception()] (auto f) {
//                     f.ignore_ready_future();
//                     return make_exception_future<>(ex);
//                 });
//             }
//         });
//     }
//     return ret.then([s] {
//         return make_ready_future<Initial>(std::move(s->result));
//     });
// }

// template <typename Range, typename Mapper, typename Initial, typename Reduce>
// GCC6_CONCEPT( requires requires (Range range, Mapper mapper, Initial initial, Reduce reduce) {
//      std::begin(range);
//      std::end(range);
//      mapper(*std::begin(range));
//      requires is_future<std::remove_reference_t<decltype(mapper(*std::begin(range)))>>::value;
//     { reduce(std::move(initial), mapper(*std::begin(range)).get0()) } -> std::same_as<Initial>;
// } )
// inline
// future<Initial>
// map_reduce(Range&& range, Mapper&& mapper, Initial initial, Reduce reduce) {
//     return map_reduce(std::begin(range), std::end(range), std::forward<Mapper>(mapper),
//             std::move(initial), std::move(reduce));
// }

// template <typename Result, typename Addend = Result>
// class adder {
// private:
//     Result _result;
// public:
//     future<> operator()(const Addend& value) {
//         _result += value;
//         return make_ready_future<>();
//     }
//     Result get() && {
//         return std::move(_result);
//     }
// };

static inline future<> now() {
    return make_ready_future<>();
}

future<> later(){
    promise<> p;
    auto f = p.get_future();
    engine().force_poll(); //把need_preempted改为true(这句是没有意义的)
    ::schedule_normal(make_task([p = std::move(p)]() mutable {
        p.set_value(); // 这段代码把一个p.set_value封装为一个task加到调度器中.
    }));
    return f;
}



namespace internal {
template<typename Future>
struct future_has_value {
    enum {
        value = !std::is_same<std::decay_t<Future>, future<>>::value
    };
};

template<typename Tuple>
struct tuple_to_future;
template<typename... Elements>
struct tuple_to_future<std::tuple<Elements...>> {
    using type = future<Elements...>;
    using promise_type = promise<Elements...>;
    static auto make_ready(std::tuple<Elements...> t) {
        auto create_future = [] (auto&&... args) {
            return make_ready_future<Elements...>(std::move(args)...);
        };
        return apply(create_future, std::move(t));
    }
    static auto make_failed(std::exception_ptr excp) {
        return make_exception_future<Elements...>(std::move(excp));
    }
};

template<typename... Futures>
class extract_values_from_futures_tuple {
    static auto transform(std::tuple<Futures...> futures) {
        auto prepare_result = [] (auto futures) {
            auto fs = tuple_filter_by_type<internal::future_has_value>(std::move(futures));
            return tuple_map(std::move(fs), [] (auto&& e) {
                return internal::untuple(e.get());
            });
        };
        using tuple_futurizer = internal::tuple_to_future<decltype(prepare_result(std::move(futures)))>;
        std::exception_ptr excp;
        tuple_for_each(futures, [&excp] (auto& f) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                }
            } else {
                f.ignore_ready_future();
            }
        });
        if (excp) {
            return tuple_futurizer::make_failed(std::move(excp));
        }
        return tuple_futurizer::make_ready(prepare_result(std::move(futures)));
    }
public:
    using future_type = decltype(transform(std::declval<std::tuple<Futures...>>()));
    using promise_type = typename future_type::promise_type;
    static void set_promise(promise_type& p, std::tuple<Futures...> tuple) {
        transform(std::move(tuple)).forward_to(std::move(p));
    }
};

template<typename Future>
struct extract_values_from_futures_vector {
    using value_type = decltype(untuple(std::declval<typename Future::value_type>()));
    using future_type = future<std::vector<value_type>>;
    static future_type run(std::vector<Future> futures) {
        std::vector<value_type> values;
        values.reserve(futures.size());

        std::exception_ptr excp;
        for (auto&& f : futures) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                } else {
                    values.emplace_back(untuple(f.get()));
                }
            } else {
                f.ignore_ready_future();
            }
        }
        if (excp) {
            return make_exception_future<std::vector<value_type>>(std::move(excp));
        }
        return make_ready_future<std::vector<value_type>>(std::move(values));
    }
};

template<>
struct extract_values_from_futures_vector<future<>> {
    using future_type = future<>;

    static future_type run(std::vector<future<>> futures) {
        std::exception_ptr excp;
        for (auto&& f : futures) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                }
            } else {
                f.ignore_ready_future();
            }
        }
        if (excp) {
            return make_exception_future<>(std::move(excp));
        }
        return make_ready_future<>();
    }
};
}



template <typename FutureIterator, typename = typename std::iterator_traits<FutureIterator>::value_type>
GCC6_CONCEPT( requires requires (FutureIterator i) {
    *i++;
    {i!= i} ->std::same_as<bool>;
     requires is_future<std::remove_reference_t<decltype(*i)>>::value;
})

// Implementation of global functions
void enable_timer(steady_clock_type::time_point when){
    engine().enable_timer(when);
}











class file_data_sink_impl : public data_sink_impl {
    file _file;
    file_output_stream_options _options;
    uint64_t _pos = 0;
    semaphore _write_behind_sem = { _options.write_behind };
    future<> _background_writes_done = make_ready_future<>();
    bool _failed = false;
public:
    file_data_sink_impl(file f, file_output_stream_options options)
            : _file(std::move(f)), _options(options) {
        _write_behind_sem.ensure_space_for_waiters(1); // So that wait() doesn't throw
    }
    future<> put(net::packet data) { abort(); }
    virtual temporary_buffer<char> allocate_buffer(size_t size) override {
        return temporary_buffer<char>::aligned(_file.memory_dma_alignment(), size);
    }
    virtual future<> put(temporary_buffer<char> buf) override {
        uint64_t pos = _pos;
        _pos += buf.size();
        if (!_options.write_behind) {
            return do_put(pos, std::move(buf));
        }
        // Write behind strategy:
        //
        // 1. Issue N writes in parallel, using a semaphore to limit to N
        // 2. Collect results in _background_writes_done, merging exception futures
        // 3. If we've already seen a failure, don't issue more writes.
        return _write_behind_sem.wait().then([this, pos, buf = std::move(buf)] () mutable {
            if (_failed) {
                _write_behind_sem.signal();
                auto ret = std::move(_background_writes_done);
                _background_writes_done = make_ready_future<>();
                return ret;
            }
            auto this_write_done = do_put(pos, std::move(buf)).finally([this] {
                _write_behind_sem.signal();
            });
            _background_writes_done = when_all(std::move(_background_writes_done), std::move(this_write_done))
                    .then([this] (std::tuple<future<>, future<>> possible_errors) {
                // merge the two errors, preferring the first
                auto& e1 = std::get<0>(possible_errors);
                auto& e2 = std::get<1>(possible_errors);
                if (e1.failed()) {
                    e2.ignore_ready_future();
                    return std::move(e1);
                } else {
                    if (e2.failed()) {
                        _failed = true;
                    }
                    return std::move(e2);
                }
            });
            return make_ready_future<>();
        });
    }
public:
    future<> do_put(uint64_t pos, temporary_buffer<char> buf) noexcept {
      try {
        // put() must usually be of chunks multiple of file::dma_alignment.
        // Only the last part can have an unaligned length. If put() was
        // called again with an unaligned pos, we have a bug in the caller.
        assert(!(pos & (_file.disk_write_dma_alignment() - 1)));
        bool truncate = false;
        auto p = static_cast<const char*>(buf.get());
        size_t buf_size = buf.size();

        if ((buf.size() & (_file.disk_write_dma_alignment() - 1)) != 0) {
            // If buf size isn't aligned, copy its content into a new aligned buf.
            // This should only happen when the user calls output_stream::flush().
            auto tmp = allocate_buffer(align_up(buf.size(), _file.disk_write_dma_alignment()));
            ::memcpy(tmp.get_write(), buf.get(), buf.size());
            buf = std::move(tmp);
            p = buf.get();
            buf_size = buf.size();
            truncate = true;
        }

        return _file.dma_write(pos, p, buf_size, _options.io_priority_class).then(
                [this, buf = std::move(buf), truncate] (size_t size) {
            if (truncate) {
                return _file.truncate(_pos);
            }
            return make_ready_future<>();
        });
      } catch (...) {
          return make_exception_future<>(std::current_exception());
      }
    }
    future<> wait() noexcept {
        // restore to pristine state; for flush() + close() sequence
        // (we allow either flush, or close, or both)
        return _write_behind_sem.wait(_options.write_behind).then([this] {
            return std::exchange(_background_writes_done, make_ready_future<>());
        }).finally([this] {
            _write_behind_sem.signal(_options.write_behind);
        });
    }
public:
    virtual future<> flush() override {
        return wait().then([this] {
            return _file.flush();
        });
    }
    virtual future<> close() noexcept {
        return wait().finally([this] {
            return _file.close();
        });
    }
};

class file_data_sink : public data_sink {
public:
    file_data_sink(file f, file_output_stream_options options)
        : data_sink(std::make_unique<file_data_sink_impl>(
                std::move(f), options)) {}
};

output_stream<char> make_file_output_stream(file f, size_t buffer_size) {
    file_output_stream_options options;
    options.buffer_size = buffer_size;
    return make_file_output_stream(std::move(f), options);
}

output_stream<char> make_file_output_stream(file f, file_output_stream_options options) {
    return output_stream<char>(file_data_sink(std::move(f), options), options.buffer_size, true);
}










/*0000000000000000000000000000000000000000000000000000000000*/
/*0000000000000000000000000000000000000000000000000000000000*/



void reactor::enable_timer(steady_clock_type::time_point when) {
    itimerspec its;
    its.it_interval = {};
    its.it_value = to_timespec(when);
    auto ret = timer_settime(_steady_clock_timer, TIMER_ABSTIME, &its, NULL);
    // throw_system_error_on(ret == -1);
}


/*  信号触发​:默认情况下，定时器到期会发送一个信号（如 SIGALRM）到进程。进程可以通过信号处理函数来处理定时器到期事件. */

void reactor::add_timer(steady_timer *tmr) {
    // std::cout<<"reactor add timer"<<std::endl;
    if (queue_timer(tmr)) {
        enable_timer(_timers.get_next_timeout());
    }
}
void reactor::add_timer(lowres_timer* tmr) {
    if (queue_timer(tmr)) {
        _lowres_next_timeout = _lowres_timers.get_next_timeout();
    }
}


bool reactor::queue_timer(lowres_timer* tmr) {
    // std::cout<< "reactor add lowres timer"<<std::endl;
    return _lowres_timers.insert(*tmr);
}



bool reactor::queue_timer(steady_timer* tmr) {
    // std::cout<< "reactor add steady timer"<<std::endl;
    return _timers.insert(*tmr);
}

/* del_timer什么时候调用? */
void reactor::del_timer(steady_timer* tmr) {
    if (tmr->_expired) {
        _expired_timers.erase(tmr->expired_it);  // 直接使用保存的迭代器
        tmr->_expired = false;
    } else {
        _timers.remove(*tmr);  // 通过 it 成员快速删除
    }
}

// 同理修改其他 del_timer 函数：
void reactor::del_timer(lowres_timer* tmr) {
    if (tmr->_expired) {
        _expired_lowres_timers.erase(tmr->expired_it);
        tmr->_expired = false;
    } else {
        _lowres_timers.remove(*tmr);
    }
}





void engine_exit(std::exception_ptr eptr) {
    if (!eptr) {
        engine().exit(0);
        return;
    }
    std::cout<<"Exception: "<< std::endl;
    engine().exit(1);
}





bool thread::try_run_one_yielded_thread() {
    if (thread_context::_preempted_threads.empty()) {
        return false;
    }
    auto* t = thread_context::_preempted_threads.front();
    t->_sched_timer.cancel();
    t->_sched_promise_ptr->set_value();
    thread_context::_preempted_threads.pop_front();
    return true;
}
thread_scheduling_group::thread_scheduling_group(std::chrono::nanoseconds period, float usage)
        : _period(period), _quota(std::chrono::duration_cast<std::chrono::nanoseconds>(usage * period)) {
}

void thread_scheduling_group::account_start() {
    auto now = thread_clock::now();
    if (now >= _this_period_ends) {
        _this_period_ends = now + _period;
        _this_period_remain = _quota;
    }
    _this_run_start = now;
}

void thread_scheduling_group::account_stop() {
    _this_period_remain -= thread_clock::now() - _this_run_start;
}

std::chrono::steady_clock::time_point*
thread_scheduling_group::next_scheduling_point() const {
    auto now = thread_clock::now();
    auto current_remain = _this_period_remain - (now - _this_run_start);
    if (current_remain > std::chrono::nanoseconds(0)) {
        return nullptr;
    }
    static std::chrono::steady_clock::time_point result;
    result = _this_period_ends - current_remain;
    return &result;
}

// Constructor that takes a callable object
template <typename Func>
thread::thread(Func func) : thread(thread_attributes(), std::move(func)){}

// Constructor that takes thread attributes and a callable object
template <typename Func>
thread::thread(thread_attributes attr, Func func)
    : _context(std::make_unique<thread_context>(std::move(attr), std::move(func))) {
        std::cout<<"创建了一个用户态线程"<<std::endl;}

//没调用过.
void thread::yield() {
    std::cout<<"thread::yield"<<std::endl;
    thread_impl::get()->yield();
}

//没调用过.
bool thread::should_yield() {
    std::cout<<"thread::should_yield"<<std::endl;
    return thread_impl::get()->should_yield();
}


// Destructor
thread::~thread() {
    assert(!_context || _context->_joined);
}

inline void jmp_buf_link::initial_switch_in(ucontext_t* initial_context, const void*, size_t)
{
    auto prev = std::exchange(g_current_context, this);
    link = prev;
    if (setjmp(prev->jmpbuf) == 0) {
        //  如果第一次setjmp
        setcontext(initial_context);
        //  这里会跳转到initial_context的入口函数中去执行.
        //  使用setcontext而不是longjmp，需要设置完整的初始上下文.
    }
    /*
    在这个过程中,已经执行完了绑定在线程上的回调函数。
    */
    std::cout<<"final long jmp"<<std::endl;
}


inline void jmp_buf_link::switch_in()
{
    auto prev = std::exchange(g_current_context, this);
    link = prev;
    if (setjmp(prev->jmpbuf) == 0) {
        longjmp(jmpbuf, 1);
    }
}

inline void jmp_buf_link::switch_out(){
    g_current_context = link;
    if (setjmp(jmpbuf) == 0) {
        longjmp(g_current_context->jmpbuf, 1);
    }
}

inline void jmp_buf_link::initial_switch_in_completed(){}

inline void jmp_buf_link::final_switch_out(){
    g_current_context = link;//link就是该context对应的上一个context(恢复).
    longjmp(g_current_context->jmpbuf, 1);//使用longjmp跳转到当前context的jmpbuf
    //这个可能没用？
}

thread_context::~thread_context() {
    _all_threads.erase(_all_it);//为什么？
    std::cout<<"thread_context析构"<<std::endl;
}


void thread_context::yield() {
    if (!_attr.scheduling_group) {
        later().get();
    }
    else
    {
        auto when = _attr.scheduling_group->next_scheduling_point();
        if (when) {
            _preempted_it = _preempted_threads.insert(_preempted_threads.end(), this);
            set_sched_promise();
            auto fut = get_sched_promise()->get_future();
            _sched_timer.arm(*when);
            fut.get();
            clear_sched_promise();
        } else if (need_preempt()) {
            later().get();
        }
    }
}

void thread_context::reschedule() {
    _preempted_threads.erase(_preempted_it);
    _sched_promise_ptr->set_value();
}

void thread_context::s_main(unsigned int lo, unsigned int hi) {
    uintptr_t q = lo | (uint64_t(hi) << 32);
    reinterpret_cast<thread_context*>(q)->main();
}

void
thread_context::main() {
    _context.initial_switch_in_completed();//这里什么都没有执行.
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_start();
        //没有执行到这里.
    }
    try {

        _func();            //执行线程绑定在context的函数.
        _done.set_value(); // done的类型是promise<>，set_value把done对应的future_state<>状态设置为result.
    } catch (...) {
        _done.set_exception(std::current_exception());
    }
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_stop();
    }
    _context.final_switch_out();
}

smp_message_queue::smp_message_queue(reactor* from, reactor* to) : _pending(to),_completed(from){ }

void smp_message_queue::stop() {
    // _metrics.clear();
}
void smp_message_queue::move_pending() {
    auto begin = _tx.a.pending_fifo.cbegin();
    auto end = _tx.a.pending_fifo.cend();
    end = _pending.push(begin, end);
    if (begin == end) {
        return;
    }
    auto nr = end - begin;
    _pending.maybe_wakeup();
    _tx.a.pending_fifo.erase(begin, end);
    _current_queue_length += nr;
    _last_snt_batch = nr;
    _sent += nr;
}

bool smp_message_queue::pure_poll_tx() const {
    // can't use read_available(), not available on older boost
    // empty() is not const, so need const_cast.
    return !const_cast<lf_queue&>(_completed).empty();
}

void smp_message_queue::submit_item(std::unique_ptr<smp_message_queue::work_item> item) {
    _tx.a.pending_fifo.push_back(item.get());
    item.release();
    if (_tx.a.pending_fifo.size() >= batch_size) {
        move_pending();
    }
}

void smp_message_queue::respond(work_item* item) {
    _completed_fifo.push_back(item);
    if (_completed_fifo.size() >= batch_size || engine()._stopped) {
        flush_response_batch();
    }
}

void smp_message_queue::flush_response_batch() {
    if (!_completed_fifo.empty()) {
        auto begin = _completed_fifo.cbegin();
        auto end = _completed_fifo.cend();
        end = _completed.push(begin, end);
        if (begin == end) {
            return;
        }
        _completed.maybe_wakeup();
        _completed_fifo.erase(begin, end);
    }
}

bool smp_message_queue::has_unflushed_responses() const {
    return !_completed_fifo.empty();
}

bool smp_message_queue::pure_poll_rx() const {
    // can't use read_available(), not available on older boost
    // empty() is not const, so need const_cast.
    return !const_cast<lf_queue&>(_pending).empty();
}

void
smp_message_queue::lf_queue::maybe_wakeup() {
    // Called after lf_queue_base::push().
    //
    // This is read-after-write, which wants memory_order_seq_cst,
    // but we insert that barrier using systemwide_memory_barrier()
    // because seq_cst is so expensive.
    //
    // However, we do need a compiler barrier:
    std::atomic_signal_fence(std::memory_order_seq_cst);
    if (remote->_sleeping.load(std::memory_order_relaxed)) {
        // We are free to clear it, because we're sending a signal now
        remote->_sleeping.store(false, std::memory_order_relaxed);
        remote->wakeup();
    }
}

template<size_t PrefetchCnt, typename Func>
size_t smp_message_queue::process_queue(lf_queue& q, Func process) {
    // copy batch to local memory in order to minimize
    // time in which cross-cpu data is accessed
    work_item* items[queue_length + PrefetchCnt];
    work_item* wi;
    if (!q.pop(wi))
        return 0;
    // start prefecthing first item before popping the rest to overlap memory
    // access with potential cache miss the second pop may cause
    prefetch<2>(wi);
    auto nr = q.pop(items);
    std::fill(std::begin(items) + nr, std::begin(items) + nr + PrefetchCnt, nr ? items[nr - 1] : wi);
    unsigned i = 0;
    do {
        prefetch_n<2>(std::begin(items) + i, std::begin(items) + i + PrefetchCnt);
        process(wi);
        wi = items[i++];
    } while(i <= nr);
    return nr + 1;
}

size_t smp_message_queue::process_completions() {
    auto nr = process_queue<prefetch_cnt*2>(_completed, [] (work_item* wi) {
        wi->complete();
        delete wi;
    });
    _current_queue_length -= nr;
    _compl += nr;
    _last_cmpl_batch = nr;
    return nr;
}

void smp_message_queue::flush_request_batch() {
    if (!_tx.a.pending_fifo.empty()) {
        move_pending();
    }
}

size_t smp_message_queue::process_incoming() {
    auto nr = process_queue<prefetch_cnt>(_pending, [this] (work_item* wi) {
        wi->process().then([this, wi] {
            respond(wi);
        });
    });
    _received += nr;
    _last_rcv_batch = nr;
    return nr;
}

void smp_message_queue::start(unsigned cpuid) {
    _tx.init();
    char instance[10];
    std::snprintf(instance, sizeof(instance), "%u-%u", engine().cpu_id(), cpuid);
}

bool reactor::do_check_lowres_timers() const{
    if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
        return false;
    }
    return lowres_clock::now() > engine()._lowres_next_timeout;
}

thread_context::stack_holder
thread_context::make_stack() {
    auto stack = stack_holder(new char[_stack_size]);
    return stack;
}

thread_context::thread_context(thread_attributes attr, std::function<void ()> func)
        : _attr(std::move(attr))
        , _func(std::move(func)) {
    setup();
    _all_threads.push_front(this);
    _all_it = _all_threads.begin();
    std::cout<<"添加用户态线程"<<std::endl;
    //为什么这里是this,而不是*this，而不是_all_it?思考
    //因为_all_threads存放的就是thread_context*，所以添加的也是指针。
}



void thread_context::stack_deleter::operator()(char* ptr) const noexcept {
    delete[] ptr;
}

void
thread_context::setup() {
    std::cout<<"thread_context setup"<<std::endl;
    // use setcontext() for the initial jump, as it allows us
    // to set up a stack, but continue with longjmp() as it's much faster.
    ucontext_t initial_context;
    auto q = uint64_t(reinterpret_cast<uintptr_t>(this));//将thread_
    auto main = reinterpret_cast<void (*)()>(&thread_context::s_main);
    auto r = getcontext(&initial_context);//保存当前上下文到initial_context中.
    // throw_system_error_on(r == -1);
    initial_context.uc_stack.ss_sp = _stack.get(); //设置栈空间
    initial_context.uc_stack.ss_size = _stack_size;
    initial_context.uc_link = nullptr;
    makecontext(&initial_context, main, 2, int(q), int(q >> 32));  //makecontext前32位，后32位.
    _context.thread = this;//_context是jmp_buf_link类型.(绑定父类型)
    _context.initial_switch_in(&initial_context, _stack.get(), _stack_size);//进入这个函数准备执行了s_main
}

void thread_context::switch_in() {
    std::cout<<"thread_context switch_in"<<std::endl;
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_start();
        _context.set_yield_at(_attr.scheduling_group->_this_run_start + _attr.scheduling_group->_this_period_remain);
    } else {
        _context.clear_yield_at();//设置_context的yield_为false
    }
    _context.switch_in();
}

void thread_context::switch_out() {
    std::cout<<"thread_context switch_out"<<std::endl;
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_stop();
    }
    _context.switch_out();
}

bool thread_context::should_yield() const {
    std::cout<<"thread_context should_yield"<<std::endl;
    if (!_attr.scheduling_group) {
        return need_preempt();
    }
    return need_preempt() || bool(_attr.scheduling_group->next_scheduling_point());
}

class vector_data_sink final : public data_sink_impl {
public:
    using vector_type = std::vector<net::packet>;
private:
    vector_type& _v;
public:
    vector_data_sink(vector_type& v) : _v(v) {}
    virtual future<> put(net::packet p) override {
        _v.push_back(std::move(p));
        return make_ready_future<>();
    }
    virtual future<> close() override {
        // TODO: close on local side
        return make_ready_future<>();
    }
};
namespace net {

class packet_data_source final : public data_source_impl {
    size_t _cur_frag = 0;
    packet _p;
public:
    explicit packet_data_source(net::packet&& p)
        : _p(std::move(p))
    {}
    virtual future<temporary_buffer<char>> get() override {
        if (_cur_frag != _p.nr_frags()) {
            auto& f = _p.fragments()[_cur_frag++];
            return make_ready_future<temporary_buffer<char>>(
                    temporary_buffer<char>(f.base, f.size,
                            make_deleter(deleter(), [p = _p.share()] () mutable {})));
        }
        return make_ready_future<temporary_buffer<char>>(temporary_buffer<char>());
    }
};

static inline input_stream<char> as_input_stream(packet&& p) {
    return input_stream<char>(data_source(std::make_unique<packet_data_source>(std::move(p))));
}

}

namespace net {

constexpr size_t packet::internal_data_size;
constexpr size_t packet::default_nr_frags;

void packet::linearize(size_t at_frag, size_t desired_size) {
    _impl->unuse_internal_data();
    size_t nr_frags = 0;
    size_t accum_size = 0;
    while (accum_size < desired_size) {
        accum_size += _impl->_frags[at_frag + nr_frags].size;
        ++nr_frags;
    }
    std::unique_ptr<char[]> new_frag{new char[accum_size]};
    auto p = new_frag.get();
    for (size_t i = 0; i < nr_frags; ++i) {
        auto& f = _impl->_frags[at_frag + i];
        p = std::copy(f.base, f.base + f.size, p);
    }
    // collapse nr_frags into one fragment
    std::copy(_impl->_frags + at_frag + nr_frags, _impl->_frags + _impl->_nr_frags,
            _impl->_frags + at_frag + 1);
    _impl->_nr_frags -= nr_frags - 1;
    _impl->_frags[at_frag] = fragment{new_frag.get(), accum_size};
    if (at_frag == 0 && desired_size == len()) {
        // We can drop the old buffer safely
        auto x = std::move(_impl->_deleter);
        _impl->_deleter = make_deleter([buf = std::move(new_frag)] {});
    } else {
        _impl->_deleter = make_deleter(std::move(_impl->_deleter), [buf = std::move(new_frag)] {});
    }
}

packet packet::free_on_cpu(unsigned cpu, std::function<void()> cb) {
    // make new deleter that runs old deleter on an origin cpu
    _impl->_deleter = make_deleter(deleter(), [d = std::move(_impl->_deleter), cpu, cb = std::move(cb)] () mutable {
        smp::submit_to(cpu, [d = std::move(d), cb = std::move(cb)] () mutable {
            // deleter needs to be moved from lambda capture to be destroyed here
            // otherwise deleter destructor will be called on a cpu that called smp::submit_to()
            // when work_item is destroyed.
            deleter xxx(std::move(d));
            cb();
        });
    });

    return packet(impl::copy(_impl.get()));
}

}











signals::signals() : _pending_signals(0) {
}

signals::~signals() {
    sigset_t mask;
    sigfillset(&mask);
    ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

signals::signal_handler::signal_handler(int signo, std::function<void ()>&& handler)
        : _handler(std::move(handler)) {
    struct sigaction sa;
    sa.sa_sigaction = action;//这个是信号处理函数.
    sa.sa_mask = make_empty_sigset_mask();
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
    auto r = ::sigaction(signo, &sa, nullptr);
    // throw_system_error_on(r == -1);
    auto mask = make_sigset_mask(signo);
    r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    throw_pthread_error(r);
}

void signals::handle_signal(int signo, std::function<void ()>&& handler) {
    _signal_handlers.emplace(std::piecewise_construct,std::make_tuple(signo), std::make_tuple(signo, std::move(handler)));
    //插入singo,和对应的handler.
}

void signals::handle_signal_once(int signo, std::function<void ()>&& handler) {
    return handle_signal(signo, [fired = false, handler = std::move(handler)] () mutable {
        if (!fired) {
            fired = true;
            handler();
        }
    });
}

bool signals::poll_signal() {
    auto signals = _pending_signals.load(std::memory_order_relaxed);
    //为什么一直是0.

    if (signals) {
            //std::cout<<"signals "<<signals<<std::endl;
        _pending_signals.fetch_and(~signals, std::memory_order_relaxed);
        for (size_t i = 0; i < sizeof(signals)*8; i++) {
            // std::cout<<"遍历"<<std::endl;
            if (signals & (1ull << i)) {
                //执行handler
                // std::cout<<"执行handler"<<std::endl;
               _signal_handlers.at(i)._handler();
            }
        }
    }
    return signals;
}
bool signals::pure_poll_signal() const {
    return _pending_signals.load(std::memory_order_relaxed);
}

void signals::action(int signo, siginfo_t* siginfo, void* ignore) {
    // std::cout<<"action########"<<std::endl;
    engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
}






// Join function
future<> thread::join() {
    _context->_joined = true;
    return _context->_done.get_future();
}



template <typename T, typename E, typename EnableFunc>
void reactor::complete_timers(T& timers, E& expired_timers, EnableFunc&& enable_fn) {
    expired_timers = timers.expire(timers.now()); // 获取过期的定时器
    //std::cout << "Expired " << expired_timers.size() << " timers" << std::endl;
    // 处理所有过期定时器
    for (auto* timer_ptr : expired_timers) {
        if (timer_ptr) {
            // std::cout << "Marking timer " << " as expired" << std::endl;
            timer_ptr->_expired = true;
        }
    }
    // arm表示定时器是否有一个过期时间.
    while (!expired_timers.empty()) {
        auto* timer_ptr = expired_timers.front();
        expired_timers.pop_front();
        if (timer_ptr) {
            // std::cout << "Processing timer "<<std::endl;
            timer_ptr->_queued = false;
            if (timer_ptr->_armed) {
                timer_ptr->_armed = false;
                if (timer_ptr->_period) {
                    // std::cout << "Re-adding periodic timer " << timer_ptr->_timerid << std::endl;
                    timer_ptr->readd_periodic();//周期定时器

                }
                try {
                    // std::cout << "Executing timer callback for " << std::endl;
                    timer_ptr->_callback();
                } catch (const std::exception& e) {
                    // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed: " << e.what() << std::endl;
                } catch (...) {
                    // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed with unknown error" << std::endl;
                }
            }
        }
    }

    enable_fn();
}

static decltype(auto) install_signal_handler_stack() {
    size_t size = SIGSTKSZ;
    auto mem = std::make_unique<char[]>(size);
    stack_t stack;
    stack_t prev_stack;
    stack.ss_sp = mem.get();
    stack.ss_flags = 0;
    stack.ss_size = size;
    auto r = sigaltstack(&stack, &prev_stack);
    assert(r == 0);
    // throw_system_error_on(r == -1);
    return defer([mem = std::move(mem), prev_stack] () mutable {
        try {
            auto r = sigaltstack(&prev_stack, NULL);
            // throw_system_error_on(r == -1);
            assert(r == 0);
        } catch (...) {
            mem.release(); // We failed to restore previous stack, must leak it.
            // std::cout<<< "Failed to restore previous signal stack" << std::endl;
        }
    });
}


bool
reactor::poll_once() {
    bool work = false;
    for (auto c : _pollers) {
        work |= c->poll();
    }
    return work;
}

template <typename Integral>
inline Integral increment_nonatomically(std::atomic<Integral>& value) {
    auto tmp = value.load(std::memory_order_relaxed);
    value.store(tmp + 1, std::memory_order_relaxed);
    return tmp;
}

int reactor::run(){
    auto signal_stack = install_signal_handler_stack();
    poller io_poller(std::make_unique<io_pollfn>(*this));
    poller sig_poller(std::make_unique<signal_pollfn>(*this));
    poller aio_poller(std::make_unique<aio_batch_submit_pollfn>(*this));
    poller batch_flush_poller(std::make_unique<batch_flush_pollfn>(*this));
    poller execution_stage_poller(std::make_unique<execution_stage_pollfn>());
    start_aio_eventfd_loop();
    if (_id == 0) {
       if (_handle_sigint) {
        //这里为true.
          _signals.handle_signal_once(SIGINT, [this] { stop(); });
       }
       _signals.handle_signal_once(SIGTERM, [this] { stop(); });
    }
    _signals.handle_signal(alarm_signal(), [this] {
        complete_timers(_timers, _expired_timers, [this] {
        if (!_timers.empty()) {
                // std::cout << "Enabling timer for " << _timers.get_next_timeout()<<std::endl;//这行是有执行的.
                enable_timer(_timers.get_next_timeout());
            }
        });
    });
    _cpu_started.wait(smp::count).then([this] {
        _network_stack->initialize().then([this] {
            _start_promise.set_value();
        });
    });
    _network_stack_ready_promise.get_future().then([this] (std::unique_ptr<network_stack> stack) {
        std::cout<<"reactor network_stack"<<std::endl;
        _network_stack = std::move(stack);
        // _network_stack->print_stack();
        for (unsigned c = 0; c < smp::count; c++) {
            smp::submit_to(c, [] {
                    engine()._cpu_started.signal();
            });
        }
    });
    // Register smp queues poller
    std::optional<poller> smp_poller;
    if (smp::count > 1) {
        smp_poller = poller(std::make_unique<smp_pollfn>(*this));
    }
    poller syscall_poller(std::make_unique<syscall_pollfn>(*this));
    _signals.handle_signal(alarm_signal(), [this] {
        complete_timers(_timers, _expired_timers, [this] {
            if (!_timers.empty()) {
                enable_timer(_timers.get_next_timeout());
            }
        });
    });
    poller drain_cross_cpu_freelist(std::make_unique<drain_cross_cpu_freelist_pollfn>());
    poller expire_lowres_timers(std::make_unique<lowres_timer_pollfn>(*this));
    using namespace std::chrono_literals;
    timer<lowres_clock> load_timer;
    auto last_idle = _total_idle;
    auto idle_start = steady_clock_type::now(), idle_end = idle_start;
    load_timer.set_callback([this, &last_idle, &idle_start, &idle_end] () mutable {
        _total_idle += idle_end - idle_start;
        auto load = double((_total_idle - last_idle).count()) / double(std::chrono::duration_cast<steady_clock_type::duration>(1s).count());
        last_idle = _total_idle;
        load = std::min(load, 1.0);
        idle_start = idle_end;
        _loads.push_front(load);
        if (_loads.size() > 5) {
            auto drop = _loads.back();
            _loads.pop_back();
            _load -= (drop/5);
        }
        _load += (load/5);
    });
    load_timer.arm_periodic(1s);
    itimerspec its = posix::to_relative_itimerspec(_task_quota, _task_quota);
    _task_quota_timer.timerfd_settime(0, its);
    auto& task_quote_itimerspec = its;
    struct sigaction sa_block_notifier = {};
    sa_block_notifier.sa_handler = &reactor::block_notifier;
    sa_block_notifier.sa_flags = SA_RESTART;
    auto r = sigaction(block_notifier_signal(), &sa_block_notifier, nullptr);
    assert(r == 0);
    bool idle = false;
    std::function<bool()> check_for_work = [this] () {
        return poll_once() || !_pending_tasks.empty() || thread::try_run_one_yielded_thread();
    };
    std::function<bool()> pure_check_for_work = [this] () {
        return pure_poll_once() || !_pending_tasks.empty() || thread::try_run_one_yielded_thread();
    };

    while(true){
        run_tasks(_pending_tasks);
        if (_stopped) {
            load_timer.cancel();
            // Final tasks may include sending the last response to cpu 0, so run them
            while (!_pending_tasks.empty()) {
                run_tasks(_pending_tasks);
            }
            while (!_at_destroy_tasks.empty()) {
                run_tasks(_at_destroy_tasks);
            }
            smp::arrive_at_event_loop_end();
            if (_id == 0) {
                smp::join_all();
            }
            break;
        }

        increment_nonatomically(_polls);

        if (check_for_work()) {
            if (idle) {
                _total_idle += idle_end - idle_start;
                idle_start = idle_end;
                idle = false;
            }
        } else {
            idle_end = steady_clock_type::now();
            if (!idle) {
                idle_start = idle_end;
                idle = true;
            }
            bool go_to_sleep = true;
            try {
                // we can't run check_for_work(), because that can run tasks in the context
                // of the idle handler which change its state, without the idle handler expecting
                // it.  So run pure_check_for_work() instead.
                auto handler_result = _idle_cpu_handler(pure_check_for_work);
                go_to_sleep = handler_result == idle_cpu_handler_result::no_more_work;
            } catch (...) {
                throw std::runtime_error("idle_cpu_handler() threw exception");
            }
            if (go_to_sleep) {
                _mm_pause();
                if (idle_end - idle_start > _max_poll_time) {
                    // Turn off the task quota timer to avoid spurious wakeiups
                    struct itimerspec zero_itimerspec = {};
                    _task_quota_timer.timerfd_settime(0, zero_itimerspec);
                    sleep();
                    // We may have slept for a while, so freshen idle_end
                    idle_end = steady_clock_type::now();
                    _task_quota_timer.timerfd_settime(0, task_quote_itimerspec);
                }
            } else {
                // We previously ran pure_check_for_work(), might not actually have performed
                // any work.
                check_for_work();
            }
        }
    }
    my_io_queue.reset(nullptr);
    return _return;
}

__thread reactor* local_engine;
reactor& engine(){
    return *local_engine;
}

boost::program_options::options_description
reactor::get_options_description() {
    namespace bpo = boost::program_options;
    bpo::options_description opts("Core options");
    // auto net_stack_names = network_stack_registry::list();
    opts.add_options()
        // ("network-stack", bpo::value<std::string>(),
        //         sprint("select network stack (valid values: %s)",
        //                 format_separated(net_stack_names.begin(), net_stack_names.end(), ", ")).c_str())
        ("no-handle-interrupt", "ignore SIGINT (for gdb)")
        ("poll-mode", "poll continuously (100% cpu use)")
        ("idle-poll-time-us", bpo::value<unsigned>()->default_value(200us / 1us),
                "idle polling time in microseconds (reduce for overprovisioned environments or laptops)")
        ("poll-aio", bpo::value<bool>()->default_value(true),
                "busy-poll for disk I/O (reduces latency and increases throughput)")
        ("task-quota-ms", bpo::value<double>()->default_value(2.0), "Max time (ms) between polls")
        ("max-task-backlog", bpo::value<unsigned>()->default_value(1000), "Maximum number of task backlog to allow; above this we ignore I/O")
        ("blocked-reactor-notify-ms", bpo::value<unsigned>()->default_value(2000), "threshold in miliseconds over which the reactor is considered blocked if no progress is made")
        ("relaxed-dma", "allow using buffered I/O if DMA is not available (reduces performance)")
        ("overprovisioned", "run in an overprovisioned environment (such as docker or a laptop); equivalent to --idle-poll-time-us 0 --thread-affinity 0 --poll-aio 0")
        ("abort-on-seastar-bad-alloc", "abort when seastar allocator cannot allocate memory");
    // opts.add(network_stack_registry::options_description());
    return opts;
}


void reactor::configure(boost::program_options::variables_map vm) {
    std::cout<<"reactor::configure"<<std::endl;
    auto network_stack_ready = network_stack_registry::create(vm);

    network_stack_ready.then([this] (std::unique_ptr<network_stack> stack) {
        std::cout<<"network_statck_ready"<<std::endl;   // 这个有被执行到.
        _network_stack_ready_promise.set_value(std::move(stack));
    });
    std::cout<<"reactor::configure完成"<<std::endl;
    /*上面这段代码什么意思？*/
    std::cout<<"reactor::configure"<<std::endl;
    _handle_sigint = !vm.count("no-handle-interrupt");
    std::cout<<"reactor::configure  _handle_sigint:"<<_handle_sigint<<std::endl;
    _task_quota = vm["task-quota-ms"].as<double>()*1ms;
    std::cout<<"reactor::configure  _task_quota:"<<std::endl;
    auto blocked_time = vm["blocked-reactor-notify-ms"].as<unsigned>()*1ms;
    std::cout<<"reactor::configure  blocked_time:"<<std::endl;
    _tasks_processed_report_threshold = unsigned(blocked_time / _task_quota);
    std::cout<<"reactor::configure  _tasks_processed_report_threshold:"<<std::endl;
    _max_task_backlog = vm["max-task-backlog"].as<unsigned>();
    std::cout<<"reactor::configure  _max_task_backlog:"<<std::endl;
    // _max_poll_time = vm["idle-poll-time-us"].as<unsigned>() * 1us;
    _max_poll_time = 200us;
    std::cout<<"reactor::configure  _max_poll_time:"<<std::endl;
    if (vm.count("poll-mode")) {
        std::cout<<"reactor::configure  poll_mode"<<std::endl;
        _max_poll_time = std::chrono::nanoseconds::max();
    }
    if (vm.count("overprovisioned")
           && vm["idle-poll-time-us"].defaulted()
           && !vm.count("poll-mode")) {
            std::cout<<"reactor::configure  overprovisioned"<<std::endl;
        _max_poll_time = 0us;
    }
    set_strict_dma(!vm.count("relaxed-dma"));
    std::cout<<"reactor::configure  _strict_dma"<<std::endl;

    // if (!vm["poll-aio"].as<bool>()
    //         || (vm["poll-aio"].defaulted() && vm.count("overprovisioned"))) {
    //     _aio_eventfd = pollable_fd(file_desc::eventfd(0, 0));
    // }
}
future<> reactor::run_exit_tasks() {
    _stop_requested.broadcast();
    _stopping = true;
    // // stop_aio_eventfd_loop();
    // return do_for_each(_exit_funcs.rbegin(), _exit_funcs.rend(), [] (auto& func) {
    //    return func();
    // });
}

reactor::reactor(unsigned id)
    : _id(id)
    , _cpu_started(0)
    , _io_context(0)
    , _io_context_available(max_aio),_reuseport(posix_reuseport_detect()),_task_quota_timer(file_desc::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC))
    , _thread_pool("sys"+id)
    {
    thread_impl::init();
    auto r = ::io_setup(max_aio, &_io_context);
    assert(r >= 0);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, alarm_signal());
    r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
    assert(r == 0);
    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD_ID;
    sev._sigev_un._tid = syscall(SYS_gettid);
    sev.sigev_signo = alarm_signal();
    r = timer_create(CLOCK_MONOTONIC, &sev, &_steady_clock_timer);
    assert(r >= 0);
    sigemptyset(&mask);
    sigaddset(&mask, block_notifier_signal());
    r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    assert(r == 0);
    memory::set_reclaim_hook([this] (std::function<void()> reclaim_fn) {
        add_high_priority_task(make_task([fn = std::move(reclaim_fn)] {
            fn();
        }));
    });
}
reactor::~reactor() {
    if (_steady_clock_timer) {
        timer_delete(_steady_clock_timer);
    }
}

void reactor::at_exit(std::function<future<> ()> func) {
    assert(!_stopping);
    _exit_funcs.push_back(std::move(func));
}

bool
reactor::posix_reuseport_detect() {
    return false; // FIXME: reuseport currently leads to heavy load imbalance. Until we fix that, just
                  // disable it unconditionally.
/*思考为什么 reuseport会引入负载不均衡*/
    try {
        file_desc fd = file_desc::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        fd.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1);
        return true;
    } catch(std::system_error& e) {
        return false;
    }
}

void reactor::run_tasks(std::deque<std::unique_ptr<task>>& tasks) {
    while (!tasks.empty()) {
        // std::cout<<"run_task开始执行"<<std::endl;
        auto tsk = std::move(tasks.front());
        tasks.pop_front();
        tsk->run();
        // std::cout<<"run_task结束执行"<<std::endl;
        tsk.reset();
    }
}

void reactor::exit(int ret) {
    smp::submit_to(0, [this, ret] { _return = ret; stop(); });
}

void reactor::stop() {
    assert(engine()._id == 0);
    smp::cleanup_cpu();
    if (!_stopping) {

    }
}



void smp::pin(unsigned cpu_id) {
    pin_this_thread(cpu_id);
}

void smp::arrive_at_event_loop_end() {
    if (_all_event_loops_done) {
        _all_event_loops_done->wait();
    }
}

void smp::allocate_reactor(unsigned id) {
    std::cout<<"开始执行smp::allocate_reactor"<<std::endl;
    assert(!reactor_holder::get());
    // we cannot just write "local_engin = new reactor" since reactor's constructor
    // uses local_engine
    void *buf;
    int r = posix_memalign(&buf, 64, sizeof(reactor));
    assert(r == 0);
    local_engine = reinterpret_cast<reactor*>(buf);
    new (buf) reactor(id); // 为什么要这样new?
    reactor_holder::get().reset(local_engine);
    std::cout<<"smp::allocate_reactor结束"<<std::endl;

}

void smp::cleanup() {
    smp::_threads = std::vector<posix_thread>();
    _thread_loops.clear();
}

void smp::cleanup_cpu() {
    size_t cpuid = engine().cpu_id();

    if (_qs) {
        for(unsigned i = 0; i < smp::count; i++) {
            _qs[i][cpuid].stop();
        }
    }
}

void smp::create_thread(std::function<void ()> thread_loop) {
    _threads.emplace_back(std::move(thread_loop));
}


static inline std::vector<char> string2vector(std::string str) {
    auto v = std::vector<char>(str.begin(), str.end());
    v.push_back('\0');
    return v;
}




size_t parse_memory_size(std::string s) {
    size_t factor = 1;
    if (s.size()) {
        auto c = s[s.size() - 1];
        static std::string suffixes = "kMGT";
        auto pos = suffixes.find(c);
        if (pos == suffixes.npos) {
            throw std::runtime_error("Cannot parse memory size");
        }
        factor <<= (pos + 1) * 10;
        s = s.substr(0, s.size() - 1);
    }
    return boost::lexical_cast<size_t>(s) * factor;
}


template <typename... A>
std::string format(const char* fmt, A&&... a) {
    return "";
}

void smp::configure(boost::program_options::variables_map configuration)
{
    // 初始化信号集，屏蔽所有信号
    sigset_t sigs;
    sigfillset(&sigs);
    for (auto sig : {SIGHUP, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV,
            SIGALRM, SIGCONT, SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU}) {
        sigdelset(&sigs, sig);  // 从信号集中移除特定信号
    }
    pthread_sigmask(SIG_BLOCK, &sigs, nullptr);  // 设置线程信号掩码


    // 安装一次性信号处理器
    install_oneshot_signal_handler<SIGSEGV, sigsegv_action>();
    install_oneshot_signal_handler<SIGABRT, sigabrt_action>();
    std::cout<<"设置thread affinity"<<std::endl;
    // 获取配置中的线程亲和性设置
    auto thread_affinity = configuration["thread-affinity"].as<bool>(); //这里出错
    std::cout<<"thread affinity end"<<std::endl;
    if (configuration.count("overprovisioned")
           && configuration["thread-affinity"].defaulted()) {
        thread_affinity = false;  // 如果过载且未显式设置，则关闭线程亲和性
    }
    if (!thread_affinity && _using_dpdk) {
        printf("警告: 在 DPDK 模式下忽略 --thread-affinity 0\n");
    }

    // 初始化 SMP（对称多处理）相关参数
    smp::count = 1;  // 默认 CPU 数量为 1
    smp::_tmain = std::this_thread::get_id();  // 主线程 ID
    auto nr_cpus = resource::nr_processing_units();  // 获取可用的 CPU 数量
    resource::cpuset cpu_set;  // CPU 集合
    for (unsigned i = 0; i < nr_cpus; ++i) {
        cpu_set.insert(i);  // 将所有 CPU 添加到集合中
    }

    // 根据配置覆盖 CPU 集合和数量
    if (configuration.count("cpuset")) {
        cpu_set = configuration["cpuset"].as<cpuset_bpo_wrapper>().value;
    }
    if (configuration.count("smp")) {
        nr_cpus = configuration["smp"].as<unsigned>();
    } else {
        nr_cpus = cpu_set.size();
    }
    smp::count = nr_cpus;  // 更新 CPU 数量
    _reactors.resize(nr_cpus);  // 调整反应器数组大小
    // 配置资源分配
    resource::configuration rc;
    if (configuration.count("memory")) {
        //没有走到这行
        std::cout<<"配置中含有memory"<<std::endl;
        rc.total_memory = parse_memory_size(configuration["memory"].as<std::string>());
    }
    if (configuration.count("reserve-memory")) {
        std::cout<<"配置中含有reserve memory"<<std::endl;
        rc.reserve_memory = parse_memory_size(configuration["reserve-memory"].as<std::string>());
    }
    // 处理大页内存路径和内存锁定
    std::optional<std::string> hugepages_path;
    if (configuration.count("hugepages")) {
        std::cout<<"配置中含有hugepages"<<std::endl;
        hugepages_path = configuration["hugepages"].as<std::string>();
    }
    auto mlock = false;
    if (configuration.count("lock-memory")) {
        std::cout<<"配置中含有lock memory"<<std::endl;
        mlock = configuration["lock-memory"].as<bool>();
    }
    if (mlock) {
        std::cout<<"lock memory"<<std::endl;
        auto r = mlockall(MCL_CURRENT | MCL_FUTURE);  // 锁定内存
        if (r) {
            printf("警告: mlockall 失败\n");
        }
    }
    // 配置资源分配参数
    rc.cpus = smp::count;//12
    rc.cpu_set = std::move(cpu_set);
    if (configuration.count("max-io-requests")) {
        std::cout<<"配置中含有max io requests"<<std::endl;
        rc.max_io_requests = configuration["max-io-requests"].as<unsigned>();
    }
    if (configuration.count("num-io-queues")) {
        std::cout<<"配置中含有num io queues"<<std::endl;
        rc.io_queues = configuration["num-io-queues"].as<unsigned>();
    }
    // 分配资源并初始化 CPU 和内存
    auto resources = resource::allocate(rc);
    std::vector<resource::cpu> allocations = std::move(resources.cpus);//allocations是CPU的vector.
/*
allocations format:
[cpu][cpu]...[cpu]
or
[cpuid(0~11),bytes,nodeid(0)]
*/

    if (thread_affinity) {
        std::cout<<"thread pind to 0"<<std::endl;
        smp::pin(allocations[0].cpu_id);  // 绑定主线程到CPU 0
    }
    std::cout<<"mem config begin "<<std::endl;
    // std::cout<<"hugepages_path is "<<hugepages_path<<std::endl;
    //这里报错
    memory::configure(allocations[0].mem, hugepages_path);//hugepages_path是空的.

    std::cout<<"mem config end "<<std::endl;
    // 启用或禁用内存分配失败时的终止行为
    if (configuration.count("abort-on-seastar-bad-alloc")) {
        memory::enable_abort_on_allocation_failure();
    }
    // 启用堆内存分析
    bool heapprof_enabled = configuration.count("heapprof");
    memory::set_heap_profiling_enabled(heapprof_enabled);
    // 创建同步屏障
    static boost::barrier reactors_registered(smp::count);
    static boost::barrier smp_queues_constructed(smp::count);
    static boost::barrier inited(smp::count);

    // 初始化 IO 队列信息
    auto io_info = std::move(resources.io_queues);
    std::vector<io_queue*> all_io_queues;
    all_io_queues.resize(io_info.coordinators.size());
    io_queue::fill_shares_array();

    // 分配 IO 队列
    auto alloc_io_queue = [io_info, &all_io_queues] (unsigned shard) {
        auto cid = io_info.shard_to_coordinator[shard];
        int vec_idx = 0;
        for (auto& coordinator: io_info.coordinators) {
            if (coordinator.id != cid) {
                vec_idx++;
                continue;
            }
            if (shard == cid) {
                all_io_queues[vec_idx] = new io_queue(coordinator.id, coordinator.capacity, io_info.shard_to_coordinator);
            }
            return vec_idx;
        }
        assert(0); // 不可能到达这里
    };
    // 分配 IO 队列给线程
    auto assign_io_queue = [&all_io_queues] (shard_id id, int queue_idx) {
        if (all_io_queues[queue_idx]->coordinator() == id) {
            engine().my_io_queue.reset(all_io_queues[queue_idx]);
        }
        engine()._io_queue = all_io_queues[queue_idx];
        engine()._io_coordinator = all_io_queues[queue_idx]->coordinator();
    };

    _all_event_loops_done.emplace(smp::count);

    // 创建额外的线程来运行反应器
    unsigned i;
    for (i = 1; i < smp::count; i++) {
        auto allocation = allocations[i];
        create_thread([configuration, hugepages_path, i, allocation, assign_io_queue, alloc_io_queue, thread_affinity, heapprof_enabled] {
            std::cout<<"create thread "<<i<<std::endl;
            auto thread_name = format("reactor-{}", i);
            pthread_setname_np(pthread_self(), thread_name.c_str());  // 设置线程名称
            if (thread_affinity) {
                smp::pin(allocation.cpu_id);  // 绑定线程到指定 CPU
            }
            memory::configure(allocation.mem, hugepages_path);
            memory::set_heap_profiling_enabled(heapprof_enabled);
            sigset_t mask;
            sigfillset(&mask);
            for (auto sig : { SIGSEGV }) {
                sigdelset(&mask, sig);  // 移除特定信号
            }
            auto r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
            throw_pthread_error(r);
            allocate_reactor(i);
            _reactors[i] = &engine();
            auto queue_idx = alloc_io_queue(i);
            reactors_registered.wait();
            smp_queues_constructed.wait();
            start_all_queues();
            assign_io_queue(i, queue_idx);
            inited.wait();
            engine().configure(configuration);
            if (&engine().net()!=nullptr)engine().net().print_stack();
            engine().run();
        });
    }
    // 主线程分配反应器
    allocate_reactor(0);
    _reactors[0] = &engine();
    auto queue_idx = alloc_io_queue(0);
    // 等待所有反应器注册完成
    reactors_registered.wait();
    std::cout<<"reactors registered done"<<std::endl;
    /*----------------------------上面代码成功执行-------------------------------------------------- */
    smp::_qs = new smp_message_queue* [smp::count];

    std::cout<<"smp qs begin"<<std::endl;
    for(unsigned i = 0; i < smp::count; i++) {
        smp::_qs[i] = reinterpret_cast<smp_message_queue*>(operator new[] (sizeof(smp_message_queue) * smp::count));
        for (unsigned j = 0; j < smp::count; ++j) {
            new (&smp::_qs[i][j]) smp_message_queue(_reactors[j], _reactors[i]);
        }
    }
    std::cout<<"smp qs end"<<std::endl;
    smp_queues_constructed.wait();
    std::cout<<"start all queues"<<std::endl;
    start_all_queues();
    std::cout<<"start all queues done"<<std::endl;
    assign_io_queue(0, queue_idx);
    std::cout<<"assign io queues done"<<std::endl;
    inited.wait();
    std::cout<<"inited done"<<std::endl;
    // 配置引擎并启动低分辨率时钟
    engine().configure(configuration);//这里出错(为什么之前的没有报错?)
    std::cout<<"engine configure done"<<std::endl;
    engine()._lowres_clock = std::make_unique<lowres_clock>();
}

bool smp::poll_queues() {
    size_t got = 0;
    for (unsigned i = 0; i < count; i++) {
        if (engine().cpu_id() != i) {
            auto& rxq = _qs[engine().cpu_id()][i];
            rxq.flush_response_batch();
            got += rxq.has_unflushed_responses();
            got += rxq.process_incoming();
            auto& txq = _qs[i][engine()._id];
            txq.flush_request_batch();
            got += txq.process_completions();
        }
    }
    return got != 0;
}

bool smp::pure_poll_queues() {
    for (unsigned i = 0; i < count; i++) {
        if (engine().cpu_id() != i) {
            auto& rxq = _qs[engine().cpu_id()][i];
            rxq.flush_response_batch();
            auto& txq = _qs[i][engine()._id];
            txq.flush_request_batch();
            if (rxq.pure_poll_rx() || txq.pure_poll_tx() || rxq.has_unflushed_responses()) {
                return true;
            }
        }
    }
    return false;
}

boost::program_options::options_description
smp::get_options_description()
{
    namespace bpo = boost::program_options;
    bpo::options_description opts("SMP options");
    opts.add_options()
        ("smp,c", bpo::value<unsigned>(), "number of threads (default: one per CPU)")
        ("cpuset", bpo::value<cpuset_bpo_wrapper>(), "CPUs to use (in cpuset(7) format; default: all))")
        ("memory,m", bpo::value<std::string>(), "memory to use, in bytes (ex: 4G) (default: all)")
        ("reserve-memory", bpo::value<std::string>(), "memory reserved to OS (if --memory not specified)")
        ("hugepages", bpo::value<std::string>(), "path to accessible hugetlbfs mount (typically /dev/hugepages/something)")
        ("lock-memory", bpo::value<bool>(), "lock all memory (prevents swapping)")
        ("thread-affinity", bpo::value<bool>()->default_value(true), "pin threads to their cpus (disable for overprovisioning)")
        ("num-io-queues", bpo::value<unsigned>(), "Number of IO queues. Each IO unit will be responsible for a fraction of the IO requests. Defaults to the number of threads")
        ("max-io-requests", bpo::value<unsigned>(), "Maximum amount of concurrent requests to be sent to the disk. Defaults to 128 times the number of IO queues")
        ;
    return opts;
}


unsigned smp::count = 1;
bool smp::_using_dpdk;

void smp::start_all_queues()
{
    for (unsigned c = 0; c < count; c++) {
        if (c != engine().cpu_id()) {
            _qs[c][engine().cpu_id()].start(c);
        }
    }
}


void smp::join_all()
{

    for (auto&& t: smp::_threads) {
        t.join();
    }
}


void* posix_thread::start_routine(void* arg) noexcept {
    auto pfunc = reinterpret_cast<std::function<void ()>*>(arg);
    (*pfunc)();
    return nullptr;
}

posix_thread::posix_thread(std::function<void ()> func)
    : posix_thread(attr{}, std::move(func)) {
}

posix_thread::posix_thread(attr a, std::function<void ()> func)
    : _func(std::make_unique<std::function<void ()>>(std::move(func))) {
    pthread_attr_t pa;
    auto r = pthread_attr_init(&pa);
    if (r) {
        throw std::system_error(r, std::system_category());
    }
    auto stack_size = a._stack_size.size;
    if (!stack_size) {
        stack_size = 2 << 20;
    }
    // allocate guard area as well
    _stack = mmap_anonymous(nullptr, stack_size + (4 << 20),
        PROT_NONE, MAP_PRIVATE | MAP_NORESERVE);
    auto stack_start = align_up(_stack.get() + 1, 2 << 20);
    mmap_area real_stack = mmap_anonymous(stack_start, stack_size,
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_STACK);
    real_stack.release(); // protected by @_stack
    ::madvise(stack_start, stack_size, MADV_HUGEPAGE);
    r = pthread_attr_setstack(&pa, stack_start, stack_size);
    if (r) {
        throw std::system_error(r, std::system_category());
    }
    r = pthread_create(&_pthread, &pa,
                &posix_thread::start_routine, _func.get());
    if (r) {
        throw std::system_error(r, std::system_category());
    }
}

posix_thread::posix_thread(posix_thread&& x)
    : _func(std::move(x._func)), _pthread(x._pthread), _valid(x._valid)
    , _stack(std::move(x._stack)) {
    x._valid = false;
}

posix_thread::~posix_thread() {
    assert(!_valid);
}

void posix_thread::join() {
    assert(_valid);
    pthread_join(_pthread, NULL);
    _valid = false;
}

void throw_system_error_on(bool condition, const char* what_arg) {
    if (condition) {
        throw std::system_error(errno, std::system_category(), what_arg);
    }
}

std::array<std::atomic<uint32_t>, io_queue::_max_classes> io_queue::_registered_shares;
std::array<std::string, io_queue::_max_classes> io_queue::_registered_names;
io_queue::io_queue(shard_id coordinator, size_t capacity, std::vector<shard_id> topology)
        : _coordinator(coordinator)
        , _capacity(capacity)
        , _io_topology(std::move(topology))
        , _priority_classes()
        , _fq(capacity) { }


io_queue::~io_queue() {
    // It is illegal to stop the I/O queue with pending requests.
    // Technically we would use a gate to guarantee that. But here, it is not
    // needed since this is expected to be destroyed only after the reactor is destroyed.
    //
    // And that will happen only when there are no more fibers to run. If we ever change
    // that, then this has to change.
    for (auto&& pclasses: _priority_classes) {
        _fq.unregister_priority_class(pclasses.second->ptr);
    }
}


void io_queue::fill_shares_array() {
    for (unsigned i = 0; i < _max_classes; ++i) {
        _registered_shares[i].store(0);
    }
}

io_priority_class io_queue::register_one_priority_class(std::string name, uint32_t shares) {
    for (unsigned i = 0; i < _max_classes; ++i) {
        uint32_t unused = 0;
        auto s = _registered_shares[i].compare_exchange_strong(unused, shares, std::memory_order_acq_rel);
        if (s) {
            io_priority_class p;
            _registered_names[i] = name;
            p.val = i;
            return std::move(p);
        };
    }
    throw std::runtime_error("No more room for new I/O priority classes");
}

// seastar::metrics::label io_queue_shard("ioshard");

io_queue::priority_class_data::priority_class_data(std::string name, priority_class_ptr ptr, shard_id owner)
    : ptr(ptr)
    , bytes(0)
    , ops(0)
    , nr_queued(0)
    , queue_time(std::chrono::seconds(1)){}
io_queue::priority_class_data& io_queue::find_or_create_class(const io_priority_class& pc, shard_id owner) {
    auto it_pclass = _priority_classes.find(pc.id());
    if (it_pclass == _priority_classes.end()) {
        auto shares = _registered_shares.at(pc.id()).load(std::memory_order_acquire);
        auto name = _registered_names.at(pc.id());
        auto ret = _priority_classes.emplace(pc.id(), std::make_shared<priority_class_data>(name, _fq.register_priority_class(shares), owner));
        it_pclass = ret.first;
    }
    return *(it_pclass->second);
}

std::atomic<lowres_clock::rep> lowres_clock::_now;
constexpr std::chrono::milliseconds lowres_clock::_granularity;

#include <iostream>
#include "../util/align.hh"
#include <new>
#include <cstdint>
#include <algorithm>
#include <limits>
#include <cassert>
#include <atomic>
#include <mutex>
#include <experimental/optional>
#include <functional>
#include <cstring>
#include <sys/uio.h>
#include <sys/mman.h>
#include "../util/backtrace.hh"
#include <unordered_set>




size_t saved_backtrace::hash() const {
    size_t h = 0;
    for (auto frame : _frames) {
        h = std::hash<unw_word_t>()(frame) + (h << 6) + (h << 16) - h;
    }
    return h;
}

saved_backtrace current_backtrace() {
    std::vector<unw_word_t> v;
    backtrace([&] (uintptr_t addr) {
        v.push_back(addr);
    });
    return saved_backtrace(std::move(v));
}


mmap_area mmap_anonymous(void* addr, size_t length, int prot, int flags) {
    auto ret = ::mmap(addr, length, prot, flags | MAP_ANONYMOUS, -1, 0);

    if(ret == MAP_FAILED){
        throw std::runtime_error("mmap failed");
    }
    return mmap_area(reinterpret_cast<char*>(ret), mmap_deleter{length});
}


void mmap_deleter::operator()(void* ptr) const {
    ::munmap(ptr, _size);
}

namespace memory {
    static thread_local int abort_on_alloc_failure_suppressed = 0;
    disable_abort_on_alloc_failure_temporarily::disable_abort_on_alloc_failure_temporarily() {
        ++abort_on_alloc_failure_suppressed;
    }
    disable_abort_on_alloc_failure_temporarily::~disable_abort_on_alloc_failure_temporarily() noexcept {
        --abort_on_alloc_failure_suppressed;
    }
    void enable_abort_on_allocation_failure() {
        abort_on_allocation_failure.store(true, std::memory_order_seq_cst);
    }
    static void on_allocation_failure(size_t size) {
        if (!abort_on_alloc_failure_suppressed && abort_on_allocation_failure.load(std::memory_order_relaxed)) {
            abort();
        }
    }
    void cpu_pages::replace_memory_backing(allocate_system_memory_fn alloc_sys_mem) {
        //We would like to use ::mremap() to atomically replace the old anonymous
        //memory with hugetlbfs backed memory, but mremap() does not support hugetlbfs
        //(for no reason at all).  So we must copy the anonymous memory to some other
        //place, map hugetlbfs in place, and copy it back, without modifying it during
        //the operation.
        auto bytes = nr_pages * page_size;
        auto old_mem = mem();
        auto relocated_old_mem = mmap_anonymous(nullptr, bytes, PROT_READ|PROT_WRITE, MAP_PRIVATE);
        std::memcpy(relocated_old_mem.get(), old_mem, bytes);
        alloc_sys_mem({old_mem}, bytes).release();
        std::memcpy(old_mem, relocated_old_mem.get(), bytes);
    }

void cpu_pages::init_virt_to_phys_map() {
    auto nr_entries = nr_pages / (huge_page_size / page_size);
    virt_to_phys_map.resize(nr_entries);
    auto fd = file_desc::open("/proc/self/pagemap", O_RDONLY | O_CLOEXEC);
    for (size_t i = 0; i != nr_entries; ++i) {
        uint64_t entry = 0;
        auto phys = std::numeric_limits<physical_address>::max();
        auto pfn = reinterpret_cast<uintptr_t>(mem() + i * huge_page_size) / page_size;
        fd.pread(&entry, 8, pfn * 8);
        assert(entry & 0x8000'0000'0000'0000);
        phys = (entry & 0x007f'ffff'ffff'ffff) << page_bits;
        virt_to_phys_map[i] = phys;
    }
}

translation cpu_pages::translate(const void* addr, size_t size) {
    auto a = reinterpret_cast<uintptr_t>(addr) - reinterpret_cast<uintptr_t>(mem());
    auto pfn = a / huge_page_size;
    if (pfn >= virt_to_phys_map.size()) {
        return {};
    }
    auto phys = virt_to_phys_map[pfn];
    if (phys == std::numeric_limits<physical_address>::max()) {
        return {};
    }
    auto translation_size = align_up(a + 1, huge_page_size) - a;
    size = std::min(size, translation_size);
    phys += a & (huge_page_size - 1);
    return translation{phys, size};
}

void free(void* obj) {
    if (cpu_mem.try_cross_cpu_free(obj)) {
        return;
    }
    ++g_frees;
    cpu_mem.free(obj);
}

void free(void* obj, size_t size) {
    if (cpu_mem.try_cross_cpu_free(obj)) {
        return;
    }
    ++g_frees;
    cpu_mem.free(obj, size);
}


void shrink(void* obj, size_t new_size) {
    ++g_frees;
    ++g_allocs; // keep them balanced
    cpu_mem.shrink(obj, new_size);
}

void set_reclaim_hook(std::function<void (std::function<void ()>)> hook) {
    cpu_mem.set_reclaim_hook(hook);
}

void configure(std::vector<resource::memory> m, std::optional<std::string> hugetlbfs_path) {
    size_t total = 0;
    // 调试：显示每个内存块的详细信息
    std::cout << "=== 开始配置内存 ===" << std::endl;
    std::cout << "计算总内存:" << std::endl;
    for (size_t i = 0; i < m.size(); ++i) {
        auto&& x = m[i];
        std::cout << "  内存块[" << i
                  << "] 大小: " << x.bytes << " 字节"
                  << (x.nodeid != static_cast<unsigned long>(-1) ?
                     " NUMA节点: " + std::to_string(x.nodeid) : "")
                  << std::endl;
        total += x.bytes;
    }
    std::cout << "总内存量: " << total << " 字节" << std::endl;
    allocate_system_memory_fn sys_alloc = allocate_anonymous_memory;
    //绑定实现函数.
    if (hugetlbfs_path) {
        std::cout << "检测到HugeTLBFS路径: " << *hugetlbfs_path << std::endl;
        auto fdp = std::make_shared<file_desc>(file_desc::temporary(*hugetlbfs_path));
        sys_alloc = [fdp](std::optional<void*> where, size_t how_much) {
            return allocate_hugetlbfs_memory(*fdp, where, how_much);
        };
        std::cout << "切换内存分配函数到HugeTLBFS专用分配器" << std::endl;
        cpu_mem.replace_memory_backing(sys_alloc);
    } else {
        std::cout << "使用默认匿名内存分配" << std::endl;
    }
    // 调试：显示内存调整操作
    std::cout << "调整内存池大小至: " << total << " 字节" << std::endl;
    cpu_mem.resize(total, sys_alloc);
    //这句代码卡住. (传入的函数都是sys_alloc)
    std::cout << "调整内存池end"<<std::endl;
    size_t pos = 0;
    for (auto&& x : m) {
        // 调试：显示每个内存块的分配进度
        std::cout << "已分配内存块 [" << pos << " -> " << (pos + x.bytes)
                  << ") 大小: " << x.bytes << " 字节" << std::endl;
        pos += x.bytes;
    }
    if (hugetlbfs_path) {
        std::cout << "初始化HugeTLBFS虚拟地址到物理地址映射" << std::endl;
        cpu_mem.init_virt_to_phys_map();
    }
    std::cout << "=== 内存配置完成 ===" << std::endl;
}

static
allocation_site_ptr get_allocation_site() {
    if (!cpu_mem.is_initialized() || !cpu_mem.collect_backtrace) {
        return nullptr;
    }
    disable_backtrace_temporarily dbt;
    allocation_site new_alloc_site;
    new_alloc_site.backtrace = get_backtrace();
    auto insert_result = cpu_mem.asu.alloc_sites.insert(std::move(new_alloc_site));
    allocation_site_ptr alloc_site = &*insert_result.first;
    if (insert_result.second) {
        alloc_site->next = cpu_mem.alloc_site_list_head;
        cpu_mem.alloc_site_list_head = alloc_site;
    }
    return alloc_site;
}


statistics stats() {
    return statistics{g_allocs, g_frees, g_cross_cpu_frees,
        cpu_mem.nr_pages * page_size, cpu_mem.nr_free_pages * page_size, g_reclaims};
}

void cpu_pages::do_resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem) {
    std::cout<<"调用resize"<<std::endl;
    auto new_pages = new_size / page_size;
    if (new_pages <= nr_pages) {
        return;
    }
    auto old_size = nr_pages * page_size;
    auto mmap_start = memory + old_size;
    auto mmap_size = new_size - old_size;
    auto mem = alloc_sys_mem({mmap_start}, mmap_size);
    mem.release();
    ::madvise(mmap_start, mmap_size, MADV_HUGEPAGE);
    // one past last page structure is a sentinel
    auto new_page_array_pages = align_up(sizeof(page[new_pages + 1]), page_size) / page_size;
    auto new_page_array
        = reinterpret_cast<page*>(allocate_large(new_page_array_pages));
    if (!new_page_array) {
        throw std::bad_alloc();
    }
    std::copy(pages, pages + nr_pages, new_page_array);
    // mark new one-past-last page as taken to avoid boundary conditions
    new_page_array[new_pages].free = false;
    auto old_pages = reinterpret_cast<char*>(pages);
    auto old_nr_pages = nr_pages;
    auto old_pages_size = align_up(sizeof(page[nr_pages + 1]), page_size);
    pages = new_page_array;
    nr_pages = new_pages;
    auto old_pages_start = (old_pages - memory) / page_size;
    if (old_pages_start == 0) {
        // keep page 0 allocated
        old_pages_start = 1;
        old_pages_size -= page_size;
    }
    free_span(old_pages_start, old_pages_size / page_size);
    free_span(old_nr_pages, new_pages - old_nr_pages);
}

void cpu_pages::resize(size_t new_size, allocate_system_memory_fn alloc_memory) {
    new_size = align_down(new_size, huge_page_size);
    if(!cpu_pages::is_initialized()){
        std::cout<<"第一次执行初始化"<<std::endl;
        cpu_pages::initialize();
    }
    while (nr_pages * page_size < new_size) {
        // don't reallocate all at once, since there might not
        // be enough free memory available to relocate the pages array
        auto tmp_size = std::min(new_size, 4 * nr_pages * page_size);
        do_resize(tmp_size, alloc_memory);
    }
}

reclaiming_result cpu_pages::run_reclaimers(reclaimer_scope scope) {
    auto target = std::max(nr_free_pages + 1, min_free_pages);
    reclaiming_result result = reclaiming_result::reclaimed_nothing;
    while (nr_free_pages < target) {
        bool made_progress = false;
        ++g_reclaims;
        for (auto&& r : reclaimers) {
            if (r->scope() >= scope) {
                made_progress |= r->do_reclaim() == reclaiming_result::reclaimed_something;
            }
        }
        if (!made_progress) {
            return result;
        }
        result = reclaiming_result::reclaimed_something;
    }
    return result;
}

void cpu_pages::schedule_reclaim() {
    current_min_free_pages = 0;
    reclaim_hook([this] {
        if (nr_free_pages < min_free_pages) {
            try {
                run_reclaimers(reclaimer_scope::async);
            } catch (...) {
                current_min_free_pages = min_free_pages;
                throw;
            }
        }
        current_min_free_pages = min_free_pages;
    });//把lamdba对象加到reactor的任务队列中.
}

memory::memory_layout cpu_pages::memory_layout() {
    assert(is_initialized());
    return {
        reinterpret_cast<uintptr_t>(memory),
        reinterpret_cast<uintptr_t>(memory) + nr_pages * page_size
    };
}

void cpu_pages::set_reclaim_hook(std::function<void (std::function<void ()>)> hook) {
    reclaim_hook = hook;
    current_min_free_pages = min_free_pages;
}

void cpu_pages::set_min_free_pages(size_t pages) {
    if (pages > std::numeric_limits<decltype(min_free_pages)>::max()) {
        throw std::runtime_error("Number of pages too large");
    }
    min_free_pages = pages;
    maybe_reclaim();
}

small_pool::small_pool(unsigned object_size) noexcept
    : _object_size(object_size), _span_size(1) {
    while (_object_size > span_bytes()
            || (_span_size < 32 && waste() > 0.05)
            || (span_bytes() / object_size < 32)) {
        _span_size *= 2;
    }
    _max_free = std::max<unsigned>(100, span_bytes() * 2 / _object_size);
    _min_free = _max_free / 2;
}

small_pool::~small_pool() {
    _min_free = _max_free = 0;
    trim_free_list();
}

// Should not throw in case of running out of memory to avoid infinite recursion,
// becaue throwing std::bad_alloc requires allocation. __cxa_allocate_exception
// falls back to the emergency pool in case malloc() returns nullptr.
void*
small_pool::allocate() {
    if (!_free) {
        add_more_objects();
    }
    if (!_free) {
        return nullptr;
    }
    auto* obj = _free;
    _free = _free->next;
    --_free_count;
    return obj;
}

void
small_pool::deallocate(void* object) {
    auto o = reinterpret_cast<free_object*>(object);
    o->next = _free;
    _free = o;
    ++_free_count;
    if (_free_count >= _max_free) {
        trim_free_list();
    }
}

void
small_pool::add_more_objects() {
    auto goal = (_min_free + _max_free) / 2;
    while (!_span_list.empty() && _free_count < goal) {
        page& span = _span_list.front(cpu_mem.pages);
        _span_list.pop_front(cpu_mem.pages);
        while (span.freelist) {
            auto obj = span.freelist;
            span.freelist = span.freelist->next;
            obj->next = _free;
            _free = obj;
            ++_free_count;
            ++span.nr_small_alloc;
        }
    }
    while (_free_count < goal) {
        disable_backtrace_temporarily dbt;
        auto data = reinterpret_cast<char*>(cpu_mem.allocate_large(_span_size));
        if (!data) {
            return;
        }
        ++_spans_in_use;
        auto span = cpu_mem.to_page(data);
        for (unsigned i = 0; i < _span_size; ++i) {
            span[i].offset_in_span = i;
            span[i].pool = this;
        }
        span->nr_small_alloc = 0;
        span->freelist = nullptr;
        for (unsigned offset = 0; offset <= span_bytes() - _object_size; offset += _object_size) {
            auto h = reinterpret_cast<free_object*>(data + offset);
            h->next = _free;
            _free = h;
            ++_free_count;
            ++span->nr_small_alloc;
        }
    }
}

void
small_pool::trim_free_list() {
    auto goal = (_min_free + _max_free) / 2;
    while (_free && _free_count > goal) {
        auto obj = _free;
        _free = _free->next;
        --_free_count;
        page* span = cpu_mem.to_page(obj);
        span -= span->offset_in_span;
        if (!span->freelist) {
            new (&span->link) page_list_link();
            _span_list.push_front(cpu_mem.pages, *span);
        }
        obj->next = span->freelist;
        span->freelist = obj;
        if (--span->nr_small_alloc == 0) {
            _span_list.erase(cpu_mem.pages, *span);
            cpu_mem.free_span(span - cpu_mem.pages, span->span_size);
            --_spans_in_use;
        }
    }
}

std::atomic<unsigned> cpu_pages::cpu_id_gen;
cpu_pages* cpu_pages::all_cpus[max_cpus];

float small_pool::waste() {
    return (span_bytes() % _object_size) / (1.0 * span_bytes());
}


void set_heap_profiling_enabled(bool enable) {
    bool is_enabled = cpu_mem.collect_backtrace;
    if (enable) {
        if (!is_enabled) {
            abort();
        }
    } else {
        if (is_enabled) {
            abort();
        }
    }
    cpu_mem.collect_backtrace = enable;
}

mmap_area
allocate_anonymous_memory(std::optional<void*> where, size_t how_much) {
    return mmap_anonymous(where.value_or(nullptr),
            how_much, PROT_READ|PROT_WRITE, MAP_PRIVATE|(where?MAP_FIXED :0));
}


void
abort_on_underflow(size_t size) {
    if (std::make_signed_t<size_t>(size) < 0) {
        // probably a logic error, stop hard
        abort();
    }
}


void* allocate_large(size_t size) {
    abort_on_underflow(size);
    unsigned size_in_pages = (size + page_size - 1) >> page_bits;
    std::cout<<"size:   "<<size<<"  size_in pages:  "<<size_in_pages<<std::endl;
    if ((size_t(size_in_pages) << page_bits) < size) {
        std::cout<<"allocate_large overflow"<<std::endl;
        throw std::bad_alloc();
    }
    return cpu_mem.allocate_large(size_in_pages);
}


void* allocate_large_aligned(size_t align, size_t size) {
    abort_on_underflow(size);
    unsigned size_in_pages = (size + page_size - 1) >> page_bits;
    unsigned align_in_pages = std::max(align, page_size) >> page_bits;
    return cpu_mem.allocate_large_aligned(align_in_pages, size_in_pages);
}

void free_large(void* ptr) {
    return cpu_mem.free_large(ptr);
}

size_t object_size(void* ptr) {
    return cpu_pages::all_cpus[object_cpu_id(ptr)]->object_size(ptr);
}


void* allocate(size_t size) {
    if (size <= sizeof(free_object)) {
        size = sizeof(free_object);
        std::cout << "Adjusted allocation size to match free_object size: " 
                  << size << std::endl;
    }
    void* ptr;
    if (size <= max_small_allocation) {
        size = object_size_with_alloc_site(size);
        std::cout << "Allocating small object with adjusted size: " << size << std::endl;
        ptr = cpu_mem.allocate_small(size);
    } else {
        std::cout<<"huge alloc"<<std::endl;
        ptr = allocate_large(size);
    }
    if (!ptr) {
        std::cout << "Allocation failed for size: " << size << std::endl;
        on_allocation_failure(size);
    }
    ++g_allocs;
    return ptr;
}


void* allocate_aligned(size_t align, size_t size) {
    size = std::max(size, align);
    if (size <= sizeof(free_object)) {
        size = sizeof(free_object);
    }
    void* ptr;
    if (size <= max_small_allocation && align <= page_size) {
        // Our small allocator only guarantees alignment for power-of-two
        // allocations which are not larger than a page.
        size = 1 << log2ceil(object_size_with_alloc_site(size));
        ptr = cpu_mem.allocate_small(size);
    } else {
        ptr = allocate_large_aligned(align, size);
    }
    if (!ptr) {
        on_allocation_failure(size);
    }
    ++g_allocs;
    return ptr;
}


mmap_area
allocate_hugetlbfs_memory(file_desc& fd, std::optional<void*> where, size_t how_much) {
    auto pos = fd.size();
    fd.truncate(pos + how_much);
    auto ret = fd.map(
            how_much,
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_POPULATE | (where ? MAP_FIXED : 0),
            pos,
            where.value_or(nullptr));
    return ret;
}

void*
cpu_pages::allocate_small(unsigned size) {
    auto idx = small_pool::size_to_idx(size);
    auto& pool = small_pools[idx];
    assert(size <= pool.object_size());
    auto ptr = pool.allocate();
    return ptr;
}

void cpu_pages::free_large(void* ptr) {
    pageidx idx = (reinterpret_cast<char*>(ptr) - mem()) / page_size;
    page* span = &pages[idx];
    free_span(idx, span->span_size);
}

size_t cpu_pages::object_size(void* ptr) {
    pageidx idx = (reinterpret_cast<char*>(ptr) - mem()) / page_size;
    page* span = &pages[idx];
    if (span->pool) {
        auto s = span->pool->object_size();
        return s;
    } else {
        return size_t(span->span_size) * page_size;
    }
}

void cpu_pages::free_cross_cpu(unsigned cpu_id, void* ptr) {
    if (!live_cpus[cpu_id].load(std::memory_order_relaxed)) {
        // Thread was destroyed; leak object
        // should only happen for boost unit-tests.
        return;
    }
    auto p = reinterpret_cast<cross_cpu_free_item*>(ptr);
    auto& list = all_cpus[cpu_id]->xcpu_freelist;
    auto old = list.load(std::memory_order_relaxed);
    do {
        p->next = old;
    } while (!list.compare_exchange_weak(old, p, std::memory_order_release, std::memory_order_relaxed));
    ++g_cross_cpu_frees;
}

bool cpu_pages::drain_cross_cpu_freelist() {
    if (!xcpu_freelist.load(std::memory_order_relaxed)) {
        return false;
    }
    auto p = xcpu_freelist.exchange(nullptr, std::memory_order_acquire);
    while (p) {
        auto n = p->next;
        ++g_frees;
        free(p);
        p = n;
    }
    return true;
}

void cpu_pages::free(void* ptr) {
    page* span = to_page(ptr);
    if (span->pool) {
        small_pool& pool = *span->pool;
        pool.deallocate(ptr);
    } else {
        free_large(ptr);
    }
}

void cpu_pages::free(void* ptr, size_t size) {
    // match action on allocate() so hit the right pool
    if (size <= sizeof(free_object)) {
        size = sizeof(free_object);
    }
    if (size <= max_small_allocation) {
        size = object_size_with_alloc_site(size);
        auto pool = &small_pools[small_pool::size_to_idx(size)];
        pool->deallocate(ptr);
    } else {
        free_large(ptr);
    }
}

bool
cpu_pages::try_cross_cpu_free(void* ptr) {
    auto obj_cpu = object_cpu_id(ptr);
    if (obj_cpu != cpu_id) {
        free_cross_cpu(obj_cpu, ptr);
        return true;
    }
    return false;
}

void cpu_pages::shrink(void* ptr, size_t new_size) {
    auto obj_cpu = object_cpu_id(ptr);
    assert(obj_cpu == cpu_id);
    page* span = to_page(ptr);
    if (span->pool) {
        return;
    }
    size_t new_size_pages = align_up(new_size, page_size) / page_size;
    auto old_size_pages = span->span_size;
    assert(old_size_pages >= new_size_pages);
    if (new_size_pages == old_size_pages) {
        return;
    }
    span->span_size = new_size_pages;
    span[new_size_pages - 1].free = false;
    span[new_size_pages - 1].span_size = new_size_pages;
    pageidx idx = span - pages;
    free_span(idx + new_size_pages, old_size_pages - new_size_pages);
}

cpu_pages::~cpu_pages() {
    live_cpus[cpu_id].store(false, std::memory_order_relaxed);
}

bool cpu_pages::is_initialized() const {
    return bool(nr_pages);
}

bool cpu_pages::initialize() {
    std::cout<<"调用初始化cpu_pages"<<std::endl;
    if (is_initialized()) {
        std::cout<<"cpu_pages已经初始化"<<std::endl;
        return false;
    }
    cpu_id = cpu_id_gen.fetch_add(1, std::memory_order_relaxed);
    assert(cpu_id < max_cpus);
    all_cpus[cpu_id] = this;
    auto base = mem_base() + (size_t(cpu_id) << cpu_id_shift);
    auto size = 32 << 20;  // Small size for bootstrap
    auto r = ::mmap(base, size,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
            -1, 0);
    if (r == MAP_FAILED) {
        abort();
    }
    ::madvise(base, size, MADV_HUGEPAGE);
    pages = reinterpret_cast<page*>(base);
    memory = base;
    nr_pages = size / page_size;
    // we reserve the end page so we don't have to special case
    // the last span.
    auto reserved = align_up(sizeof(page) * (nr_pages + 1), page_size) / page_size;
    for (pageidx i = 0; i < reserved; ++i) {
        pages[i].free = false;
    }
    pages[nr_pages].free = false;
    free_span_no_merge(reserved, nr_pages - reserved);
    live_cpus[cpu_id].store(true, std::memory_order_relaxed);
    return true;
}

reclaimer::reclaimer(reclaim_fn reclaim, reclaimer_scope scope)
    : _reclaim(std::move(reclaim))
    , _scope(scope) {
    cpu_mem.reclaimers.push_back(this);
}

reclaimer::~reclaimer() {
    auto& r = cpu_mem.reclaimers;
    r.erase(std::find(r.begin(), r.end(), this));
}


void
cpu_pages::unlink(page_list& list, page* span) {
    list.erase(pages, *span);
}

void
cpu_pages::link(page_list& list, page* span) {
    list.push_front(pages, *span);
}

void cpu_pages::free_span_no_merge(uint32_t span_start, uint32_t nr_pages) {
    assert(nr_pages);
    nr_free_pages += nr_pages;
    auto span = &pages[span_start];
    auto span_end = &pages[span_start + nr_pages - 1];
    span->free = span_end->free = true;
    span->span_size = span_end->span_size = nr_pages;
    auto idx = index_of(nr_pages);
    link(fsu.free_spans[idx], span);
}

void cpu_pages::free_span(uint32_t span_start, uint32_t nr_pages) {
    page* before = &pages[span_start - 1];
    if (before->free) {
        auto b_size = before->span_size;
        assert(b_size);
        span_start -= b_size;
        nr_pages += b_size;
        nr_free_pages -= b_size;
        unlink(fsu.free_spans[index_of(b_size)], before - (b_size - 1));
    }
    page* after = &pages[span_start + nr_pages];
    if (after->free) {
        auto a_size = after->span_size;
        assert(a_size);
        nr_pages += a_size;
        nr_free_pages -= a_size;
        unlink(fsu.free_spans[index_of(a_size)], after);
    }
    free_span_no_merge(span_start, nr_pages);
}

page*
cpu_pages::find_and_unlink_span(unsigned n_pages) {
    std::cout<<"find_and_unlink_span"<<std::endl;
    std::cout<<"n_pages: "<<n_pages<<std::endl;
    /* 找到合适的index. */
    auto idx = index_of_conservative(n_pages);
    std::cout<<"查找的idx: "<<idx<<std::endl;
    auto orig_idx = idx;
    if (n_pages >= (2u << idx)) {
        throw std::bad_alloc();
    }
    while (idx < nr_span_lists && fsu.free_spans[idx].empty()) {
        ++idx;
    }
    //找到idx可能有free span的idx.
    if (idx == nr_span_lists) {
        if (initialize()) {
            return find_and_unlink_span(n_pages);
        }
        // Can smaller list possibly hold object?
        idx = index_of(n_pages);
        if (idx == orig_idx) {   // was exact power of two
            return nullptr;
        }
    }

    /*通过idx获取这个free_list*/
    auto& list = fsu.free_spans[idx];
    page* span = list.find(n_pages, pages);
    if (!span) {
        return nullptr;
    }
    unlink(list, span);
    return span;
}
/*
find_and_unlink_span(unsigned n_pages)：
Seastar 内存分配器中用于处理大内存块分配的核心函数之一。
在当前CPU的空闲链表(free_spans)中查找一个包含至少n_pages个连续页的span,
并将其从链表中移除.
当找不到合适的span时,该函数会尝试初始化当前 CPU 的内存池(如果尚未初始化),然后重试.
如果仍然失败，则返回 nullptr.
🔍 第一部分：查找非空的空闲链表
while (idx < nr_span_lists && fsu.free_spans[idx].empty()) {
    ++idx;
}
目的：
查找第一个 非空的 free_span 链表，其索引大于等于通过 index_of_conservative(n_pages) 计算出的初始值。
每个 free_spans[i] 中的 span 至少包含 2^𝑖个页面。
细节说明：
fsu.free_spans 是一个数组，每个元素是一个 page_list 类型，存储着一组大小相近的 span。
如果当前索引 idx 对应的链表为空，则递增 idx，尝试更大的 span。
当循环结束时，idx 可能是：
一个有效的非空链表索引；
或者等于 nr_span_lists，表示所有链表都为空。
处理找不到合适span的情况，尝试初始化内存池或检查更小的链表。
详细流程:
检查是否需要初始化内存池：如果idx == nr_span_lists，说明当前 CPU 的空闲链表全部为空。
调用initialize()初始化当前 CPU 的内存池（首次调用时才会执行）。
如果初始化成功（返回 true），则递归调用 find_and_unlink_span(n_pages)，因为此时可能已经有可用的 span。
如果初始化失败或已初始化过：
使用 index_of(n_pages) 找到最接近 n_pages 的精确幂次方对应的索引。
如果这个新的 idx 等于原来的 orig_idx，说明请求的大小正好是某个 2 的幂次方，且没有可用的 span，因此直接返回 nullptr。
否则继续在更小的索引上搜索。 */

page* cpu_pages::find_and_unlink_span_reclaiming(unsigned n_pages) {
    while(true) {
        auto span = find_and_unlink_span(n_pages);
        if(span){
            return span;
        }
        if (run_reclaimers(reclaimer_scope::sync) == reclaiming_result::reclaimed_nothing) {
            return nullptr;
        }
    }
}

void cpu_pages::maybe_reclaim() {
    if (nr_free_pages < current_min_free_pages) {
        drain_cross_cpu_freelist();
        run_reclaimers(reclaimer_scope::sync);
        if (nr_free_pages < current_min_free_pages) {
            schedule_reclaim();
        }
    }
}

template <typename Trimmer>
void* cpu_pages::allocate_large_and_trim(unsigned n_pages, Trimmer trimmer) {
    // Avoid exercising the reclaimers for requests we'll not be able to satisfy
    // nr_pages might be zero during startup, so check for that too
    std::cout<<"开始执行allocate_large_and_trim"<<std::endl;
    std::cout<<"n_pages: "<<n_pages<<" nr_pages: "<<nr_pages<<std::endl;
    if (nr_pages && n_pages >= nr_pages) {
        std::cout<<"n_pages >= nr_pages"<<std::endl;
        return nullptr;
    }
    page* span = find_and_unlink_span_reclaiming(n_pages);
    if (!span) {
        return nullptr; // 找不到合适的span
    }
    auto span_size = span->span_size;
    auto span_idx = span - pages;
    nr_free_pages -= span->span_size;
    trim t = trimmer(span_idx, nr_pages);
    if (t.offset) {
        free_span_no_merge(span_idx, t.offset);
        span_idx += t.offset;
        span_size -= t.offset;
        span = &pages[span_idx];

    }
    if (t.nr_pages < span_size) {
        free_span_no_merge(span_idx + t.nr_pages, span_size - t.nr_pages);
    }
    auto span_end = &pages[span_idx + t.nr_pages - 1];
    span->free = span_end->free = false;
    span->span_size = span_end->span_size = t.nr_pages;
    span->pool = nullptr;
    maybe_reclaim();
    return mem() + span_idx * page_size;
}

void* cpu_pages::allocate_large(unsigned n_pages) {
    std::cout<<"调用cpu_pages::allocate_large"<<std::endl;
    return allocate_large_and_trim(n_pages, [n_pages] (unsigned idx, unsigned n) {
        return trim{0, std::min(n, n_pages)};
    });
}

void* cpu_pages::allocate_large_aligned(unsigned align_pages, unsigned n_pages) {
    return allocate_large_and_trim(n_pages + align_pages - 1, [=] (unsigned idx, unsigned n) {
        return trim{align_up(idx, align_pages) - idx, n_pages};
    });
}


void set_min_free_pages(size_t pages) {
    cpu_mem.set_min_free_pages(pages);
}



}




inline bool engine_is_ready() {
    return local_engine != nullptr;
}


template <typename Clock>
inline timer<Clock>::timer(callback_t&& callback) : _callback(std::move(callback)) {
}

template <typename Clock>
inline typename timer<Clock>::time_point timer<Clock>::get_timeout() {
    return _expiry;
}

template <typename Clock>
inline bool timer<Clock>::cancel() {
    if (!_armed) {
        return false;
    }
    _armed = false;
    if (_queued) {
        engine().del_timer(this);
        _queued = false;
    }
    return true;
}


template <typename Clock>
inline
timer<Clock>::~timer() {
    if (_queued) {
        engine().del_timer(this);
    }
}

template <typename Clock>
inline
void timer<Clock>::readd_periodic() {
    arm_state(Clock::now() + _period.value(), {_period.value()});
    engine().queue_timer(this);
}

template <typename Clock>
inline
void timer<Clock>::arm_state(time_point until, std::optional<duration> period) {
    assert(!_armed);
    _period = period;
    _armed = true;
    _expired = false;
    _expiry = until;
    _queued = true;
}

lowres_clock::lowres_clock() {
    update();
    _timer.set_callback(&lowres_clock::update);
    _timer.arm_periodic(_granularity);
}

void lowres_clock::update() {
    using namespace std::chrono;
    auto now = steady_clock_type::now();
    auto ticks = duration_cast<milliseconds>(now.time_since_epoch()).count();
    _now.store(ticks, std::memory_order_relaxed);
}


template <typename Func>
future<io_event> io_queue::queue_request(shard_id coordinator, const io_priority_class& pc, size_t len, Func prepare_io) {
    auto start = std::chrono::steady_clock::now();
    return smp::submit_to(coordinator, [start, &pc, len, prepare_io = std::move(prepare_io), owner = engine().cpu_id()] {
        auto& queue = *(engine()._io_queue);
        unsigned weight = 1 + len/(16 << 10);
        // First time will hit here, and then we create the class. It is important
        // that we create the shared pointer in the same shard it will be used at later.
        auto& pclass = queue.find_or_create_class(pc, owner);
        pclass.bytes += len;
        pclass.ops++;
        pclass.nr_queued++;
        return queue._fq.queue(pclass.ptr, weight, [&pclass, start, prepare_io = std::move(prepare_io)] {
            pclass.nr_queued--;
            pclass.queue_time = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - start);
            return engine().submit_io(std::move(prepare_io));
        });
    });
}


template <typename Func>
future<io_event>
reactor::submit_io(Func prepare_io) {
    return _io_context_available.wait(1).then([this, prepare_io = std::move(prepare_io)] () mutable {
        auto pr = std::make_unique<promise<io_event>>();
        iocb io;
        prepare_io(io);
        if (_aio_eventfd) {
            io_set_eventfd(&io, _aio_eventfd->get_fd());
        }
        auto f = pr->get_future();
        io.data = pr.get();
        _pending_aio.push_back(io);
        pr.release();
        if ((_io_queue->queued_requests() > 0) ||
            (_pending_aio.size() >= std::min(max_aio / 4, _io_queue->_capacity / 2))) {
            flush_pending_aio();
        }
        return f;
    });
}

// file_desc
// file_desc::temporary(sstring directory) {
//     // FIXME: add O_TMPFILE support one day
//     directory += "/XXXXXX";
//     std::vector<char> templat(directory.c_str(), directory.c_str() + directory.size() + 1);
//     int fd = ::mkstemp(templat.data());
//     throw_system_error_on(fd == -1);
//     int r = ::unlink(templat.data());
//     throw_system_error_on(r == -1); // leaks created file, but what can we do?
//     return file_desc(fd);
// }

// void mmap_deleter::operator()(void* ptr) const {
//     ::munmap(ptr, _size);
// }



template <typename Clock>
inline
void timer<Clock>::arm(time_point until, std::optional<duration> period) {
    arm_state(until, period);
    engine().add_timer(this);
}

template <typename Clock>
inline
void timer<Clock>::rearm(time_point until, std::optional<duration> period) {
    if (_armed) {
        cancel();
    }
    arm(until, period);
}

template <typename Clock>
inline
void timer<Clock>::arm(duration delta) {
    return arm(Clock::now() + delta);
}

template <typename Clock>
inline
void timer<Clock>::arm_periodic(duration delta) {
    arm(Clock::now() + delta, {delta});
}

template <typename Clock>
inline
void timer<Clock>::set_callback(callback_t&& callback) {
    _callback = std::move(callback);
}


inline future<temporary_buffer<char>> data_source_impl::skip(uint64_t n)
{
    return do_with(uint64_t(n), [this] (uint64_t& n) {
        return repeat_until_value([&] {
            return get().then([&] (temporary_buffer<char> buffer) -> std::optional<temporary_buffer<char>> {
                if (buffer.size() >= n) {
                    buffer.trim_front(n);
                    return std::move(buffer);
                }
                n -= buffer.size();
                return { };
            });
        });
    });
}



void add_to_flush_poller(output_stream<char>* os) {
    engine()._flush_batching.emplace_back(os);
}


thread_pool::thread_pool(std::string name) : _worker_thread([this, name] { work(name); }), _notify(pthread_self()) {
    engine()._signals.handle_signal(SIGUSR1, [this] { inter_thread_wq.complete(); });
}

void thread_pool::work(std::string name) {
    pthread_setname_np(pthread_self(), name.c_str());
    sigset_t mask;
    sigfillset(&mask);
    auto r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
    throw_pthread_error(r);
    std::array<syscall_work_queue::work_item*, syscall_work_queue::queue_length> tmp_buf;
    while (true) {
        uint64_t count;
        auto r = ::read(inter_thread_wq._start_eventfd.get_read_fd(), &count, sizeof(count));
        assert(r == sizeof(count));
        if (_stopped.load(std::memory_order_relaxed)) {
            break;
        }
        auto end = tmp_buf.data();
        inter_thread_wq._pending.consume_all([&] (syscall_work_queue::work_item* wi) {
            *end++ = wi;
        });
        for (auto p = tmp_buf.data(); p != end; ++p) {
            auto wi = *p;
            wi->process();
            inter_thread_wq._completed.push(wi);
        }
        if (_main_thread_idle.load(std::memory_order_seq_cst)) {
            pthread_kill(_notify, SIGUSR1);
        }
    }
}

thread_pool::~thread_pool() {
    _stopped.store(true, std::memory_order_relaxed);
    inter_thread_wq._start_eventfd.signal(1);
    _worker_thread.join();
}

void reactor::start_epoll() {
    if (!_epoll_poller) {
        _epoll_poller = poller(std::make_unique<epoll_pollfn>(*this));
    }
}

void reactor::start_aio_eventfd_loop() {
    if (!_aio_eventfd){
        //执行了这行代码.
        std::cout<<"没有设置aio_eventfd"<<std::endl;
        return;
    }
    future<> loop_done = repeat([this] {
        return _aio_eventfd->readable().then([this] {
            char garbage[8];
            ::read(_aio_eventfd->get_fd(), garbage, 8); // totally uninteresting
            return _stopping ? stop_iteration::yes : stop_iteration::no;
        });
    });
    // must use std::make_shared, because at_exit expects a copyable function
    at_exit([loop_done = std::make_shared<future<>>(std::move(loop_done))] {
        return std::move(*loop_done);
    });
}



reactor::poller::~poller() {
    // We can't just remove the poller from reactor::_pollers, because we
    // may be running inside a poller ourselves, and so in the middle of
    // iterating reactor::_pollers itself.  So we schedule a task to remove
    // the poller instead.
    //
    // Since we don't want to call the poller after we exit the destructor,
    // we replace it atomically with another one, and schedule a task to
    // delete the replacement.
    if (_pollfn) {
        if (_registration_task) {
            // not added yet, so don't do it at all.
            _registration_task->cancel();
        }
        else
        {
            auto dummy = make_pollfn([] { return false; });
            auto dummy_p = dummy.get();
            auto task = std::make_unique<deregistration_task>(std::move(dummy));
            engine().add_task(std::move(task));
            engine().replace_poller(_pollfn.get(), dummy_p);
        }
    }
}

syscall_work_queue::syscall_work_queue()
    : _pending()
    , _completed()
    , _start_eventfd(0) {
}

void reactor::replace_poller(pollfn* old, pollfn* neww) {
    std::replace(_pollers.begin(), _pollers.end(), old, neww);
}


void syscall_work_queue::submit_item(std::unique_ptr<syscall_work_queue::work_item> item) {
    _queue_has_room.wait().then([this, item = std::move(item)] () mutable {
        _pending.push(item.release());
        _start_eventfd.signal(1);
    });
}


unsigned syscall_work_queue::complete() {
    std::array<work_item*, queue_length> tmp_buf;
    auto end = tmp_buf.data();
    auto nr = _completed.consume_all([&] (work_item* wi) {
        *end++ = wi;
    });
    for (auto p = tmp_buf.data(); p != end; ++p) {
        auto wi = *p;
        wi->complete();
        delete wi;
    }
    _queue_has_room.signal(nr);
    return nr;
}


void
reactor::poller::do_register() {
    // We can't just insert a poller into reactor::_pollers, because we
    // may be running inside a poller ourselves, and so in the middle of
    // iterating reactor::_pollers itself.  So we schedule a task to add
    // the poller instead.
    auto task = std::make_unique<registration_task>(this);
    auto tmp = task.get();
    engine().add_task(std::move(task));
    _registration_task = tmp;
}


bool
reactor::do_expire_lowres_timers() {
    if (_lowres_next_timeout == lowres_clock::time_point()) {
        return false;
    }
    auto now = lowres_clock::now();
    if (now > _lowres_next_timeout) {
        complete_timers(_lowres_timers, _expired_lowres_timers, [this] {
            if (!_lowres_timers.empty()) {
                _lowres_next_timeout = _lowres_timers.get_next_timeout();
            } else {
                _lowres_next_timeout = lowres_clock::time_point();
            }
        });
        return true;
    }
    return false;
}

alignas(64) reactor::smp_pollfn::aligned_flag reactor::smp_pollfn::_membarrier_lock;
inline
pollable_fd_state::~pollable_fd_state() {
    engine().forget(*this);
}
void
reactor::sleep() {
    for (auto i = _pollers.begin(); i != _pollers.end(); ++i) {
        auto ok = (*i)->try_enter_interrupt_mode();
        if (!ok) {
            while (i != _pollers.begin()) {
                (*--i)->exit_interrupt_mode();
            }
            return;
        }
    }
    wait_and_process(-1, &_active_sigmask);
    for (auto i = _pollers.rbegin(); i != _pollers.rend(); ++i) {
        (*i)->exit_interrupt_mode();
    }
}



void network_stack_registry::register_stack(std::string name,
        boost::program_options::options_description opts,
        std::function<future<std::unique_ptr<network_stack>> (options opts)> create, bool make_default) {
    _map()[name] = std::move(create);
    options_description().add(opts);
    if (make_default) {
        _default() = name;
    }
}

std::string network_stack_registry::default_stack() {
    return _default();
}

std::vector<std::string> network_stack_registry::list() {
    std::vector<std::string> ret;
    for (auto&& ns : _map()) {
        ret.push_back(ns.first);
    }
    return ret;
}

future<std::unique_ptr<network_stack>>
network_stack_registry::create(options opts) {
    return create(_default(), opts);
}

future<std::unique_ptr<network_stack>>
network_stack_registry::create(std::string name, options opts) {
    if (!_map().count(name)) {
        throw std::runtime_error("network stack not registered");
    }
    return _map()[name](opts);
}

network_stack_registrator::network_stack_registrator(std::string name,
        boost::program_options::options_description opts,
        std::function<future<std::unique_ptr<network_stack>>(options opts)> factory,
        bool make_default) {
        std::cout<<"network_stack_registrator:"<<name<<"make_default"<<make_default<<std::endl;
        // network_stack_registrator:posixmake_default1
        network_stack_registry::register_stack(name, opts, factory, make_default);
}

network_stack_registrator nsr_posix{"posix",
    boost::program_options::options_description(),
    [](boost::program_options::variables_map ops) {
        return smp::main_thread() ? net::posix_network_stack::create(ops) : net::posix_ap_network_stack::create(ops);
    },
    true
};



namespace net {

template <transport Transport>
class posix_connected_socket_operations;

template <>
class posix_connected_socket_operations<transport::TCP> {
public:
    void set_nodelay(file_desc& _fd, bool nodelay) {
        _fd.setsockopt(IPPROTO_TCP, TCP_NODELAY, int(nodelay));
    }
    bool get_nodelay(file_desc& _fd) const {
        return _fd.getsockopt<int>(IPPROTO_TCP, TCP_NODELAY);
    }
    void set_keepalive(file_desc& _fd, bool keepalive) {
        _fd.setsockopt(SOL_SOCKET, SO_KEEPALIVE, int(keepalive));
    }
    bool get_keepalive(file_desc& _fd) const {
        return _fd.getsockopt<int>(SOL_SOCKET, SO_KEEPALIVE);
    }
    void set_keepalive_parameters(file_desc& _fd, const keepalive_params& params) {
        const tcp_keepalive_params& pms = params;
        _fd.setsockopt(IPPROTO_TCP, TCP_KEEPCNT, pms.count);
        _fd.setsockopt(IPPROTO_TCP, TCP_KEEPIDLE, int(pms.idle.count()));
        _fd.setsockopt(IPPROTO_TCP, TCP_KEEPINTVL, int(pms.interval.count()));
    }
    keepalive_params get_keepalive_parameters(file_desc& _fd) const {
        return tcp_keepalive_params {
            std::chrono::seconds(_fd.getsockopt<int>(IPPROTO_TCP, TCP_KEEPIDLE)),
            std::chrono::seconds(_fd.getsockopt<int>(IPPROTO_TCP, TCP_KEEPINTVL)),
            _fd.getsockopt<unsigned>(IPPROTO_TCP, TCP_KEEPCNT)
        };
    }
};

// 弃用
template <> class posix_connected_socket_operations<transport::SCTP> {
public:
    void set_nodelay(file_desc& _fd, bool nodelay) {
        // _fd.setsockopt(SOL_SCTP, SCTP_NODELAY, int(nodelay));
    }
    bool get_nodelay(file_desc& _fd) const {
        // return _fd.getsockopt<int>(SOL_SCTP, SCTP_NODELAY);
        return true;
    }
    void set_keepalive(file_desc& _fd, bool keepalive) {
        // auto heartbeat = _fd.getsockopt<sctp_paddrparams>(SOL_SCTP, SCTP_PEER_ADDR_PARAMS);
        // if (keepalive) {
        //     heartbeat.spp_flags |= SPP_HB_ENABLE;
        // } else {
        //     heartbeat.spp_flags &= ~SPP_HB_ENABLE;
        // }
        // _fd.setsockopt(SOL_SCTP, SCTP_PEER_ADDR_PARAMS, heartbeat);
    }
    bool get_keepalive(file_desc& _fd) const {
        // return _fd.getsockopt<sctp_paddrparams>(SOL_SCTP, SCTP_PEER_ADDR_PARAMS).spp_flags & SPP_HB_ENABLE;
        return true;
    }
    void set_keepalive_parameters(file_desc& _fd, const keepalive_params& kpms) {
        // const sctp_keepalive_params& pms = std::get<sctp_keepalive_params>(kpms);
        // auto params = _fd.getsockopt<sctp_paddrparams>(SOL_SCTP, SCTP_PEER_ADDR_PARAMS);
        // params.spp_hbinterval = pms.interval.count() * 1000; // in milliseconds
        // params.spp_pathmaxrxt = pms.count;
        // _fd.setsockopt(SOL_SCTP, SCTP_PEER_ADDR_PARAMS, params);
    }
    keepalive_params get_keepalive_parameters(file_desc& _fd) const {
        // auto params = _fd.getsockopt<sctp_paddrparams>(SOL_SCTP, SCTP_PEER_ADDR_PARAMS);
        // return sctp_keepalive_params {
            // std::chrono::seconds(params.spp_hbinterval/1000), // in seconds
            // params.spp_pathmaxrxt
        // };
        // return nullptr;
        return tcp_keepalive_params {
            std::chrono::seconds(_fd.getsockopt<int>(IPPROTO_TCP, TCP_KEEPIDLE)),
            std::chrono::seconds(_fd.getsockopt<int>(IPPROTO_TCP, TCP_KEEPINTVL)),
            _fd.getsockopt<unsigned>(IPPROTO_TCP, TCP_KEEPCNT)
        };
    }
};

template <transport Transport>
class posix_connected_socket_impl final : public connected_socket_impl, posix_connected_socket_operations<Transport> {
    std::shared_ptr<pollable_fd> _fd;
    using _ops = posix_connected_socket_operations<Transport>;
private:
    explicit posix_connected_socket_impl(std::shared_ptr<pollable_fd> fd) : _fd(std::move(fd)) {}
public:
    virtual data_source source() override {
        return data_source(std::make_unique< posix_data_source_impl>(_fd));
    }
    virtual data_sink sink() override {
        return data_sink(std::make_unique< posix_data_sink_impl>(_fd));
    }
    virtual void shutdown_input() override {
        _fd->shutdown(SHUT_RD);
    }
    virtual void shutdown_output() override {
        _fd->shutdown(SHUT_WR);
    }
    virtual void set_nodelay(bool nodelay) override {
        return _ops::set_nodelay(_fd->get_file_desc(), nodelay);
    }
    virtual bool get_nodelay() const override {
        return _ops::get_nodelay(_fd->get_file_desc());
    }
    void set_keepalive(bool keepalive) override {
        return _ops::set_keepalive(_fd->get_file_desc(), keepalive);
    }
    bool get_keepalive() const override {
        return _ops::get_keepalive(_fd->get_file_desc());
    }
    void set_keepalive_parameters(const keepalive_params& p) override {
        return _ops::set_keepalive_parameters(_fd->get_file_desc(), p);
    }
    keepalive_params get_keepalive_parameters() const override {
        return _ops::get_keepalive_parameters(_fd->get_file_desc());
    }
    friend class posix_server_socket_impl<Transport>;
    friend class posix_ap_server_socket_impl<Transport>;
    friend class posix_reuseport_server_socket_impl<Transport>;
    friend class posix_network_stack;
    friend class posix_ap_network_stack;
    friend class posix_socket_impl;
};
using posix_connected_tcp_socket_impl = posix_connected_socket_impl<transport::TCP>;
using posix_connected_sctp_socket_impl = posix_connected_socket_impl<transport::SCTP>;

class posix_socket_impl final : public socket_impl {
    std::shared_ptr<pollable_fd> _fd;
public:
    posix_socket_impl() = default;

    virtual future<connected_socket> connect(net::socket_address sa, net::socket_address local, transport proto = transport::TCP) override {
        _fd = engine().make_pollable_fd(sa, proto);
        return engine().posix_connect(_fd, sa, local).then([fd = _fd, proto]() mutable {
            std::unique_ptr<connected_socket_impl> csi;
            if (proto == transport::TCP) {
                csi.reset(new posix_connected_tcp_socket_impl(std::move(fd)));
            } else {
                csi.reset(new posix_connected_sctp_socket_impl(std::move(fd)));
            }
            return make_ready_future<connected_socket>(connected_socket(std::move(csi)));
        });
    }

    virtual void shutdown() override {
        if (_fd) {
            try {
                _fd->shutdown(SHUT_RDWR);
            } catch (std::system_error& e) {
                if (e.code().value() != ENOTCONN) {
                    throw;
                }
            }
        }
    }
};

template <transport Transport>
future<connected_socket, net::socket_address>
posix_server_socket_impl<Transport>::accept() {
    return _lfd.accept().then([this] (pollable_fd fd, net::socket_address sa) {
        static unsigned balance = 0;
        auto cpu = balance++ % smp::count;

        if (cpu == engine().cpu_id()) {
            std::unique_ptr<connected_socket_impl> csi(
                    new posix_connected_socket_impl<Transport>(std::make_shared<pollable_fd>(std::move(fd))));
            return make_ready_future<connected_socket, net::socket_address>(
                    connected_socket(std::move(csi)), sa);
        } else {
            smp::submit_to(cpu, [this, fd = std::move(fd.get_file_desc()), sa] () mutable {
                posix_ap_server_socket_impl<Transport>::move_connected_socket(_sa, pollable_fd(std::move(fd)), sa);
            });
            return accept();
        }
    });
}

template <transport Transport>
void
posix_server_socket_impl<Transport>::abort_accept() {
    _lfd.abort_reader(std::make_exception_ptr(std::system_error(ECONNABORTED, std::system_category())));
}

template <transport Transport>
future<connected_socket, net::socket_address> posix_ap_server_socket_impl<Transport>::accept() {
    auto conni = get_conn_q().find(_sa.as_posix_sockaddr_in());
    if (conni != get_conn_q().end()) {
        connection c = std::move(conni->second);
        get_conn_q().erase(conni);
        try {
            std::unique_ptr<connected_socket_impl> csi(
                    new posix_connected_socket_impl<Transport>(std::make_shared<pollable_fd>(std::move(c.fd))));
            return make_ready_future<connected_socket, net::socket_address>(connected_socket(std::move(csi)), std::move(c.addr));
        } catch (...) {
            return make_exception_future<connected_socket, net::socket_address>(std::current_exception());
        }
    } else {
        try {
            auto i = get_sockets().emplace(std::piecewise_construct, std::make_tuple(_sa.as_posix_sockaddr_in()), std::make_tuple());
            assert(i.second);
            return i.first->second.get_future();
        } catch (...) {
            return make_exception_future<connected_socket, net::socket_address>(std::current_exception());
        }
    }
}

template <transport Transport>
void
posix_ap_server_socket_impl<Transport>::abort_accept() {
    get_conn_q().erase(_sa.as_posix_sockaddr_in());
    auto i = get_sockets().find(_sa.as_posix_sockaddr_in());
    if (i != get_sockets().end()) {
        i->second.set_exception(std::system_error(ECONNABORTED, std::system_category()));
        get_sockets().erase(i);
    }
}

template <transport Transport>
future<connected_socket, net::socket_address>
posix_reuseport_server_socket_impl<Transport>::accept() {
    return _lfd.accept().then([] (pollable_fd fd, net::socket_address sa) {
        std::unique_ptr<connected_socket_impl> csi(new posix_connected_socket_impl<Transport>(std::make_shared<pollable_fd>(std::move(fd))));
        return make_ready_future<connected_socket, net::socket_address>(
            connected_socket(std::move(csi)), sa);
    });
}

template <transport Transport>
void
posix_reuseport_server_socket_impl<Transport>::abort_accept() {
    _lfd.abort_reader(std::make_exception_ptr(std::system_error(ECONNABORTED, std::system_category())));
}

template <transport Transport>
void  posix_ap_server_socket_impl<Transport>::move_connected_socket(net::socket_address sa, pollable_fd fd, net::socket_address addr) {
    auto i = get_sockets().find(sa.as_posix_sockaddr_in());
    if (i != get_sockets().end()) {
        try {
            std::unique_ptr<connected_socket_impl> csi(new posix_connected_socket_impl<Transport>(std::make_shared<pollable_fd>(std::move(fd))));
            i->second.set_value(connected_socket(std::move(csi)), std::move(addr));
        } catch (...) {
            i->second.set_exception(std::current_exception());
        }
        get_sockets().erase(i);
    } else {
        get_conn_q().emplace(std::piecewise_construct, std::make_tuple(sa.as_posix_sockaddr_in()), std::make_tuple(std::move(fd), std::move(addr)));
    }
}

future<temporary_buffer<char>>
posix_data_source_impl::get() {
    return _fd->read_some(_buf.get_write(), _buf_size).then([this] (size_t size) {
        _buf.trim(size);
        auto ret = std::move(_buf);
        _buf = temporary_buffer<char>(_buf_size);
        return make_ready_future<temporary_buffer<char>>(std::move(ret));
    });
}

future<> posix_data_source_impl::close() {
    _fd->shutdown(SHUT_RD);
    return make_ready_future<>();
}

std::vector<struct iovec> to_iovec(const packet& p) {
    std::vector<struct iovec> v;
    v.reserve(p.nr_frags());
    for (auto&& f : p.fragments()) {
        v.push_back({.iov_base = f.base, .iov_len = f.size});
    }
    return v;
}

std::vector<iovec> to_iovec(std::vector<temporary_buffer<char>>& buf_vec) {
    std::vector<iovec> v;
    v.reserve(buf_vec.size());
    for (auto& buf : buf_vec) {
        v.push_back({.iov_base = buf.get_write(), .iov_len = buf.size()});
    }
    return v;
}

future<>
posix_data_sink_impl::put(temporary_buffer<char> buf) {
    return _fd->write_all(buf.get(), buf.size()).then([d = buf.release()] {});
}

future<>
posix_data_sink_impl::put(packet p) {
    _p = std::move(p);
    return _fd->write_all(_p).then([this] { _p.reset(); });
}

future<>
posix_data_sink_impl::close() {
    _fd->shutdown(SHUT_WR);
    return make_ready_future<>();
}

server_socket
posix_network_stack::listen(net::socket_address sa, listen_options opt) {
    if (opt.proto == transport::TCP) {
        return _reuseport ?
            server_socket(std::make_unique<posix_reuseport_server_tcp_socket_impl>(sa, engine().posix_listen(sa, opt)))
            :
            server_socket(std::make_unique<posix_server_tcp_socket_impl>(sa, engine().posix_listen(sa, opt)));
    } else {
        return _reuseport ?
            server_socket(std::make_unique<posix_reuseport_server_sctp_socket_impl>(sa, engine().posix_listen(sa, opt)))
            :
            server_socket(std::make_unique<posix_server_sctp_socket_impl>(sa, engine().posix_listen(sa, opt)));
    }
}

net::socket posix_network_stack::socket() {
    return net::socket(std::make_unique<posix_socket_impl>());
}


server_socket
posix_ap_network_stack::listen(net::socket_address sa, listen_options opt) {
    if (opt.proto == transport::TCP) {
        return _reuseport ?
            server_socket(std::make_unique<posix_reuseport_server_tcp_socket_impl>(sa, engine().posix_listen(sa, opt)))
            :
            server_socket(std::make_unique<posix_tcp_ap_server_socket_impl>(sa));
    } else {
        return _reuseport ?
            server_socket(std::make_unique<posix_reuseport_server_sctp_socket_impl>(sa, engine().posix_listen(sa, opt)))
            :
            server_socket(std::make_unique<posix_sctp_ap_server_socket_impl>(sa));
    }
}

struct cmsg_with_pktinfo {
    struct cmsghdrcmh;
    struct in_pktinfo pktinfo;
};

class posix_udp_channel : public udp_channel_impl {
private:
    static constexpr int MAX_DATAGRAM_SIZE = 65507;
    struct recv_ctx {
        struct msghdr _hdr;
        struct iovec _iov;
        net::socket_address _src_addr;
        char* _buffer;
        cmsg_with_pktinfo _cmsg;

        recv_ctx() {
            memset(&_hdr, 0, sizeof(_hdr));
            _hdr.msg_iov = &_iov;
            _hdr.msg_iovlen = 1;
            _hdr.msg_name = &_src_addr.u.sa;
            _hdr.msg_namelen = sizeof(_src_addr.u.sas);
            memset(&_cmsg, 0, sizeof(_cmsg));
            _hdr.msg_control = &_cmsg;
            _hdr.msg_controllen = sizeof(_cmsg);
        }

        void prepare() {
            _buffer = new char[MAX_DATAGRAM_SIZE];
            _iov.iov_base = _buffer;
            _iov.iov_len = MAX_DATAGRAM_SIZE;
        }
    };
    struct send_ctx {
        struct msghdr _hdr;
        std::vector<struct iovec> _iovecs;
        net::socket_address _dst;
        packet _p;

        send_ctx() {
            memset(&_hdr, 0, sizeof(_hdr));
            _hdr.msg_name = &_dst.u.sa;
            _hdr.msg_namelen = sizeof(_dst.u.sas);
        }

        void prepare(ipv4_addr dst, packet p) {
            _dst = make_ipv4_address(dst);
            _p = std::move(p);
            _iovecs = std::move(to_iovec(_p));
            _hdr.msg_iov = _iovecs.data();
            _hdr.msg_iovlen = _iovecs.size();
        }
    };
    std::unique_ptr<pollable_fd> _fd;
    ipv4_addr _address;
    recv_ctx _recv;
    send_ctx _send;
    bool _closed;
public:
    posix_udp_channel(ipv4_addr bind_address)
            : _closed(false) {
        std::cout<<"sa start"<<std::endl;
        auto sa = make_ipv4_address(bind_address);
        std::cout<<"posix udp channel fd socket"<<std::endl;
        file_desc fd = file_desc::socket(sa.u.sa.sa_family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        std::cout<<"fd setsockpt"<<std::endl;
        fd.setsockopt(SOL_IP, IP_PKTINFO, true);
        std::cout<<"engine reuseport"<<std::endl;
        if (engine().posix_reuseport_available()) {
            fd.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1);
        }
        std::cout<<"bind start"<<std::endl;
        fd.bind(sa.u.sa, sizeof(sa.u.sas));
        std::cout<<"bind end"<<std::endl;
        std::cout<<"_address"<<std::endl;
        _address = ipv4_addr(fd.get_address());
        std::cout<<"_fd"<<std::endl;
        _fd = std::make_unique<pollable_fd>(std::move(fd));
    }
    virtual ~posix_udp_channel() { if (!_closed) close(); };
    virtual future<udp_datagram> receive() override;
    virtual future<> send(ipv4_addr dst, const char *msg);
    virtual future<> send(ipv4_addr dst, packet p);
    virtual void close() override {
        _closed = true;
        _fd->abort_reader(std::make_exception_ptr(std::system_error(EPIPE, std::system_category())));
        _fd->abort_writer(std::make_exception_ptr(std::system_error(EPIPE, std::system_category())));
        _fd.reset();
    }
    virtual bool is_closed() const override { return _closed; }
};

future<> posix_udp_channel::send(ipv4_addr dst, const char *message) {
    auto len = strlen(message);
    return _fd->sendto(make_ipv4_address(dst), message, len)
            .then([len] (size_t size) { assert(size == len); });
}

future<> posix_udp_channel::send(ipv4_addr dst, packet p) {
    auto len = p.len();
    _send.prepare(dst, std::move(p));
    return _fd->sendmsg(&_send._hdr)
            .then([len] (size_t size) { assert(size == len); });
}

net::udp_channel::udp_channel(){}

net::udp_channel::udp_channel(std::unique_ptr<udp_channel_impl> impl) : _impl(std::move(impl)){
    std::cout<<"net::udp_channel consturct"<<std::endl;
}



udp_channel
posix_network_stack::make_udp_channel(ipv4_addr addr) {
    std::cout<<"posix make udp channel  "<<"addr:"<<addr.port<<std::endl;//执行到了这一步.
    auto chan = udp_channel(std::make_unique<posix_udp_channel>(addr));
    std::cout<<"posix udp channel end########"<<std::endl;
    return chan;
}





class posix_datagram : public udp_datagram_impl {
private:
    ipv4_addr _src;
    ipv4_addr _dst;
    packet _p;
public:
    posix_datagram(ipv4_addr src, ipv4_addr dst, packet p) : _src(src), _dst(dst), _p(std::move(p)) {}
    virtual ipv4_addr get_src() override { return _src; }
    virtual ipv4_addr get_dst() override { return _dst; }
    virtual uint16_t get_dst_port() override { return _dst.port; }
    virtual packet& get_data() override { return _p; }
};

future<udp_datagram>
posix_udp_channel::receive() {
    _recv.prepare();
    return _fd->recvmsg(&_recv._hdr).then([this] (size_t size) {
        auto dst = ipv4_addr(_recv._cmsg.pktinfo.ipi_addr.s_addr, _address.port);
        return make_ready_future<udp_datagram>(udp_datagram(std::make_unique<posix_datagram>(
            _recv._src_addr, dst, packet(fragment{_recv._buffer, size}, make_deleter([buf = _recv._buffer] { delete[] buf; })))));
    }).handle_exception([p = _recv._buffer](auto ep) {
        delete[] p;
        return make_exception_future<udp_datagram>(std::move(ep));
    });
}
}




net::udp_channel::~udp_channel()
{}

net::udp_channel::udp_channel(udp_channel&&) = default;
net::udp_channel& net::udp_channel::operator=(udp_channel&&) = default;

future<net::udp_datagram> net::udp_channel::receive() {
    return _impl->receive();
}

future<> net::udp_channel::send(ipv4_addr dst, const char* msg) {
    return _impl->send(std::move(dst), msg);
}

future<> net::udp_channel::send(ipv4_addr dst, packet p) {
    return _impl->send(std::move(dst), std::move(p));
}

bool net::udp_channel::is_closed() const {
    return _impl->is_closed();
}

void net::udp_channel::close() {
    return _impl->close();
}

connected_socket::connected_socket()
{}

connected_socket::connected_socket(
        std::unique_ptr<net::connected_socket_impl> csi)
        : _csi(std::move(csi)) {
}

connected_socket::connected_socket(connected_socket&& cs) noexcept = default;
connected_socket& connected_socket::operator=(connected_socket&& cs) noexcept = default;

connected_socket::~connected_socket(){ }

input_stream<char> connected_socket::input() {
    return input_stream<char>(_csi->source());
}

output_stream<char> connected_socket::output(size_t buffer_size) {
    // TODO: allow user to determine buffer size etc
    return output_stream<char>(_csi->sink(), buffer_size, false, true);
}

void connected_socket::set_nodelay(bool nodelay) {
    _csi->set_nodelay(nodelay);
}

bool connected_socket::get_nodelay() const {
    return _csi->get_nodelay();
}


void connected_socket::set_keepalive(bool keepalive) {
    _csi->set_keepalive(keepalive);
}

bool connected_socket::get_keepalive() const {
    return _csi->get_keepalive();
}

void connected_socket::set_keepalive_parameters(const keepalive_params& p) {
    _csi->set_keepalive_parameters(p);
}

keepalive_params connected_socket::get_keepalive_parameters() const {
    return _csi->get_keepalive_parameters();
}

void connected_socket::shutdown_output() {
    _csi->shutdown_output();
}

void connected_socket::shutdown_input() {
    _csi->shutdown_input();
}

net::socket::~socket(){}
net::socket::socket(
        std::unique_ptr<::net::socket_impl> si)
        : _si(std::move(si)) {
}

net::socket::socket(net::socket&&) noexcept = default;
net::socket& net::socket::operator=(net::socket&&) noexcept = default;

future<connected_socket> net::socket::connect(net::socket_address sa, net::socket_address local, transport proto) {
    return _si->connect(sa, local, proto);
}

void net::socket::shutdown() {
    _si->shutdown();
}

server_socket::server_socket() {}

server_socket::server_socket(std::unique_ptr<net::server_socket_impl> ssi)
        : _ssi(std::move(ssi)) {}
server_socket::server_socket(server_socket&& ss) noexcept = default;
server_socket& server_socket::operator=(server_socket&& cs) noexcept = default;
server_socket::~server_socket() {
}
future<connected_socket, net::socket_address> server_socket::accept() {
    return _ssi->accept();
}
void server_socket::abort_accept() {
    _ssi->abort_accept();
}

net::socket_address::socket_address(ipv4_addr addr)
    :net::socket_address(make_ipv4_address(addr))
{}


bool net::socket_address::operator==(const net::socket_address& a) const {
    // TODO: handle ipv6
    return std::tie(u.in.sin_family, u.in.sin_port, u.in.sin_addr.s_addr)
                    == std::tie(a.u.in.sin_family, a.u.in.sin_port,
                                    a.u.in.sin_addr.s_addr);
}

pollable_fd
reactor::posix_listen(net::socket_address sa, listen_options opts) {
    file_desc fd = file_desc::socket(sa.u.sa.sa_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, int(opts.proto));
    if (opts.reuse_address) {
        fd.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1);
    }
    if (_reuseport)
        fd.setsockopt(SOL_SOCKET, SO_REUSEPORT, 1);

    fd.bind(sa.u.sa, sizeof(sa.u.sas));
    fd.listen(100);
    return pollable_fd(std::move(fd));
}


std::shared_ptr<pollable_fd>
reactor::make_pollable_fd(net::socket_address sa, transport proto) {
    file_desc fd = file_desc::socket(sa.u.sa.sa_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, int(proto));
    return std::make_shared<pollable_fd>(pollable_fd(std::move(fd)));
}

future<>
reactor::posix_connect(std::shared_ptr<pollable_fd> pfd, net::socket_address sa, net::socket_address local) {
    pfd->get_file_desc().bind(local.u.sa, sizeof(sa.u.sas));
    pfd->get_file_desc().connect(sa.u.sa, sizeof(sa.u.sas));
    return pfd->writeable().then([pfd]() mutable {
        auto err = pfd->get_file_desc().getsockopt<int>(SOL_SOCKET, SO_ERROR);
        if (err != 0) {
            throw std::system_error(err, std::system_category());
        }
        return make_ready_future<>();
    });
}

server_socket
reactor::listen(net::socket_address sa, listen_options opt) {
    return server_socket(_network_stack->listen(sa, opt));
}

future<connected_socket>
reactor::connect(net::socket_address sa) {
    return _network_stack->connect(sa);
}

future<connected_socket>
reactor::connect(net::socket_address sa, net::socket_address local, transport proto) {
    return _network_stack->connect(sa, local, proto);
}




inline
future<pollable_fd, net::socket_address>
reactor::accept(pollable_fd_state& listenfd) {
    return readable(listenfd).then([&listenfd] () mutable {
        net::socket_address sa;
        socklen_t sl = sizeof(&sa.u.sas);
        file_desc fd = listenfd.fd.accept(sa.u.sa, sl, SOCK_NONBLOCK | SOCK_CLOEXEC);
        pollable_fd pfd(std::move(fd), pollable_fd::speculation(EPOLLOUT));
        return make_ready_future<pollable_fd, net::socket_address>(std::move(pfd), std::move(sa));
    });
}

inline
future<size_t>
reactor::read_some(pollable_fd_state& fd, void* buffer, size_t len) {
    return readable(fd).then([this, &fd, buffer, len] () mutable {
        auto r = fd.fd.read(buffer, len);
        if (!r) {
            return read_some(fd, buffer, len);
        }
        if (size_t(*r) == len) {
            fd.speculate_epoll(EPOLLIN);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<size_t>
reactor::read_some(pollable_fd_state& fd, const std::vector<iovec>& iov) {
    return readable(fd).then([this, &fd, iov = iov] () mutable {
        ::msghdr mh = {};
        mh.msg_iov = &iov[0];
        mh.msg_iovlen = iov.size();
        auto r = fd.fd.recvmsg(&mh, 0);
        if (!r) {
            return read_some(fd, iov);
        }
        if (size_t(*r) == iovec_len(iov)) {
            fd.speculate_epoll(EPOLLIN);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<size_t>
reactor::write_some(pollable_fd_state& fd, const void* buffer, size_t len) {
    return writeable(fd).then([this, &fd, buffer, len] () mutable {
        auto r = fd.fd.send(buffer, len, MSG_NOSIGNAL);
        if (!r) {
            return write_some(fd, buffer, len);
        }
        if (size_t(*r) == len) {
            fd.speculate_epoll(EPOLLOUT);
        }
        return make_ready_future<size_t>(*r);
    });
}

inline
future<>
reactor::write_all_part(pollable_fd_state& fd, const void* buffer, size_t len, size_t completed) {
    if (completed == len) {
        return make_ready_future<>();
    } else {
        return write_some(fd, static_cast<const char*>(buffer) + completed, len - completed).then(
                [&fd, buffer, len, completed, this] (size_t part) mutable {
            return write_all_part(fd, buffer, len, completed + part);
        });
    }
}

inline
future<>
reactor::write_all(pollable_fd_state& fd, const void* buffer, size_t len) {
    assert(len);
    return write_all_part(fd, buffer, len, 0);
}



/// \file

/// Returns a future which completes after a specified time has elapsed.
///
/// \param dur minimum amount of time before the returned future becomes
///            ready.
/// \return A \ref future which becomes ready when the sleep duration elapses.
template <typename Clock = steady_clock_type, typename Rep, typename Period>
future<> sleep(std::chrono::duration<Rep, Period> dur) {
    struct sleeper {
        promise<> done;
        timer<Clock> tmr;
        sleeper(std::chrono::duration<Rep, Period> dur)
            : tmr([this] { done.set_value(); })
        {
            tmr.arm(dur);
        }
    };
    sleeper *s = new sleeper(dur);
    future<> fut = s->done.get_future();
    return fut.then([s] { delete s; });
}

/// exception that is thrown when application is in process of been stopped
class sleep_aborted : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Sleep is aborted";
    }
};

/// Returns a future which completes after a specified time has elapsed
/// or throws \ref sleep_aborted exception if application is aborted
///
/// \param dur minimum amount of time before the returned future becomes
///            ready.
/// \return A \ref future which becomes ready when the sleep duration elapses.
template <typename Rep, typename Period>
future<> sleep_abortable(std::chrono::duration<Rep, Period> dur) {
    return engine().wait_for_stop(dur).then([] {
        throw sleep_aborted();
    }).handle_exception([] (std::exception_ptr ep) {
        try {
            std::rethrow_exception(ep);
        } catch(condition_variable_timed_out&) {};
    });
}

namespace net
{
    std::ostream& operator<<(std::ostream& os, const packet& p) {
        os << "packet{";
        bool first = true;
        for (auto&& frag : p.fragments()) {
            if (!first) {
                os << ", ";
            }
            first = false;
            if (std::all_of(frag.base, frag.base + frag.size, [] (int c) { return c >= 9 && c <= 0x7f; })) {
                os << '"';
                for (auto p = frag.base; p != frag.base + frag.size; ++p) {
                    auto c = *p;
                    if (isprint(c)) {
                        os << c;
                    } else if (c == '\r') {
                        os << "\\r";
                    } else if (c == '\n') {
                        os << "\\n";
                    } else if (c == '\t') {
                        os << "\\t";
                    } else {
                        uint8_t b = c;
                        os << "\\x" << (b / 16) << (b % 16);
                    }
                }
                os << '"';
            } else {
                os << "{";
                bool nfirst = true;
                for (auto p = frag.base; p != frag.base + frag.size; ++p) {
                    if (!nfirst) {
                        os << " ";
                    }
                    nfirst = false;
                    uint8_t b = *p;
                    char buf[3]; // Enough for 2 hex digits + null terminator
                    std::snprintf(buf, sizeof(buf), "%02x", static_cast<unsigned>(b));
                    os << buf;
                }
                os << "}";
            }
        }
        os << "}";
        return os;
    }
}




file_impl* file_impl::get_file_impl(file& f) {
    return f._file_impl.get();
}

posix_file_impl::posix_file_impl(int fd, file_open_options options)
        : _fd(fd) {
    query_dma_alignment();
}

posix_file_impl::~posix_file_impl() {
    if (_refcount && _refcount->fetch_add(-1, std::memory_order_relaxed) != 1) {
        return;
    }
    delete _refcount;
    if (_fd != -1) {
        // Note: close() can be a blocking operation on NFS
        ::close(_fd);
    }
}

void
posix_file_impl::query_dma_alignment() {
    // dioattr da;
    // auto r = ioctl(_fd, XFS_IOC_DIOINFO, &da);
    // if (r == 0) {
    //     _memory_dma_alignment = da.d_mem;
    //     _disk_read_dma_alignment = da.d_miniosz;
    //     // xfs wants at least the block size for writes
    //     // FIXME: really read the block size
    //     _disk_write_dma_alignment = std::max<unsigned>(da.d_miniosz, 4096);
    // }
    return;
}

future<size_t>
posix_file_impl::write_dma(uint64_t pos, const void* buffer, size_t len, const io_priority_class& io_priority_class) {
    return engine().submit_io_write(io_priority_class, len, [fd = _fd, pos, buffer, len] (iocb& io) {
        io_prep_pwrite(&io, fd, const_cast<void*>(buffer), len, pos);
    }).then([] (io_event ev) {
        throw(long(ev.res));
        return make_ready_future<size_t>(size_t(ev.res));
    });
}

future<size_t>
posix_file_impl::write_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& io_priority_class) {
    auto len = boost::accumulate(iov | boost::adaptors::transformed(std::mem_fn(&iovec::iov_len)), size_t(0));
    auto iov_ptr = std::make_unique<std::vector<iovec>>(std::move(iov));
    auto size = iov_ptr->size();
    auto data = iov_ptr->data();
    return engine().submit_io_write(io_priority_class, len, [fd = _fd, pos, data, size] (iocb& io) {
        io_prep_pwritev(&io, fd, data, size, pos);
    }).then([iov_ptr = std::move(iov_ptr)] (io_event ev) {
        throw(long(ev.res));
        return make_ready_future<size_t>(size_t(ev.res));
    });
}

future<size_t>
posix_file_impl::read_dma(uint64_t pos, void* buffer, size_t len, const io_priority_class& io_priority_class) {
    return engine().submit_io_read(io_priority_class, len, [fd = _fd, pos, buffer, len] (iocb& io) {
        io_prep_pread(&io, fd, buffer, len, pos);
    }).then([] (io_event ev) {
        throw(long(ev.res));
        return make_ready_future<size_t>(size_t(ev.res));
    });
}

future<size_t>
posix_file_impl::read_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& io_priority_class) {
    auto len = boost::accumulate(iov | boost::adaptors::transformed(std::mem_fn(&iovec::iov_len)), size_t(0));
    auto iov_ptr = std::make_unique<std::vector<iovec>>(std::move(iov));
    auto size = iov_ptr->size();
    auto data = iov_ptr->data();
    return engine().submit_io_read(io_priority_class, len, [fd = _fd, pos, data, size] (iocb& io) {
        io_prep_preadv(&io, fd, data, size, pos);
    }).then([iov_ptr = std::move(iov_ptr)] (io_event ev) {
        throw(long(ev.res));
        return make_ready_future<size_t>(size_t(ev.res));
    });
}

future<temporary_buffer<uint8_t>>
posix_file_impl::dma_read_bulk(uint64_t offset, size_t range_size, const io_priority_class& pc) {
    using tmp_buf_type = typename file::read_state<uint8_t>::tmp_buf_type;

    auto front = offset & (_disk_read_dma_alignment - 1);
    offset -= front;
    range_size += front;

    auto rstate = std::make_shared<file::read_state<uint8_t>>(offset, front,
                                                       range_size,
                                                       _memory_dma_alignment,
                                                       _disk_read_dma_alignment);

    //
    // First, try to read directly into the buffer. Most of the reads will
    // end here.
    //
    auto read = read_dma(offset, rstate->buf.get_write(),
                         rstate->buf.size(), pc);

    return read.then([rstate, this, &pc] (size_t size) mutable {
        rstate->pos = size;

        //
        // If we haven't read all required data at once -
        // start read-copy sequence. We can't continue with direct reads
        // into the previously allocated buffer here since we have to ensure
        // the aligned read length and thus the aligned destination buffer
        // size.
        //
        // The copying will actually take place only if there was a HW glitch.
        // In EOF case or in case of a persistent I/O error the only overhead is
        // an extra allocation.
        //
        return do_until(
            [rstate] { return rstate->done(); },
            [rstate, this, &pc] () mutable {
            return read_maybe_eof(
                rstate->cur_offset(), rstate->left_to_read(), pc).then(
                    [rstate] (auto buf1) mutable {
                if (buf1.size()) {
                    rstate->append_new_data(buf1);
                } else {
                    rstate->eof = true;
                }

                return make_ready_future<>();
            });
        }).then([rstate] () mutable {
            //
            // If we are here we are promised to have read some bytes beyond
            // "front" so we may trim straight away.
            //
            rstate->trim_buf_before_ret();
            return make_ready_future<tmp_buf_type>(std::move(rstate->buf));
        });
    });
}

future<temporary_buffer<uint8_t>>
posix_file_impl::read_maybe_eof(uint64_t pos, size_t len, const io_priority_class& pc) {
    //
    // We have to allocate a new aligned buffer to make sure we don't get
    // an EINVAL error due to unaligned destination buffer.
    //
    temporary_buffer<uint8_t> buf = temporary_buffer<uint8_t>::aligned(
               _memory_dma_alignment, align_up(len, size_t(_disk_read_dma_alignment)));

    // try to read a single bulk from the given position
    auto dst = buf.get_write();
    auto buf_size = buf.size();
    return read_dma(pos, dst, buf_size, pc).then_wrapped(
            [buf = std::move(buf)](future<size_t> f) mutable {
        try {
            size_t size = std::get<0>(f.get());

            buf.trim(size);

            return std::move(buf);
        } catch (std::system_error& e) {
            //
            // TODO: implement a non-trowing file_impl::dma_read() interface to
            //       avoid the exceptions throwing in a good flow completely.
            //       Otherwise for users that don't want to care about the
            //       underlying file size and preventing the attempts to read
            //       bytes beyond EOF there will always be at least one
            //       exception throwing at the file end for files with unaligned
            //       length.
            //
            if (e.code().value() == EINVAL) {
                buf.trim(0);
                return std::move(buf);
            } else {
                throw;
            }
        }
    });
}

append_challenged_posix_file_impl::append_challenged_posix_file_impl(int fd, file_open_options options,
        unsigned max_size_changing_ops)
        : posix_file_impl(fd, options), _max_size_changing_ops(max_size_changing_ops) {
    auto r = ::lseek(fd, 0, SEEK_END);
    throw(r == -1);
    _committed_size = _logical_size = r;
    _sloppy_size = options.sloppy_size;
    auto hint = align_up<uint64_t>(options.sloppy_size_hint, _disk_write_dma_alignment);
    if (_sloppy_size && _committed_size < hint) {
        auto r = ::ftruncate(_fd, hint);
        // We can ignore errors, since it's just a hint.
        if (r != -1) {
            _committed_size = hint;
        }
    }
}

append_challenged_posix_file_impl::~append_challenged_posix_file_impl() {
}

bool
append_challenged_posix_file_impl::must_run_alone(const op& candidate) const noexcept {
    // checks if candidate is a non-write, size-changing operation.
    return (candidate.type == opcode::truncate)
            || (_sloppy_size && candidate.type == opcode::flush);
}

bool
append_challenged_posix_file_impl::size_changing(const op& candidate) const noexcept {
    return (candidate.type == opcode::write && candidate.pos + candidate.len > _committed_size)
            || must_run_alone(candidate);
}

bool
append_challenged_posix_file_impl::may_dispatch(const op& candidate) const noexcept {
    if (size_changing(candidate)) {
        return !_current_size_changing_ops && !_current_non_size_changing_ops;
    } else {
        return !_current_size_changing_ops;
    }
}

void
append_challenged_posix_file_impl::dispatch(op& candidate) noexcept {
    unsigned* op_counter = size_changing(candidate)
            ? &_current_size_changing_ops : &_current_non_size_changing_ops;
    ++*op_counter;
    candidate.run().then([this, op_counter] {
        --*op_counter;
        process_queue();
    });
}

// If we have a bunch of size-extending writes in the queue,
// issue an ftruncate() extending the file size, so they can
// be issued concurrently.
void
append_challenged_posix_file_impl::optimize_queue() noexcept {
    if (_current_non_size_changing_ops || _current_size_changing_ops) {
        // Can't issue an ftruncate() if something is going on
        return;
    }
    auto speculative_size = _committed_size;
    unsigned n_appending_writes = 0;
    for (const auto& op : _q) {
        // stop calculating speculative size after a non-write, size-changing
        // operation is found to prevent an useless truncate from being issued.
        if (must_run_alone(op)) {
            break;
        }
        if (op.type == opcode::write && op.pos + op.len > _committed_size) {
            speculative_size = std::max(speculative_size, op.pos + op.len);
            ++n_appending_writes;
        }
    }
    if (n_appending_writes > _max_size_changing_ops
            || (n_appending_writes && _sloppy_size)) {
        if (_sloppy_size && speculative_size < 2 * _committed_size) {
            speculative_size = align_up<uint64_t>(2 * _committed_size, _disk_write_dma_alignment);
        }
        // We're all alone, so issuing the ftruncate() in the reactor
        // thread won't block us.
        //
        // Issuing it in the syscall thread is too slow; this can happen
        // every several ops, and the syscall thread latency can be very
        // high.
        auto r = ::ftruncate(_fd, speculative_size);
        if (r != -1) {
            _committed_size = speculative_size;
            // If we failed, the next write will pick it up.
        }
    }
}

void
append_challenged_posix_file_impl::process_queue() noexcept {
    optimize_queue();
    while (!_q.empty() && may_dispatch(_q.front())) {
        op candidate = std::move(_q.front());
        _q.pop_front();
        dispatch(candidate);
    }
    if (may_quit()) {
        _completed.set_value();
        _done = false; // prevents _completed to be signaled again in case of recursion
    }
}

void
append_challenged_posix_file_impl::enqueue(op&& op) {
    _q.push_back(std::move(op));
    process_queue();
}

bool
append_challenged_posix_file_impl::may_quit() const noexcept {
    return _done && _q.empty() && !_current_non_size_changing_ops && !_current_size_changing_ops;
}

void
append_challenged_posix_file_impl::commit_size(uint64_t size) noexcept {
    _committed_size = std::max(size, _committed_size);
    _logical_size = std::max(size, _logical_size);
}

future<size_t>
append_challenged_posix_file_impl::read_dma(uint64_t pos, void* buffer, size_t len, const io_priority_class& pc) {
    if (pos >= _logical_size) {
        // later() avoids tail recursion
        return later().then([] {
            return size_t(0);
        });
    }
    len = std::min(pos + len, align_up<uint64_t>(_logical_size, _disk_read_dma_alignment)) - pos;
    auto pr = std::make_shared<promise<size_t>>(promise<size_t>());
    enqueue({
        opcode::read,
        pos,
        len,
        [this, pr, pos, buffer, len, &pc] {
            return futurize_apply([this, pos, buffer, len, &pc] () mutable {
                return posix_file_impl::read_dma(pos, buffer, len, pc);
            }).then_wrapped([pr] (future<size_t> f) {
                f.forward_to(std::move(*pr));
            });
        }
    });
    return pr->get_future();
}

future<size_t>
append_challenged_posix_file_impl::read_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc) {
    if (pos >= _logical_size) {
        // later() avoids tail recursion
        return later().then([] {
            return size_t(0);
        });
    }
    size_t len = 0;
    auto i = iov.begin();
    while (i != iov.end() && pos + len + i->iov_len <= _logical_size) {
        len += i++->iov_len;
    }
    auto aligned_logical_size = align_up<uint64_t>(_logical_size, _disk_read_dma_alignment);
    if (i != iov.end()) {
        auto last_len = pos + len + i->iov_len - aligned_logical_size;
        if (last_len) {
            i++->iov_len = last_len;
        }
        iov.erase(i, iov.end());
    }
    auto pr = std::make_shared<promise<size_t>>(promise<size_t>());
    enqueue({
        opcode::read,
        pos,
        len,
        [this, pr, pos, iov = std::move(iov), &pc] () mutable {
            return futurize_apply([this, pos, iov = std::move(iov), &pc] () mutable {
                return posix_file_impl::read_dma(pos, std::move(iov), pc);
            }).then_wrapped([pr] (future<size_t> f) {
                f.forward_to(std::move(*pr));
            });
        }
    });
    return pr->get_future();
}

future<size_t>
append_challenged_posix_file_impl::write_dma(uint64_t pos, const void* buffer, size_t len, const io_priority_class& pc) {
    auto pr = std::make_shared<promise<size_t>>(promise<size_t>());
    enqueue({
        opcode::write,
        pos,
        len,
        [this, pr, pos, buffer, len, &pc] {
            return futurize_apply([this, pos, buffer, len, &pc] () mutable {
                return posix_file_impl::write_dma(pos, buffer, len, pc);
            }).then_wrapped([this, pos, pr] (future<size_t> f) {
                if (!f.failed()) {
                    auto ret = f.get0();
                    commit_size(pos + ret);
                    // Can't use forward_to(), because future::get0() invalidates the future.
                    pr->set_value(ret);
                } else {
                    f.forward_to(std::move(*pr));
                }
            });
        }
    });
    return pr->get_future();
}

future<size_t>
append_challenged_posix_file_impl::write_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc) {
    auto pr = std::make_shared<promise<size_t>>(promise<size_t>());
    auto len = boost::accumulate(iov | boost::adaptors::transformed(std::mem_fn(&iovec::iov_len)), size_t(0));
    enqueue({
        opcode::write,
        pos,
        len,
        [this, pr, pos, iov = std::move(iov), &pc] () mutable {
            return futurize_apply([this, pos, iov = std::move(iov), &pc] () mutable {
                return posix_file_impl::write_dma(pos, std::move(iov), pc);
            }).then_wrapped([this, pos, pr] (future<size_t> f) {
                if (!f.failed()) {
                    auto ret = f.get0();
                    commit_size(pos + ret);
                    // Can't use forward_to(), because future::get0() invalidates the future.
                    pr->set_value(ret);
                } else {
                    f.forward_to(std::move(*pr));
                }
            });
        }
    });
    return pr->get_future();
}

future<>
append_challenged_posix_file_impl::flush() {
    if (!_sloppy_size || _logical_size == _committed_size) {
        // FIXME: determine if flush can block concurrent reads or writes
        return posix_file_impl::flush();
    } else {
        auto pr = std::make_shared<promise<>>(promise<>());
        enqueue({
            opcode::flush,
            0,
            0,
            [this, pr] () {
                return futurize_apply([this] {
                    if (_logical_size != _committed_size) {
                        // We're all alone, so can truncate in reactor thread
                        auto r = ::ftruncate(_fd, _logical_size);
                        throw(-1);
                        _committed_size = _logical_size;
                    }
                    return posix_file_impl::flush();
                }).then_wrapped([pr] (future<> f) {
                    f.forward_to(std::move(*pr));
                });
            }
        });
        return pr->get_future();
    }
}

future<struct stat>
append_challenged_posix_file_impl::stat() {
    // FIXME: can this conflict with anything?
    return posix_file_impl::stat().then([this] (struct stat stat) {
        stat.st_size = _logical_size;
        return stat;
    });
}

future<>
append_challenged_posix_file_impl::truncate(uint64_t length) {
    auto pr = std::make_shared<promise<>>(promise<>());
    enqueue({
        opcode::truncate,
        length,
        0,
        [this, pr, length] () mutable {
            return futurize_apply([this, length] {
                return posix_file_impl::truncate(length);
            }).then_wrapped([this, pr, length] (future<> f) {
                if (!f.failed()) {
                    _committed_size = _logical_size = length;
                }
                f.forward_to(std::move(*pr));
            });
        }
    });
    return pr->get_future();
}

future<uint64_t>
append_challenged_posix_file_impl::size() {
    return make_ready_future<size_t>(_logical_size);
}

future<>
append_challenged_posix_file_impl::close() noexcept {
    // Caller should have drained all pending I/O
    _done = true;
    process_queue();
    return _completed.get_future().then([this] {
        if (_logical_size != _committed_size) {
            auto r = ::ftruncate(_fd, _logical_size);
            if (r != -1) {
                _committed_size = _logical_size;
            }
        }
        return posix_file_impl::close();
    });
}

static
unsigned
xfs_concurrency_from_kernel_version() {
    return 0;
}

inline
std::shared_ptr<file_impl>
make_file_impl(int fd, file_open_options options) {
    auto r = ::ioctl(fd, BLKGETSIZE);
    if (r != -1) {
        return std::make_shared<blockdev_file_impl>(fd, options);
    } else {
        // FIXME: obtain these flags from somewhere else
        auto flags = ::fcntl(fd, F_GETFL);
        if(flags == -1)throw(-1);
        throw(flags == -1);
        if ((flags & O_ACCMODE) == O_RDONLY) {
            return std::make_shared<posix_file_impl>(fd, options);
        }
        struct stat st;
        auto r = ::fstat(fd, &st);
        if(r==-1){
            throw(-1);
        }
        if (S_ISDIR(st.st_mode)) {
            return std::make_shared<posix_file_impl>(fd, options);
        }
        struct append_support {
            bool append_challenged;
            unsigned append_concurrency;
        };
        static thread_local std::unordered_map<decltype(st.st_dev), append_support> s_fstype;
        if (!s_fstype.count(st.st_dev)) {
            struct statfs sfs;
            auto r = ::fstatfs(fd, &sfs);
            if(r==-1)throw(-1);
            // throw_system_error_on(r == -1);
            append_support as;
            switch (sfs.f_type) {
            case 0x58465342: /* XFS */
                as.append_challenged = true;
                static auto xc = xfs_concurrency_from_kernel_version();
                as.append_concurrency = xc;
                break;
            case 0x6969: /* NFS */
                as.append_challenged = false;
                as.append_concurrency = 0;
                break;
            default:
                as.append_challenged = true;
                as.append_concurrency = 0;
            }
            s_fstype[st.st_dev] = as;
        }
        auto as = s_fstype[st.st_dev];
        if (!as.append_challenged) {
            return std::make_shared<posix_file_impl>(fd, options);
        }
        return std::make_shared<append_challenged_posix_file_impl>(fd, options, as.append_concurrency);
    }
}

file::file(int fd, file_open_options options)
        : _file_impl(make_file_impl(fd, options)) {
}


future<file>
reactor::open_file_dma(std::string name, open_flags flags, file_open_options options) {
    static constexpr mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644
    return _thread_pool.submit<syscall_result<int>>([name, flags, options, strict_o_direct = _strict_o_direct] {
        // We want O_DIRECT, except in two cases:
        //   - tmpfs (which doesn't support it, but works fine anyway)
        //   - strict_o_direct == false (where we forgive it being not supported)
        // Because open() with O_DIRECT will fail, we open it without O_DIRECT, try
        // to update it to O_DIRECT with fcntl(), and if that fails, see if we
        // can forgive it.
        auto is_tmpfs = [] (int fd) {
            struct ::statfs buf;
            auto r = ::fstatfs(fd, &buf);
            if (r == -1) {
                return false;
            }
            return buf.f_type == 0x01021994; // TMPFS_MAGIC
        };
        auto open_flags = O_CLOEXEC | static_cast<int>(flags);
        int fd = ::open(name.c_str(), open_flags, mode);
        if (fd == -1) {
            return wrap_syscall<int>(fd);
        }
        int r = ::fcntl(fd, F_SETFL, open_flags | O_DIRECT);
        auto maybe_ret = wrap_syscall<int>(r);  // capture errno (should be EINVAL)
        if (r == -1  && strict_o_direct && !is_tmpfs(fd)) {
            ::close(fd);
            return maybe_ret;
        }
        // if (fd != -1) {
        //     fsxattr attr = {};
        //     if (options.extent_allocation_size_hint) {
        //         attr.fsx_xflags |= XFS_XFLAG_EXTSIZE;
        //         attr.fsx_extsize = options.extent_allocation_size_hint;
        //     }
        //     // Ignore error; may be !xfs, and just a hint anyway
        //     ::ioctl(fd, XFS_IOC_FSSETXATTR, &attr);
        // }
        return wrap_syscall<int>(fd);
    }).then([options] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<file>(file(sr.result, options));
    });
}

future<>
reactor::remove_file(std::string pathname) {
    return engine()._thread_pool.submit<syscall_result<int>>([pathname] {
        return wrap_syscall<int>(::remove(pathname.c_str()));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<>();
    });
}

future<>
reactor::rename_file(std::string old_pathname, std::string new_pathname) {
    return engine()._thread_pool.submit<syscall_result<int>>([old_pathname, new_pathname] {
        return wrap_syscall<int>(::rename(old_pathname.c_str(), new_pathname.c_str()));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<>();
    });
}

future<>
reactor::link_file(std::string oldpath, std::string newpath) {
    return engine()._thread_pool.submit<syscall_result<int>>([oldpath = std::move(oldpath), newpath = std::move(newpath)] {
        return wrap_syscall<int>(::link(oldpath.c_str(), newpath.c_str()));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<>();
    });
}

directory_entry_type stat_to_entry_type(__mode_t type) {
    if (S_ISDIR(type)) {
        return directory_entry_type::directory;
    }
    if (S_ISBLK(type)) {
        return directory_entry_type::block_device;
    }
    if (S_ISCHR(type)) {
            return directory_entry_type::char_device;
    }
    if (S_ISFIFO(type)) {
        return directory_entry_type::fifo;
    }
    if (S_ISLNK(type)) {
        return directory_entry_type::link;
    }
    return directory_entry_type::regular;

}

future<std::optional<directory_entry_type>>
reactor::file_type(std::string name) {
    return _thread_pool.submit<syscall_result_extra<struct stat>>([name] {
        struct stat st;
        auto ret = stat(name.c_str(), &st);
        return wrap_syscall(ret, st);
    }).then([] (syscall_result_extra<struct stat> sr) {
        if (long(sr.result) == -1) {
            if (sr.error != ENOENT && sr.error != ENOTDIR) {
                sr.throw_if_error();
            }
            return make_ready_future<std::optional<directory_entry_type> >
                (std::optional<directory_entry_type>() );
        }
        return make_ready_future<std::optional<directory_entry_type> >
            (std::optional<directory_entry_type>(stat_to_entry_type(sr.extra.st_mode)) );
    });
}

future<uint64_t>
reactor::file_size(std::string pathname) {
    return _thread_pool.submit<syscall_result_extra<struct stat>>([pathname] {
        struct stat st;
        auto ret = stat(pathname.c_str(), &st);
        return wrap_syscall(ret, st);
    }).then([] (syscall_result_extra<struct stat> sr) {
        sr.throw_if_error();
        return make_ready_future<uint64_t>(sr.extra.st_size);
    });
}

future<bool> reactor::file_exists(std::string pathname) {
    return _thread_pool.submit<syscall_result_extra<struct stat>>([pathname] {
        struct stat st;
        auto ret = stat(pathname.c_str(), &st);
        return wrap_syscall(ret, st);
    }).then([] (syscall_result_extra<struct stat> sr) {
        if (sr.result < 0 && sr.error == ENOENT) {
            return make_ready_future<bool>(false);
        }
        sr.throw_if_error();
        return make_ready_future<bool>(true);
    });
}

future<fs_type>
reactor::file_system_at(std::string pathname) {
    return _thread_pool.submit<syscall_result_extra<struct statfs>>([pathname] {
        struct statfs st;
        auto ret = statfs(pathname.c_str(), &st);
        return wrap_syscall(ret, st);
    }).then([] (syscall_result_extra<struct statfs> sr) {
        static std::unordered_map<long int, fs_type> type_mapper = {
            { 0x58465342, fs_type::xfs },
            { EXT2_SUPER_MAGIC, fs_type::ext2 },
            { EXT3_SUPER_MAGIC, fs_type::ext3 },
            { EXT4_SUPER_MAGIC, fs_type::ext4 },
            { BTRFS_SUPER_MAGIC, fs_type::btrfs },
            { 0x4244, fs_type::hfs },
            { TMPFS_MAGIC, fs_type::tmpfs },
        };
        sr.throw_if_error();

        fs_type ret = fs_type::other;
        if (type_mapper.count(sr.extra.f_type) != 0) {
            ret = type_mapper.at(sr.extra.f_type);
        }
        return make_ready_future<fs_type>(ret);
    });
}

future<file>
reactor::open_directory(std::string name) {
    return _thread_pool.submit<syscall_result<int>>([name] {
        return wrap_syscall<int>(::open(name.c_str(), O_DIRECTORY | O_CLOEXEC | O_RDONLY));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<file>(file(sr.result, file_open_options()));
    });
}

future<>
reactor::make_directory(std::string name) {
    return _thread_pool.submit<syscall_result<int>>([name = std::move(name)] {
        return wrap_syscall<int>(::mkdir(name.c_str(), S_IRWXU));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
    });
}

future<>
reactor::touch_directory(std::string name) {
    return engine()._thread_pool.submit<syscall_result<int>>([name = std::move(name)] {
        return wrap_syscall<int>(::mkdir(name.c_str(), S_IRWXU));
    }).then([] (syscall_result<int> sr) {
        if (sr.error != EEXIST) {
            sr.throw_if_error();
        }
    });
}

namespace File {

file_handle::file_handle(const file_handle& x)
        : _impl(x._impl ? x._impl->clone() : std::unique_ptr<file_handle_impl>()) {
}

file_handle::file_handle(file_handle&& x) noexcept = default;

file_handle&
file_handle::operator=(const file_handle& x) {
    return operator=(file_handle(x));
}

file_handle&
file_handle::operator=(file_handle&&) noexcept = default;

file
file_handle::to_file() const & {
    return file_handle(*this).to_file();
}

file
file_handle::to_file() && {
    return file(std::move(*_impl).to_file());
}

}

file::file(File::file_handle&& handle)
        : _file_impl(std::move(std::move(handle).to_file()._file_impl)) {
}

File::file_handle
file::dup() {
    return File::file_handle(_file_impl->dup());
}

std::unique_ptr<File::file_handle_impl>
file_impl::dup() {
    throw std::runtime_error("this file type cannot be duplicated");
}

std::unique_ptr<File::file_handle_impl>
posix_file_impl::dup() {
    if (!_refcount) {
        _refcount = new std::atomic<unsigned>(1u);
    }
    auto ret = std::make_unique<posix_file_handle_impl>(_fd, _refcount);
    _refcount->fetch_add(1, std::memory_order_relaxed);
    return std::move(ret);
}

posix_file_impl::posix_file_impl(int fd, std::atomic<unsigned>* refcount)
        : _refcount(refcount), _fd(fd) {
}

posix_file_handle_impl::~posix_file_handle_impl() {
    if (_refcount && _refcount->fetch_add(-1, std::memory_order_relaxed) == 1) {
        ::close(_fd);
        delete _refcount;
    }
}

std::unique_ptr<File::file_handle_impl>
posix_file_handle_impl::clone() const {
    auto ret = std::make_unique<posix_file_handle_impl>(_fd, _refcount);
    if (_refcount) {
        _refcount->fetch_add(1, std::memory_order_relaxed);
    }
    return std::move(ret);
}

std::shared_ptr<file_impl>
posix_file_handle_impl::to_file() && {
    auto ret = std::make_shared<posix_file_impl>(_fd, _refcount);
    _fd = -1;
    _refcount = nullptr;
    return ret;
}

future<>
posix_file_impl::flush(void) {
    ++engine()._fsyncs;
    return engine()._thread_pool.submit<syscall_result<int>>([this] {
        return wrap_syscall<int>(::fdatasync(_fd));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<>();
    });
}

future<struct stat>
posix_file_impl::stat(void) {
    return engine()._thread_pool.submit<syscall_result_extra<struct stat>>([this] {
        struct stat st;
        auto ret = ::fstat(_fd, &st);
        return wrap_syscall(ret, st);
    }).then([] (syscall_result_extra<struct stat> ret) {
        ret.throw_if_error();
        return make_ready_future<struct stat>(ret.extra);
    });
}

future<>
posix_file_impl::truncate(uint64_t length) {
    return engine()._thread_pool.submit<syscall_result<int>>([this, length] {
        return wrap_syscall<int>(::ftruncate(_fd, length));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<>();
    });
}

blockdev_file_impl::blockdev_file_impl(int fd, file_open_options options)
        : posix_file_impl(fd, options) {
}

future<>
blockdev_file_impl::truncate(uint64_t length) {
    return make_ready_future<>();
}

future<>
posix_file_impl::discard(uint64_t offset, uint64_t length) {
    return engine()._thread_pool.submit<syscall_result<int>>([this, offset, length] () mutable {
        return wrap_syscall<int>(::fallocate(_fd, FALLOC_FL_PUNCH_HOLE|FALLOC_FL_KEEP_SIZE,
            offset, length));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<>();
    });
}

future<>
posix_file_impl::allocate(uint64_t position, uint64_t length) {
    return make_ready_future<>();
}

future<>
blockdev_file_impl::discard(uint64_t offset, uint64_t length) {
    return engine()._thread_pool.submit<syscall_result<int>>([this, offset, length] () mutable {
        uint64_t range[2] { offset, length };
        return wrap_syscall<int>(::ioctl(_fd, BLKDISCARD, &range));
    }).then([] (syscall_result<int> sr) {
        sr.throw_if_error();
        return make_ready_future<>();
    });
}

future<>
blockdev_file_impl::allocate(uint64_t position, uint64_t length) {
    // nothing to do for block device
    return make_ready_future<>();
}

future<uint64_t>
posix_file_impl::size() {
    auto r = ::lseek(_fd, 0, SEEK_END);
    if (r == -1) {
        return make_exception_future<uint64_t>(std::system_error(errno, std::system_category()));
    }
    return make_ready_future<uint64_t>(r);
}

future<>
posix_file_impl::close() noexcept {
    if (_fd == -1) {
        throw("double close detected,contact support");
        return make_ready_future<>();
    }
    auto fd = _fd;
    _fd = -1;  // Prevent a concurrent close (which is illegal) from closing another file's fd
    if (_refcount && _refcount->fetch_add(-1, std::memory_order_relaxed) != 1) {
        _refcount = nullptr;
        return make_ready_future<>();
    }
    delete _refcount;
    _refcount = nullptr;
    auto closed = [fd] () noexcept {
        try {
            return engine()._thread_pool.submit<syscall_result<int>>([fd] {
                return wrap_syscall<int>(::close(fd));
            });
        } catch (...) {
            throw("Running::close() in reactor thread, submission failed with exception");
            return make_ready_future<syscall_result<int>>(wrap_syscall<int>(::close(fd)));
        }
    }();
    return closed.then([] (syscall_result<int> sr) {
        sr.throw_if_error();
    });
}

future<uint64_t>
blockdev_file_impl::size(void) {
    return engine()._thread_pool.submit<syscall_result_extra<size_t>>([this] {
        uint64_t size;
        int ret = ::ioctl(_fd, BLKGETSIZE64, &size);
        return wrap_syscall(ret, size);
    }).then([] (syscall_result_extra<uint64_t> ret) {
        ret.throw_if_error();
        return make_ready_future<uint64_t>(ret.extra);
    });
}

subscription<directory_entry>
posix_file_impl::list_directory(std::function<future<> (directory_entry de)> next) {
    struct work {
        stream<directory_entry> s;
        unsigned current = 0;
        unsigned total = 0;
        bool eof = false;
        int error = 0;
        char buffer[8192];
    };

    // While it would be natural to use fdopendir()/readdir(),
    // our syscall thread pool doesn't support malloc(), which is
    // required for this to work.  So resort to using getdents()
    // instead.

    // From getdents(2):
    struct linux_dirent {
        unsigned long  d_ino;     /* Inode number */
        unsigned long  d_off;     /* Offset to next linux_dirent */
        unsigned short d_reclen;  /* Length of this linux_dirent */
        char           d_name[];  /* Filename (null-terminated) */
        /* length is actually (d_reclen - 2 -
                             offsetof(struct linux_dirent, d_name)) */
        /*
        char           pad;       // Zero padding byte
        char           d_type;    // File type (only since Linux
                                  // 2.6.4); offset is (d_reclen - 1)
         */
    };

    auto w = std::make_shared<work>();
    auto ret = w->s.listen(std::move(next));
    w->s.started().then([w, this] {
        auto eofcond = [w] { return w->eof; };
        return do_until(eofcond, [w, this] {
            if (w->current == w->total) {
                return engine()._thread_pool.submit<syscall_result<long>>([w , this] () {
                    auto ret = ::syscall(__NR_getdents, _fd, reinterpret_cast<linux_dirent*>(w->buffer), sizeof(w->buffer));
                    return wrap_syscall(ret);
                }).then([w] (syscall_result<long> ret) {
                    ret.throw_if_error();
                    if (ret.result == 0) {
                        w->eof = true;
                    } else {
                        w->current = 0;
                        w->total = ret.result;
                    }
                });
            }
            auto start = w->buffer + w->current;
            auto de = reinterpret_cast<linux_dirent*>(start);
            std::optional<directory_entry_type> type;
            switch (start[de->d_reclen - 1]) {
            case DT_BLK:
                type = directory_entry_type::block_device;
                break;
            case DT_CHR:
                type = directory_entry_type::char_device;
                break;
            case DT_DIR:
                type = directory_entry_type::directory;
                break;
            case DT_FIFO:
                type = directory_entry_type::fifo;
                break;
            case DT_REG:
                type = directory_entry_type::regular;
                break;
            case DT_SOCK:
                type = directory_entry_type::socket;
                break;
            default:
                // unknown, ignore
                ;
            }
            w->current += de->d_reclen;
            std::string name = de->d_name;
            if (name == "." || name == "..") {
                return make_ready_future<>();
            }
            return w->s.produce({std::move(name), type});
        });
    }).then([w] {
        w->s.close();
    });
    return ret;
}


template <typename Func>
future<io_event>
reactor::submit_io_read(const io_priority_class& pc, size_t len, Func prepare_io) {
    ++_io_stats.aio_reads;
    _io_stats.aio_read_bytes += len;
    return io_queue::queue_request(_io_coordinator, pc, len, std::move(prepare_io));
}

template <typename Func>
future<io_event>
reactor::submit_io_write(const io_priority_class& pc, size_t len, Func prepare_io) {
    ++_io_stats.aio_writes;
    _io_stats.aio_write_bytes += len;
    return io_queue::queue_request(_io_coordinator, pc, len, std::move(prepare_io));
}

const io_priority_class& default_priority_class() {
    static thread_local auto shard_default_class = [] {
        return engine().register_one_priority_class("default", 1);
    }();
    return shard_default_class;
}


future<file> open_file_dma(std::string name, open_flags flags) {
    return engine().open_file_dma(std::move(name), flags, file_open_options());
}

future<> remove_file(std::string pathname) {
    return engine().remove_file(std::move(pathname));
}


future<> check_direct_io_support(std::string path) {
    struct w {
        std::string path;
        open_flags flags;
        std::function<future<>()> cleanup;
        static w parse(std::string path, std::optional<directory_entry_type> type) {
            if (!type) {
                throw std::invalid_argument("Could not open file. Make sure it exists");
            }
            if (type == directory_entry_type::directory) {
                auto fpath = path + "/.o_direct_test";
                return w{fpath, open_flags::wo | open_flags::create | open_flags::truncate, [fpath] { return remove_file(fpath); }};
            } else if ((type == directory_entry_type::regular) || (type == directory_entry_type::link)) {
                return w{path, open_flags::ro, [] { return make_ready_future<>(); }};
            } else {
                throw std::invalid_argument("neither a directory nor file. Can't be opened with O_DIRECT");
            }
        };
    };
    return engine().file_type(path).then([path] (auto type) {
        auto w = w::parse(path, type);
        return open_file_dma(w.path, w.flags).then_wrapped([path = w.path, cleanup = std::move(w.cleanup)] (future<file> f) {
            try {
                f.get0();
                return cleanup();
            } catch (std::system_error& e) {
                if (e.code() == std::error_code(EINVAL, std::system_category())) {
                    throw("Could not open file. Does your filesystem support O_DIRECT?");
                }
                throw;
            }
        });
    });
}



future<file> open_file_dma(std::string name, open_flags flags, file_open_options options) {
    return engine().open_file_dma(std::move(name), flags, options);
}

future<file> open_directory(std::string name) {
    return engine().open_directory(std::move(name));
}

future<> make_directory(std::string name) {
    return engine().make_directory(std::move(name));
}

future<> touch_directory(std::string name) {
    return engine().touch_directory(std::move(name));
}

future<> sync_directory(std::string name) {
    return open_directory(std::move(name)).then([] (file f) {
        return do_with(std::move(f), [] (file& f) {
            return f.flush().then([&f] () mutable {
                return f.close();
            });
        });
    });
}

future<> do_recursive_touch_directory(std::string base, std::string name) {
    static const std::string::value_type separator = '/';

    if (name.empty()) {
        return make_ready_future<>();
    }

    size_t pos = std::min(name.find(separator), name.size() - 1);
    base += name.substr(0 , pos + 1);
    name = name.substr(pos + 1);
    return touch_directory(base).then([base, name] {
        return do_recursive_touch_directory(base, name);
    }).then([base] {
        // We will now flush the directory that holds the entry we potentially
        // created. Technically speaking, we only need to touch when we did
        // create. But flushing the unchanged ones should be cheap enough - and
        // it simplifies the code considerably.
        if (base.empty()) {
            return make_ready_future<>();
        }

        return sync_directory(base);
    });
}


future<> recursive_touch_directory(std::string name) {
    // If the name is empty,  it will be of the type a/b/c, which should be interpreted as
    // a relative path. This means we have to flush our current directory
    sstring base = "";
    if (name[0] != '/' || name[0] == '.') {
        base = "./";
    }
    return do_recursive_touch_directory(base, name);
}


future<> rename_file(std::string old_pathname, std::string new_pathname) {
    return engine().rename_file(std::move(old_pathname), std::move(new_pathname));
}

future<fs_type> file_system_at(std::string name) {
    return engine().file_system_at(name);
}

future<uint64_t> file_size(std::string name) {
    return engine().file_size(name);
}

future<bool> file_exists(std::string name) {
    return engine().file_exists(name);
}

future<> link_file(std::string oldpath, std::string newpath) {
    return engine().link_file(std::move(oldpath), std::move(newpath));
}

server_socket listen(net::socket_address sa) {
    return engine().listen(sa);
}

server_socket listen(net::socket_address sa, listen_options opts) {
    return engine().listen(sa, opts);
}

future<connected_socket> connect(net::socket_address sa) {
    return engine().connect(sa);
}

future<connected_socket> connect(net::socket_address sa, net::socket_address local, transport proto = transport::TCP) {
    return engine().connect(sa, local, proto);
}

input_stream<char> make_file_input_stream(
        file f, uint64_t offset, uint64_t len, file_input_stream_options options) {
    return input_stream<char>(file_data_source(std::move(f), offset, len, std::move(options)));
}

input_stream<char> make_file_input_stream(
        file f, uint64_t offset, file_input_stream_options options) {
    return make_file_input_stream(std::move(f), offset, std::numeric_limits<uint64_t>::max(), std::move(options));
}

input_stream<char> make_file_input_stream(
        file f, file_input_stream_options options) {
    return make_file_input_stream(std::move(f), 0, std::move(options));
}


// 辅助函数：手动分割字符串
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// 辅助函数：将 IP 地址字符串转换为 uint32_t
uint32_t string_to_ip(const std::string& addr) {
    struct in_addr in;
    if (inet_pton(AF_INET, addr.c_str(), &in) != 1) {
        throw std::invalid_argument("Invalid IP address: " + addr);
    }
    return net::ntoh(in.s_addr); // 转换为主机字节序
}

ipv4_addr::ipv4_addr(const std::string &addr) {
    try {
        std::vector<std::string> items = split(addr, ':');
        if (items.size() == 1) {
            ip = string_to_ip(items[0]);
            port = 0;
        } else if (items.size() == 2) {
            ip = string_to_ip(items[0]);
            port = std::stoul(items[1]);
        } else {
            throw std::invalid_argument("invalid format: " + addr);
        }
    } catch (const std::exception& e) {
        throw std::invalid_argument("Invalid IP address: " + addr + ", error: " + e.what());
    }
}

ipv4_addr::ipv4_addr(const std::string &addr, uint16_t port_) {
    try {
        ip = string_to_ip(addr);
        port = port_;
    } catch (const std::exception& e) {
        throw std::invalid_argument("Invalid IP address: " + addr + ", error: " + e.what());
    }
}

namespace memory{
bool drain_cross_cpu_freelist() {
    return cpu_mem.drain_cross_cpu_freelist();
}

translation translate(const void* addr, size_t size) {
    auto cpu_id = object_cpu_id(addr);
    if (cpu_id >= max_cpus) {
        return {};
    }
    auto cp = cpu_pages::all_cpus[cpu_id];
    if (!cp) {
        return {};
    }
    return cp->translate(addr, size);
}

memory_layout get_memory_layout() {
    return cpu_mem.memory_layout();
}

   size_t min_free_memory() {
    return cpu_mem.min_free_pages * page_size;
}

}






// ipv4_addr::ipv4_addr(const net::inet_address& a, uint16_t port)
//     : ipv4_addr([&a] {
//         ::in_addr in = a;
//         return net::ntoh(in.s_addr);
//     }(), port){}