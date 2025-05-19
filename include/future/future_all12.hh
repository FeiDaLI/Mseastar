#pragma once
// #include <xfs/linux.h>
// #define min min    /* prevent xfs.h from defining min() as a macro */
// #include <xfs/xfs.h>
// #undef min
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/fs.h> // 或者适当的头文件来定义BLKGETSIZE
#include <sys/statfs.h> // 为fstatfs提供声明
#include "../task/task.hh"
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
#include <boost/intrusive/list.hpp>
#include <setjmp.h>
#include <ucontext.h>
#include <boost/range/adaptors.hpp>
#include <boost/range/numeric.hpp> // boost::accumulate
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
// #include "../util/backtrace.hh"
#include "../util/bool_class.hh"

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


inline
size_t iovec_len(const iovec* begin, size_t len)
{
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













#define GCC6_CONCEPT(x...)
#define GCC6_NO_CONCEPT(x...) x

__thread bool g_need_preempt;
inline bool need_preempt() {
    return true;
    // prevent compiler from eliminating loads in a loop
    std::atomic_signal_fence(std::memory_order_seq_cst);
    return g_need_preempt;
}


void
systemwide_memory_barrier() {
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




using shard_id = unsigned;
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

/*----------------------------------------------------------*/
struct mmap_deleter {
    size_t _size;
    void operator()(void* ptr) const;
};
/*
    mmap_area起始地址是char[],长度在mmap_deleter中保存。
    思考：为什么unique_ptr第一个参数是char[]，而不是char *，或者void *，或者char。
*/
using mmap_area = std::unique_ptr<char[], mmap_deleter>;
/*--------------------------------------------------------------*/

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

#include <atomic>
#include <boost/mpl/range_c.hpp>
#include <boost/mpl/for_each.hpp>
#include "../util/align.hh"
#include "../util/spinlock.hh"

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








#include <bitset>
#include <limits>
#include <variant>

namespace bitsets {
static constexpr int ulong_bits = std::numeric_limits<unsigned long>::digits;
template<typename T>
inline size_t count_leading_zeros(T value);
template<typename T>
static inline size_t count_trailing_zeros(T value);
template<>
inline size_t count_leading_zeros<unsigned long>(unsigned long value)
{
    return __builtin_clzl(value);
}
template<>
inline size_t count_leading_zeros<long>(long value)
{
    return __builtin_clzl((unsigned long)value) - 1;
}
template<>
inline size_t count_leading_zeros<long long>(long long value)
{
    return __builtin_clzll((unsigned long long)value) - 1;
}
template<>
inline
size_t count_trailing_zeros<unsigned long>(unsigned long value)
{
    return __builtin_ctzl(value);
}

template<>
inline
size_t count_trailing_zeros<long>(long value)
{
    return __builtin_ctzl((unsigned long)value);
}
template<size_t N>
static inline size_t get_first_set(const std::bitset<N>& bitset)
{
    static_assert(N <= ulong_bits, "bitset too large");
    return count_trailing_zeros(bitset.to_ulong());
}

template<size_t N>
static inline size_t get_last_set(const std::bitset<N>& bitset)
{
    static_assert(N <= ulong_bits, "bitset too large");
    return ulong_bits - 1 - count_leading_zeros(bitset.to_ulong());
}

template<size_t N>
class set_iterator : public std::iterator<std::input_iterator_tag, int>
{
private:
    void advance()
    {
        if (_bitset.none()) {
            _index = -1;
        } else {
            auto shift = get_first_set(_bitset) + 1;
            _index += shift;
            _bitset >>= shift;
        }
    }
public:
    set_iterator(std::bitset<N> bitset, int offset = 0)
        : _bitset(bitset)
        , _index(offset - 1)
    {
        static_assert(N <= ulong_bits, "This implementation is inefficient for large bitsets");
        _bitset >>= offset;
        advance();
    }

    void operator++()
    {
        advance();
    }

    int operator*() const
    {
        return _index;
    }

    bool operator==(const set_iterator& other) const
    {
        return _index == other._index;
    }

    bool operator!=(const set_iterator& other) const
    {
        return !(*this == other);
    }
private:
    std::bitset<N> _bitset;
    int _index;
};

template<size_t N>
class set_range
{
public:
    using iterator = set_iterator<N>;
    using value_type = int;

    set_range(std::bitset<N> bitset, int offset = 0)
        : _bitset(bitset)
        , _offset(offset)
    {
    }

    iterator begin() const { return iterator(_bitset, _offset); }
    iterator end() const { return iterator(0); }
private:
    std::bitset<N> _bitset;
    int _offset;
};

template<size_t N>
static inline set_range<N> for_each_set(std::bitset<N> bitset, int offset = 0)
{
    return set_range<N>(bitset, offset);
}

}




inline
sigset_t make_empty_sigset_mask() {
    sigset_t set;
    sigemptyset(&set);
    return set;
}

inline int alarm_signal() {
    // We don't want to use SIGALRM, because the boost unit test library
    // also plays with it.
    return SIGALRM;
    return SIGRTMIN;
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



struct signals {
        signals();
        ~signals();
        bool poll_signal();
        bool pure_poll_signal() const;
        void handle_signal(int signo, std::function<void ()>&& handler);
        void handle_signal_once(int signo, std::function<void ()>&& handler);
        static void action(int signo, siginfo_t* siginfo, void* ignore);
        struct signal_handler {
            signal_handler(int signo, std::function<void ()>&& handler);
            std::function<void ()> _handler;
        };
        std::atomic<uint64_t> _pending_signals;
        std::unordered_map<int, signal_handler> _signal_handlers;

};




/*---------------------------basic_string------------------------------------------*/


class deleter final {
public:
    /// \cond internal
    struct impl;
    struct raw_object_tag {};
    /// \endcond
private:
    // if bit 0 set, point to object to be freed directly.
    impl* _impl = nullptr;
public:
    /// Constructs an empty deleter that does nothing in its destructor.
    deleter() = default;
    deleter(const deleter&) = delete;
    /// Moves a deleter.
    deleter(deleter&& x) noexcept : _impl(x._impl) { x._impl = nullptr; }
    /// \cond internal
    explicit deleter(impl* i) : _impl(i) {}
    deleter(raw_object_tag tag, void* object)
        : _impl(from_raw_object(object)) {}
    /// \endcond
    /// Destroys the deleter and carries out the encapsulated action.
    ~deleter();
    deleter& operator=(deleter&& x);
    deleter& operator=(deleter&) = delete;
    /// Performs a sharing operation.  The encapsulated action will only
    /// be carried out after both the original deleter and the returned
    /// deleter are both destroyed.
    ///
    /// \return a deleter with the same encapsulated action as this one.
    deleter share();
    /// Checks whether the deleter has an associated action.
    explicit operator bool() const { return bool(_impl); }
    /// \cond internal
    void reset(impl* i) {
        this->~deleter();
        new (this) deleter(i);
    }
    /// \endcond
    /// Appends another deleter to this deleter.  When this deleter is
    /// destroyed, both encapsulated actions will be carried out.
    void append(deleter d);
private:
    static bool is_raw_object(impl* i) {
        auto x = reinterpret_cast<uintptr_t>(i);
        return x & 1;
    }
    bool is_raw_object() const {
        return is_raw_object(_impl);
    }
    static void* to_raw_object(impl* i) {
        auto x = reinterpret_cast<uintptr_t>(i);
        return reinterpret_cast<void*>(x & ~uintptr_t(1));
    }
    void* to_raw_object() const {
        return to_raw_object(_impl);
    }
    impl* from_raw_object(void* object) {
        auto x = reinterpret_cast<uintptr_t>(object);
        return reinterpret_cast<impl*>(x | 1);
    }
};

/// \cond internal
struct deleter::impl {
    unsigned refs = 1;
    deleter next;
    impl(deleter next) : next(std::move(next)) {}
    virtual ~impl() {}
};
/// \endcond

inline
deleter::~deleter() {
    if (is_raw_object()) {
        std::free(to_raw_object());
        return;
    }
    if (_impl && --_impl->refs == 0) {
        delete _impl;
    }
}

inline
deleter& deleter::operator=(deleter&& x) {
    if (this != &x) {
        this->~deleter();
        new (this) deleter(std::move(x));
    }
    return *this;
}

/// \cond internal
template <typename Deleter>
struct lambda_deleter_impl final : deleter::impl {
    Deleter del;
    lambda_deleter_impl(deleter next, Deleter&& del)
        : impl(std::move(next)), del(std::move(del)) {}
    virtual ~lambda_deleter_impl() override { del(); }
};

template <typename Object>
struct object_deleter_impl final : deleter::impl {
    Object obj;
    object_deleter_impl(deleter next, Object&& obj)
        : impl(std::move(next)), obj(std::move(obj)) {}
};

template <typename Object>
inline
object_deleter_impl<Object>* make_object_deleter_impl(deleter next, Object obj) {
    return new object_deleter_impl<Object>(std::move(next), std::move(obj));
}
/// \endcond

/// Makes a \ref deleter that encapsulates the action of
/// destroying an object, as well as running another deleter.  The input
/// object is moved to the deleter, and destroyed when the deleter is destroyed.
///
/// \param d deleter that will become part of the new deleter's encapsulated action
/// \param o object whose destructor becomes part of the new deleter's encapsulated action
/// \related deleter
template <typename Object>
deleter
make_deleter(deleter next, Object o) {
    return deleter(new lambda_deleter_impl<Object>(std::move(next), std::move(o)));
}

/// Makes a \ref deleter that encapsulates the action of destroying an object.  The input
/// object is moved to the deleter, and destroyed when the deleter is destroyed.
///
/// \param o object whose destructor becomes the new deleter's encapsulated action
/// \related deleter
template <typename Object>
deleter
make_deleter(Object o) {
    return make_deleter(deleter(), std::move(o));
}

/// \cond internal
struct free_deleter_impl final : deleter::impl {
    void* obj;
    free_deleter_impl(void* obj) : impl(deleter()), obj(obj) {}
    virtual ~free_deleter_impl() override { std::free(obj); }
};
/// \endcond

inline
deleter
deleter::share() {
    if (!_impl) {
        return deleter();
    }
    if (is_raw_object()) {
        _impl = new free_deleter_impl(to_raw_object());
    }
    ++_impl->refs;
    return deleter(_impl);
}

// Appends 'd' to the chain of deleters. Avoids allocation if possible. For
// performance reasons the current chain should be shorter and 'd' should be
// longer.
inline
void deleter::append(deleter d) {
    if (!d._impl) {
        return;
    }
    impl* next_impl = _impl;
    deleter* next_d = this;
    while (next_impl) {
        assert(next_impl != d._impl);
        if (is_raw_object(next_impl)) {
            next_d->_impl = next_impl = new free_deleter_impl(to_raw_object(next_impl));
        }
        if (next_impl->refs != 1) {
            next_d->_impl = next_impl = make_object_deleter_impl(std::move(next_impl->next), deleter(next_impl));
        }
        next_d = &next_impl->next;
        next_impl = next_d->_impl;
    }
    next_d->_impl = d._impl;
    d._impl = nullptr;
}

/// Makes a deleter that calls \c std::free() when it is destroyed.
///
/// \param obj object to free.
/// \related deleter
inline
deleter
make_free_deleter(void* obj) {
    if (!obj) {
        return deleter();
    }
    return deleter(deleter::raw_object_tag(), obj);
}

/// Makes a deleter that calls \c std::free() when it is destroyed, as well
/// as invoking the encapsulated action of another deleter.
///
/// \param d deleter to invoke.
/// \param obj object to free.
/// \related deleter
inline
deleter
make_free_deleter(deleter next, void* obj) {
    return make_deleter(std::move(next), [obj] () mutable { std::free(obj); });
}

/// \see make_deleter(Object)
/// \related deleter
template <typename T>
inline
deleter
make_object_deleter(T&& obj) {
    return deleter{make_object_deleter_impl(deleter(), std::move(obj))};
}

/// \see make_deleter(deleter, Object)
/// \related deleter
template <typename T>
inline
deleter
make_object_deleter(deleter d, T&& obj) {
    return deleter{make_object_deleter_impl(std::move(d), std::move(obj))};
}





template <typename CharType>
class temporary_buffer {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    CharType* _buffer;
    size_t _size;
    deleter _deleter;
public:
    explicit temporary_buffer(size_t size)
        : _buffer(static_cast<CharType*>(malloc(size * sizeof(CharType)))), _size(size)
        , _deleter(make_free_deleter(_buffer)) {
        if (size && !_buffer) {
            throw std::bad_alloc();
        }
    }
    //explicit temporary_buffer(CharType* borrow, size_t size) : _buffer(borrow), _size(size) {}
    /// Creates an empty \c temporary_buffer that does not point at anything.
    temporary_buffer()
        : _buffer(nullptr)
        , _size(0) {}
    temporary_buffer(const temporary_buffer&) = delete;
    /// Moves a \c temporary_buffer.
    temporary_buffer(temporary_buffer&& x) noexcept : _buffer(x._buffer), _size(x._size), _deleter(std::move(x._deleter)) {
        x._buffer = nullptr;
        x._size = 0;
    }
    temporary_buffer(CharType* buf, size_t size, deleter d)
        : _buffer(buf), _size(size), _deleter(std::move(d)) {}
    temporary_buffer(const CharType* src, size_t size) : temporary_buffer(size) {
        std::copy_n(src, size, _buffer);
    }
    void operator=(const temporary_buffer&) = delete;
    /// Moves a \c temporary_buffer.
    temporary_buffer& operator=(temporary_buffer&& x) {
        if (this != &x) {
            _buffer = x._buffer;
            _size = x._size;
            _deleter = std::move(x._deleter);
            x._buffer = nullptr;
            x._size = 0;
        }
        return *this;
    }
    /// Gets a pointer to the beginning of the buffer.
    const CharType* get() const { return _buffer; }
    /// Gets a writable pointer to the beginning of the buffer.  Use only
    /// when you are certain no user expects the buffer data not to change.
    CharType* get_write() { return _buffer; }
    /// Gets the buffer size.
    size_t size() const { return _size; }
    /// Gets a pointer to the beginning of the buffer.
    const CharType* begin() const { return _buffer; }
    /// Gets a pointer to the end of the buffer.
    const CharType* end() const { return _buffer + _size; }
    temporary_buffer prefix(size_t size) && {
        auto ret = std::move(*this);
        ret._size = size;
        return ret;
    }
    CharType operator[](size_t pos) const {
        return _buffer[pos];
    }
    bool empty() const { return !size(); }
    explicit operator bool() const { return size(); }
    temporary_buffer share() {
        return temporary_buffer(_buffer, _size, _deleter.share());
    }
    temporary_buffer share(size_t pos, size_t len) {
        auto ret = share();
        ret._buffer += pos;
        ret._size = len;
        return ret;
    }
    void trim_front(size_t pos) {
        _buffer += pos;
        _size -= pos;
    }
    void trim(size_t pos) {
        _size = pos;
    }
    deleter release() {
        return std::move(_deleter);
    }
    static temporary_buffer aligned(size_t alignment, size_t size) {
        void *ptr = nullptr;
        auto ret = ::posix_memalign(&ptr, alignment, size * sizeof(CharType));
        auto buf = static_cast<CharType*>(ptr);
        if (ret) {
            throw std::bad_alloc();
        }
        return temporary_buffer(buf, size, make_free_deleter(buf));
    }

    /// Compare contents of this buffer with another buffer for equality
    ///
    /// \param o buffer to compare with
    /// \return true if and only if contents are the same
    bool operator==(const temporary_buffer<char>& o) const {
        return size() == o.size() && std::equal(begin(), end(), o.begin());
    }

    /// Compare contents of this buffer with another buffer for inequality
    ///
    /// \param o buffer to compare with
    /// \return true if and only if contents are not the same
    bool operator!=(const temporary_buffer<char>& o) const {
        return !(*this == o);
    }
};

template <typename char_type, typename Size, Size max_size>
class basic_sstring;

using sstring = basic_sstring<char, uint32_t, 15>;

template <typename string_type = sstring, typename T>
inline string_type to_sstring(T value);

template <typename char_type, typename Size, Size max_size>
class basic_sstring {
    static_assert(
            (std::is_same<char_type, char>::value
             || std::is_same<char_type, signed char>::value
             || std::is_same<char_type, unsigned char>::value),
            "basic_sstring only supports single byte char types");
    union contents {
        struct external_type {
            char_type* str;
            Size size;
            int8_t pad;
        } external;
        struct internal_type {
            char_type str[max_size];
            int8_t size;
        } internal;
        static_assert(sizeof(external_type) <= sizeof(internal_type), "max_size too small");
        static_assert(max_size <= 127, "max_size too large");
    } u;
    bool is_internal() const noexcept {
        return u.internal.size >= 0;
    }
    bool is_external() const noexcept {
        return !is_internal();
    }
    const char_type* str() const {
        return is_internal() ? u.internal.str : u.external.str;
    }
    char_type* str() {
        return is_internal() ? u.internal.str : u.external.str;
    }

    template <typename string_type, typename T>
    static inline string_type to_sstring_sprintf(T value, const char* fmt) {
        char tmp[sizeof(value) * 3 + 2];
        auto len = std::sprintf(tmp, fmt, value);
        using ch_type = typename string_type::value_type;
        return string_type(reinterpret_cast<ch_type*>(tmp), len);
    }

    template <typename string_type>
    static inline string_type to_sstring(int value) {
        return to_sstring_sprintf<string_type>(value, "%d");
    }

    template <typename string_type>
    static inline string_type to_sstring(unsigned value) {
        return to_sstring_sprintf<string_type>(value, "%u");
    }

    template <typename string_type>
    static inline string_type to_sstring(long value) {
        return to_sstring_sprintf<string_type>(value, "%ld");
    }

    template <typename string_type>
    static inline string_type to_sstring(unsigned long value) {
        return to_sstring_sprintf<string_type>(value, "%lu");
    }

    template <typename string_type>
    static inline string_type to_sstring(long long value) {
        return to_sstring_sprintf<string_type>(value, "%lld");
    }

    template <typename string_type>
    static inline string_type to_sstring(unsigned long long value) {
        return to_sstring_sprintf<string_type>(value, "%llu");
    }

    template <typename string_type>
    static inline string_type to_sstring(float value) {
        return to_sstring_sprintf<string_type>(value, "%g");
    }

    template <typename string_type>
    static inline string_type to_sstring(double value) {
        return to_sstring_sprintf<string_type>(value, "%g");
    }

    template <typename string_type>
    static inline string_type to_sstring(long double value) {
        return to_sstring_sprintf<string_type>(value, "%Lg");
    }

    template <typename string_type>
    static inline string_type to_sstring(const char* value) {
        return string_type(value);
    }

    template <typename string_type>
    static inline string_type to_sstring(sstring value) {
        return value;
    }

    template <typename string_type>
    static inline string_type to_sstring(const temporary_buffer<char>& buf) {
        return string_type(buf.get(), buf.size());
    }
public:
    using value_type = char_type;
    using traits_type = std::char_traits<char_type>;
    using allocator_type = std::allocator<char_type>;
    using reference = char_type&;
    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;
    using iterator = char_type*;
    using const_iterator = const char_type*;
    // FIXME: add reverse_iterator and friend
    using difference_type = ssize_t;  // std::make_signed_t<Size> can be too small
    using size_type = Size;
    static constexpr size_type  npos = static_cast<size_type>(-1);
public:
    struct initialized_later {};

    basic_sstring() noexcept {
        u.internal.size = 0;
        u.internal.str[0] = '\0';
    }
    basic_sstring(const basic_sstring& x) {
        if (x.is_internal()) {
            u.internal = x.u.internal;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(x.u.external.size + 1));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            std::copy(x.u.external.str, x.u.external.str + x.u.external.size + 1, u.external.str);
            u.external.size = x.u.external.size;
        }
    }
    basic_sstring(basic_sstring&& x) noexcept {
        u = x.u;
        x.u.internal.size = 0;
        x.u.internal.str[0] = '\0';
    }
    basic_sstring(initialized_later, size_t size) {
        if (size_type(size) != size) {
            throw std::overflow_error("sstring overflow");
        }
        if (size + 1 <= sizeof(u.internal.str)) {
            u.internal.str[size] = '\0';
            u.internal.size = size;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(size + 1));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            u.external.size = size;
            u.external.str[size] = '\0';
        }
    }
    basic_sstring(const char_type* x, size_t size) {
        if (size_type(size) != size) {
            throw std::overflow_error("sstring overflow");
        }
        if (size + 1 <= sizeof(u.internal.str)) {
            std::copy(x, x + size, u.internal.str);
            u.internal.str[size] = '\0';
            u.internal.size = size;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(size + 1));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            u.external.size = size;
            std::copy(x, x + size, u.external.str);
            u.external.str[size] = '\0';
        }
    }
    basic_sstring(size_t size, char_type x) : basic_sstring(initialized_later(), size) {
        memset(begin(), x, size);
    }
    basic_sstring(const char* x) : basic_sstring(reinterpret_cast<const char_type*>(x), std::strlen(x)) {}
    basic_sstring(std::basic_string<char_type>& x) : basic_sstring(x.c_str(), x.size()) {}
    basic_sstring(std::initializer_list<char_type> x) : basic_sstring(x.begin(), x.end() - x.begin()) {}
    basic_sstring(const char_type* b, const char_type* e) : basic_sstring(b, e - b) {}
    basic_sstring(const std::basic_string<char_type>& s)
        : basic_sstring(s.data(), s.size()) {}
    template <typename InputIterator>
    basic_sstring(InputIterator first, InputIterator last)
            : basic_sstring(initialized_later(), std::distance(first, last)) {
        std::copy(first, last, begin());
    }
    ~basic_sstring() noexcept {
        if (is_external()) {
            std::free(u.external.str);
        }
    }
    basic_sstring& operator=(const basic_sstring& x) {
        basic_sstring tmp(x);
        swap(tmp);
        return *this;
    }
    basic_sstring& operator=(basic_sstring&& x) noexcept {
        if (this != &x) {
            swap(x);
            x.reset();
        }
        return *this;
    }
    operator std::basic_string<char_type>() const {
        return { str(), size() };
    }
    size_t size() const noexcept {
        return is_internal() ? u.internal.size : u.external.size;
    }

    size_t length() const noexcept {
        return size();
    }

    size_t find(char_type t, size_t pos = 0) const noexcept {
        const char_type* it = str() + pos;
        const char_type* end = str() + size();
        while (it < end) {
            if (*it == t) {
                return it - str();
            }
            it++;
        }
        return npos;
    }

    size_t find(const basic_sstring& s, size_t pos = 0) const noexcept {
        const char_type* it = str() + pos;
        const char_type* end = str() + size();
        const char_type* c_str = s.str();
        const char_type* c_str_end = s.str() + s.size();

        while (it < end) {
            auto i = it;
            auto j = c_str;
            while ( i < end && j < c_str_end && *i == *j) {
                i++;
                j++;
            }
            if (j == c_str_end) {
                return it - str();
            }
            it++;
        }
        return npos;
    }

    /**
     * find_last_of find the last occurrence of c in the string.
     * When pos is specified, the search only includes characters
     * at or before position pos.
     *
     */
    size_t find_last_of (char_type c, size_t pos = npos) const noexcept {
        const char_type* str_start = str();
        if (size()) {
            if (pos >= size()) {
                pos = size() - 1;
            }
            const char_type* p = str_start + pos + 1;
            do {
                p--;
                if (*p == c) {
                    return (p - str_start);
                }
            } while (p != str_start);
        }
        return npos;
    }

    /**
     *  Append a C substring.
     *  @param s  The C string to append.
     *  @param n  The number of characters to append.
     *  @return  Reference to this string.
     */
    basic_sstring& append (const char_type* s, size_t n) {
        basic_sstring ret(initialized_later(), size() + n);
        std::copy(begin(), end(), ret.begin());
        std::copy(s, s + n, ret.begin() + size());
        *this = std::move(ret);
        return *this;
    }

    /**
     *  Resize string.
     *  @param n  new size.
     *  @param c  if n greater than current size character to fill newly allocated space with.
     */
    void resize(size_t n, const char_type c  = '\0') {
        if (n > size()) {
            *this += sstring(n - size(), c);
        } else if (n < size()) {
            if (is_internal()) {
                u.internal.size = n;
            } else if (n + 1 <= sizeof(u.internal.str)) {
                *this = sstring(u.external.str, n);
            } else {
                u.external.size = n;
            }
        }
    }

    /**
     *  Replace characters with a value of a C style substring.
     *
     */
    basic_sstring& replace(size_type pos, size_type n1, const char_type* s,
             size_type n2) {
        if (pos > size()) {
            throw std::out_of_range("sstring::replace out of range");
        }

        if (n1 > size() - pos) {
            n1 = size() - pos;
        }

        if (n1 == n2) {
            if (n2) {
                std::copy(s, s + n2, begin() + pos);
            }
            return *this;
        }
        basic_sstring ret(initialized_later(), size() + n2 - n1);
        char_type* p= ret.begin();
        std::copy(begin(), begin() + pos, p);
        p += pos;
        if (n2) {
            std::copy(s, s + n2, p);
        }
        p += n2;
        std::copy(begin() + pos + n1, end(), p);
        *this = std::move(ret);
        return *this;
    }

    template <class InputIterator>
    basic_sstring& replace (const_iterator i1, const_iterator i2,
            InputIterator first, InputIterator last) {
        if (i1 < begin() || i1 > end() || i2 < begin()) {
            throw std::out_of_range("sstring::replace out of range");
        }
        if (i2 > end()) {
            i2 = end();
        }

        if (i2 - i1 == last - first) {
            //in place replacement
            std::copy(first, last, const_cast<char_type*>(i1));
            return *this;
        }
        basic_sstring ret(initialized_later(), size() + (last - first) - (i2 - i1));
        char_type* p = ret.begin();
        p = std::copy(cbegin(), i1, p);
        p = std::copy(first, last, p);
        std::copy(i2, cend(), p);
        *this = std::move(ret);
        return *this;
    }

    iterator erase(iterator first, iterator last) {
        size_t pos = first - begin();
        replace(pos, last - first, nullptr, 0);
        return begin() + pos;
    }

    /**
     * Inserts additional characters into the string right before
     * the character indicated by p.
     */
    template <class InputIterator>
    void insert(const_iterator p, InputIterator beg, InputIterator end) {
        replace(p, p, beg, end);
    }

    /**
     *  Returns a read/write reference to the data at the last
     *  element of the string.
     *  This function shall not be called on empty strings.
     */
    reference
    back() noexcept {
        return operator[](size() - 1);
    }

    /**
     *  Returns a  read-only (constant) reference to the data at the last
     *  element of the string.
     *  This function shall not be called on empty strings.
     */
    const_reference
    back() const noexcept {
        return operator[](size() - 1);
    }

    basic_sstring substr(size_t from, size_t len = npos)  const {
        if (from > size()) {
            throw std::out_of_range("sstring::substr out of range");
        }
        if (len > size() - from) {
            len = size() - from;
        }
        if (len == 0) {
            return "";
        }
        return { str() + from , len };
    }

    const char_type& at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range("sstring::at out of range");
        }
        return *(str() + pos);
    }

    char_type& at(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range("sstring::at out of range");
        }
        return *(str() + pos);
    }

    bool empty() const noexcept {
        return u.internal.size == 0;
    }
    void reset() noexcept {
        if (is_external()) {
            std::free(u.external.str);
        }
        u.internal.size = 0;
        u.internal.str[0] = '\0';
    }
    temporary_buffer<char_type> release() && {
        if (is_external()) {
            auto ptr = u.external.str;
            auto size = u.external.size;
            u.external.str = nullptr;
            u.external.size = 0;
            return temporary_buffer<char_type>(ptr, size, make_free_deleter(ptr));
        } else {
            auto buf = temporary_buffer<char_type>(u.internal.size);
            std::copy(u.internal.str, u.internal.str + u.internal.size, buf.get_write());
            u.internal.size = 0;
            u.internal.str[0] = '\0';
            return buf;
        }
    }
    int compare(const basic_sstring& x) const noexcept {
        auto n = traits_type::compare(begin(), x.begin(), std::min(size(), x.size()));
        if (n != 0) {
            return n;
        }
        if (size() < x.size()) {
            return -1;
        } else if (size() > x.size()) {
            return 1;
        } else {
            return 0;
        }
    }

    int compare(size_t pos, size_t sz, const basic_sstring& x) const {
        if (pos > size()) {
            throw std::out_of_range("pos larger than string size");
        }

        sz = std::min(size() - pos, sz);
        auto n = traits_type::compare(begin() + pos, x.begin(), std::min(sz, x.size()));
        if (n != 0) {
            return n;
        }
        if (sz < x.size()) {
            return -1;
        } else if (sz > x.size()) {
            return 1;
        } else {
            return 0;
        }
    }

    void swap(basic_sstring& x) noexcept {
        contents tmp;
        tmp = x.u;
        x.u = u;
        u = tmp;
    }
    char_type* data() {
        return str();
    }
    const char_type* data() const {
        return str();
    }
    const char_type* c_str() const {
        return str();
    }
    const char_type* begin() const { return str(); }
    const char_type* end() const { return str() + size(); }
    const char_type* cbegin() const { return str(); }
    const char_type* cend() const { return str() + size(); }
    char_type* begin() { return str(); }
    char_type* end() { return str() + size(); }
    bool operator==(const basic_sstring& x) const {
        return size() == x.size() && std::equal(begin(), end(), x.begin());
    }
    bool operator!=(const basic_sstring& x) const {
        return !operator==(x);
    }
    bool operator<(const basic_sstring& x) const {
        return compare(x) < 0;
    }
    basic_sstring operator+(const basic_sstring& x) const {
        basic_sstring ret(initialized_later(), size() + x.size());
        std::copy(begin(), end(), ret.begin());
        std::copy(x.begin(), x.end(), ret.begin() + size());
        return ret;
    }
    basic_sstring& operator+=(const basic_sstring& x) {
        return *this = *this + x;
    }
    char_type& operator[](size_type pos) {
        return str()[pos];
    }
    const char_type& operator[](size_type pos) const {
        return str()[pos];
    }

    operator std::basic_string_view<char_type>() const {
        return std::basic_string_view<char_type>(str(), size());
    }

    template <typename string_type, typename T>
    friend inline string_type to_sstring(T value);
};
template <typename char_type, typename Size, Size max_size>
constexpr Size basic_sstring<char_type, Size, max_size>::npos;

template <typename char_type, typename size_type, size_type Max, size_type N>
inline
basic_sstring<char_type, size_type, Max>
operator+(const char(&s)[N], const basic_sstring<char_type, size_type, Max>& t) {
    using sstring = basic_sstring<char_type, size_type, Max>;
    // don't copy the terminating NUL character
    sstring ret(typename sstring::initialized_later(), N-1 + t.size());
    auto p = std::copy(std::begin(s), std::end(s)-1, ret.begin());
    std::copy(t.begin(), t.end(), p);
    return ret;
}

template <size_t N>
static inline
size_t str_len(const char(&s)[N]) { return N - 1; }

template <size_t N>
static inline
const char* str_begin(const char(&s)[N]) { return s; }

template <size_t N>
static inline
const char* str_end(const char(&s)[N]) { return str_begin(s) + str_len(s); }

template <typename char_type, typename size_type, size_type max_size>
static inline
const char_type* str_begin(const basic_sstring<char_type, size_type, max_size>& s) { return s.begin(); }

template <typename char_type, typename size_type, size_type max_size>
static inline
const char_type* str_end(const basic_sstring<char_type, size_type, max_size>& s) { return s.end(); }

template <typename char_type, typename size_type, size_type max_size>
static inline
size_type str_len(const basic_sstring<char_type, size_type, max_size>& s) { return s.size(); }

template <typename First, typename Second, typename... Tail>
static inline
const size_t str_len(const First& first, const Second& second, const Tail&... tail) {
    return str_len(first) + str_len(second, tail...);
}

template <typename char_type, typename size_type, size_type max_size>
inline
void swap(basic_sstring<char_type, size_type, max_size>& x,
          basic_sstring<char_type, size_type, max_size>& y) noexcept
{
    return x.swap(y);
}

template <typename char_type, typename size_type, size_type max_size, typename char_traits>
inline
std::basic_ostream<char_type, char_traits>&
operator<<(std::basic_ostream<char_type, char_traits>& os,
        const basic_sstring<char_type, size_type, max_size>& s) {
    return os.write(s.begin(), s.size());
}

template <typename char_type, typename size_type, size_type max_size, typename char_traits>
inline
std::basic_istream<char_type, char_traits>&
operator>>(std::basic_istream<char_type, char_traits>& is,
        basic_sstring<char_type, size_type, max_size>& s) {
    std::string tmp;
    is >> tmp;
    s = tmp;
    return is;
}

namespace std {

template <typename char_type, typename size_type, size_type max_size>
struct hash<basic_sstring<char_type, size_type, max_size>> {
    size_t operator()(const basic_sstring<char_type, size_type, max_size>& s) const {
        return std::hash<std::basic_string_view<char_type>>()(s);
    }
};

}

static inline
char* copy_str_to(char* dst) {
    return dst;
}

template <typename Head, typename... Tail>
static inline
char* copy_str_to(char* dst, const Head& head, const Tail&... tail) {
    return copy_str_to(std::copy(str_begin(head), str_end(head), dst), tail...);
}

template <typename String = sstring, typename... Args>
static String make_sstring(Args&&... args)
{
    String ret(sstring::initialized_later(), str_len(args...));
    copy_str_to(ret.begin(), args...);
    return ret;
}

template <typename string_type = sstring, typename T>
inline string_type to_sstring(T value) {
    return sstring::to_sstring<string_type>(value);
}

namespace std {
template <typename T>
inline
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    bool first = true;
    os << "{";
    for (auto&& elem : v) {
        if (!first) {
            os << ", ";
        } else {
            first = false;
        }
        os << elem;
    }
    os << "}";
    return os;
}

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<Key, T, Hash, KeyEqual, Allocator>& v) {
    bool first = true;
    os << "{";
    for (auto&& elem : v) {
        if (!first) {
            os << ", ";
        } else {
            first = false;
        }
        os << "{ " << elem.first << " -> " << elem.second << "}";
    }
    os << "}";
    return os;
}
}











class reactor;
reactor& engine();
template <typename Clock> class timer;
using steady_clock_type = std::chrono::steady_clock;
void schedule_normal(std::unique_ptr<task> t);
void schedule_urgent(std::unique_ptr<task> t);

class manual_clock {
    public:
        using rep = int64_t;
        using period = std::chrono::nanoseconds::period;
        using duration = std::chrono::duration<rep, period>;
        using time_point = std::chrono::time_point<manual_clock, duration>;
    private:
        static std::atomic<rep> _now;
    public:
        manual_clock();
        static time_point now() {
            return time_point(duration(_now.load(std::memory_order_relaxed)));
        }
        static void advance(duration d);
        static void expire_timers();
};
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




// Utility function for error handling
inline void throw_system_error_on(bool condition) {
    if (condition) {
        throw std::system_error(errno, std::system_category());
    }
}


// timer 有默认参数,所以timer<>表示一个steady_clock_type
template <typename Clock = steady_clock_type>
class timer {
public:
    timer() = default;
    using time_point = typename Clock::time_point;
    using duration = typename Clock::duration;
    typedef Clock clock;
    using callback_t = std::function<void()>;
    using iterator = typename std::list<timer*>::iterator;
    iterator it; // 新增的迭代器成员
    iterator expired_it;  // 过期链表中的位置
    // boost::intrusive::list_member_hook<> _link;
    callback_t _callback;
    time_point _expiry; //到期时间点,每个timer都有一个到期时间点.
    std::optional<duration> _period;
    bool _armed = false;
    bool _queued = false;
    bool _expired = false;
    void readd_periodic();
    void arm_state(time_point until, std::optional<duration> period);
    timer(timer&& t) noexcept;
    explicit timer(callback_t&& callback);
    ~timer();
    // future<> expired();
    void set_callback(callback_t&& callback);
    void arm(time_point until, std::optional<duration> period = {});
    void rearm(time_point until, std::optional<duration> period = {});
    void rearm(duration delta) { rearm(Clock::now() + delta); }
    void arm(duration delta);
    void arm_periodic(duration delta);
    bool armed() const { return _armed; }
    bool cancel();
    time_point get_timeout();
};


class lowres_clock {
public:
    typedef int64_t rep;
    // The lowres_clock's resolution is 10ms. However, to make it is easier to
    // do calcuations with std::chrono::milliseconds, we make the clock's
    // period to 1ms instead of 10ms.
    typedef std::ratio<1, 1000> period;
    typedef std::chrono::duration<rep, period> duration;
    typedef std::chrono::time_point<lowres_clock, duration> time_point;
    lowres_clock();
    // ~lowres_clock();
    static time_point now() {
        auto nr = _now.load(std::memory_order_relaxed);
        return time_point(duration(nr));
    }
private:
    static void update();
    // _now is updated by cpu0 and read by other cpus. Make _now on its own
    // cache line to avoid false sharing.
    static std::atomic<rep> _now [[gnu::aligned(64)]];
    // High resolution timer to drive this low resolution clock
    struct timer_deleter {
        void operator()(void*) const;
    };
    timer<steady_clock_type> _timer; //?
    // High resolution timer expires every 10 milliseconds
    static constexpr std::chrono::milliseconds _granularity{10};
};

std::atomic<lowres_clock::rep> lowres_clock::_now;
std::atomic<manual_clock::rep> manual_clock::_now;
constexpr std::chrono::milliseconds lowres_clock::_granularity;


// Forward declarations for timer-related functions
void enable_timer(steady_clock_type::time_point when);
// Function declarations
bool queue_timer(timer<steady_clock_type>* tmr);
void add_timer(timer<steady_clock_type>* tmr);
void del_timer(timer<steady_clock_type>* tmr);
bool queue_timer(timer<lowres_clock>* tmr);
void add_timer(timer<lowres_clock>* tmr);
void del_timer(timer<lowres_clock>* tmr);
bool queue_timer(timer<manual_clock>* tmr);
void add_timer(timer<manual_clock>* tmr);
void del_timer(timer<manual_clock>* tmr);

template<typename Timer>
class timer_set {
public:
    using time_point = typename Timer::time_point;
    using timer_list_t = std::list<Timer*>;
    using duration = typename Timer::duration;
    using timestamp_t = typename duration::rep;
    static constexpr timestamp_t max_timestamp = std::numeric_limits<timestamp_t>::max();
    static constexpr int timestamp_bits = std::numeric_limits<timestamp_t>::digits;//63
    static constexpr int n_buckets = timestamp_bits + 1;//64
    std::array<timer_list_t, n_buckets> _buckets;
    timestamp_t _last;
    timestamp_t _next;
    std::bitset<n_buckets> _non_empty_buckets;
    /// \brief 获取时间点对应的时间戳（计数）
    /// \param tp 时间点对象
    /// \return 时间点自纪元以来的计数值
    static timestamp_t get_timestamp(time_point tp) {
        return tp.time_since_epoch().count();
    }
    /// \brief 获取定时器的超时时间戳
    /// \param timer 定时器对象
    /// \return 定时器超时时间的时间戳
    static timestamp_t get_timestamp(Timer& timer) {
        return get_timestamp(timer.get_timeout());
    }
    /// \brief 根据时间戳计算对应的桶索引
    /// \param timestamp 要计算的定时器时间戳
    /// \return 对应的桶索引
    int get_index(timestamp_t timestamp) const {
        if (timestamp <= _last) {
            return n_buckets - 1;
        }
        auto index = bitsets::count_leading_zeros(timestamp ^ _last);
        assert(index < n_buckets - 1);
        return index;
    }
    /*
    这个需要手动推导一下
    */

    /// \brief 获取定时器对应的桶索引
    /// \param timer 定时器对象
    /// \return 对应的桶索引
    int get_index(Timer& timer) const {
        return get_index(get_timestamp(timer));
    }
    /*
    一个timer有唯一的一个过期时间。
    */

    /// \brief 获取最后一个非空桶的索引
    /// \return 最后一个非空桶的索引
    int get_last_non_empty_bucket() const {
        return bitsets::get_last_set(_non_empty_buckets);
    }

public:
    /// \brief 构造函数初始化成员变量
    timer_set() : _last(0), _next(max_timestamp), _non_empty_buckets(0) {}

    ~timer_set() {
        // 清理所有定时器资源
        for (auto& list : _buckets) {
            while (!list.empty()) {
                auto* timer = list.front();
                list.pop_front();
                timer->cancel();
            }
        }
    }

    /// \brief 将定时器插入到对应的桶中
    /// \param timer 要插入的定时器对象
    /// \return true 如果插入后_next被更新为更小的值，否则false
    bool insert(Timer& timer) {
        auto timestamp = get_timestamp(timer);
        auto index = get_index(timestamp);
        auto& list = _buckets[index];
        list.push_back(&timer);
        timer.it = --list.end();//timer.it是timer在list中的迭代器(使用尾插法,所以end前一个位置就是最后一个元素前开后闭)
        _non_empty_buckets[index] = true;
        if (timestamp < _next) {
            _next = timestamp;
            return true;
        }
        return false;
    }
    /*
        next是边插入边维护的一个变量.表示下一次过期的时间.
    */

    /// \brief 从集合中移除定时器
    /// \param timer 要移除的定时器对象
    void remove(Timer& timer) {
        auto index = get_index(timer);
        auto& list = _buckets[index];
        list.erase(timer.it);//erase一个节点会造成内存泄漏吗? 不会:见 STL源码, 解析
        if (list.empty()) {
            _non_empty_buckets[index] = false;
        }
    }
    /**
     *
     * 这个地方像是RTOS优先级位图
     *
    */
    /// \brief 获取已到期的定时器列表
    /// \param now 当前时间点
    /// \return 包含所有已到期定时器的列表
    timer_list_t expire(time_point now) {
        timer_list_t exp;
        auto timestamp = get_timestamp(now);
        if (timestamp < _last) {
            abort();
        }
        //当前时间一定>=_last
        auto index = get_index(timestamp);
        // 处理所有在当前时间之前的非空桶
        for (int i : bitsets::for_each_set(_non_empty_buckets, index + 1)) {
            exp.splice(exp.end(), _buckets[i]);
            _non_empty_buckets[i] = false;
        }
        /*
            把所有过期的链表添加到exp后面(exp是一个临时的链表，操作时间间复杂度O(1)).
        */
        _last = timestamp;//所以_last就是最后一次处理过期定时器的时间.
        _next = max_timestamp;//_next设置为无穷.
        auto& list = _buckets[index];
        // 处理当前索引的桶中的定时器
        while (!list.empty()) {
            auto* timer = list.front();
            list.pop_front();
            if (timer->get_timeout() <= now) {
                exp.push_back(timer);
            } else {
                insert(*timer);
            }
        }
        _non_empty_buckets[index] = !list.empty();
        if (_next == max_timestamp && _non_empty_buckets.any()) {
            // 更新_next为最后一个非空桶中的最小时间戳
            for (auto* timer : _buckets[get_last_non_empty_bucket()]) {
                _next = std::min(_next, get_timestamp(*timer));
            }
        }
        return exp;//返回这个链表
    }

    time_point get_next_timeout() const {
        return time_point(duration(std::max(_last, _next)));
    }
    void clear() {
        for (auto& list : _buckets) {
            list.clear();
        }
        _non_empty_buckets.reset();
    }
    size_t size() const {
        size_t res = 0;
        for (const auto& list : _buckets) {
            res += list.size();
        }
        return res;
    }
    bool empty() const {
        return _non_empty_buckets.none();
    }
    time_point now() {
        return Timer::clock::now();
    }
};


template<typename T>
struct function_traits;

template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)>
{
    using return_type = Ret;
    using args_as_tuple = std::tuple<Args...>;
    using signature = Ret (Args...);
    static constexpr std::size_t arity = sizeof...(Args);
    template <std::size_t N>
    struct arg
    {
        static_assert(N < arity, "no such parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> : public function_traits<Ret(Args...)>
{};

template <typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...)> : public function_traits<Ret(Args...)>
{};

template <typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...) const> : public function_traits<Ret(Args...)>
{};

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

template<typename T>
struct function_traits<T&> : public function_traits<std::remove_reference_t<T>>
{};
template <typename... T> class future;
template <typename... T> class promise;
template <typename... T> struct future_state;


/// \cond internal
template <typename... T>
struct future_state {
    static constexpr bool copy_noexcept = std::is_nothrow_copy_constructible<std::tuple<T...>>::value;
    static_assert(std::is_nothrow_move_constructible<std::tuple<T...>>::value,
                  "Types must be no-throw move constructible");
    static_assert(std::is_nothrow_destructible<std::tuple<T...>>::value,
                  "Types must be no-throw destructible");
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    enum class state {
         invalid,
         future,
         result,
         exception,
    } _state = state::future;
    union any {
        any() {}
        ~any() {}
        std::tuple<T...> value;
        std::exception_ptr ex;
    } _u;
    future_state() noexcept {}
    [[gnu::always_inline]]
    future_state(future_state&& x) noexcept
            : _state(x._state) {
        switch (_state) {
        case state::future:
            break;
        case state::result:
            new (&_u.value) std::tuple<T...>(std::move(x._u.value));
            x._u.value.~tuple();
            break;
        case state::exception:
            new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
            x._u.ex.~exception_ptr();
            break;
        case state::invalid:
            break;
        default:
            abort();
        }
        x._state = state::invalid;
    }
    __attribute__((always_inline))
    ~future_state() noexcept {
        switch (_state) {
        case state::invalid:
            break;
        case state::future:
            break;
        case state::result:
            _u.value.~tuple();
            break;
        case state::exception:
            _u.ex.~exception_ptr();
            break;
        default:
            abort();
        }
    }
    future_state& operator=(future_state&& x) noexcept {
        if (this != &x) {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        return *this;
    }
    bool available() const noexcept { return _state == state::result || _state == state::exception; }
    bool failed() const noexcept { return _state == state::exception; }
    void wait();
    void set(const std::tuple<T...>& value) noexcept {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(value);
        _state = state::result;
    }
    void set(std::tuple<T...>&& value) noexcept {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(std::move(value));
        _state = state::result;
    }
    template <typename... A>
    void set(A&&... a) {
        assert(_state == state::future);
        new (&_u.value) std::tuple<T...>(std::forward<A>(a)...);
        _state = state::result;
    }
    void set_exception(std::exception_ptr ex) noexcept {
        assert(_state == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        _state = state::exception;
    }
    std::exception_ptr get_exception() && noexcept {
        assert(_state == state::exception);
        // Move ex out so future::~future() knows we've handled it
        _state = state::invalid;
        auto ex = std::move(_u.ex);
        _u.ex.~exception_ptr();
        return ex;
    }
    std::exception_ptr get_exception() const& noexcept {
        assert(_state == state::exception);
        return _u.ex;
    }
    std::tuple<T...> get_value() && noexcept {
        assert(_state == state::result);
        return std::move(_u.value);
    }
    template<typename U = std::tuple<T...>>
    std::enable_if_t<std::is_copy_constructible<U>::value, U> get_value() const& noexcept(copy_noexcept) {
        assert(_state == state::result);
        return _u.value;
    }
    std::tuple<T...> get() && {
        assert(_state != state::future);
        if (_state == state::exception) {
            _state = state::invalid;
            auto ex = std::move(_u.ex);
            _u.ex.~exception_ptr();
            // Move ex out so future::~future() knows we've handled it
            std::rethrow_exception(std::move(ex));
        }
        return std::move(_u.value);
    }
    std::tuple<T...> get() const& {
        assert(_state != state::future);
        if (_state == state::exception) {
            std::rethrow_exception(_u.ex);
        }
        return _u.value;
    }
    void ignore() noexcept {
        assert(_state != state::future);
        this->~future_state();
        _state = state::invalid;
    }
    using get0_return_type = std::tuple_element_t<0, std::tuple<T...>>;
    static get0_return_type get0(std::tuple<T...>&& x) {
        return std::get<0>(std::move(x));
    }
    void forward_to(promise<T...>& pr) noexcept;
        // assert(_state != state::future);
        // if (_state == state::exception) {
        //     pr.set_urgent_exception(std::move(_u.ex));
        //     _u.ex.~exception_ptr();
        // } else {
        //     pr.set_urgent_value(std::move(_u.value));
        //     _u.value.~tuple();
        // }
        // _state = state::invalid;
    //}
};

template <>
struct future_state<> {
    static_assert(sizeof(std::exception_ptr) == sizeof(void*), "exception_ptr not a pointer");
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    static constexpr bool copy_noexcept = true;
    enum class state : uintptr_t {
         invalid = 0,
         future = 1,
         result = 2,
         exception_min = 3,  // or anything greater
    };
    union any {
        any() { st = state::future; }
        ~any() {}
        state st;
        std::exception_ptr ex;
    } _u;
    future_state() noexcept {}
    [[gnu::always_inline]]
    future_state(future_state&& x) noexcept {
        if (x._u.st < state::exception_min) {
            _u.st = x._u.st;
        } else {
            // Move ex out so future::~future() knows we've handled it
            // Moving it will reset us to invalid state
            new (&_u.ex) std::exception_ptr(std::move(x._u.ex));
            x._u.ex.~exception_ptr();
        }
        x._u.st = state::invalid;
    }
    [[gnu::always_inline]]
    ~future_state() noexcept {
        if (_u.st >= state::exception_min) {
            _u.ex.~exception_ptr();
        }
    }
    future_state& operator=(future_state&& x) noexcept {
        if (this != &x) {
            this->~future_state();
            new (this) future_state(std::move(x));
        }
        return *this;
    }
    bool available() const noexcept { return _u.st == state::result || _u.st >= state::exception_min; }
    bool failed() const noexcept { return _u.st >= state::exception_min; }
    void set(const std::tuple<>& value) noexcept {
        assert(_u.st == state::future);
        _u.st = state::result;
    }
    void set(std::tuple<>&& value) noexcept {
        assert(_u.st == state::future);
        _u.st = state::result;
    }
    void set() {
        assert(_u.st == state::future);
        _u.st = state::result;//
    }
    void set_exception(std::exception_ptr ex) noexcept {
        assert(_u.st == state::future);
        new (&_u.ex) std::exception_ptr(ex);
        assert(_u.st >= state::exception_min);
    }
    std::tuple<> get() && {
        assert(_u.st != state::future);
        if (_u.st >= state::exception_min) {
            // Move ex out so future::~future() knows we've handled it
            // Moving it will reset us to invalid state
            std::rethrow_exception(std::move(_u.ex));
        }
        return {};
    }
    std::tuple<> get() const& {
        assert(_u.st != state::future);
        if (_u.st >= state::exception_min) {
            std::rethrow_exception(_u.ex);
        }
        return {};
    }
    void ignore() noexcept {
        assert(_u.st != state::future);
        this->~future_state();
        _u.st = state::invalid;
    }
    using get0_return_type = void;
    static get0_return_type get0(std::tuple<>&&) {
        return;
    }
    std::exception_ptr get_exception() && noexcept {
        assert(_u.st >= state::exception_min);
        // Move ex out so future::~future() knows we've handled it
        // Moving it will reset us to invalid state
        return std::move(_u.ex);
    }
    std::exception_ptr get_exception() const& noexcept {
        assert(_u.st >= state::exception_min);
        return _u.ex;
    }
    std::tuple<> get_value() const noexcept {
        assert(_u.st == state::result);
        return {};
    }
    void forward_to(promise<>& pr) noexcept;
};

template <typename Func, typename... T>
struct continuation final : task {
    continuation(Func&& func,future_state<T...>&& state,std::string name="unamed"):_state(std::move(state)),_func(std::move(func)),_name(std::move(name)){
    }
    continuation(Func&& func,std::string name = "unamed") : _func(std::move(func)),_name(std::move(name)) {
    }
    virtual void run() noexcept override {
        _func(std::move(_state));
    }
    future_state<T...> _state;
    Func _func;
    std::string _name;
};

template <typename... T>
class future;
template <typename... T> struct is_future : std::false_type {};
template <typename... T> struct is_future<future<T...>> : std::true_type {};
struct ready_future_marker {};
struct ready_future_from_tuple_marker {};
struct exception_future_marker {};
template <typename T>
struct futurize;

template <typename T>
using futurize_t = typename futurize<T>::type;


template <typename T>
struct futurize {
    /// If \c T is a future, \c T; otherwise \c future<T>
    using type = future<T>;
    /// The promise type associated with \c type.
    using promise_type = promise<T>;
    /// The value tuple type associated with \c type
    using value_type = std::tuple<T>;

    /// Apply a function to an argument list (expressed as a tuple)
    /// and return the result, as a future (if it wasn't already).
    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    /// Apply a function to an argument list
    /// and return the result, as a future (if it wasn't already).
    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;
    /// Convert a value or a future to a future
    static inline type convert(T&& value) {
        return make_ready_future<T>(std::move(value));
    }
    // 如果convert传入的是值, 使用 make_ready_future 转为future类型

    static inline type convert(type&& value){
        return std::move(value);
    }
    // 如果convert传入的是future，直接把future使用std::move变为右值.

    /// Convert the tuple representation into a future
    static type from_tuple(value_type&& value);
    /// Convert the tuple representation into a future
    static type from_tuple(const value_type& value);

    /// Makes an exceptional future of type \ref type.
    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};

/// \cond internal
template <>
struct futurize<void> {
    using type = future<>;
    using promise_type = promise<>;
    using value_type = std::tuple<>;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    static inline type from_tuple(value_type&& value);
    static inline type from_tuple(const value_type& value);

    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};

template <typename... Args>
struct futurize<future<Args...>> {
    using type = future<Args...>;
    using promise_type = promise<Args...>;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    static inline type convert(Args&&... values) { return make_ready_future<Args...>(std::move(values)...); }
    static inline type convert(type&& value) { return std::move(value); }

    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};


GCC6_CONCEPT(
template <typename T>
concept Future = is_future<T>::value;

template <typename Func, typename... T>
concept CanApply = requires (Func f, T... args) {
    f(std::forward<T>(args)...);
};

template <typename Func, typename Return, typename... T>
concept ApplyReturns = requires (Func f, T... args) {
    { f(std::forward<T>(args)...) } -> std::convertible_to<Return>;
};

template <typename Func, typename... T>
concept ApplyReturnsAnyFuture = requires (Func f, T... args) {
    requires is_future<decltype(f(std::forward<T>(args)...))>::value;
};
)

void engine_exit(std::exception_ptr eptr = {});
void report_failed_future(std::exception_ptr ex);

template <typename... T>
class future {
public:
    promise<T...>* _promise;
    future_state<T...> _local_state;  // valid if !_promise
    static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
    future(promise<T...>* pr) noexcept : _promise(pr) {
        _promise->_future = this;
    }
    template <typename... A>
    future(ready_future_marker, A&&... a) : _promise(nullptr) {
        _local_state.set(std::forward<A>(a)...);
    }
    template <typename... A>
    future(ready_future_from_tuple_marker, std::tuple<A...>&& data) : _promise(nullptr) {
        _local_state.set(std::move(data));
    }
    future(exception_future_marker, std::exception_ptr ex) noexcept : _promise(nullptr) {
        _local_state.set_exception(std::move(ex));
    }
    [[gnu::always_inline]]
    explicit future(future_state<T...>&& state) noexcept
            : _promise(nullptr), _local_state(std::move(state)) {
    }
    [[gnu::always_inline]]
    future_state<T...>* state() noexcept {
        return _promise ? _promise->_state : &_local_state;
    }

    template <typename Func>
    void schedule(Func&& func,std::string name="unamed"){
        if (state()->available()) {
            ::schedule_normal(std::make_unique<continuation<Func, T...>>(std::move(func), std::move(*state()),std::move(name)));
        } else {
            //走这一条
            assert(_promise);
            _promise->schedule(std::move(func),name);
            _promise->_future = nullptr;
            _promise = nullptr;
        }
    }

    [[gnu::always_inline]]
    future_state<T...> get_available_state() noexcept {
        auto st = state();
        if (_promise) {
            _promise->_future = nullptr;
            _promise = nullptr;
        }
        return std::move(*st);
    }

    [[gnu::noinline]]
    future<T...> rethrow_with_nested() {
        if (!failed()) {
            return make_exception_future<T...>(std::current_exception());
        } else {
            std::nested_exception f_ex;
            try {
                get();
            } catch (...) {
                std::throw_with_nested(f_ex);
            }
        }
        assert(0 && "we should not be here");
    }

    template<typename... U>
    friend class shared_future;
public:
    /// \brief The data type carried by the future.
    using value_type = std::tuple<T...>;
    /// \brief The data type carried by the future.
    using promise_type = promise<T...>;
    /// \brief Moves the future into a new object.
    [[gnu::always_inline]]
    future(future&& x) noexcept : _promise(x._promise) {
        if (!_promise) {
            _local_state = std::move(x._local_state);
        }
        x._promise = nullptr;
        if (_promise) {
            _promise->_future = this;
        }
    }
    future(const future&) = delete;
    future& operator=(future&& x) noexcept {
        if (this != &x) {
            this->~future();
            new (this) future(std::move(x));
        }
        return *this;
    }
    void operator=(const future&) = delete;
    __attribute__((always_inline))
    ~future() {
        if (_promise) {
            _promise->_future = nullptr;
        }
        if (failed()) {
            report_failed_future(state()->get_exception());
        }
    }
    std::tuple<T...> get();

     std::exception_ptr get_exception() {
        return get_available_state().get_exception();
    }
    typename future_state<T...>::get0_return_type get0() {
        return future_state<T...>::get0(get());
    }

    /// \cond internal
    void wait();
    [[gnu::always_inline]]
    bool available() noexcept {
        return state()->available();
    }
    [[gnu::always_inline]]
    bool failed() noexcept {
        return state()->failed();
    }
    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
    GCC6_CONCEPT( requires CanApply<Func, T...> )
    Result
    then(Func&& func,std::string name = "unnamed") noexcept {
        using futurator = futurize<std::result_of_t<Func(T&&...)>>;
        // 如果当前 future,已经完成且不需要抢占.
        if (available() && !need_preempt()) {
            //调试的时候这里不会执行到,need_preempt永远为true.
            if (failed()) {
                // 如果失败，传播异常
                return futurator::make_exception_future(get_available_state().get_exception());
            } else {
                // 如果成功，执行回调函数
                return futurator::apply(std::forward<Func>(func), get_available_state().get_value());
            }
        }
        // 如果 future 还未完成，创建新的 promise 和 future
        typename futurator::promise_type pr; // 这行代码是什么意思?
        auto fut = pr.get_future();
        try {
            // std::cout<<"开始执行schedule"<<std::endl;
            //schedule接受一个lambda函数,捕捉pr和func,参数为state
            schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable
            {
                //这个地方看不懂.auto &&state和state()有什么区别？为什么state不用引用捕获？
                if (state.failed()) {
                    pr.set_exception(std::move(state).get_exception());
                }
                else{
                    // 执行这个
                    futurator::apply(std::forward<Func>(func), std::move(state).get_value()).forward_to(std::move(pr));
                    // futuator::apply首先执行func(value)，返回类型是T.
                    // 返回一个 future<T>. 然后调用future的forward_to.
                }
            },std::move(name));
        } catch (...) {
            abort();
        }
        return fut;
    }

    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(future)>>>
    GCC6_CONCEPT( requires CanApply<Func, future> )
    Result
    then_wrapped(Func&& func) noexcept {
        using futurator = futurize<std::result_of_t<Func(future)>>;
        if (available() && !need_preempt()) {
            return futurator::apply(std::forward<Func>(func), future(get_available_state()));
        }
        typename futurator::promise_type pr;
        auto fut = pr.get_future();
        try {
            schedule([pr = std::move(pr), func = std::forward<Func>(func)] (auto&& state) mutable {
                futurator::apply(std::forward<Func>(func), future(std::move(state))).forward_to(std::move(pr));
            });
        } catch (...) {
            abort();
        }
        return fut;
    }
    void forward_to(promise<T...>&& pr) noexcept {
        if (state()->available()) {
            // std::cout<<"state available future调用forward_to"<<std::endl;
            state()->forward_to(pr);

        } else {
            // std::cout<<"state unavailable future调用forward_to"<<std::endl;
            _promise->_future = nullptr;
            *_promise = std::move(pr);
            _promise = nullptr;
        }
    }

    template <typename Func>
    GCC6_CONCEPT( requires CanApply<Func> )
    future<T...> finally(Func&& func) noexcept {
        return then_wrapped(finally_body<Func, is_future<std::result_of_t<Func()>>::value>(std::forward<Func>(func)));
    }


    template <typename Func, bool FuncReturnsFuture>
    struct finally_body;

    template <typename Func>
    struct finally_body<Func, true> {
        Func _func;

        finally_body(Func&& func) : _func(std::forward<Func>(func))
        { }

        future<T...> operator()(future<T...>&& result) {
            using futurator = futurize<std::result_of_t<Func()>>;
            return futurator::apply(_func).then_wrapped([result = std::move(result)](auto f_res) mutable {
                if (!f_res.failed()) {
                    return std::move(result);
                } else {
                    try {
                        f_res.get();
                    } catch (...) {
                        return result.rethrow_with_nested();
                    }
                    assert(0 && "we should not be here");
                }
            });
        }
    };

    template <typename Func>
    struct finally_body<Func, false> {
        Func _func;
        finally_body(Func&& func) : _func(std::forward<Func>(func))
        {}
        future<T...> operator()(future<T...>&& result) {
            try {
                _func();
                return std::move(result);
            } catch (...) {
                return result.rethrow_with_nested();
            }
        };
    };

    future<> or_terminate() noexcept {
        return then_wrapped([] (auto&& f) {
            try {
                f.get();
            } catch (...) {
                engine_exit(std::current_exception());
            }
        });
    }

    future<> discard_result() noexcept {
        return then([] (T&&...) {});
    }
    template <typename Func>
    future<T...> handle_exception(Func&& func) noexcept {
        using func_ret = std::result_of_t<Func(std::exception_ptr)>;
        return then_wrapped([func = std::forward<Func>(func)]
                             (auto&& fut) -> future<T...> {
            if (!fut.failed()) {
                return make_ready_future<T...>(fut.get());
            } else {
                return futurize<func_ret>::apply(func, fut.get_exception());
            }
        });
    }
    template <typename Func>
    future<T...> handle_exception_type(Func&& func) noexcept {
        using trait = function_traits<Func>;
        static_assert(trait::arity == 1, "func can take only one parameter");
        using ex_type = typename trait::template arg<0>::type;
        using func_ret = typename trait::return_type;
        return then_wrapped([func = std::forward<Func>(func)]
                             (auto&& fut) -> future<T...> {
            try {
                return make_ready_future<T...>(fut.get());
            } catch(ex_type& ex) {
                return futurize<func_ret>::apply(func, ex);
            }
        });
    }
    void ignore_ready_future() noexcept {
        state()->ignore();
    }
    /// \cond internal
    template <typename... U>
    friend class promise;
    template <typename... U, typename... A>
    friend future<U...> make_rFeady_future(A&&... value);
    template <typename... U>
    friend future<U...> make_exception_future(std::exception_ptr ex) noexcept;
    template <typename... U, typename Exception>
    friend future<U...> make_exception_future(Exception&& ex) noexcept;
    /// \endcondF
};







template <typename... T>
class promise {
public:
    enum class urgent { no, yes };
    future<T...>* _future = nullptr;
    future_state<T...> _local_state;
    future_state<T...>* _state;
    std::unique_ptr<task> _task;
    static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
    /// \brief Constructs an empty \c promise.
    ///
    /// Creates promise with no associated future yet (see get_future()).
    promise() noexcept : _state(&_local_state) {}

    /// \brief Moves a \c promise object.
    promise(promise&& x) noexcept : _future(x._future), _state(x._state), _task(std::move(x._task)) {
        if (_state == &x._local_state) {
            _state = &_local_state;
            _local_state = std::move(x._local_state);
        }
        x._future = nullptr;
        x._state = nullptr;
        migrated();
    }
    promise(const promise&) = delete;
    __attribute__((always_inline))
    ~promise() noexcept {
        abandoned();
    }
    promise& operator=(promise&& x) noexcept {
        if (this != &x) {
            this->~promise();
            new (this) promise(std::move(x));
        }
        return *this;
    }
    void operator=(const promise&) = delete;

    future<T...> get_future() noexcept;

    void set_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
        do_set_value<urgent::no>(result);
    }

    void set_value(std::tuple<T...>&& result) noexcept {
        do_set_value<urgent::no>(std::move(result));
    }

    template <typename... A>
    void set_value(A&&... a) noexcept {
        assert(_state);
        _state->set(std::forward<A>(a)...);
        make_ready<urgent::no>();
    }

    void set_exception(std::exception_ptr ex) noexcept {
        do_set_exception<urgent::no>(std::move(ex));
    }

    template<typename Exception>
    void set_exception(Exception&& e) noexcept {
        set_exception(make_exception_ptr(std::forward<Exception>(e)));
    }
    template<urgent Urgent>
    void do_set_value(std::tuple<T...> result) noexcept {
        assert(_state);
        _state->set(std::move(result));
        make_ready<Urgent>();
    }

    void set_urgent_value(const std::tuple<T...>& result) noexcept(copy_noexcept) {
        do_set_value<urgent::yes>(result);
    }

    void set_urgent_value(std::tuple<T...>&& result) noexcept {
        do_set_value<urgent::yes>(std::move(result));
    }

    template<urgent Urgent>
    void do_set_exception(std::exception_ptr ex) noexcept {
        assert(_state);
        _state->set_exception(std::move(ex));
        make_ready<Urgent>();
    }

    void set_urgent_exception(std::exception_ptr ex) noexcept {
        do_set_exception<urgent::yes>(std::move(ex));
    }
    template <typename Func>
    void schedule(Func&& func,std::string name) {
        auto tws = std::make_unique<continuation<Func, T...>>(std::move(func),std::move(name));
        _state = &tws->_state;
        _task = std::move(tws);
    }
    template<urgent Urgent>
    void make_ready() noexcept;
    void migrated() noexcept;
    void abandoned() noexcept;
    template <typename... U>
    friend class future;
    friend class future_state<T...>;
};

template<>
class promise<void> : public promise<> {};



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

template <typename... T>
class stream;

template <typename... T>
class subscription;

template <typename... T>
class stream {
    subscription<T...>* _sub = nullptr;
    promise<> _done;
    promise<> _ready;
public:
    using next_fn = std::function<future<> (T...)>;
    stream() = default;
    stream(const stream&) = delete;
    stream(stream&&) = delete;
    ~stream();
    void operator=(const stream&) = delete;
    void operator=(stream&&) = delete;

    // Returns a subscription that reads value from this
    // stream.
    subscription<T...> listen();

    // Returns a subscription that reads value from this
    // stream, and also sets up the listen function.
    subscription<T...> listen(next_fn next);

    // Becomes ready when the listener is ready to accept
    // values.  Call only once, when beginning to produce
    // values.
    future<> started();

    // Produce a value.  Call only after started(), and after
    // a previous produce() is ready.
    future<> produce(T... data);

    // End the stream.   Call only after started(), and after
    // a previous produce() is ready.  No functions may be called
    // after this.
    void close();

    // Signal an error.   Call only after started(), and after
    // a previous produce() is ready.  No functions may be called
    // after this.
    template <typename E>
    void set_exception(E ex);
private:
    void pause(future<> can_continue);
    void start();
    friend class subscription<T...>;
};

template <typename... T>
class subscription {
public:
    using next_fn = typename stream<T...>::next_fn;
private:
    stream<T...>* _stream;
    next_fn _next;
private:
    explicit subscription(stream<T...>* s);
public:
    subscription(subscription&& x);
    ~subscription();

    /// \brief Start receiving events from the stream.
    ///
    /// \param next Callback to call for each event
    void start(std::function<future<> (T...)> next);

    // Becomes ready when the stream is empty, or when an error
    // happens (in that case, an exception is held).
    future<> done();

    friend class stream<T...>;
};


template <typename... T>
inline
stream<T...>::~stream() {
    if (_sub) {
        _sub->_stream = nullptr;
    }
}

template <typename... T>
inline
subscription<T...>
stream<T...>::listen() {
    return subscription<T...>(this);
}

template <typename... T>
inline
subscription<T...>
stream<T...>::listen(next_fn next) {
    auto sub = subscription<T...>(this);
    sub.start(std::move(next));
    return sub;
}

template <typename... T>
inline
future<>
stream<T...>::started() {
    return _ready.get_future();
}

template <typename... T>
inline
future<>
stream<T...>::produce(T... data) {
    auto ret = futurize<void>::apply(_sub->_next, std::move(data)...);
    if (ret.available() && !ret.failed()) {
        // Native network stack depends on stream::produce() returning
        // a ready future to push packets along without dropping.  As
        // a temporary workaround, special case a ready, unfailed future
        // and return it immediately, so that then_wrapped(), below,
        // doesn't convert a ready future to an unready one.
        return ret;
    }
    return ret.then_wrapped([this] (auto&& f) {
        try {
            f.get();
        } catch (...) {
            _done.set_exception(std::current_exception());
            // FIXME: tell the producer to stop producing
            throw;
        }
    });
}

template <typename... T>
inline
void
stream<T...>::close() {
    _done.set_value();
}

template <typename... T>
template <typename E>
inline
void
stream<T...>::set_exception(E ex) {
    _done.set_exception(ex);
}

template <typename... T>
inline
subscription<T...>::subscription(stream<T...>* s)
        : _stream(s) {
    assert(!_stream->_sub);
    _stream->_sub = this;
}

template <typename... T>
inline
void
subscription<T...>::start(std::function<future<> (T...)> next) {
    _next = std::move(next);
    _stream->_ready.set_value();
}

template <typename... T>
inline
subscription<T...>::~subscription() {
    if (_stream) {
        _stream->_sub = nullptr;
    }
}

template <typename... T>
inline
subscription<T...>::subscription(subscription&& x)
    : _stream(x._stream), _next(std::move(x._next)) {
    x._stream = nullptr;
    if (_stream) {
        _stream->_sub = this;
    }
}

template <typename... T>
inline
future<>
subscription<T...>::done() {
    return _stream->_done.get_future();
}




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


void throw_system_error_on(bool condition, const char* what_arg = "!") {
    if (condition) {
        throw std::system_error(errno, std::system_category(), what_arg);
    }
}









#include <arpa/inet.h>
#include <iosfwd>
#include <utility>

namespace net {

enum class ip_protocol_num : uint8_t {
    icmp = 1, tcp = 6, udp = 17, unused = 255
};

enum class eth_protocol_num : uint16_t {
    ipv4 = 0x0800, arp = 0x0806, ipv6 = 0x86dd
};

const uint8_t eth_hdr_len = 14;
const uint8_t tcp_hdr_len_min = 20;
const uint8_t ipv4_hdr_len_min = 20;
const uint8_t ipv6_hdr_len_min = 40;
const uint16_t ip_packet_len_max = 65535;

}
inline uint64_t ntohq(uint64_t v) {
    return __builtin_bswap64(v);
}
inline uint64_t htonq(uint64_t v) {
    return __builtin_bswap64(v);
}

namespace net {
inline void ntoh() {}
inline void hton() {}
inline uint8_t ntoh(uint8_t x) { return x; }
inline uint8_t hton(uint8_t x) { return x; }
inline uint16_t ntoh(uint16_t x) { return ntohs(x); }
inline uint16_t hton(uint16_t x) { return htons(x); }
inline uint32_t ntoh(uint32_t x) { return ntohl(x); }
inline uint32_t hton(uint32_t x) { return htonl(x); }
inline uint64_t ntoh(uint64_t x) { return ntohq(x); }
inline uint64_t hton(uint64_t x) { return htonq(x); }
inline int8_t ntoh(int8_t x) { return x; }
inline int8_t hton(int8_t x) { return x; }
inline int16_t ntoh(int16_t x) { return ntohs(x); }
inline int16_t hton(int16_t x) { return htons(x); }
inline int32_t ntoh(int32_t x) { return ntohl(x); }
inline int32_t hton(int32_t x) { return htonl(x); }
inline int64_t ntoh(int64_t x) { return ntohq(x); }
inline int64_t hton(int64_t x) { return htonq(x); }
// Deprecated alias net::packed<> for unaligned<> from unaligned.hh.
// TODO: get rid of this alias.
template <typename T> using packed = unaligned<T>;

template <typename T>
inline T ntoh(const packed<T>& x) {
    T v = x;
    return ntoh(v);
}

template <typename T>
inline T hton(const packed<T>& x) {
    T v = x;
    return hton(v);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const packed<T>& v) {
    auto x = v.raw;
    return os << x;
}

inline void ntoh_inplace() {}
inline
void hton_inplace() {};

template <typename First, typename... Rest>
inline
void ntoh_inplace(First& first, Rest&... rest) {
    first = ntoh(first);
    ntoh_inplace(std::forward<Rest&>(rest)...);
}

template <typename First, typename... Rest>
inline
void hton_inplace(First& first, Rest&... rest) {
    first = hton(first);
    hton_inplace(std::forward<Rest&>(rest)...);
}

template <class T>
inline
T ntoh(const T& x) {
    T tmp = x;
    tmp.adjust_endianness([] (auto&&... what) { ntoh_inplace(std::forward<decltype(what)&>(what)...); });
    return tmp;
}

template <class T>
inline
T hton(const T& x) {
    T tmp = x;
    tmp.adjust_endianness([] (auto&&... what) { hton_inplace(std::forward<decltype(what)&>(what)...); });
    return tmp;
}
}

enum class transport {
    TCP = IPPROTO_TCP,
    SCTP = IPPROTO_SCTP
};

namespace net {
class inet_address;
}

struct listen_options {
    transport proto = transport::TCP;
    bool reuse_address = false;
    listen_options(bool rua = false)
        : reuse_address(rua)
    {}
};


namespace net {


class inet_address {
public:
    enum class family {
        INET = AF_INET, INET6 = AF_INET6
    };
private:
    family _in_family;
    union {
        ::in_addr _in;
        ::in6_addr _in6;
    };
public:
    inet_address();
    inet_address(::in_addr i);
    inet_address(::in6_addr i);
    // NOTE: does _not_ resolve the address. Only parses
    // ipv4/ipv6 numerical address
    inet_address(const std::string&);
    inet_address(inet_address&&) = default;
    inet_address(const inet_address&) = default;
    inet_address& operator=(const inet_address&) = default;
    bool operator==(const inet_address&) const;
    family in_family() const {
        return _in_family;
    }
    size_t size() const;
    const void * data() const;
    operator const ::in_addr&() const;
    operator const ::in6_addr&() const;
    future<std::string> hostname() const;
    future<std::vector<std::string>> aliases() const;
    static future<inet_address> find(const std::string&);
    static future<inet_address> find(const std::string&, family);
    static future<std::vector<inet_address>> find_all(const std::string&);
    static future<std::vector<inet_address>> find_all(const std::string&, family);
};
std::ostream& operator<<(std::ostream&, const inet_address&);
std::ostream& operator<<(std::ostream&, const inet_address::family&);
}

class unknown_host : public std::invalid_argument {
public:
    using invalid_argument::invalid_argument;
};

class ipv4_addr;
class socket_address {
public:
    union {
        ::sockaddr_storage sas;
        ::sockaddr sa;
        ::sockaddr_in in;
    } u;
    socket_address(sockaddr_in sa) {
        u.in = sa;
    }
    socket_address(ipv4_addr);
    socket_address() = default;
    ::sockaddr& as_posix_sockaddr() { return u.sa; }
    ::sockaddr_in& as_posix_sockaddr_in() { return u.in; }
    const ::sockaddr& as_posix_sockaddr() const { return u.sa; }
    const ::sockaddr_in& as_posix_sockaddr_in() const { return u.in; }

    bool operator==(const socket_address&) const;
};

struct ipv4_addr {
    uint32_t ip;
    uint16_t port;

    ipv4_addr() : ip(0), port(0) {}
    ipv4_addr(uint32_t ip, uint16_t port) : ip(ip), port(port) {}
    ipv4_addr(uint16_t port) : ip(0), port(port) {}
    ipv4_addr(const std::string &addr);
    ipv4_addr(const std::string &addr, uint16_t port);
    ipv4_addr(const net::inet_address&, uint16_t);
    ipv4_addr(const socket_address &sa) {
        ip = net::ntoh(sa.u.in.sin_addr.s_addr);
        port = net::ntoh(sa.u.in.sin_port);
    }
    ipv4_addr(socket_address &&sa) : ipv4_addr(sa) {}
};



namespace net {

struct fragment {
    char* base;
    size_t size;
};

struct offload_info {
    ip_protocol_num protocol = ip_protocol_num::unused;
    bool needs_csum = false;
    uint8_t ip_hdr_len = 20;
    uint8_t tcp_hdr_len = 20;
    uint8_t udp_hdr_len = 8;
    bool needs_ip_csum = false;
    bool reassembled = false;
    uint16_t tso_seg_size = 0;
    // HW stripped VLAN header (CPU order)
    std::optional<uint16_t> vlan_tci;
};

// Zero-copy friendly packet class
//
// For implementing zero-copy, we need a flexible destructor that can
// destroy packet data in different ways: decrementing a reference count,
// or calling a free()-like function.
//
// Moreover, we need different destructors for each set of fragments within
// a single fragment. For example, a header and trailer might need delete[]
// to be called, while the internal data needs a reference count to be
// released.  Matters are complicated in that fragments can be split
// (due to virtual/physical translation).
//
// To implement this, we associate each packet with a single destructor,
// but allow composing a packet from another packet plus a fragment to
// be added, with its own destructor, causing the destructors to be chained.
//
// The downside is that the data needed for the destructor is duplicated,
// if it is already available in the fragment itself.
//
// As an optimization, when we allocate small fragments, we allocate some
// extra space, so prepending to the packet does not require extra
// allocations.  This is useful when adding headers.
//
class packet final {
    // enough for lots of headers, not quite two cache lines:
    static constexpr size_t internal_data_size = 128 - 16;
    static constexpr size_t default_nr_frags = 4;

    struct pseudo_vector {
        fragment* _start;
        fragment* _finish;
        pseudo_vector(fragment* start, size_t nr)
            : _start(start), _finish(_start + nr) {}
        fragment* begin() { return _start; }
        fragment* end() { return _finish; }
        fragment& operator[](size_t idx) { return _start[idx]; }
    };

    struct impl {
        // when destroyed, virtual destructor will reclaim resources
        deleter _deleter;
        unsigned _len = 0;
        uint16_t _nr_frags = 0;
        uint16_t _allocated_frags;
        offload_info _offload_info;
        std::optional<uint32_t> _rss_hash;
        char _data[internal_data_size]; // only _frags[0] may use
        unsigned _headroom = internal_data_size; // in _data
        // FIXME: share _data/_frags space

        fragment _frags[];

        impl(size_t nr_frags = default_nr_frags);
        impl(const impl&) = delete;
        impl(fragment frag, size_t nr_frags = default_nr_frags);

        pseudo_vector fragments() { return { _frags, _nr_frags }; }

        static std::unique_ptr<impl> allocate(size_t nr_frags) {
            nr_frags = std::max(nr_frags, default_nr_frags);
            return std::unique_ptr<impl>(new (nr_frags) impl(nr_frags));
        }

        static std::unique_ptr<impl> copy(impl* old, size_t nr) {
            auto n = allocate(nr);
            n->_deleter = std::move(old->_deleter);
            n->_len = old->_len;
            n->_nr_frags = old->_nr_frags;
            n->_headroom = old->_headroom;
            n->_offload_info = old->_offload_info;
            n->_rss_hash = old->_rss_hash;
            std::copy(old->_frags, old->_frags + old->_nr_frags, n->_frags);
            old->copy_internal_fragment_to(n.get());
            return std::move(n);
        }

        static std::unique_ptr<impl> copy(impl* old) {
            return copy(old, old->_nr_frags);
        }

        static std::unique_ptr<impl> allocate_if_needed(std::unique_ptr<impl> old, size_t extra_frags) {
            if (old->_allocated_frags >= old->_nr_frags + extra_frags) {
                return std::move(old);
            }
            return copy(old.get(), std::max<size_t>(old->_nr_frags + extra_frags, 2 * old->_nr_frags));
        }
        void* operator new(size_t size, size_t nr_frags = default_nr_frags) {
            assert(nr_frags == uint16_t(nr_frags));
            return ::operator new(size + nr_frags * sizeof(fragment));
        }
        // Matching the operator new above
        void operator delete(void* ptr, size_t nr_frags) {
            return ::operator delete(ptr);
        }
        // Since the above "placement delete" hides the global one, expose it
        void operator delete(void* ptr) {
            return ::operator delete(ptr);
        }

        bool using_internal_data() const {
            return _nr_frags
                    && _frags[0].base >= _data
                    && _frags[0].base < _data + internal_data_size;
        }

        void unuse_internal_data() {
            if (!using_internal_data()) {
                return;
            }
            auto buf = static_cast<char*>(::malloc(_frags[0].size));
            if (!buf) {
                throw std::bad_alloc();
            }
            deleter d = make_free_deleter(buf);
            std::copy(_frags[0].base, _frags[0].base + _frags[0].size, buf);
            _frags[0].base = buf;
            _deleter.append(std::move(d));
            _headroom = internal_data_size;
        }
        void copy_internal_fragment_to(impl* to) {
            if (!using_internal_data()) {
                return;
            }
            to->_frags[0].base = to->_data + _headroom;
            std::copy(_frags[0].base, _frags[0].base + _frags[0].size,
                    to->_frags[0].base);
        }
    };
    packet(std::unique_ptr<impl>&& impl) : _impl(std::move(impl)) {}
    std::unique_ptr<impl> _impl;
public:
    static packet from_static_data(const char* data, size_t len) {
        return {fragment{const_cast<char*>(data), len}, deleter()};
    }

    // build empty packet
    packet();
    // build empty packet with nr_frags allocated
    packet(size_t nr_frags);
    // move existing packet
    packet(packet&& x) noexcept;
    // copy data into packet
    packet(const char* data, size_t len);
    // copy data into packet
    packet(fragment frag);
    // zero-copy single fragment
    packet(fragment frag, deleter del);
    // zero-copy multiple fragments
    packet(std::vector<fragment> frag, deleter del);
    // build packet with iterator
    template <typename Iterator>
    packet(Iterator begin, Iterator end, deleter del);
    // append fragment (copying new fragment)
    packet(packet&& x, fragment frag);
    // prepend fragment (copying new fragment, with header optimization)
    packet(fragment frag, packet&& x);
    // prepend fragment (zero-copy)
    packet(fragment frag, deleter del, packet&& x);
    // append fragment (zero-copy)
    packet(packet&& x, fragment frag, deleter d);
    // append temporary_buffer (zero-copy)
    packet(packet&& x, temporary_buffer<char> buf);
    // create from temporary_buffer (zero-copy)
    packet(temporary_buffer<char> buf);
    // append deleter
    packet(packet&& x, deleter d);

    packet& operator=(packet&& x) {
        if (this != &x) {
            this->~packet();
            new (this) packet(std::move(x));
        }
        return *this;
    }

    unsigned len() const { return _impl->_len; }
    unsigned memory() const { return len() +  sizeof(packet::impl); }

    fragment frag(unsigned idx) const { return _impl->_frags[idx]; }
    fragment& frag(unsigned idx) { return _impl->_frags[idx]; }

    unsigned nr_frags() const { return _impl->_nr_frags; }
    pseudo_vector fragments() const { return { _impl->_frags, _impl->_nr_frags }; }
    fragment* fragment_array() const { return _impl->_frags; }

    // share packet data (reference counted, non COW)
    packet share();
    packet share(size_t offset, size_t len);

    void append(packet&& p);

    void trim_front(size_t how_much);
    void trim_back(size_t how_much);

    // get a header pointer, linearizing if necessary
    template <typename Header>
    Header* get_header(size_t offset = 0);

    // get a header pointer, linearizing if necessary
    char* get_header(size_t offset, size_t size);

    // prepend a header (default-initializing it)
    template <typename Header>
    Header* prepend_header(size_t extra_size = 0);

    // prepend a header (uninitialized!)
    char* prepend_uninitialized_header(size_t size);

    packet free_on_cpu(unsigned cpu, std::function<void()> cb = []{});

    void linearize() { return linearize(0, len()); }

    void reset() { _impl.reset(); }

    void reserve(int n_frags) {
        if (n_frags > _impl->_nr_frags) {
            auto extra = n_frags - _impl->_nr_frags;
            _impl = impl::allocate_if_needed(std::move(_impl), extra);
        }
    }
    std::optional<uint32_t> rss_hash() {
        return _impl->_rss_hash;
    }
    std::optional<uint32_t> set_rss_hash(uint32_t hash) {
        return _impl->_rss_hash = hash;
    }
    // Call `func` for each fragment, avoiding data copies when possible
    // `func` is called with a temporary_buffer<char> parameter
    template <typename Func>
    void release_into(Func&& func) {
        unsigned idx = 0;
        if (_impl->using_internal_data()) {
            auto&& f = frag(idx++);
            func(temporary_buffer<char>(f.base, f.size));
        }
        while (idx < nr_frags()) {
            auto&& f = frag(idx++);
            func(temporary_buffer<char>(f.base, f.size, _impl->_deleter.share()));
        }
    }
    std::vector<temporary_buffer<char>> release() {
        std::vector<temporary_buffer<char>> ret;
        ret.reserve(_impl->_nr_frags);
        release_into([&ret] (temporary_buffer<char>&& frag) {
            ret.push_back(std::move(frag));
        });
        return ret;
    }
    explicit operator bool() {
        return bool(_impl);
    }
    static packet make_null_packet() {
        return net::packet(nullptr);
    }
private:
    void linearize(size_t at_frag, size_t desired_size);
    bool allocate_headroom(size_t size);
public:
    class offload_info offload_info() const { return _impl->_offload_info; }
    class offload_info& offload_info_ref() { return _impl->_offload_info; }
    void set_offload_info(class offload_info oi) { _impl->_offload_info = oi; }
};

std::ostream& operator<<(std::ostream& os, const packet& p);

inline
packet::packet(packet&& x) noexcept
    : _impl(std::move(x._impl)) {
}

inline
packet::impl::impl(size_t nr_frags)
    : _len(0), _allocated_frags(nr_frags) {
}

inline
packet::impl::impl(fragment frag, size_t nr_frags)
    : _len(frag.size), _allocated_frags(nr_frags) {
    assert(_allocated_frags > _nr_frags);
    if (frag.size <= internal_data_size) {
        _headroom -= frag.size;
        _frags[0] = { _data + _headroom, frag.size };
    } else {
        auto buf = static_cast<char*>(::malloc(frag.size));
        if (!buf) {
            throw std::bad_alloc();
        }
        deleter d = make_free_deleter(buf);
        _frags[0] = { buf, frag.size };
        _deleter.append(std::move(d));
    }
    std::copy(frag.base, frag.base + frag.size, _frags[0].base);
    ++_nr_frags;
}

inline
packet::packet()
    : _impl(impl::allocate(1)) {
}

inline
packet::packet(size_t nr_frags)
    : _impl(impl::allocate(nr_frags)) {
}

inline
packet::packet(fragment frag) : _impl(new impl(frag)) {
}

inline
packet::packet(const char* data, size_t size) : packet(fragment{const_cast<char*>(data), size}) {
}

inline
packet::packet(fragment frag, deleter d)
    : _impl(impl::allocate(1)) {
    _impl->_deleter = std::move(d);
    _impl->_frags[_impl->_nr_frags++] = frag;
    _impl->_len = frag.size;
}

inline
packet::packet(std::vector<fragment> frag, deleter d)
    : _impl(impl::allocate(frag.size())) {
    _impl->_deleter = std::move(d);
    std::copy(frag.begin(), frag.end(), _impl->_frags);
    _impl->_nr_frags = frag.size();
    _impl->_len = 0;
    for (auto&& f : _impl->fragments()) {
        _impl->_len += f.size;
    }
}

template <typename Iterator>
inline
packet::packet(Iterator begin, Iterator end, deleter del) {
    unsigned nr_frags = 0, len = 0;
    nr_frags = std::distance(begin, end);
    std::for_each(begin, end, [&] (const fragment& frag) { len += frag.size; });
    _impl = impl::allocate(nr_frags);
    _impl->_deleter = std::move(del);
    _impl->_len = len;
    _impl->_nr_frags = nr_frags;
    std::copy(begin, end, _impl->_frags);
}

inline
packet::packet(packet&& x, fragment frag)
    : _impl(impl::allocate_if_needed(std::move(x._impl), 1)) {
    _impl->_len += frag.size;
    std::unique_ptr<char[]> buf(new char[frag.size]);
    std::copy(frag.base, frag.base + frag.size, buf.get());
    _impl->_frags[_impl->_nr_frags++] = {buf.get(), frag.size};
    _impl->_deleter = make_deleter(std::move(_impl->_deleter), [buf = buf.release()] {
        delete[] buf;
    });
}

inline
bool
packet::allocate_headroom(size_t size) {
    if (_impl->_headroom >= size) {
        _impl->_len += size;
        if (!_impl->using_internal_data()) {
            _impl = impl::allocate_if_needed(std::move(_impl), 1);
            std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
                    _impl->_frags + _impl->_nr_frags + 1);
            _impl->_frags[0] = { _impl->_data + internal_data_size, 0 };
            ++_impl->_nr_frags;
        }
        _impl->_headroom -= size;
        _impl->_frags[0].base -= size;
        _impl->_frags[0].size += size;
        return true;
    } else {
        return false;
    }
}


inline
packet::packet(fragment frag, packet&& x)
    : _impl(std::move(x._impl)) {
    // try to prepend into existing internal fragment
    if (allocate_headroom(frag.size)) {
        std::copy(frag.base, frag.base + frag.size, _impl->_frags[0].base);
        return;
    } else {
        // didn't work out, allocate and copy
        _impl->unuse_internal_data();
        _impl = impl::allocate_if_needed(std::move(_impl), 1);
        _impl->_len += frag.size;
        std::unique_ptr<char[]> buf(new char[frag.size]);
        std::copy(frag.base, frag.base + frag.size, buf.get());
        std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
                _impl->_frags + _impl->_nr_frags + 1);
        ++_impl->_nr_frags;
        _impl->_frags[0] = {buf.get(), frag.size};
        _impl->_deleter = make_deleter(std::move(_impl->_deleter),
                [buf = std::move(buf)] {});
    }
}

inline
packet::packet(packet&& x, fragment frag, deleter d)
    : _impl(impl::allocate_if_needed(std::move(x._impl), 1)) {
    _impl->_len += frag.size;
    _impl->_frags[_impl->_nr_frags++] = frag;
    d.append(std::move(_impl->_deleter));
    _impl->_deleter = std::move(d);
}

inline
packet::packet(packet&& x, deleter d)
    : _impl(std::move(x._impl)) {
    _impl->_deleter.append(std::move(d));
}

inline
packet::packet(packet&& x, temporary_buffer<char> buf)
    : packet(std::move(x), fragment{buf.get_write(), buf.size()}, buf.release()) {
}

inline
packet::packet(temporary_buffer<char> buf)
    : packet(fragment{buf.get_write(), buf.size()}, buf.release()) {}

inline
void packet::append(packet&& p) {
    if (!_impl->_len) {
        *this = std::move(p);
        return;
    }
    _impl = impl::allocate_if_needed(std::move(_impl), p._impl->_nr_frags);
    _impl->_len += p._impl->_len;
    p._impl->unuse_internal_data();
    std::copy(p._impl->_frags, p._impl->_frags + p._impl->_nr_frags,
            _impl->_frags + _impl->_nr_frags);
    _impl->_nr_frags += p._impl->_nr_frags;
    p._impl->_deleter.append(std::move(_impl->_deleter));
    _impl->_deleter = std::move(p._impl->_deleter);
}

inline
char* packet::get_header(size_t offset, size_t size) {
    if (offset + size > _impl->_len) {
        return nullptr;
    }
    size_t i = 0;
    while (i != _impl->_nr_frags && offset >= _impl->_frags[i].size) {
        offset -= _impl->_frags[i++].size;
    }
    if (i == _impl->_nr_frags) {
        return nullptr;
    }
    if (offset + size > _impl->_frags[i].size) {
        linearize(i, offset + size);
    }
    return _impl->_frags[i].base + offset;
}

template <typename Header>
inline
Header* packet::get_header(size_t offset) {
    return reinterpret_cast<Header*>(get_header(offset, sizeof(Header)));
}

inline
void packet::trim_front(size_t how_much) {
    assert(how_much <= _impl->_len);
    _impl->_len -= how_much;
    size_t i = 0;
    while (how_much && how_much >= _impl->_frags[i].size) {
        how_much -= _impl->_frags[i++].size;
    }
    std::copy(_impl->_frags + i, _impl->_frags + _impl->_nr_frags, _impl->_frags);
    _impl->_nr_frags -= i;
    if (!_impl->using_internal_data()) {
        _impl->_headroom = internal_data_size;
    }
    if (how_much) {
        if (_impl->using_internal_data()) {
            _impl->_headroom += how_much;
        }
        _impl->_frags[0].base += how_much;
        _impl->_frags[0].size -= how_much;
    }
}

inline
void packet::trim_back(size_t how_much) {
    assert(how_much <= _impl->_len);
    _impl->_len -= how_much;
    size_t i = _impl->_nr_frags - 1;
    while (how_much && how_much >= _impl->_frags[i].size) {
        how_much -= _impl->_frags[i--].size;
    }
    _impl->_nr_frags = i + 1;
    if (how_much) {
        _impl->_frags[i].size -= how_much;
        if (i == 0 && _impl->using_internal_data()) {
            _impl->_headroom += how_much;
        }
    }
}

template <typename Header>
Header*
packet::prepend_header(size_t extra_size) {
    auto h = prepend_uninitialized_header(sizeof(Header) + extra_size);
    return new (h) Header{};
}

// prepend a header (uninitialized!)
inline
char* packet::prepend_uninitialized_header(size_t size) {
    if (!allocate_headroom(size)) {
        // didn't work out, allocate and copy
        _impl->unuse_internal_data();
        // try again, after unuse_internal_data we may have space after all
        if (!allocate_headroom(size)) {
            // failed
            _impl->_len += size;
            _impl = impl::allocate_if_needed(std::move(_impl), 1);
            std::unique_ptr<char[]> buf(new char[size]);
            std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
                    _impl->_frags + _impl->_nr_frags + 1);
            ++_impl->_nr_frags;
            _impl->_frags[0] = {buf.get(), size};
            _impl->_deleter = make_deleter(std::move(_impl->_deleter),
                    [buf = std::move(buf)] {});
        }
    }
    return _impl->_frags[0].base;
}

inline
packet packet::share() {
    return share(0, _impl->_len);
}

inline
packet packet::share(size_t offset, size_t len) {
    _impl->unuse_internal_data(); // FIXME: eliminate?
    packet n;
    n._impl = impl::allocate_if_needed(std::move(n._impl), _impl->_nr_frags);
    size_t idx = 0;
    while (offset > 0 && offset >= _impl->_frags[idx].size) {
        offset -= _impl->_frags[idx++].size;
    }
    while (n._impl->_len < len) {
        auto& f = _impl->_frags[idx++];
        auto fsize = std::min(len - n._impl->_len, f.size - offset);
        n._impl->_frags[n._impl->_nr_frags++] = { f.base + offset, fsize };
        n._impl->_len += fsize;
        offset = 0;
    }
    n._impl->_offload_info = _impl->_offload_info;
    assert(!n._impl->_deleter);
    n._impl->_deleter = _impl->_deleter.share();
    return n;
}

}



class file_desc {
    int _fd;
public:
    file_desc() = delete;
    file_desc(const file_desc&) = delete;
    file_desc(file_desc&& x) : _fd(x._fd) { x._fd = -1; }
    ~file_desc() { if (_fd != -1) { ::close(_fd); } }
    void operator=(const file_desc&) = delete;
    file_desc& operator=(file_desc&& x) {
        if (this != &x) {
            std::swap(_fd, x._fd);
            if (x._fd != -1) {
                x.close();
            }
        }
        return *this;
    }
    void close() {
        assert(_fd != -1);
        auto r = ::close(_fd);
        throw_system_error_on(r == -1, "close");
        _fd = -1;
    }
    int get() const { return _fd; }
    static file_desc open(std::string name, int flags, mode_t mode = 0) {
        int fd = ::open(name.c_str(), flags, mode);
        throw_system_error_on(fd == -1, "open");
        return file_desc(fd);
    }
    static file_desc socket(int family, int type, int protocol = 0) {
        int fd = ::socket(family, type, protocol);
        throw_system_error_on(fd == -1, "socket");
        return file_desc(fd);
    }
    static file_desc eventfd(unsigned initval, int flags) {
        int fd = ::eventfd(initval, flags);
        throw_system_error_on(fd == -1, "eventfd");
        return file_desc(fd);
    }
    static file_desc epoll_create(int flags = 0) {
        int fd = ::epoll_create1(flags);
        throw_system_error_on(fd == -1, "epoll_create1");
        return file_desc(fd);
    }
    static file_desc timerfd_create(int clockid, int flags) {
        int fd = ::timerfd_create(clockid, flags);
        throw_system_error_on(fd == -1, "timerfd_create");
        return file_desc(fd);
    }
    static file_desc temporary(std::string directory);
    file_desc dup() const {
        int fd = ::dup(get());
        throw_system_error_on(fd == -1, "dup");
        return file_desc(fd);
    }
    file_desc accept(sockaddr& sa, socklen_t& sl, int flags = 0) {
        auto ret = ::accept4(_fd, &sa, &sl, flags);
        throw_system_error_on(ret == -1, "accept4");
        return file_desc(ret);
    }
    void shutdown(int how) {
        auto ret = ::shutdown(_fd, how);
        if (ret == -1 && errno != ENOTCONN) {
            throw_system_error_on(ret == -1, "shutdown");
        }
    }
    void truncate(size_t size) {
        auto ret = ::ftruncate(_fd, size);
        throw_system_error_on(ret, "ftruncate");
    }
    int ioctl(int request) {
        return ioctl(request, 0);
    }
    int ioctl(int request, int value) {
        int r = ::ioctl(_fd, request, value);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    int ioctl(int request, unsigned int value) {
        int r = ::ioctl(_fd, request, value);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int ioctl(int request, X& data) {
        int r = ::ioctl(_fd, request, &data);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int ioctl(int request, X&& data) {
        int r = ::ioctl(_fd, request, &data);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int setsockopt(int level, int optname, X&& data) {
        int r = ::setsockopt(_fd, level, optname, &data, sizeof(data));
        throw_system_error_on(r == -1, "setsockopt");
        return r;
    }
    int setsockopt(int level, int optname, const char* data) {
        int r = ::setsockopt(_fd, level, optname, data, strlen(data) + 1);
        throw_system_error_on(r == -1, "setsockopt");
        return r;
    }
    template <typename Data>
    Data getsockopt(int level, int optname) {
        Data data;
        socklen_t len = sizeof(data);
        memset(&data, 0, len);
        int r = ::getsockopt(_fd, level, optname, &data, &len);
        throw_system_error_on(r == -1, "getsockopt");
        return data;
    }
    int getsockopt(int level, int optname, char* data, socklen_t len) {
        int r = ::getsockopt(_fd, level, optname, data, &len);
        throw_system_error_on(r == -1, "getsockopt");
        return r;
    }
    size_t size() {
        struct stat buf;
        auto r = ::fstat(_fd, &buf);
        throw_system_error_on(r == -1, "fstat");
        return buf.st_size;
    }
    std::optional<size_t> read(void* buffer, size_t len) {
        auto r = ::read(_fd, buffer, len);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "read");
        return { size_t(r) };
    }
    std::optional<ssize_t> recv(void* buffer, size_t len, int flags) {
        auto r = ::recv(_fd, buffer, len, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "recv");
        return { ssize_t(r) };
    }
    std::optional<size_t> recvmsg(msghdr* mh, int flags) {
        auto r = ::recvmsg(_fd, mh, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "recvmsg");
        return { size_t(r) };
    }
    std::optional<size_t> send(const void* buffer, size_t len, int flags) {
        auto r = ::send(_fd, buffer, len, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "send");
        return { size_t(r) };
    }
    std::optional<size_t> sendto(socket_address& addr, const void* buf, size_t len, int flags) {
        auto r = ::sendto(_fd, buf, len, flags, &addr.u.sa, sizeof(addr.u.sas));
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "sendto");
        return { size_t(r) };
    }
    std::optional<size_t> sendmsg(const msghdr* msg, int flags) {
        auto r = ::sendmsg(_fd, msg, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "sendmsg");
        return { size_t(r) };
    }
    void bind(sockaddr& sa, socklen_t sl) {
        if(sa.sa_family == AF_INET){

            sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(&sa);
            std::cout << "Bound to: "
                      << inet_ntoa(sin->sin_addr) << ":"
                      << ntohs(sin->sin_port) << std::endl;
        }
        auto r = ::bind(_fd, &sa, sl);
        if (r == -1) {
            std::cerr << "bind failed: "; // 先输出错误前缀
            throw_system_error_on(true, "bind"); // 这会抛出异常并输出错误信息
        } else {
            std::cout << "bind successful" << std::endl;
            // 如果需要，可以打印绑定的地址和端口信息
            if (sa.sa_family == AF_INET) {
                sockaddr_in* sin = reinterpret_cast<sockaddr_in*>(&sa);
                std::cout << "Bound to: "
                          << inet_ntoa(sin->sin_addr) << ":"
                          << ntohs(sin->sin_port) << std::endl;
            }
            // 对于IPv6的情况可以类似处理
        }
    }
    void connect(sockaddr& sa, socklen_t sl) {
        auto r = ::connect(_fd, &sa, sl);
        if (r == -1 && errno == EINPROGRESS) {
            return;
        }
        throw_system_error_on(r == -1, "connect");
    }
    socket_address get_address() {
        socket_address addr;
        auto len = (socklen_t) sizeof(addr.u.sas);
        auto r = ::getsockname(_fd, &addr.u.sa, &len);
        throw_system_error_on(r == -1, "getsockname");
        return addr;
    }
    void listen(int backlog) {
        auto fd = ::listen(_fd, backlog);
        throw_system_error_on(fd == -1, "listen");
    }
    std::optional<size_t> write(const void* buf, size_t len) {
        auto r = ::write(_fd, buf, len);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "write");
        return { size_t(r) };
    }
    std::optional<size_t> writev(const iovec *iov, int iovcnt) {
        auto r = ::writev(_fd, iov, iovcnt);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "writev");
        return { size_t(r) };
    }
    size_t pread(void* buf, size_t len, off_t off) {
        auto r = ::pread(_fd, buf, len, off);
        throw_system_error_on(r == -1, "pread");
        return size_t(r);
    }
    void timerfd_settime(int flags, const itimerspec& its) {
        auto r = ::timerfd_settime(_fd, flags, &its, NULL);
        throw_system_error_on(r == -1, "timerfd_settime");
    }

    mmap_area map(size_t size, unsigned prot, unsigned flags, size_t offset,
            void* addr = nullptr) {
        void *x = mmap(addr, size, prot, flags, _fd, offset);
        throw_system_error_on(x == MAP_FAILED, "mmap");
        return mmap_area(static_cast<char*>(x), mmap_deleter{size});
    }

    mmap_area map_shared_rw(size_t size, size_t offset) {
        return map(size, PROT_READ | PROT_WRITE, MAP_SHARED, offset);
    }

    mmap_area map_shared_ro(size_t size, size_t offset) {
        return map(size, PROT_READ, MAP_SHARED, offset);
    }

    mmap_area map_private_rw(size_t size, size_t offset) {
        return map(size, PROT_READ | PROT_WRITE, MAP_PRIVATE, offset);
    }

    mmap_area map_private_ro(size_t size, size_t offset) {
        return map(size, PROT_READ, MAP_PRIVATE, offset);
    }

private:
    file_desc(int fd) : _fd(fd) {}
 };

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





/*---------------------------------------------------memory相关-----------------------------------------------------*/

namespace memory {

/// \cond internal
// TODO: Use getpagesize() in order to learn a size of a system PAGE.
static constexpr size_t page_bits = 12;
static constexpr size_t page_size = 1 << page_bits;       // 4K
static constexpr size_t huge_page_size = 512 * page_size; // 2M

void configure(std::vector<resource::memory> m,
        std::optional<std::string> hugetlbfs_path = {});

void enable_abort_on_allocation_failure();

class disable_abort_on_alloc_failure_temporarily {
public:
    disable_abort_on_alloc_failure_temporarily();
    ~disable_abort_on_alloc_failure_temporarily() noexcept;
};

void set_heap_profiling_enabled(bool);

enum class reclaiming_result {
    reclaimed_nothing,
    reclaimed_something
};

enum class reclaimer_scope {
    async,
    sync
};

class reclaimer {
public:
    using reclaim_fn = std::function<reclaiming_result ()>;
private:
    reclaim_fn _reclaim;
    reclaimer_scope _scope;
public:
    reclaimer(reclaim_fn reclaim, reclaimer_scope scope = reclaimer_scope::async);
    ~reclaimer();
    reclaiming_result do_reclaim() { return _reclaim(); }
    reclaimer_scope scope() const { return _scope; }
};

bool drain_cross_cpu_freelist();

void set_reclaim_hook(
        std::function<void (std::function<void ()>)> hook);

using physical_address = uint64_t;

struct translation {
    translation() = default;
    translation(physical_address a, size_t s) : addr(a), size(s) {}
    physical_address addr = 0;
    size_t size = 0;
};

translation translate(const void* addr, size_t size);
class statistics;
statistics stats();

/// Memory allocation statistics.
class statistics {
    uint64_t _mallocs;
    uint64_t _frees;
    uint64_t _cross_cpu_frees;
    size_t _total_memory;
    size_t _free_memory;
    uint64_t _reclaims;
private:
    statistics(uint64_t mallocs, uint64_t frees, uint64_t cross_cpu_frees,
            uint64_t total_memory, uint64_t free_memory, uint64_t reclaims)
        : _mallocs(mallocs), _frees(frees), _cross_cpu_frees(cross_cpu_frees)
        , _total_memory(total_memory), _free_memory(free_memory), _reclaims(reclaims) {}
public:
    uint64_t mallocs() const { return _mallocs; }
    uint64_t frees() const { return _frees; }
    uint64_t cross_cpu_frees() const { return _cross_cpu_frees; }
    /// Total number of objects which were allocated but not freed.
    size_t live_objects() const { return mallocs() - frees(); }
    size_t free_memory() const { return _free_memory; }
    /// Total allocated memory (in bytes)
    size_t allocated_memory() const { return _total_memory - _free_memory; }
    /// Total memory (in bytes)
    size_t total_memory() const { return _total_memory; }
    /// Number of reclaims performed due to low memory
    uint64_t reclaims() const { return _reclaims; }
    friend statistics stats();
};

struct memory_layout {
    uintptr_t start;
    uintptr_t end;
};

// Discover virtual address range used by the allocator on current shard.
// Supported only when seastar allocator is enabled.
memory_layout get_memory_layout();

/// Returns the value of free memory low water mark in bytes.
/// When free memory is below this value, reclaimers are invoked until it goes above again.
size_t min_free_memory();

/// Sets the value of free memory low water mark in memory::page_size units.
void set_min_free_pages(size_t pages);
// Memory allocation functions
    void* allocate(size_t size);
    void* allocate_aligned(size_t align, size_t size);
    void* allocate_large(size_t size);
    void* allocate_large_aligned(size_t align, size_t size);
    void free(void* ptr);
    void free(void* ptr, size_t size);
    void free_large(void* ptr);
    size_t object_size(void* ptr);
    void shrink(void* ptr, size_t new_size);

}

class with_alignment {
    size_t _align;
public:
    with_alignment(size_t align) : _align(align) {}
    size_t alignment() const { return _align; }
};

void* operator new(size_t size, with_alignment wa);
void* operator new[](size_t size, with_alignment wa);
void operator delete(void* ptr, with_alignment wa);
void operator delete[](void* ptr, with_alignment wa);



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

class timed_out_error : public std::exception {
public:
    virtual const char* what() const noexcept {
        return "timedout";
    }
};

template<typename T>
struct dummy_expiry {
    void operator()(T&) noexcept {};
};
template<typename... T>
struct promise_expiry {
    void operator()(promise<T...>& pr) noexcept {
        pr.set_exception(std::make_exception_ptr(timed_out_error()));
    };
};
template <typename T, typename OnExpiry = dummy_expiry<T>, typename Clock = lowres_clock>
class expiring_fifo {
public:
    using clock = Clock;
    using time_point = typename Clock::time_point;
private:
    struct entry {
        std::optional<T> payload;
        timer<Clock> tr;
        entry(T&& payload_) : payload(std::move(payload_)) {}
        entry(const T& payload_) : payload(payload_) {}
        entry(T payload_, expiring_fifo& ef, time_point timeout)
                : payload(std::move(payload_))
                , tr([this, &ef] {
                    ef._on_expiry(*payload);
                    payload = std::nullopt;
                    --ef._size;
                    ef.drop_expired_front();
                })
        {
            tr.arm(timeout);
        }
        entry(entry&& x) = delete;
        entry(const entry& x) = delete;
    };
    std::deque<entry> _list;
    OnExpiry _on_expiry;
    size_t _size = 0;
    void drop_expired_front() {
        while (!_list.empty() && !_list.front().payload) {
            _list.pop_front();
        }
    }
public:
    expiring_fifo() = default;
    expiring_fifo(OnExpiry on_expiry) : _on_expiry(std::move(on_expiry)) {}
    bool empty() const {
        return _size == 0;
    }
    explicit operator bool() const {
        return !empty();
    }

    T& front() {
        return *_list.front().payload;
    }

    const T& front() const {
        return *_list.front().payload;
    }

    size_t size() const {
        return _size;
    }
    void reserve(size_t size) {
        return _list.reserve(size);
    }
    void push_back(const T& payload) {
        _list.emplace_back(payload);
        ++_size;
    }
    void push_back(T&& payload) {
        _list.emplace_back(std::move(payload));
        ++_size;
    }
    void push_back(T payload, time_point timeout) {
        if (timeout < time_point::max()) {
            _list.emplace_back(std::move(payload), *this, timeout);
        } else {
            _list.emplace_back(std::move(payload));
        }
        ++_size;
    }
    void pop_front() {
        _list.pop_front();
        --_size;
        drop_expired_front();
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








class broken_semaphore : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Semaphore broken";
    }
};
class semaphore_timed_out : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Semaphore timedout";
    }
};
struct semaphore_default_exception_factory {
    static semaphore_timed_out timeout() {
        return semaphore_timed_out();
    }
    static broken_semaphore broken() {
        return broken_semaphore();
    }
};


template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
class basic_semaphore {
public:
    using duration = typename timer<Clock>::duration;
    using clock = typename timer<Clock>::clock;
    using time_point = typename timer<Clock>::time_point;//这里类似类型萃取.
private:
    ssize_t _count;
    std::exception_ptr _ex;
    struct entry {
        promise<> pr;
        size_t nr;
        entry(promise<>&& pr_, size_t nr_) : pr(std::move(pr_)), nr(nr_) {}
    };
    struct expiry_handler {
        void operator()(entry& e) noexcept {
            e.pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout()));
        }
    };
    expiring_fifo<entry, expiry_handler, clock> _wait_list;
    bool has_available_units(size_t nr) const {
        return _count >= 0 && (static_cast<size_t>(_count) >= nr);
    }
    bool may_proceed(size_t nr) const {
        return has_available_units(nr) && _wait_list.empty();
    }
public:
    static constexpr size_t max_counter() {
        return std::numeric_limits<decltype(_count)>::max();
    }
    basic_semaphore(size_t count) : _count(count) {}
    future<> wait(size_t nr = 1) {
        return wait(time_point::max(), nr);
    }

    future<> wait(duration timeout, size_t nr = 1) {
        return wait(Clock::now() + timeout, nr);
    }
    future<> wait(time_point timeout, size_t nr = 1) {
        if (may_proceed(nr)) {
            _count -= nr;
            return make_ready_future<>();
        }
        if (_ex) {
            return make_exception_future(_ex);
        }
        promise<> pr;
        auto fut = pr.get_future();
        _wait_list.push_back(entry(std::move(pr), nr), timeout);
        return fut;
    }
     void signal(size_t nr = 1) {
        if (_ex) {
            return;
        }
        _count += nr;
        while (!_wait_list.empty() && has_available_units(_wait_list.front().nr)) {
            auto& x = _wait_list.front();
            _count -= x.nr;
            x.pr.set_value();
            _wait_list.pop_front();
        }
    }

    void consume(size_t nr = 1) {
        if (_ex) {
            return;
        }
        _count -= nr;
    }
    bool try_wait(size_t nr = 1) {
        if (may_proceed(nr)) {
            _count -= nr;
            return true;
        } else {
            return false;
        }
    }
    size_t current() const { return std::max(_count, ssize_t(0)); }
    ssize_t available_units() const { return _count; }
    size_t waiters() const { return _wait_list.size(); }
    void broken() { broken(std::make_exception_ptr(ExceptionFactory::broken())); }
    template <typename Exception>
    void broken(const Exception& ex) {
        broken(std::make_exception_ptr(ex));
    }
    void broken(std::exception_ptr ex);
    void ensure_space_for_waiters(size_t n) {
        _wait_list.reserve(n);
    }
};

template<typename ExceptionFactory = semaphore_default_exception_factory, typename Clock = typename timer<>::clock>
class semaphore_units {
    basic_semaphore<ExceptionFactory, Clock>& _sem;
    size_t _n;
public:
    semaphore_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t n) noexcept : _sem(sem), _n(n) {}
    semaphore_units(semaphore_units&& o) noexcept : _sem(o._sem), _n(o._n) {
        o._n = 0;
    }
    semaphore_units& operator=(semaphore_units&& o) noexcept {
        if (this != &o) {
            this->~semaphore_units();
            new (this) semaphore_units(std::move(o));
        }
        return *this;
    }
    semaphore_units(const semaphore_units&) = delete;
    ~semaphore_units() noexcept {
        if (_n) {
            _sem.signal(_n);
        }
    }
    /// Releases ownership of the units. The semaphore will not be signalled.
    ///
    /// \return the number of units held
    size_t release() {
        return std::exchange(_n, 0);
    }
};

using semaphore = basic_semaphore<semaphore_default_exception_factory>;

class broken_condition_variable : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Condition variable is broken";
    }
};

class condition_variable_timed_out : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Condition variable timed out";
    }
};


class condition_variable {
    using duration = semaphore::duration;
    using clock = semaphore::clock;
    using time_point = semaphore::time_point;
    struct condition_variable_exception_factory {
        static condition_variable_timed_out timeout() {
            return condition_variable_timed_out();
        }
        static broken_condition_variable broken() {
            return broken_condition_variable();
        }
    };
    basic_semaphore<condition_variable_exception_factory> _sem;
public:
    condition_variable() : _sem(0) {}
    future<> wait() {
        return _sem.wait();
    }

    future<> wait(time_point timeout) {
        return _sem.wait(timeout);
    }
    future<> wait(duration timeout) {
        return _sem.wait(timeout);
    }
    template<typename Pred>
    future<> wait(Pred&& pred) {
        return do_until(std::forward<Pred>(pred), [this] {
            return wait();
        });
    }
    template<typename Pred>
    future<> wait(time_point timeout, Pred&& pred) {
        return do_until(std::forward<Pred>(pred), [this, timeout] () mutable {
            return wait(timeout);
        });
    }
    template<typename Pred>
    future<> wait(duration timeout, Pred&& pred) {
        return wait(clock::now() + timeout, std::forward<Pred>(pred));
    }
    void signal() {
        if (_sem.waiters()) {
            _sem.signal();
        }
    }
    void broadcast() {
        _sem.signal(_sem.waiters());
    }
    void broken() {
        _sem.broken();
    }
};

class priority_class {
    struct request {
        promise<> pr;
        unsigned weight;
    };
    friend class fair_queue;
    uint32_t _shares = 0;
    float _accumulated = 0;
    std::deque<request> _queue;
    bool _queued = false;
    // friend struct shared_ptr_no_esft<priority_class>;
    explicit priority_class(uint32_t shares) : _shares(shares) {}
};
using priority_class_ptr = std::shared_ptr<priority_class>;


class fair_queue {
    friend priority_class;
    struct class_compare {
        bool operator() (const priority_class_ptr& lhs, const priority_class_ptr& rhs) const {
            return lhs->_accumulated > rhs->_accumulated;
        }
    };
    semaphore _sem;
    unsigned _capacity;
    using clock_type = std::chrono::steady_clock::time_point;
    clock_type _base;
    std::chrono::microseconds _tau;
    using prioq = std::priority_queue<priority_class_ptr, std::vector<priority_class_ptr>, class_compare>;
    prioq _handles;
    std::unordered_set<priority_class_ptr> _all_classes;
    void push_priority_class(priority_class_ptr pc) {
        if (!pc->_queued) {
            _handles.push(pc);
            pc->_queued = true;
        }
    }
    priority_class_ptr pop_priority_class() {
        assert(!_handles.empty());
        auto h = _handles.top();
        _handles.pop();
        assert(h->_queued);
        h->_queued = false;
        return h;
    }
    void execute_one() {
        _sem.wait().then([this] {
            priority_class_ptr h;
            do {
                h = pop_priority_class();
            } while (h->_queue.empty());

            auto req = std::move(h->_queue.front());
            h->_queue.pop_front();
            req.pr.set_value();
            auto delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _base);
            auto req_cost  = float(req.weight) / h->_shares;
            auto cost  = expf(1.0f/_tau.count() * delta.count()) * req_cost;
            float next_accumulated = h->_accumulated + cost;
            while (std::isinf(next_accumulated)) {
                normalize_stats();
                // If we have renormalized, our time base will have changed. This should happen very infrequently
                delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _base);
                cost  = expf(1.0f/_tau.count() * delta.count()) * req_cost;
                next_accumulated = h->_accumulated + cost;
            }
            h->_accumulated = next_accumulated;
            if (!h->_queue.empty()) {
                push_priority_class(h);
            }
            return make_ready_future<>();
        });
    }
    float normalize_factor() const {
        return std::numeric_limits<float>::min();
    }
    void normalize_stats() {
        auto time_delta = std::log(normalize_factor()) * _tau;
        // time_delta is negative; and this may advance _base into the future
        _base -= std::chrono::duration_cast<clock_type::duration>(time_delta);
        for (auto& pc: _all_classes) {
            pc->_accumulated *= normalize_factor();
        }
    }
public:
    explicit fair_queue(unsigned capacity, std::chrono::microseconds tau = std::chrono::milliseconds(100))
                                           : _sem(capacity)
                                           , _capacity(capacity)
                                           , _base(std::chrono::steady_clock::now())
                                           , _tau(tau) {
    }
    priority_class_ptr register_priority_class(uint32_t shares) {
        priority_class_ptr pclass = std::make_shared<priority_class>(shares);
        _all_classes.insert(pclass);
        return pclass;
    }
    void unregister_priority_class(priority_class_ptr pclass) {
        assert(pclass->_queue.empty());
        _all_classes.erase(pclass);
    }
    size_t waiters() const {
        return _sem.waiters();
    }

    template <typename Func>
    futurize_t<std::result_of_t<Func()>> queue(priority_class_ptr pc, unsigned weight, Func func) {
        // We need to return a future in this function on which the caller can wait.
        // Since we don't know which queue we will use to execute the next request - if ours or
        // someone else's, we need a separate promise at this point.
        promise<> pr;
        auto fut = pr.get_future();

        push_priority_class(pc);
        pc->_queue.push_back(priority_class::request{std::move(pr), weight});
        try {
            execute_one();
        } catch (...) {
            pc->_queue.pop_back();
            throw;
        }
        return fut.then([func = std::move(func)] {
            return func();
        }).finally([this] {
            _sem.signal();
        });
    }

    /// Updates the current shares of this priority class
    ///
    /// \param new_shares the new number of shares for this priority class
    static void update_shares(priority_class_ptr pc, uint32_t new_shares) {
        pc->_shares = new_shares;
    }
};

class io_queue;
class io_priority_class {
    unsigned val;
    friend io_queue;
public:
    unsigned id() const {
        return val;
    }
};

class smp;
class io_queue {
private:
    shard_id _coordinator;
    size_t _capacity;
    std::vector<shard_id> _io_topology;
    struct priority_class_data {
        priority_class_ptr ptr;
        size_t bytes;
        uint64_t ops;
        uint32_t nr_queued;
        std::chrono::duration<double> queue_time;
        // metrics::metric_groups _metric_groups;
        priority_class_data(std::string name, priority_class_ptr ptr, shard_id owner);
    };
    std::unordered_map<unsigned, std::shared_ptr<priority_class_data>> _priority_classes;
    fair_queue _fq;
    static constexpr unsigned _max_classes = 1024;
    static std::array<std::atomic<uint32_t>, _max_classes> _registered_shares;
    static std::array<std::string, _max_classes> _registered_names;
    static io_priority_class register_one_priority_class(std::string name, uint32_t shares);
    priority_class_data& find_or_create_class(const io_priority_class& pc, shard_id owner);
    static void fill_shares_array();
    friend smp;
public:
    io_queue(shard_id coordinator, size_t capacity, std::vector<shard_id> topology);
    ~io_queue();
    template <typename Func>
    static future<io_event>
    queue_request(shard_id coordinator, const io_priority_class& pc, size_t len, Func do_io);
    size_t capacity() const {
        return _capacity;
    }
    size_t queued_requests() const {
        return _fq.waiters();
    }

    shard_id coordinator() const {
        return _coordinator;
    }
    shard_id coordinator_of_shard(shard_id shard) const {
        return _io_topology[shard];
    }
    friend class reactor;
};
std::array<std::atomic<uint32_t>, io_queue::_max_classes> io_queue::_registered_shares;
std::array<std::string, io_queue::_max_classes> io_queue::_registered_names;
io_queue::io_queue(shard_id coordinator, size_t capacity, std::vector<shard_id> topology)
        : _coordinator(coordinator)
        , _capacity(capacity)
        , _io_topology(std::move(topology))
        , _priority_classes()
        , _fq(capacity) {
}

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
    , queue_time(1s){}

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
/*---------------------------------------------------socket相关----------------------------------------------------------------*/



namespace net { class packet; }

class data_source_impl {
public:
    virtual ~data_source_impl() {}
    virtual future<temporary_buffer<char>> get() = 0;
    virtual future<temporary_buffer<char>> skip(uint64_t n);
    virtual future<> close() { return make_ready_future<>(); }
};

class data_source {
    std::unique_ptr<data_source_impl> _dsi;
protected:
    data_source_impl* impl() const { return _dsi.get(); }
public:
    data_source() = default;
    explicit data_source(std::unique_ptr<data_source_impl> dsi) : _dsi(std::move(dsi)) {}
    data_source(data_source&& x) = default;
    data_source& operator=(data_source&& x) = default;
    future<temporary_buffer<char>> get() { return _dsi->get(); }
    future<temporary_buffer<char>> skip(uint64_t n) { return _dsi->skip(n); }
    future<> close() { return _dsi->close(); }
};

class data_sink_impl {
public:
    virtual ~data_sink_impl() {}
    virtual temporary_buffer<char> allocate_buffer(size_t size) {
        return temporary_buffer<char>(size);
    }
    virtual future<> put(net::packet data) = 0;
    virtual future<> put(std::vector<temporary_buffer<char>> data) {
        net::packet p;
        p.reserve(data.size());
        for (auto& buf : data) {
            p = net::packet(std::move(p), net::fragment{buf.get_write(), buf.size()}, buf.release());
        }
        return put(std::move(p));
    }
    virtual future<> put(temporary_buffer<char> buf) {
        return put(net::packet(net::fragment{buf.get_write(), buf.size()}, buf.release()));
    }
    virtual future<> flush() {
        return make_ready_future<>();
    }
    virtual future<> close() = 0;
};

class data_sink {
    std::unique_ptr<data_sink_impl> _dsi;
public:
    data_sink() = default;
    explicit data_sink(std::unique_ptr<data_sink_impl> dsi) : _dsi(std::move(dsi)) {}
    data_sink(data_sink&& x) = default;
    data_sink& operator=(data_sink&& x) = default;
    temporary_buffer<char> allocate_buffer(size_t size) {
        return _dsi->allocate_buffer(size);
    }
    future<> put(std::vector<temporary_buffer<char>> data) {
        return _dsi->put(std::move(data));
    }
    future<> put(temporary_buffer<char> data) {
        return _dsi->put(std::move(data));
    }
    future<> put(net::packet p) {
        return _dsi->put(std::move(p));
    }
    future<> flush() {
        return _dsi->flush();
    }
    future<> close() { return _dsi->close(); }
};




template <typename CharType>
class scattered_message {
private:
    using fragment = net::fragment;
    using packet = net::packet;
    using char_type = CharType;
    packet _p;
public:
    scattered_message() {}
    scattered_message(scattered_message&&) = default;
    scattered_message(const scattered_message&) = delete;

    void append_static(const char_type* buf, size_t size) {
        if (size) {
            _p = packet(std::move(_p), fragment{(char_type*)buf, size}, deleter());
        }
    }

    template <size_t N>
    void append_static(const char_type(&s)[N]) {
        append_static(s, N - 1);
    }

    void append_static(const char_type* s) {
        append_static(s, strlen(s));
    }

    template <typename size_type, size_type max_size>
    void append_static(const basic_sstring<char_type, size_type, max_size>& s) {
        append_static(s.begin(), s.size());
    }

    void append_static(const std::string_view& s) {
        append_static(s.data(), s.size());
    }

    template <typename size_type, size_type max_size>
    void append(basic_sstring<char_type, size_type, max_size> s) {
        if (s.size()) {
            _p = packet(std::move(_p), std::move(s).release());
        }
    }

    template <typename size_type, size_type max_size, typename Callback>
    void append(const basic_sstring<char_type, size_type, max_size>& s, Callback callback) {
        if (s.size()) {
            _p = packet(std::move(_p), fragment{s.begin(), s.size()}, make_deleter(std::move(callback)));
        }
    }

    void reserve(int n_frags) {
        _p.reserve(n_frags);
    }

    packet release() && {
        return std::move(_p);
    }

    template <typename Callback>
    void on_delete(Callback callback) {
        _p = packet(std::move(_p), make_deleter(std::move(callback)));
    }

    operator bool() const {
        return _p.len();
    }

    size_t size() {
        return _p.len();
    }
};







template <typename CharType>
class input_stream final {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    data_source _fd;
    temporary_buffer<CharType> _buf;
    bool _eof = false;
private:
    using tmp_buf = temporary_buffer<CharType>;
    size_t available() const { return _buf.size(); }
protected:
    void reset() { _buf = {}; }
    data_source* fd() { return &_fd; }
public:
    // Consumer concept, for consume() method:
    using unconsumed_remainder = std::optional<tmp_buf>;
    struct ConsumerConcept {
        // The consumer should operate on the data given to it, and
        // return a future "unconsumed remainder", which can be undefined
        // if the consumer consumed all the input given to it and is ready
        // for more, or defined when the consumer is done (and in that case
        // the value is the unconsumed part of the last data buffer - this
        // can also happen to be empty).
        future<unconsumed_remainder> operator()(tmp_buf data);
    };
    using char_type = CharType;
    input_stream() = default;
    explicit input_stream(data_source fd) : _fd(std::move(fd)), _buf(0) {}
    input_stream(input_stream&&) = default;
    input_stream& operator=(input_stream&&) = default;
    future<temporary_buffer<CharType>> read_exactly(size_t n);
    template <typename Consumer>
    future<> consume(Consumer& c);
    bool eof() { return _eof; }
    /// Returns some data from the stream, or an empty buffer on end of
    /// stream.
    future<tmp_buf> read();
    /// Returns up to n bytes from the stream, or an empty buffer on end of
    /// stream.
    future<tmp_buf> read_up_to(size_t n);
    /// Detaches the \c input_stream from the underlying data source.
    ///
    /// Waits for any background operations (for example, read-ahead) to
    /// complete, so that the any resources the stream is using can be
    /// safely destroyed.  An example is a \ref file resource used by
    /// the stream returned by make_file_input_stream().
    ///
    /// \return a future that becomes ready when this stream no longer
    ///         needs the data source.
    future<> close() {
        return _fd.close();
    }
    /// Ignores n next bytes from the stream.
    future<> skip(uint64_t n);
private:
    future<temporary_buffer<CharType>> read_exactly_part(size_t n, tmp_buf buf, size_t completed);
};

// Facilitates data buffering before it's handed over to data_sink.
// When trim_to_size is true it's guaranteed that data sink will not receive
// chunks larger than the configured size, which could be the case when a
// single write call is made with data larger than the configured size.
// The data sink will not receive empty chunks.

template <typename CharType>
class output_stream final {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    data_sink _fd;
    temporary_buffer<CharType> _buf;
    net::packet _zc_bufs = net::packet::make_null_packet(); //zero copy buffers
    size_t _size = 0;
    size_t _begin = 0;
    size_t _end = 0;
    bool _trim_to_size = false;
    bool _batch_flushes = false;
    std::optional<promise<>> _in_batch;
    bool _flush = false;
    bool _flushing = false;
    std::exception_ptr _ex;
private:
    size_t available() const { return _end - _begin; }
    size_t possibly_available() const { return _size - _begin; }
    future<> split_and_put(temporary_buffer<CharType> buf);
    future<> put(temporary_buffer<CharType> buf);
    void poll_flush();
    future<> zero_copy_put(net::packet p);
    future<> zero_copy_split_and_put(net::packet p);
public:
    using char_type = CharType;
    output_stream() = default;
    output_stream(data_sink fd, size_t size, bool trim_to_size = false, bool batch_flushes = false)
        : _fd(std::move(fd)), _size(size), _trim_to_size(trim_to_size), _batch_flushes(batch_flushes) {}
    output_stream(output_stream&&) = default;
    output_stream& operator=(output_stream&&) = default;
    ~output_stream() { assert(!_in_batch); }
    future<> write(const char_type* buf, size_t n);
    future<> write(const char_type* buf);

    template <typename StringChar, typename SizeType, SizeType MaxSize>
    future<> write(const basic_sstring<StringChar, SizeType, MaxSize>& s);
    future<> write(const std::basic_string<char_type>& s);

    future<> write(net::packet p);
    future<> write(scattered_message<char_type> msg);
    future<> write(temporary_buffer<char_type>);
    future<> flush();
    future<> close();
private:
    friend class reactor;
};










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

static inline
socket_address make_ipv4_address(ipv4_addr addr) {
    socket_address sa;
    sa.u.in.sin_family = AF_INET;
    sa.u.in.sin_port = htons(addr.port);
    sa.u.in.sin_addr.s_addr = htonl(addr.ip);
    return sa;
}
inline
socket_address make_ipv4_address(uint32_t ip, uint16_t port) {
    socket_address sa;
    sa.u.in.sin_family = AF_INET;
    sa.u.in.sin_port = htons(port);
    sa.u.in.sin_addr.s_addr = htonl(ip);
    return sa;
}




namespace net {

// see linux tcp(7) for parameter explanation
struct tcp_keepalive_params {
    std::chrono::seconds idle; // TCP_KEEPIDLE
    std::chrono::seconds interval; // TCP_KEEPINTVL
    unsigned count; // TCP_KEEPCNT
};

// see linux sctp(7) for parameter explanation
struct sctp_keepalive_params {
    std::chrono::seconds interval; // spp_hbinterval
    unsigned count; // spp_pathmaxrt
};

using keepalive_params = std::variant<tcp_keepalive_params, sctp_keepalive_params>;

class connected_socket_impl;
class socket_impl;
class server_socket_impl;
class udp_channel_impl;
class get_impl;


class udp_datagram_impl {
public:
    virtual ~udp_datagram_impl() {};
    virtual ipv4_addr get_src() = 0;
    virtual ipv4_addr get_dst() = 0;
    virtual uint16_t get_dst_port() = 0;
    virtual packet& get_data() = 0;
};

class udp_datagram final {
private:
    std::unique_ptr<udp_datagram_impl> _impl;
public:
    udp_datagram(std::unique_ptr<udp_datagram_impl>&& impl) : _impl(std::move(impl)) {};
    ipv4_addr get_src() { return _impl->get_src(); }
    ipv4_addr get_dst() { return _impl->get_dst(); }
    uint16_t get_dst_port() { return _impl->get_dst_port(); }
    packet& get_data() { return _impl->get_data(); }
};
class udp_channel_impl {
public:
        virtual ~udp_channel_impl() {};
        virtual future<udp_datagram> receive() = 0;
        virtual future<> send(ipv4_addr dst, const char* msg) = 0;
        virtual future<> send(ipv4_addr dst, packet p) = 0;
        virtual bool is_closed() const = 0;
        virtual void close() = 0;
};
class posix_udp_channel;
class udp_channel {
public:
    std::unique_ptr<udp_channel_impl> _impl;
    udp_channel();
    udp_channel(std::unique_ptr<udp_channel_impl>);
    ~udp_channel();
    udp_channel(udp_channel&&);
    udp_channel& operator=(udp_channel&&);
    future<udp_datagram> receive();
    future<> send(ipv4_addr dst, const char* msg);
    future<> send(ipv4_addr dst, packet p);
    bool is_closed() const;
    void close();
};
}


class connected_socket {
    friend class net::get_impl;
    std::unique_ptr<net::connected_socket_impl> _csi;
public:
    connected_socket();
    ~connected_socket();
    explicit connected_socket(std::unique_ptr<net::connected_socket_impl> csi);
    connected_socket(connected_socket&& cs) noexcept;
    connected_socket& operator=(connected_socket&& cs) noexcept;
    input_stream<char> input();
    output_stream<char> output(size_t buffer_size = 8192);
    void set_nodelay(bool nodelay);
    bool get_nodelay() const;
    void set_keepalive(bool keepalive);
    bool get_keepalive() const;
    void set_keepalive_parameters(const net::keepalive_params& p);
    net::keepalive_params get_keepalive_parameters() const;
    void shutdown_output();
    void shutdown_input();
};

namespace net{
class socket {
    std::unique_ptr<::net::socket_impl> _si;
public:
    ~socket();
    explicit socket(std::unique_ptr<::net::socket_impl> si);
    socket(socket&&) noexcept;
    socket& operator=(socket&&) noexcept;
    future<connected_socket> connect(socket_address sa, socket_address local = socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}}),transport proto = transport::TCP);
    void shutdown();
};
}

class server_socket {
    std::unique_ptr<net::server_socket_impl> _ssi;
public:
    server_socket();
    explicit server_socket(std::unique_ptr<net::server_socket_impl> ssi);
    server_socket(server_socket&& ss) noexcept;
    ~server_socket();
    /// Move-assigns a \c server_socket object.
    server_socket& operator=(server_socket&& cs) noexcept;
    future<connected_socket, socket_address> accept();
    void abort_accept();
};

class network_stack
{
public:
    virtual void print_stack(){}

    virtual ~network_stack() {}
    virtual server_socket listen(socket_address sa, listen_options opts) = 0;
    // FIXME: local parameter assumes ipv4 for now, fix when adding other AF
    future<connected_socket> connect(socket_address sa, socket_address local = socket_address(::sockaddr_in{AF_INET, INADDR_ANY, {0}}), transport proto = transport::TCP) {
        return socket().connect(sa, local, proto);
    }
    virtual net::socket socket() = 0;
    virtual ::net::udp_channel make_udp_channel(ipv4_addr addr = {}) = 0;
    virtual future<> initialize() {
        return make_ready_future();
    }
    virtual bool has_per_core_namespace() = 0;
};


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


namespace net {

/// \cond internal
class connected_socket_impl {
public:
    virtual ~connected_socket_impl() {}
    virtual data_source source() = 0;
    virtual data_sink sink() = 0;
    virtual void shutdown_input() = 0;
    virtual void shutdown_output() = 0;
    virtual void set_nodelay(bool nodelay) = 0;
    virtual bool get_nodelay() const = 0;
    virtual void set_keepalive(bool keepalive) = 0;
    virtual bool get_keepalive() const = 0;
    virtual void set_keepalive_parameters(const keepalive_params&) = 0;
    virtual keepalive_params get_keepalive_parameters() const = 0;
};

class socket_impl {
public:
    virtual ~socket_impl() {}
    virtual future<connected_socket> connect(socket_address sa, socket_address local, transport proto = transport::TCP) = 0;
    virtual void shutdown() = 0;
};

class server_socket_impl {
public:
    virtual ~server_socket_impl() {}
    virtual future<connected_socket, socket_address> accept() = 0;
    virtual void abort_accept() = 0;
};



/// \endcond

}



/*-------------------------------------------------reactor类定义----------------------------------------------------------------*/
#include "../resource/resource.hh"
#include <boost/thread/barrier.hpp>
#include <boost/range/irange.hpp>
#include <sys/syscall.h>

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
    future<pollable_fd, socket_address> accept();
    future<size_t> sendmsg(struct msghdr *msg);
    future<size_t> recvmsg(struct msghdr *msg);
    future<size_t> sendto(socket_address addr, const void* buf, size_t len);
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

#include <boost/lockfree/spsc_queue.hpp>

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
output_stream<char> make_file_output_stream(
        file file,
        uint64_t buffer_size = 8192);

/// Create an output_stream for writing starting at the position zero of a
/// newly created file.
/// NOTE: flush() should be the last thing to be called on a file output stream.
output_stream<char> make_file_output_stream(
        file file,
        file_output_stream_options options);















/*---------------------------------------------file end------------------------------------------------------------------*/



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
    timer_set<timer<manual_clock>> _manual_timers;
    typename timer_set<timer<manual_clock>>::timer_list_t _expired_manual_timers;
    using steady_timer = timer<steady_clock_type>;
    using lowres_timer = timer<lowres_clock>;
    using manual_timer = timer<manual_clock>;
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
    void add_timer(manual_timer* tmr);
    bool queue_timer(manual_timer* tmr);
    void del_timer(manual_timer* tmr);
    void enable_timer(steady_clock_type::time_point when);
    bool do_expire_lowres_timers();
    template <typename T, typename E, typename EnableFunc>
    void complete_timers(T&, E&, EnableFunc&& enable_fn);
    bool do_check_lowres_timers() const;
    void expire_manual_timers();


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
    /*----------------------------------IO相关--------------------------------------------*/
    std::deque<output_stream<char>* > _flush_batching;
    io_context_t _io_context;
    std::vector<struct ::iocb> _pending_aio;
    bool process_io();
    void start_epoll();
    void start_aio_eventfd_loop();
    server_socket listen(socket_address sa, listen_options opts = {});
    future<connected_socket> connect(socket_address sa);
    future<connected_socket> connect(socket_address, socket_address, transport proto = transport::TCP);
    pollable_fd posix_listen(socket_address sa, listen_options opts = {});
    bool posix_reuseport_available() const { return _reuseport; }
    std::shared_ptr<pollable_fd> make_pollable_fd(socket_address sa, transport proto = transport::TCP);
    future<> posix_connect(std::shared_ptr<pollable_fd> pfd, socket_address sa, socket_address local);
    future<pollable_fd, socket_address> accept(pollable_fd_state& listen_fd);
    future<size_t> read_some(pollable_fd_state& fd, void* buffer, size_t size);
    future<size_t> read_some(pollable_fd_state& fd, const std::vector<iovec>& iov);
    future<size_t> write_some(pollable_fd_state& fd, const void* buffer, size_t size);
    future<> write_all(pollable_fd_state& fd, const void* buffer, size_t size);
    future<file> open_file_dma(std::string name, open_flags flags, file_open_options options = {});
    future<file> open_directory(std::string name);
    future<> make_directory(std::string name);
    future<> touch_directory(std::string name);
    future<std::optional<directory_entry_type>> cfile_type(std::string name);
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


__thread reactor* local_engine;
reactor& engine(){
    return *local_engine;
}



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
        _read_buffers.reserve(ra); // prevent push_back() failure
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
        socket_address addr;
        connection(pollable_fd xfd, socket_address xaddr) : fd(std::move(xfd)), addr(xaddr) {}
    };
    static auto &get_sockets() {
        static thread_local std::unordered_map<::sockaddr_in, promise<connected_socket, socket_address>,sockaddr_in_hash,sockaddr_in_equal> sockets;
        return sockets;
    }
  //  static thread_local std::unordered_map<::sockaddr_in, promise<connected_socket, socket_address>,sockaddr_in_hash,sockaddr_in_equal> sockets;
    static auto &get_conn_q() {
        static thread_local std::unordered_multimap<::sockaddr_in, connection,sockaddr_in_hash,sockaddr_in_equal> conn_q;
        return conn_q;
    }
    // static thread_local std::unordered_map<::sockaddr_in, promise<connected_socket, socket_address>,sockaddr_in_hash,sockaddr_in_equal> sockets;
    // static thread_local std::unordered_multimap<::sockaddr_in, connection,sockaddr_in_hash,sockaddr_in_equal> conn_q;
    socket_address _sa;
public:
    explicit posix_ap_server_socket_impl(socket_address sa) : _sa(sa) {}
    virtual future<connected_socket, socket_address> accept() override;
    virtual void abort_accept() override;
    static void move_connected_socket(socket_address sa, pollable_fd fd, socket_address addr);
};
using posix_tcp_ap_server_socket_impl = posix_ap_server_socket_impl<transport::TCP>;
using posix_sctp_ap_server_socket_impl = posix_ap_server_socket_impl<transport::SCTP>;



template <transport Transport>
class posix_server_socket_impl : public server_socket_impl {
    socket_address _sa;
    pollable_fd _lfd;
public:
    explicit posix_server_socket_impl(socket_address sa, pollable_fd lfd) : _sa(sa), _lfd(std::move(lfd)) {}
    virtual future<connected_socket, socket_address> accept();
    virtual void abort_accept() override;
};
using posix_server_tcp_socket_impl = posix_server_socket_impl<transport::TCP>;
using posix_server_sctp_socket_impl = posix_server_socket_impl<transport::SCTP>;

template <transport Transport>
class posix_reuseport_server_socket_impl : public server_socket_impl {
    socket_address _sa;
    pollable_fd _lfd;
public:
    explicit posix_reuseport_server_socket_impl(socket_address sa, pollable_fd lfd) : _sa(sa), _lfd(std::move(lfd)) {}
    virtual future<connected_socket, socket_address> accept();
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
    virtual server_socket listen(socket_address sa, listen_options opts) override;
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
    virtual server_socket listen(socket_address sa, listen_options opts) override;
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
                return make_ready_future();
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

void add_timer(timer<manual_clock>* tmr) {
     engine().add_timer(tmr);
}

bool queue_timer(timer<manual_clock>* tmr) {
    return engine().queue_timer(tmr);
}

bool queue_timer(timer<lowres_clock>* tmr) {
    return engine().queue_timer(tmr);
}


void manual_clock::advance(manual_clock::duration d) {
    _now.fetch_add(d.count());
    // engine().schedule_urgent(make_task(&manual_clock::expire_timers));
    //smp::invoke_on_all(&manual_clock::expire_timers);
    return;
}

void del_timer(timer<lowres_clock>* tmr) {
    engine().del_timer(tmr);
}

void del_timer(timer<steady_clock_type>* tmr) {
    engine().del_timer(tmr);
}
void del_timer(timer<manual_clock>* tmr) {
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
template <class... T>
class promise;
template <class... T>
class future;
template<>
class promise<void>;

template <typename... T, typename... A>
future<T...> make_ready_future(A&&... value);

template <typename... T>
future<T...> make_exception_future(std::exception_ptr value) noexcept;

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
future<pollable_fd, socket_address> pollable_fd::accept() {
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
future<size_t> pollable_fd::sendto(socket_address addr, const void* buf, size_t len) {
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
struct stop_iteration_tag { };
using stop_iteration = bool_class<stop_iteration_tag>;


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
/// Executes a callable in a seastar thread.
/// Runs a block of code in a threaded context,
/// which allows it to block (using \ref future::get()).  The
/// result of the callable is returned as a future.
/// \param func a callable to be executed in a thread
/// \param args a parameter pack to be forwarded to \c func.
/// \return whatever \c func returns, as a future.
/// Clock used for scheduling threads
using thread_clock = std::chrono::steady_clock;
struct thread_attributes {
        thread_scheduling_group* scheduling_group = nullptr;
};

class thread_scheduling_group {
    public:
        std::chrono::nanoseconds _period;
        std::chrono::nanoseconds _quota;
        std::chrono::time_point<thread_clock> _this_period_ends = {};
        std::chrono::time_point<thread_clock> _this_run_start = {};
        std::chrono::nanoseconds _this_period_remain = {};
        /// \brief Constructs a \c thread_scheduling_group object
        ///
        /// \param period a duration representing the period
        /// \param usage which fraction of the \c period to assign for the scheduling group. Expected between 0 and 1.
        thread_scheduling_group(std::chrono::nanoseconds period, float usage);
        /// \brief changes the current maximum usage per period
        ///
        /// \param new_usage The new fraction of the \c period (Expected between 0 and 1) during which to run
        void update_usage(float new_usage) {
            _quota = std::chrono::duration_cast<std::chrono::nanoseconds>(new_usage * _period);
        }
        void account_start();
        void account_stop();
        std::chrono::steady_clock::time_point* next_scheduling_point() const;
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

thread_local jmp_buf_link g_unthreaded_context; //在jmp_buf_link init_switch_in的时候用来初始化g_current_context
thread_local jmp_buf_link* g_current_context;

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
    inline thread_context* get() {
        return g_current_context->thread;
    }
    inline bool should_yield() {
        if (need_preempt()) {
            return true;
        } else if (g_current_context->get_yield_at()) {
            return std::chrono::steady_clock::now() >= *(g_current_context->get_yield_at());
        } else {
            return false;
        }
    }
    void yield(){
        g_current_context->thread->yield();
    }
    void switch_in(thread_context* to){
        to->switch_in();
    }
    void switch_out(thread_context* from){
        from->switch_out();
    }
    void init(){
        g_unthreaded_context.link = nullptr;
        g_unthreaded_context.thread = nullptr;
        g_current_context = &g_unthreaded_context;
    }
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

    static bool running_in_thread() {
        return thread_impl::get() != nullptr;
    }
    static bool try_run_one_yielded_thread();
};



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
#include "../util/bool_class.hh"
#include <tuple>
#include <iterator>
#include <vector>
#include <experimental/optional>
// #include "util/tuple_utils.hh"
extern __thread size_t task_quota;
struct parallel_for_each_state {
    // use optional<> to avoid out-of-line constructor
    std::optional<std::exception_ptr> ex;
    size_t waiting = 0;
    promise<> pr;
    void complete() {
        if (--waiting == 0) {
            if (ex) {
                pr.set_exception(std::move(*ex));
            } else {
                pr.set_value();
            }
        }
    }
};

//这里？
template <typename Iterator, typename Func>
GCC6_CONCEPT(requires requires (Func f, Iterator i) { { f(*i++) } -> std::same_as<future<>>; })
inline
future<>
parallel_for_each(Iterator begin, Iterator end, Func&& func) {
    if (begin == end) {
        return make_ready_future<>();
    }
    return do_with(parallel_for_each_state(), [&] (parallel_for_each_state& state) -> future<> {
        // increase ref count to ensure all functions run
        ++state.waiting;
        while (begin != end) {
            ++state.waiting;
            try {
                func(*begin++).then_wrapped([&] (future<> f) {
                    if (f.failed()) {
                        // We can only store one exception.  For more, use when_all().
                        if (!state.ex) {
                            state.ex = f.get_exception();
                        } else {
                            f.ignore_ready_future();
                        }
                    }
                    state.complete();
                });
            } catch (...) {
                if (!state.ex) {
                    state.ex = std::move(std::current_exception());
                }
                state.complete();
            }
        }
        // match increment on top
        state.complete();
        return state.pr.get_future();
    });
}


template <typename Range, typename Func>
GCC6_CONCEPT(requires requires (Func f, Range r) { { f(*r.begin()) } -> std::same_as<future<>>; })
inline
future<>
parallel_for_each(Range&& range, Func&& func) {
    return parallel_for_each(std::begin(range), std::end(range),
            std::forward<Func>(func));
}


template<typename AsyncAction, typename StopCondition>
static inline
void do_until_continued(StopCondition&& stop_cond, AsyncAction&& action, promise<> p) {
    while (!stop_cond()) {
        try {
            auto&& f = action();
            if (!f.available() || need_preempt()) {
                f.then_wrapped([action = std::forward<AsyncAction>(action),
                                stop_cond = std::forward<StopCondition>(stop_cond),
                                p = std::move(p)]  // 修复：移动捕获p
                                (std::result_of_t<AsyncAction()> fut) mutable {
                    if (!fut.failed()) {
                        do_until_continued(std::forward<StopCondition>(stop_cond),
                                          std::forward<AsyncAction>(action),
                                          std::move(p));  // 修复：移动p
                    } else {
                        p.set_exception(fut.get_exception());  // 此时p已经被捕获
                    }
                });
                return;
            }
            if (f.failed()) {
                f.forward_to(std::move(p));
                return;
            }
        } catch (...) {
            p.set_exception(std::current_exception());
            return;
        }
    }
    p.set_value();
}





template<typename AsyncAction>
GCC6_CONCEPT( requires ApplyReturns<AsyncAction, stop_iteration> || ApplyReturns<AsyncAction, future<stop_iteration>> )
static inline
future<> repeat(AsyncAction&& action) {
    using futurator = futurize<std::result_of_t<AsyncAction()>>;
    static_assert(std::is_same<future<stop_iteration>, typename futurator::type>::value, "bad AsyncAction signature");

    try {
        do {
            auto f = futurator::apply(action);

            if (!f.available()) {
                return f.then([action = std::forward<AsyncAction>(action)] (stop_iteration stop) mutable {
                    if (stop == stop_iteration::yes) {
                        return make_ready_future<>();
                    } else {
                        return repeat(std::forward<AsyncAction>(action));
                    }
                });
            }

            if (f.get0() == stop_iteration::yes) {
                return make_ready_future<>();
            }
        } while (!need_preempt());

        promise<> p;
        auto f = p.get_future();
        schedule_normal(make_task([action = std::forward<AsyncAction>(action), p = std::move(p)]() mutable {
            repeat(std::forward<AsyncAction>(action)).forward_to(std::move(p));
        }));
        return f;
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}


template <typename T>
struct repeat_until_value_type_helper;


/// Type helper for repeat_until_value()
template <typename T>
struct repeat_until_value_type_helper<future<std::optional<T>>> {
    using value_type = T;
    using optional_type = std::optional<T>;
    using future_type = future<value_type>;
    using future_optional_type = future<optional_type>;
};

template <typename AsyncAction>
using repeat_until_value_return_type
        = typename repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>::future_type;

template<typename AsyncAction>
GCC6_CONCEPT( requires requires (AsyncAction aa) {
    requires is_future<decltype(aa())>::value;
    bool(aa().get0());
    aa().get0().value();
} )
repeat_until_value_return_type<AsyncAction>
repeat_until_value(AsyncAction&& action) {
    using type_helper = repeat_until_value_type_helper<std::result_of_t<AsyncAction()>>;
    // the "T" in the documentation
    using value_type = typename type_helper::value_type;
    using optional_type = typename type_helper::optional_type;
    using futurator = futurize<typename type_helper::future_optional_type>;
    do {
        auto f = futurator::apply(action);

        if (!f.available()) {
            return f.then([action = std::forward<AsyncAction>(action)] (auto&& optional) mutable {
                if (optional) {
                    return make_ready_future<value_type>(std::move(optional.value()));
                } else {
                    return repeat_until_value(std::forward<AsyncAction>(action));
                }
            });
        }

        if (f.failed()) {
            return make_exception_future<value_type>(f.get_exception());
        }

        optional_type&& optional = std::move(f).get0();
        if (optional) {
            return make_ready_future<value_type>(std::move(optional.value()));
        }
    } while (!need_preempt());

    try {
        promise<value_type> p;
        auto f = p.get_future();
        schedule_normal(make_task([action = std::forward<AsyncAction>(action), p = std::move(p)] () mutable {
            repeat_until_value(std::forward<AsyncAction>(action)).forward_to(std::move(p));
        }));
        return f;
    } catch (...) {
        return make_exception_future<value_type>(std::current_exception());
    }
}

template<typename AsyncAction, typename StopCondition>
GCC6_CONCEPT( requires ApplyReturns<StopCondition, bool> && ApplyReturns<AsyncAction, future<>> )
static inline
future<> do_until(StopCondition&& stop_cond, AsyncAction&& action) {
    promise<> p;
    auto f = p.get_future();
    do_until_continued(std::forward<StopCondition>(stop_cond),
        std::forward<AsyncAction>(action), std::move(p));
    return f;
}

template<typename AsyncAction>
// GCC6_CONCEPT( requires ApplyReturns<AsyncAction, future<>> )
static inline
future<> keep_doing(AsyncAction&& action) {
    return repeat([action = std::forward<AsyncAction>(action)] () mutable {
        return action().then([] {
            return stop_iteration::no;
        });
    });
}




template<typename Iterator, typename AsyncAction>
GCC6_CONCEPT( requires requires (Iterator i, AsyncAction aa) { { aa(*i) } -> std::same_as<future<>>; } )
static inline
future<> do_for_each(Iterator begin, Iterator end, AsyncAction&& action) {
    if (begin == end) {
        return make_ready_future<>();
    }
    while (true) {
        auto f = action(*begin++);
        if (begin == end) {
            return f;
        }
        if (!f.available() || need_preempt()) {
            return std::move(f).then([action = std::forward<AsyncAction>(action),
                    begin = std::move(begin), end = std::move(end)] () mutable {
                return do_for_each(std::move(begin), std::move(end), std::forward<AsyncAction>(action));
            });
        }
        if (f.failed()) {
            return std::move(f);
        }
    }
}

template<typename Container, typename AsyncAction>
GCC6_CONCEPT( requires requires (Container c, AsyncAction aa) { { aa(*c.begin()) } -> std::same_as<future<>>; } )
static inline
future<> do_for_each(Container& c, AsyncAction&& action) {
    return do_for_each(std::begin(c), std::end(c), std::forward<AsyncAction>(action));
}

namespace internal {

template<typename... Futures>
struct identity_futures_tuple {
    using future_type = future<std::tuple<Futures...>>;
    using promise_type = typename future_type::promise_type;

    static void set_promise(promise_type& p, std::tuple<Futures...> futures) {
        p.set_value(std::move(futures));
    }
};

template<typename ResolvedTupleTransform, typename... Futures>
class when_all_state{
    using type = std::tuple<Futures...>;
    type tuple;
public:
    typename ResolvedTupleTransform::promise_type p;
    when_all_state(Futures&&... t) : tuple(std::make_tuple(std::move(t)...)) {}
    ~when_all_state() {
        ResolvedTupleTransform::set_promise(p, std::move(tuple));
    }
private:
    template<size_t Idx>
    int wait() {
        auto& f = std::get<Idx>(tuple);
        static_assert(is_future<std::remove_reference_t<decltype(f)>>::value, "when_all parameter must be a future");
        if (!f.available()) {
            f = f.then_wrapped([s = this->shared_from_this()] (auto&& f) {
                return std::move(f);
            });
        }
        return 0;
    }
public:
    template <size_t... Idx>
    typename ResolvedTupleTransform::future_type wait_all(std::index_sequence<Idx...>) {
        [] (...) {} (this->template wait<Idx>()...);
        return p.get_future();
    }
};
}


GCC6_CONCEPT(
namespace impl {
// Want: folds
template <typename T>
struct is_tuple_of_futures : std::false_type {
};

template <>
struct is_tuple_of_futures<std::tuple<>> : std::true_type {
};

template <typename... T, typename... Rest>
struct is_tuple_of_futures<std::tuple<future<T...>, Rest...>> : is_tuple_of_futures<std::tuple<Rest...>> {
};
}

template <typename... Futs>
concept AllAreFutures = impl::is_tuple_of_futures<std::tuple<Futs...>>::value;



// template <typename Func, typename... T>
// concept ApplyReturnsAnyFuture = requires (Func f, T... args) {
//     requires is_future<decltype(f(std::forward<T>(args)...))>::value;
// };
)


template <typename... Futs>
GCC6_CONCEPT( requires AllAreFutures<Futs...> )
inline
future<std::tuple<Futs...>>
when_all(Futs&&... futs) {
    namespace si = internal;
    using state = si::when_all_state<si::identity_futures_tuple<Futs...>, Futs...>;
    auto s = std::make_shared<state>(std::forward<Futs>(futs)...);
    return s->wait_all(std::make_index_sequence<sizeof...(Futs)>());
}

/// \cond internal
namespace internal {

template <typename Iterator, typename IteratorCategory>
inline
size_t
when_all_estimate_vector_capacity(Iterator begin, Iterator end, IteratorCategory category) {
    // For InputIterators we can't estimate needed capacity
    return 0;
}

template <typename Iterator>
inline
size_t
when_all_estimate_vector_capacity(Iterator begin, Iterator end, std::forward_iterator_tag category) {
    // May be linear time below random_access_iterator_tag, but still better than reallocation
    return std::distance(begin, end);
}

template<typename Future>
struct identity_futures_vector {
    using future_type = future<std::vector<Future>>;
    static future_type run(std::vector<Future> futures) {
        return make_ready_future<std::vector<Future>>(std::move(futures));
    }
};

// Internal function for when_all().
template <typename ResolvedVectorTransform, typename Future>
inline
typename ResolvedVectorTransform::future_type
complete_when_all(std::vector<Future>&& futures, typename std::vector<Future>::iterator pos) {
    // If any futures are already ready, skip them.
    while (pos != futures.end() && pos->available()) {
        ++pos;
    }
    // Done?
    if (pos == futures.end()) {
        return ResolvedVectorTransform::run(std::move(futures));
    }
    // Wait for unready future, store, and continue.
    return pos->then_wrapped([futures = std::move(futures), pos] (auto fut) mutable {
        *pos++ = std::move(fut);
        return complete_when_all<ResolvedVectorTransform>(std::move(futures), pos);
    });
}

template<typename ResolvedVectorTransform, typename FutureIterator>
inline auto
do_when_all(FutureIterator begin, FutureIterator end) {
    using itraits = std::iterator_traits<FutureIterator>;
    std::vector<typename itraits::value_type> ret;
    ret.reserve(when_all_estimate_vector_capacity(begin, end, typename itraits::iterator_category()));
    // Important to invoke the *begin here, in case it's a function iterator,
    // so we launch all computation in parallel.
    std::move(begin, end, std::back_inserter(ret));
    return complete_when_all<ResolvedVectorTransform>(std::move(ret), ret.begin());
}

}


template <typename FutureIterator>
GCC6_CONCEPT( requires requires (FutureIterator i) { { *i++ }; requires is_future<std::remove_reference_t<decltype(*i)>>::value; } )





inline
future<std::vector<typename std::iterator_traits<FutureIterator>::value_type>>
when_all(FutureIterator begin, FutureIterator end) {
    namespace si = internal;
    using itraits = std::iterator_traits<FutureIterator>;
    using result_transform = si::identity_futures_vector<typename itraits::value_type>;
    return si::do_when_all<result_transform>(std::move(begin), std::move(end));
}

template <typename T, bool IsFuture>
struct reducer_with_get_traits;

template <typename T>
struct reducer_with_get_traits<T, false> {
    using result_type = decltype(std::declval<T>().get());
    using future_type = future<result_type>;
    static future_type maybe_call_get(future<> f, std::shared_ptr<T> r) {
        return f.then([r = std::move(r)] () mutable {
            return make_ready_future<result_type>(std::move(*r).get());
        });
    }
};

template <typename T>
struct reducer_with_get_traits<T, true> {
    using future_type = decltype(std::declval<T>().get());
    static future_type maybe_call_get(future<> f, std::shared_ptr<T> r) {
        return f.then([r = std::move(r)] () mutable {
            return r->get();
        }).then_wrapped([r] (future_type f) {
            return f;
        });
    }
};

template <typename T, typename V = void>
struct reducer_traits {
    using future_type = future<>;
    static future_type maybe_call_get(future<> f, std::shared_ptr<T> r) {
        return f.then([r = std::move(r)] {});
    }
};

template <typename T>
struct reducer_traits<T, decltype(std::declval<T>().get(), void())> : public reducer_with_get_traits<T, is_future<std::result_of_t<decltype(&T::get)(T)>>::value> {};


template <typename Iterator, typename Mapper, typename Reducer>
inline
auto
map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
    -> typename reducer_traits<Reducer>::future_type
{
    auto r_ptr = std::make_shared<Reducer>(std::forward<Reducer>(r));
    future<> ret = make_ready_future<>();
    using futurator = futurize<decltype(mapper(*begin))>;
    while (begin != end) {
        ret = futurator::apply(mapper, *begin++).then_wrapped([ret = std::move(ret), r_ptr] (auto f) mutable {
            return ret.then_wrapped([f = std::move(f), r_ptr] (auto rf) mutable {
                if (rf.failed()) {
                    f.ignore_ready_future();
                    return std::move(rf);
                } else {
                    return futurize<void>::apply(*r_ptr, std::move(f.get()));
                }
            });
        });
    }
    return reducer_traits<Reducer>::maybe_call_get(std::move(ret), r_ptr);
}


template <typename Iterator, typename Mapper, typename Initial, typename Reduce>
GCC6_CONCEPT( requires requires (Iterator i, Mapper mapper, Initial initial, Reduce reduce) {
    *i++;
    { i != i } -> std::same_as<bool>;//为什么?
    mapper(*i);
    requires is_future<decltype(mapper(*i))>::value;
    { reduce(std::move(initial), mapper(*i).get0()) } -> std::same_as<Initial>;
} )
inline
future<Initial>
map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Initial initial, Reduce reduce) {
    struct state {
        Initial result;
        Reduce reduce;
    };
    auto s = std::make_shared(state{std::move(initial), std::move(reduce)});
    future<> ret = make_ready_future<>();
    using futurator = futurize<decltype(mapper(*begin))>;
    while (begin != end) {
        ret = futurator::apply(mapper, *begin++).then_wrapped([s = s.get(), ret = std::move(ret)] (auto f) mutable {
            try {
                s->result = s->reduce(std::move(s->result), std::move(f.get0()));
                return std::move(ret);
            } catch (...) {
                return std::move(ret).then_wrapped([ex = std::current_exception()] (auto f) {
                    f.ignore_ready_future();
                    return make_exception_future<>(ex);
                });
            }
        });
    }
    return ret.then([s] {
        return make_ready_future<Initial>(std::move(s->result));
    });
}

template <typename Range, typename Mapper, typename Initial, typename Reduce>
GCC6_CONCEPT( requires requires (Range range, Mapper mapper, Initial initial, Reduce reduce) {
     std::begin(range);
     std::end(range);
     mapper(*std::begin(range));
     requires is_future<std::remove_reference_t<decltype(mapper(*std::begin(range)))>>::value;
    { reduce(std::move(initial), mapper(*std::begin(range)).get0()) } -> std::same_as<Initial>;
} )
inline
future<Initial>
map_reduce(Range&& range, Mapper&& mapper, Initial initial, Reduce reduce) {
    return map_reduce(std::begin(range), std::end(range), std::forward<Mapper>(mapper),
            std::move(initial), std::move(reduce));
}

template <typename Result, typename Addend = Result>
class adder {
private:
    Result _result;
public:
    future<> operator()(const Addend& value) {
        _result += value;
        return make_ready_future<>();
    }
    Result get() && {
        return std::move(_result);
    }
};

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



struct default_timeout_exception_factory {
    static auto timeout() {
        return timed_out_error();
    }
};

template<typename ExceptionFactory = default_timeout_exception_factory, typename Clock, typename Duration, typename... T>
future<T...> with_timeout(std::chrono::time_point<Clock, Duration> timeout, future<T...> f) {
    if (f.available()) {
        return f;
    }
    auto pr = std::make_unique<promise<T...>>();
    auto result = pr->get_future();
    timer<Clock> timer([&pr = *pr] {
        pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout()));
    });
    timer.arm(timeout);
    f.then_wrapped([pr = std::move(pr), timer = std::move(timer)] (auto&& f) mutable {
        if (timer.cancel()) {
            f.forward_to(std::move(*pr));
        } else {
            f.ignore_ready_future();
        }
    });
    return result;
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
template<typename... Futures>
GCC6_CONCEPT( requires AllAreFutures<Futures...> )
inline auto when_all_succeed(Futures&&... futures) {
    using state = internal::when_all_state<internal::extract_values_from_futures_tuple<Futures...>, Futures...>;
    auto s = std::make_shared<state>(std::forward<Futures>(futures)...);
    return s->wait_all(std::make_index_sequence<sizeof...(Futures)>());
}

template <typename FutureIterator, typename = typename std::iterator_traits<FutureIterator>::value_type>
GCC6_CONCEPT( requires requires (FutureIterator i) {
    *i++;
    {i!= i} ->std::same_as<bool>;
     requires is_future<std::remove_reference_t<decltype(*i)>>::value;
})

inline auto when_all_succeed(FutureIterator begin, FutureIterator end) {
    using itraits = std::iterator_traits<FutureIterator>;
    using result_transform = internal::extract_values_from_futures_vector<typename itraits::value_type>;
    return internal::do_when_all<result_transform>(std::move(begin), std::move(end));
}
// Define the static member
thread_local thread* thread::_current = nullptr;
// Implementation of global functions
void enable_timer(steady_clock_type::time_point when)
{
    engine().enable_timer(when);
}





/// if sharded service inherits from this class sharded::stop() will wait
/// untill all references to a service on each shard will dissapper before
/// returning. It is still service's own responcibility to track its references
/// in asyncronous code by calling shared_from_this() and keeping returned smart
/// pointer as long as object is in use.
template<typename T>
class async_sharded_service{
protected:
    std::function<void()> _delete_cb;
    ~async_sharded_service() {
        if (_delete_cb) {
            _delete_cb();
        }
    }
    template <typename Service> friend class sharded;
};
/// Exception thrown when a \ref sharded object does not exist
class no_sharded_instance_exception : public std::exception {
public:
    virtual const char* what() const noexcept override {
        return "sharded instance does not exists";
    }
};

#include <boost/iterator/counting_iterator.hpp>


/// \defgroup smp-module Multicore
///
/// \brief Support for exploiting multiple cores on a server.
///
/// Seastar supports multicore servers by using \i sharding.  Each logical
/// core (lcore) runs a separate event loop, with its own memory allocator,
/// TCP/IP stack, and other services.  Shards communicate by explicit message
/// passing, rather than using locks and condition variables as with traditional
/// threaded programming.

/// \addtogroup smp-module
/// @{

/// Template helper to distribute a service across all logical cores.
///
/// The \c sharded template manages a sharded service, by creating
/// a copy of the service on each logical core, providing mechanisms to communicate
/// with each shard's copy, and a way to stop the service.
///
/// \tparam Service a class to be instantiated on each core.  Must expose
///         a \c stop() method that returns a \c future<>, to be called when
///         the service is stopped.
template <typename Service>
class sharded {
    struct entry {
        std::shared_ptr<Service> service;
        promise<> freed;
    };
    std::vector<entry> _instances;
private:
    void service_deleted() {
        _instances[engine().cpu_id()].freed.set_value();
    }
    template <typename U, bool async>
    // friend struct std::shared_ptr_make_helper;

public:
    /// Constructs an empty \c sharded object.  No instances of the service are
    /// created.
    sharded() {}
    sharded(const sharded& other) = delete;
    /// Moves a \c sharded object.
    sharded(sharded&& other) = default;
    sharded& operator=(const sharded& other) = delete;
    /// Moves a \c sharded object.
    sharded& operator=(sharded&& other) = default;
    /// Destroyes a \c sharded object.  Must not be in a started state.
    ~sharded();

    /// Starts \c Service by constructing an instance on every logical core
    /// with a copy of \c args passed to the constructor.
    ///
    /// \param args Arguments to be forwarded to \c Service constructor
    /// \return a \ref future<> that becomes ready when all instances have been
    ///         constructed.
    template <typename... Args>
    future<> start(Args&&... args);

    /// Starts \c Service by constructing an instance on a single logical core
    /// with a copy of \c args passed to the constructor.
    ///
    /// \param args Arguments to be forwarded to \c Service constructor
    /// \return a \ref future<> that becomes ready when the instance has been
    ///         constructed.
    template <typename... Args>
    future<> start_single(Args&&... args);

    /// Stops all started instances and destroys them.
    ///
    /// For every started instance, its \c stop() method is called, and then
    /// it is destroyed.
    future<> stop();

    // Invoke a method on all instances of @Service.
    // The return value becomes ready when all instances have processed
    // the message.
    template <typename... Args>
    future<> invoke_on_all(future<> (Service::*func)(Args...), Args... args);

    /// Invoke a method on all \c Service instances in parallel.
    ///
    /// \param func member function to be called.  Must return \c void or
    ///             \c future<>.
    /// \param args arguments to be passed to \c func.
    /// \return future that becomes ready when the method has been invoked
    ///         on all instances.
    template <typename... Args>
    future<> invoke_on_all(void (Service::*func)(Args...), Args... args);

    /// Invoke a callable on all instances of  \c Service.
    ///
    /// \param func a callable with the signature `void (Service&)`
    ///             or `future<> (Service&)`, to be called on each core
    ///             with the local instance as an argument.
    /// \return a `future<>` that becomes ready when all cores have
    ///         processed the message.
    template <typename Func>
    future<> invoke_on_all(Func&& func);

    /// Invoke a method on all instances of `Service` and reduce the results using
    /// `Reducer`.
    ///
    /// \see map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
    template <typename Reducer, typename Ret, typename... FuncArgs, typename... Args>
    inline
    auto
    map_reduce(Reducer&& r, Ret (Service::*func)(FuncArgs...), Args&&... args)
        -> typename reducer_traits<Reducer>::future_type
    {
        return ::map_reduce(boost::counting_iterator<unsigned>(0),
                            boost::counting_iterator<unsigned>(_instances.size()),
            [this, func, args = std::make_tuple(std::forward<Args>(args)...)] (unsigned c) mutable {
                return smp::submit_to(c, [this, func, args] () mutable {
                    return apply([this, func] (Args&&... args) mutable {
                        auto inst = _instances[engine().cpu_id()].service;
                        if (inst) {
                            return ((*inst).*func)(std::forward<Args>(args)...);
                        } else {
                            throw no_sharded_instance_exception();
                        }
                    }, std::move(args));
                });
            }, std::forward<Reducer>(r));
    }

    /// Invoke a callable on all instances of `Service` and reduce the results using
    /// `Reducer`.
    ///
    /// \see map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
    template <typename Reducer, typename Func>
    inline
    auto map_reduce(Reducer&& r, Func&& func) -> typename reducer_traits<Reducer>::future_type
    {
        return ::map_reduce(boost::counting_iterator<unsigned>(0),
                            boost::counting_iterator<unsigned>(_instances.size()),
            [this, &func] (unsigned c) mutable {
                return smp::submit_to(c, [this, func] () mutable {
                    auto inst = get_local_service();
                    return func(*inst);
                });
            }, std::forward<Reducer>(r));
    }

    /// Applies a map function to all shards, then reduces the output by calling a reducer function.
    ///
    /// \param map callable with the signature `Value (Service&)` or
    ///               `future<Value> (Service&)` (for some `Value` type).
    ///               used as the second input to \c reduce
    /// \param initial initial value used as the first input to \c reduce.
    /// \param reduce binary function used to left-fold the return values of \c map
    ///               into \c initial .
    ///
    /// Each \c map invocation runs on the shard associated with the service.
    ///
    /// \tparam  Mapper unary function taking `Service&` and producing some result.
    /// \tparam  Initial any value type
    /// \tparam  Reduce a binary function taking two Initial values and returning an Initial
    /// \return  Result of applying `map` to each instance in parallel, reduced by calling
    ///          `reduce()` on each adjacent pair of results.
    template <typename Mapper, typename Initial, typename Reduce>
    inline
    future<Initial>
    map_reduce0(Mapper map, Initial initial, Reduce reduce) {
        auto wrapped_map = [this, map] (unsigned c) {
            return smp::submit_to(c, [this, map] {
                auto inst = get_local_service();
                return map(*inst);
            });
        };
        return ::map_reduce(smp::all_cpus().begin(), smp::all_cpus().end(),
                            std::move(wrapped_map),
                            std::move(initial),
                            std::move(reduce));
    }

    /// Applies a map function to all shards, and return a vector of the result.
    ///
    /// \param mapper callable with the signature `Value (Service&)` or
    ///               `future<Value> (Service&)` (for some `Value` type).
    ///
    /// Each \c map invocation runs on the shard associated with the service.
    ///
    /// \tparam  Mapper unary function taking `Service&` and producing some result.
    /// \return  Result vector of applying `map` to each instance in parallel
    template <typename Mapper, typename return_type = std::result_of_t<Mapper(Service&)>>
    inline future<std::vector<return_type>> map(Mapper mapper) {
        return do_with(std::vector<return_type>(),
                [&mapper, this] (std::vector<return_type>& vec) mutable {
            vec.resize(smp::count);
            return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this, &vec, mapper] (unsigned c) {
                return smp::submit_to(c, [this, mapper] {
                    auto inst = get_local_service();
                    return mapper(*inst);
                }).then([&vec, c] (auto res) {
                    vec[c] = res;
                });
            }).then([&vec] {
                return make_ready_future<std::vector<return_type>>(std::move(vec));
            });
        });
    }

    /// Invoke a method on a specific instance of `Service`.
    ///
    /// \param id shard id to call
    /// \param func a method of `Service`
    /// \param args arguments to be passed to `func`
    /// \return result of calling `func(args)` on the designated instance
    template <typename Ret, typename... FuncArgs, typename... Args, typename FutureRet = futurize_t<Ret>>
    FutureRet
    invoke_on(unsigned id, Ret (Service::*func)(FuncArgs...), Args&&... args) {
        using futurator = futurize<Ret>;
        return smp::submit_to(id, [this, func, args = std::make_tuple(std::forward<Args>(args)...)] () mutable {
            auto inst = get_local_service();
            return futurator::apply(std::mem_fn(func), std::tuple_cat(std::make_tuple<>(inst), std::move(args)));
        });
    }

    /// Invoke a callable on a specific instance of `Service`.
    ///
    /// \param id shard id to call
    /// \param func a callable with signature `Value (Service&)` or
    ///        `future<Value> (Service&)` (for some `Value` type)
    /// \return result of calling `func(instance)` on the designated instance
    template <typename Func, typename Ret = futurize_t<std::result_of_t<Func(Service&)>>>
    Ret
    invoke_on(unsigned id, Func&& func) {
        return smp::submit_to(id, [this, func = std::forward<Func>(func)] () mutable {
            auto inst = get_local_service();
            return func(*inst);
        });
    }

    /// Gets a reference to the local instance.
    Service& local();

    /// Gets a shared pointer to the local instance.
    std::shared_ptr<Service> local_shared();

    /// Checks whether the local instance has been initialized.
    bool local_is_initialized();

private:
    void track_deletion(std::shared_ptr<Service>& s, std::false_type) {
        // do not wait for instance to be deleted since it is not going to notify us
        service_deleted();
    }

    void track_deletion(std::shared_ptr<Service>& s, std::true_type) {
        s->_delete_cb = std::bind(std::mem_fn(&sharded<Service>::service_deleted), this);
    }

    template <typename... Args>
    std::shared_ptr<Service> create_local_service(Args&&... args) {
        auto s = std::make_shared<Service>(std::forward<Args>(args)...);
        track_deletion(s, std::is_base_of<async_sharded_service<Service>, Service>());
        return s;
    }

    std::shared_ptr<Service> get_local_service() {
        auto inst = _instances[engine().cpu_id()].service;
        if (!inst) {
            throw no_sharded_instance_exception();
        }
        return inst;
    }
};

template <typename Service>
sharded<Service>::~sharded() {
	assert(_instances.empty());
}

template <typename Service>
template <typename... Args>
future<>
sharded<Service>::start(Args&&... args) {
    _instances.resize(smp::count);
    return parallel_for_each(
        boost::irange<unsigned>(0, _instances.size()),
        [this, args = std::make_tuple(std::forward<Args>(args)...)] (unsigned c) mutable {
            return smp::submit_to(c, [this, args] () mutable {
                _instances[engine().cpu_id()].service = apply([this] (Args... args) {
                    return create_local_service(std::forward<Args>(args)...);
                }, args);
            });
    }).then_wrapped([this] (future<> f) {
        try {
            f.get();
            return make_ready_future<>();
        } catch (...) {
            return this->stop().then([e = std::current_exception()] () mutable {
                std::rethrow_exception(e);
            });
        }
    });
}

template <typename Service>
template <typename... Args>
future<>
sharded<Service>::start_single(Args&&... args) {
    assert(_instances.empty());
    _instances.resize(1);
    return smp::submit_to(0, [this, args = std::make_tuple(std::forward<Args>(args)...)] () mutable {
        _instances[0].service = apply([this] (Args... args) {
            return create_local_service(std::forward<Args>(args)...);
        }, args);
    }).then_wrapped([this] (future<> f) {
        try {
            f.get();
            return make_ready_future<>();
        } catch (...) {
            return this->stop().then([e = std::current_exception()] () mutable {
                std::rethrow_exception(e);
            });
        }
    });
}

template <typename Service>
future<>
sharded<Service>::stop() {
    return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this] (unsigned c) mutable {
        return smp::submit_to(c, [this] () mutable {
            auto inst = _instances[engine().cpu_id()].service;
            if (!inst) {
                return make_ready_future<>();
            }
            _instances[engine().cpu_id()].service = nullptr;
            return inst->stop().then([this, inst] {
                return _instances[engine().cpu_id()].freed.get_future();
            });
        });
    }).then([this] {
        _instances.clear();
        _instances = std::vector<sharded<Service>::entry>();
    });
}

template <typename Service>
template <typename... Args>
inline
future<>
sharded<Service>::invoke_on_all(future<> (Service::*func)(Args...), Args... args) {
    return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this, func, args...] (unsigned c) {
        return smp::submit_to(c, [this, func, args...] {
            auto inst = get_local_service();
            return ((*inst).*func)(args...);
        });
    });
}

template <typename Service>
template <typename... Args>
inline
future<>
sharded<Service>::invoke_on_all(void (Service::*func)(Args...), Args... args) {
    return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this, func, args...] (unsigned c) {
        return smp::submit_to(c, [this, func, args...] {
            auto inst = get_local_service();
            ((*inst).*func)(args...);
        });
    });
}

template <typename Service>
template <typename Func>
inline
future<>
sharded<Service>::invoke_on_all(Func&& func) {
    static_assert(std::is_same<futurize_t<std::result_of_t<Func(Service&)>>, future<>>::value,
                  "invoke_on_all()'s func must return void or future<>");
    return parallel_for_each(boost::irange<unsigned>(0, _instances.size()), [this, &func] (unsigned c) {
        return smp::submit_to(c, [this, func] {
            auto inst = get_local_service();
            return func(*inst);
        });
    });
}

template <typename Service>
Service& sharded<Service>::local() {
    assert(local_is_initialized());
    return *_instances[engine().cpu_id()].service;
}

template <typename Service>
std::shared_ptr<Service> sharded<Service>::local_shared() {
    assert(local_is_initialized());
    return _instances[engine().cpu_id()].service;
}

template <typename Service>
inline bool sharded<Service>::local_is_initialized() {
    return _instances.size() > engine().cpu_id() &&
           _instances[engine().cpu_id()].service;
}


template <typename Service>
using distributed = sharded<Service>;








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

void reactor::add_timer(manual_timer* tmr) {
    queue_timer(tmr);
}
bool reactor::queue_timer(lowres_timer* tmr) {
    return _lowres_timers.insert(*tmr);
}

bool reactor::queue_timer(manual_timer* tmr) {
    return _manual_timers.insert(*tmr);
}

bool reactor::queue_timer(steady_timer* tmr) {
    // std::cout<<"reaactor queue timer"<<std::endl;
    return _timers.insert(*tmr);
}


/*
    del_timer什么时候调用?
*/
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

void reactor::del_timer(manual_timer* tmr) {
    if (tmr->_expired) {
        _expired_manual_timers.erase(tmr->expired_it);
        tmr->_expired = false;
    } else {
        _manual_timers.remove(*tmr);
    }
}



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
            // 这里是修复的地方
            if (_qs != nullptr) {
                return _qs[t][engine().cpu_id()].submit(std::forward<Func>(func));
            } else {
                return futurize<std::result_of_t<Func()>>::make_exception_future(std::runtime_error("smp::_qs is null"));
            }
        }
}




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

void engine_exit(std::exception_ptr eptr) {
    if (!eptr) {
        engine().exit(0);
        return;
    }
    std::cout<<"Exception: "<< std::endl;
    engine().exit(1);
}




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
    : _context(std::make_unique<thread_context>(std::move(attr), std::move(func))) {}
    /*
        因为context是使用unique_ptr管理,所以当退出作用域时，unique会析构到，在析构时自动释放管理的内存.
    */


void thread::yield() {
    thread_impl::get()->yield();
}

bool thread::should_yield() {
    return thread_impl::get()->should_yield();
}


// Destructor
thread::~thread() {
    assert(!_context || _context->_joined);
}

void reactor::expire_manual_timers() {
    complete_timers(engine()._manual_timers, engine()._expired_manual_timers, []{
    });
}

void manual_clock::expire_timers() {
    engine().expire_manual_timers();
}

inline void jmp_buf_link::initial_switch_in(ucontext_t* initial_context, const void*, size_t)
{
    if(g_current_context){
    }else{
    }
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
    //为什么这里是this,而不是*this，而不是_all_it?思考
    //因为_all_threads存放的就是thread_context*，所以添加的也是指针。
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



void thread_context::stack_deleter::operator()(char* ptr) const noexcept {
    delete[] ptr;
}

void
thread_context::setup() {
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
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_start();
        _context.set_yield_at(_attr.scheduling_group->_this_run_start + _attr.scheduling_group->_this_period_remain);
    } else {
        _context.clear_yield_at();//设置_context的yield_为false
    }
    _context.switch_in();
}

void thread_context::switch_out() {
    if (_attr.scheduling_group) {
        _attr.scheduling_group->account_stop();
    }
    _context.switch_out();
}

bool thread_context::should_yield() const {
    if (!_attr.scheduling_group) {
        return need_preempt();
    }
    return need_preempt() || bool(_attr.scheduling_group->next_scheduling_point());
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




/// \brief Waits for the future to become available
/// This method blocks the current thread until the future becomes available.
template <typename... T>
void future<T...>::wait() {
    auto thread = thread_impl::get();
    assert(thread);//这里报错.

    schedule([this, thread] (future_state<T...>&& new_state) {
        *state() = std::move(new_state);
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
    memory::set_reclaim_hook([this] (std::function<void ()> reclaim_fn) {
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


#include <boost/lexical_cast.hpp>

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



#include <iostream>


namespace memory {

static thread_local int abort_on_alloc_failure_suppressed = 0;

disable_abort_on_alloc_failure_temporarily::disable_abort_on_alloc_failure_temporarily() {
    ++abort_on_alloc_failure_suppressed;
}

disable_abort_on_alloc_failure_temporarily::~disable_abort_on_alloc_failure_temporarily() noexcept {
    --abort_on_alloc_failure_suppressed;
}

}



#include "../util/align.hh"
#include "../fd/posix.hh"
// #include "../util/shared_ptr.hh"
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
#include <sys/uio.h>  // For writev
#include <boost/intrusive/list.hpp>
#include <sys/mman.h>
#include "../util/backtrace.hh"
#include <unordered_set>


struct allocation_site {
    mutable size_t count = 0; // number of live objects allocated at backtrace.
    mutable size_t size = 0; // amount of bytes in live objects allocated at backtrace.
    mutable const allocation_site* next = nullptr;
    saved_backtrace backtrace;

    bool operator==(const allocation_site& o) const {
        return backtrace == o.backtrace;
    }

    bool operator!=(const allocation_site& o) const {
        return !(*this == o);
    }
};

namespace std {

template<>
struct hash<::allocation_site> {
    size_t operator()(const ::allocation_site& bi) const {
        return std::hash<saved_backtrace>()(bi.backtrace);
    }
};

}

using allocation_site_ptr = const allocation_site*;

namespace memory {

static allocation_site_ptr get_allocation_site() __attribute__((unused));

static std::atomic<bool> abort_on_allocation_failure{false};

void enable_abort_on_allocation_failure() {
    abort_on_allocation_failure.store(true, std::memory_order_seq_cst);
}

static void on_allocation_failure(size_t size) {
    if (!abort_on_alloc_failure_suppressed
            && abort_on_allocation_failure.load(std::memory_order_relaxed)) {
        abort();
    }
}

static constexpr unsigned cpu_id_shift = 36; // FIXME: make dynamic
static constexpr unsigned max_cpus = 256;
static constexpr size_t cache_line_size = 64;
using pageidx = uint32_t;
struct page;
class page_list;
static std::atomic<bool> live_cpus[max_cpus];
static thread_local uint64_t g_allocs;
static thread_local uint64_t g_frees;
static thread_local uint64_t g_cross_cpu_frees;
static thread_local uint64_t g_reclaims;
#include <functional>  // for std::function
#include <optional>    // for std::optional
#include <memory>      // for std::unique_ptr
using allocate_system_memory_fn = std::function<mmap_area(std::optional<void*> where, size_t how_much)>;

namespace bi = boost::intrusive;

inline
unsigned object_cpu_id(const void* ptr) {
    return (reinterpret_cast<uintptr_t>(ptr) >> cpu_id_shift) & 0xff;
}

class page_list_link {
    uint32_t _prev;
    uint32_t _next;
    friend class page_list;
};

static char* mem_base() {
    static char* known;
    static std::once_flag flag;
    std::call_once(flag, [] {
        size_t alloc = size_t(1) << 44;
        auto r = ::mmap(NULL, 2 * alloc,
                    PROT_NONE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
                    -1, 0);
        if (r == MAP_FAILED) {
            abort();
        }
        ::madvise(r, 2 * alloc, MADV_DONTDUMP);
        auto cr = reinterpret_cast<char*>(r);
        known = align_up(cr, alloc);
        ::munmap(cr, known - cr);
        ::munmap(known + alloc, cr + 2 * alloc - (known + alloc));
    });
    return known;
}

constexpr bool is_page_aligned(size_t size) {
    return (size & (page_size - 1)) == 0;
}

constexpr size_t next_page_aligned(size_t size) {
    return (size + (page_size - 1)) & ~(page_size - 1);
}

class small_pool;

struct free_object {
    free_object* next;
};

struct page {
    bool free;
    uint8_t offset_in_span;
    uint16_t nr_small_alloc;
    uint32_t span_size; // in pages, if we're the head or the tail
    page_list_link link;
    small_pool* pool;  // if used in a small_pool
    free_object* freelist;
};

class page_list {
    uint32_t _front = 0;
    uint32_t _back = 0;
public:
    page& front(page* ary) { return ary[_front]; }
    page& back(page* ary) { return ary[_back]; }
    bool empty() const { return !_front; }
    void erase(page* ary, page& span) {
        if (span.link._next) {
            ary[span.link._next].link._prev = span.link._prev;
        } else {
            _back = span.link._prev;
        }
        if (span.link._prev) {
            ary[span.link._prev].link._next = span.link._next;
        } else {
            _front = span.link._next;
        }
    }
    void push_front(page* ary, page& span) {
        auto idx = &span - ary;
        if (_front) {
            ary[_front].link._prev = idx;
        } else {
            _back = idx;
        }
        span.link._next = _front;
        span.link._prev = 0;
        _front = idx;
    }
    void pop_front(page* ary) {
        if (ary[_front].link._next) {
            ary[ary[_front].link._next].link._prev = 0;
        } else {
            _back = 0;
        }
        _front = ary[_front].link._next;
    }
    page* find(uint32_t n_pages, page* ary) {
        auto n = _front;
        while (n && ary[n].span_size < n_pages) {
            n = ary[n].link._next;
        }
        if (!n) {
            return nullptr;
        }
        return &ary[n];
    }
};

class small_pool {
    unsigned _object_size;
    unsigned _span_size;
    free_object* _free = nullptr;
    size_t _free_count = 0;
    unsigned _min_free;
    unsigned _max_free;
    unsigned _spans_in_use = 0;
    page_list _span_list;
    static constexpr unsigned idx_frac_bits = 2;
private:
    size_t span_bytes() const { return _span_size * page_size; }
public:
    explicit small_pool(unsigned object_size) noexcept;
    ~small_pool();
    void* allocate();
    void deallocate(void* object);
    unsigned object_size() const { return _object_size; }
    bool objects_page_aligned() const { return is_page_aligned(_object_size); }
    static constexpr unsigned size_to_idx(unsigned size);
    static constexpr unsigned idx_to_size(unsigned idx);
    allocation_site_ptr& alloc_site_holder(void* ptr);
private:
    void add_more_objects();
    void trim_free_list();
    float waste();
};

// index 0b0001'1100 -> size (1 << 4) + 0b11 << (4 - 2)

constexpr unsigned
small_pool::idx_to_size(unsigned idx) {
    return (((1 << idx_frac_bits) | (idx & ((1 << idx_frac_bits) - 1)))
              << (idx >> idx_frac_bits))
                  >> idx_frac_bits;
}

constexpr unsigned
small_pool::size_to_idx(unsigned size) {
    return ((log2floor(size) << idx_frac_bits) - ((1 << idx_frac_bits) - 1))
            + ((size - 1) >> (log2floor(size) - idx_frac_bits));
}

class small_pool_array {
public:
    static constexpr unsigned nr_small_pools = small_pool::size_to_idx(4 * page_size) + 1;
private:
    union u {
        small_pool a[nr_small_pools];
        u() {
            for (unsigned i = 0; i < nr_small_pools; ++i) {
                new (&a[i]) small_pool(small_pool::idx_to_size(i));
            }
        }
        ~u() {
            // cannot really call destructor, since other
            // objects may be freed after we are gone.
        }
    } _u;
public:
    small_pool& operator[](unsigned idx) { return _u.a[idx]; }
};

static constexpr size_t max_small_allocation
    = small_pool::idx_to_size(small_pool_array::nr_small_pools - 1);

constexpr size_t object_size_with_alloc_site(size_t size) {

    return size;
}

struct cross_cpu_free_item {
    cross_cpu_free_item* next;
};

struct cpu_pages {
    uint32_t min_free_pages = 20000000 / page_size;
    char* memory;
    page* pages;
    uint32_t nr_pages;
    uint32_t nr_free_pages;
    uint32_t current_min_free_pages = 0;
    unsigned cpu_id = -1U;
    std::function<void (std::function<void ()>)> reclaim_hook;
    std::vector<reclaimer*> reclaimers;
    static constexpr unsigned nr_span_lists = 32;
    union pla {
        pla() {
            for (auto&& e : free_spans) {
                // std::cout<<"开始初始化free_spans"<<std::endl;
                new (&e) page_list;
            }
        }
        ~pla() {
            // no destructor -- might be freeing after we die
        }
        page_list free_spans[nr_span_lists];  // contains spans with span_size >= 2^idx
    } fsu;
    small_pool_array small_pools;
    alignas(cache_line_size) std::atomic<cross_cpu_free_item*> xcpu_freelist;
    alignas(cache_line_size) std::vector<physical_address> virt_to_phys_map;
    static std::atomic<unsigned> cpu_id_gen;
    static cpu_pages* all_cpus[max_cpus];
    union asu {
        using alloc_sites_type = std::unordered_set<allocation_site>;
        asu() {
            new (&alloc_sites) alloc_sites_type();
        }
        ~asu() {} // alloc_sites live forever
        alloc_sites_type alloc_sites;
    } asu;
    allocation_site_ptr alloc_site_list_head = nullptr; // For easy traversal of asu.alloc_sites from scylla-gdb.py
    bool collect_backtrace = false;
    char* mem() { return memory; }

    void link(page_list& list, page* span);
    void unlink(page_list& list, page* span);
    struct trim {
        unsigned offset;
        unsigned nr_pages; // 这个是什么意思?
    };
    void maybe_reclaim();
    template <typename Trimmer>
    void* allocate_large_and_trim(unsigned nr_pages, Trimmer trimmer);
    void* allocate_large(unsigned nr_pages);
    void* allocate_large_aligned(unsigned align_pages, unsigned nr_pages);
    page* find_and_unlink_span(unsigned nr_pages);
    page* find_and_unlink_span_reclaiming(unsigned n_pages);
    void free_large(void* ptr);
    void free_span(pageidx start, uint32_t nr_pages);
    void free_span_no_merge(pageidx start, uint32_t nr_pages);
    void* allocate_small(unsigned size);
    void free(void* ptr);
    void free(void* ptr, size_t size);
    bool try_cross_cpu_free(void* ptr);
    void shrink(void* ptr, size_t new_size);
    void free_cross_cpu(unsigned cpu_id, void* ptr);
    bool drain_cross_cpu_freelist();
    size_t object_size(void* ptr);
    page* to_page(void* p) {
        return &pages[(reinterpret_cast<char*>(p) - mem()) / page_size];
    }

    bool is_initialized() const;
    bool initialize();
    reclaiming_result run_reclaimers(reclaimer_scope);
    void schedule_reclaim();
    void set_reclaim_hook(std::function<void (std::function<void ()>)> hook);
    void set_min_free_pages(size_t pages);
    void resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem);
    void do_resize(size_t new_size, allocate_system_memory_fn alloc_sys_mem);
    void replace_memory_backing(allocate_system_memory_fn alloc_sys_mem);
    void init_virt_to_phys_map();
    memory::memory_layout memory_layout();
    translation translate(const void* addr, size_t size);
    ~cpu_pages();
};

static thread_local cpu_pages cpu_mem;
std::atomic<unsigned> cpu_pages::cpu_id_gen;
cpu_pages* cpu_pages::all_cpus[max_cpus];

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

// Free spans are store in the largest index i such that nr_pages >= 1 << i.
static inline
unsigned index_of(unsigned pages) {
    return std::numeric_limits<unsigned>::digits - count_leading_zeros(pages) - 1;
}

// Smallest index i such that all spans stored in the index are >= pages.
static inline
unsigned index_of_conservative(unsigned pages) {
    if (pages == 1) {
        return 0;
    }
    return std::numeric_limits<unsigned>::digits - count_leading_zeros(pages - 1);
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
    auto idx = index_of_conservative(n_pages);
    auto orig_idx = idx;
    if (n_pages >= (2u << idx)) {
        throw std::bad_alloc();
    }
    while (idx < nr_span_lists && fsu.free_spans[idx].empty()) {
        ++idx;
    }
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
    auto& list = fsu.free_spans[idx];
    page* span = list.find(n_pages, pages);
    if (!span) {
        return nullptr;
    }
    unlink(list, span);
    return span;
}

page*
cpu_pages::find_and_unlink_span_reclaiming(unsigned n_pages) {
    while (true) {
        auto span = find_and_unlink_span(n_pages);
        if (span) {
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
void*
cpu_pages::allocate_large_and_trim(unsigned n_pages, Trimmer trimmer) {
    // Avoid exercising the reclaimers for requests we'll not be able to satisfy
    // nr_pages might be zero during startup, so check for that too
    if (nr_pages && n_pages >= nr_pages) {
        return nullptr;
    }
    page* span = find_and_unlink_span_reclaiming(n_pages);
    if (!span) {
        return nullptr;
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

void*
cpu_pages::allocate_large(unsigned n_pages) {
    return allocate_large_and_trim(n_pages, [n_pages] (unsigned idx, unsigned n) {
        return trim{0, std::min(n, n_pages)};
    });
}

void*
cpu_pages::allocate_large_aligned(unsigned align_pages, unsigned n_pages) {
    return allocate_large_and_trim(n_pages + align_pages - 1, [=] (unsigned idx, unsigned n) {
        return trim{align_up(idx, align_pages) - idx, n_pages};
    });
}



struct disable_backtrace_temporarily {
    ~disable_backtrace_temporarily() {}
};


static
saved_backtrace get_backtrace() noexcept {
    disable_backtrace_temporarily dbt;
    return current_backtrace();
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
        return false;
    }
    cpu_id = cpu_id_gen.fetch_add(1, std::memory_order_relaxed);
    assert(cpu_id < max_cpus);
    all_cpus[cpu_id] = this;
    auto base = mem_base() + (size_t(cpu_id) << cpu_id_shift);
    auto size = 32 << 20;  // Small size for bootstrap
    auto r = ::mmap(base, size,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
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


mmap_area
allocate_anonymous_memory(std::optional<void*> where, size_t how_much) {
    return mmap_anonymous(where.value_or(nullptr),
            how_much, PROT_READ|PROT_WRITE, MAP_PRIVATE|(where?MAP_FIXED :0));
}








/*
    映射匿名area.
*/

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

void cpu_pages::replace_memory_backing(allocate_system_memory_fn alloc_sys_mem) {
    // We would like to use ::mremap() to atomically replace the old anonymous
    // memory with hugetlbfs backed memory, but mremap() does not support hugetlbfs
    // (for no reason at all).  So we must copy the anonymous memory to some other
    // place, map hugetlbfs in place, and copy it back, without modifying it during
    // the operation.
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
    });
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

float small_pool::waste() {
    return (span_bytes() % _object_size) / (1.0 * span_bytes());
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
    }
    void* ptr;
    if (size <= max_small_allocation) {
        size = object_size_with_alloc_site(size);
        ptr = cpu_mem.allocate_small(size);
    } else {
        std::cout<<"huge alloc"<<std::endl;
        ptr = allocate_large(size);
    }
    if (!ptr) {
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

reclaimer::reclaimer(reclaim_fn reclaim, reclaimer_scope scope)
    : _reclaim(std::move(reclaim))
    , _scope(scope) {
    cpu_mem.reclaimers.push_back(this);
}

reclaimer::~reclaimer() {
    auto& r = cpu_mem.reclaimers;
    r.erase(std::find(r.begin(), r.end(), this));
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



statistics stats() {
    return statistics{g_allocs, g_frees, g_cross_cpu_frees,
        cpu_mem.nr_pages * page_size, cpu_mem.nr_free_pages * page_size, g_reclaims};
}

bool drain_cross_cpu_freelist() {
    return cpu_mem.drain_cross_cpu_freelist();
}

translation
translate(const void* addr, size_t size) {
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

void set_min_free_pages(size_t pages) {
    cpu_mem.set_min_free_pages(pages);
}

} // namespace memory










inline bool engine_is_ready() {
    return local_engine != nullptr;
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
        std::cout<<"promise abandoned"<<std::endl;
        report_failed_future(_state->get_exception());
    }
}

template <typename Clock>
inline
timer<Clock>::timer(callback_t&& callback) : _callback(std::move(callback)) {
}


template <typename Clock>
inline
typename timer<Clock>::time_point timer<Clock>::get_timeout() {
    return _expiry;
}


template <typename Clock>
inline
bool timer<Clock>::cancel() {
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


template <typename... T>
inline
void promise<T...>::migrated() noexcept {
    if (_future) {
        _future->_promise = this;
    }
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

template <typename... T>
inline
future<T...>
promise<T...>::get_future() noexcept {
    assert(!_future && _state && !_task);
    return future<T...>(this);
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

template<typename CharType>
inline
future<> output_stream<CharType>::write(const char_type* buf) {
    return write(buf, strlen(buf));
}

template<typename CharType>
template<typename StringChar, typename SizeType, SizeType MaxSize>
inline
future<> output_stream<CharType>::write(const basic_sstring<StringChar, SizeType, MaxSize>& s) {
    return write(reinterpret_cast<const CharType *>(s.c_str()), s.size());
}

template<typename CharType>
inline
future<> output_stream<CharType>::write(const std::basic_string<CharType>& s) {
    return write(s.c_str(), s.size());
}

template<typename CharType>
future<> output_stream<CharType>::write(scattered_message<CharType> msg) {
    return write(std::move(msg).release());
}

template<typename CharType>
future<>
output_stream<CharType>::zero_copy_put(net::packet p) {
    // if flush is scheduled, disable it, so it will not try to write in parallel
    _flush = false;
    if (_flushing) {
        // flush in progress, wait for it to end before continuing
        return _in_batch.value().get_future().then([this, p = std::move(p)] () mutable {
            return _fd.put(std::move(p));
        });
    } else {
        return _fd.put(std::move(p));
    }
}

// Writes @p in chunks of _size length. The last chunk is buffered if smaller.
template <typename CharType>
future<>
output_stream<CharType>::zero_copy_split_and_put(net::packet p) {
    return repeat([this, p = std::move(p)] () mutable {
        if (p.len() < _size) {
            if (p.len()) {
                _zc_bufs = std::move(p);
            } else {
                _zc_bufs = net::packet::make_null_packet();
            }
            return make_ready_future<stop_iteration>(stop_iteration::yes);
        }
        auto chunk = p.share(0, _size);
        p.trim_front(_size);
        return zero_copy_put(std::move(chunk)).then([] {
            return stop_iteration::no;
        });
    });
}

template<typename CharType>
future<> output_stream<CharType>::write(net::packet p) {
    static_assert(std::is_same<CharType, char>::value, "packet works on char");

    if (p.len() != 0) {
        assert(!_end && "Mixing buffered writes and zero-copy writes not supported yet");

        if (_zc_bufs) {
            _zc_bufs.append(std::move(p));
        } else {
            _zc_bufs = std::move(p);
        }

        if (_zc_bufs.len() >= _size) {
            if (_trim_to_size) {
                return zero_copy_split_and_put(std::move(_zc_bufs));
            } else {
                return zero_copy_put(std::move(_zc_bufs));
            }
        }
    }
    return make_ready_future<>();
}

template<typename CharType>
future<> output_stream<CharType>::write(temporary_buffer<CharType> p) {
    if (p.empty()) {
        return make_ready_future<>();
    }
    assert(!_end && "Mixing buffered writes and zero-copy writes not supported yet");

    return write(net::packet(std::move(p)));
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_exactly_part(size_t n, tmp_buf out, size_t completed) {
    if (available()) {
        auto now = std::min(n - completed, available());
        std::copy(_buf.get(), _buf.get() + now, out.get_write() + completed);
        _buf.trim_front(now);
        completed += now;
    }
    if (completed == n) {
        return make_ready_future<tmp_buf>(std::move(out));
    }

    // _buf is now empty
    return _fd.get().then([this, n, out = std::move(out), completed] (auto buf) mutable {
        if (buf.size() == 0) {
            _eof = true;
            return make_ready_future<tmp_buf>(std::move(buf));
        }
        _buf = std::move(buf);
        return this->read_exactly_part(n, std::move(out), completed);
    });
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_exactly(size_t n) {
    if (_buf.size() == n) {
        // easy case: steal buffer, return to caller
        return make_ready_future<tmp_buf>(std::move(_buf));
    } else if (_buf.size() > n) {
        // buffer large enough, share it with caller
        auto front = _buf.share(0, n);
        _buf.trim_front(n);
        return make_ready_future<tmp_buf>(std::move(front));
    } else if (_buf.size() == 0) {
        // buffer is empty: grab one and retry
        return _fd.get().then([this, n] (auto buf) mutable {
            if (buf.size() == 0) {
                _eof = true;
                return make_ready_future<tmp_buf>(std::move(buf));
            }
            _buf = std::move(buf);
            return this->read_exactly(n);
        });
    } else {
        // buffer too small: start copy/read loop
        tmp_buf b(n);
        return read_exactly_part(n, std::move(b), 0);
    }
}

template <typename CharType>
template <typename Consumer>
future<>
input_stream<CharType>::consume(Consumer& consumer) {
    return repeat([&consumer, this] {
        if (_buf.empty() && !_eof) {
            return _fd.get().then([this] (tmp_buf buf) {
                _buf = std::move(buf);
                _eof = _buf.empty();
                return make_ready_future<stop_iteration>(stop_iteration::no);
            });
        }
        future<unconsumed_remainder> unconsumed = consumer(std::move(_buf));
        if (unconsumed.available()) {
            unconsumed_remainder u = std::get<0>(unconsumed.get());
            if (u) {
                // consumer is done
                _buf = std::move(u.value());
                return make_ready_future<stop_iteration>(stop_iteration::yes);
            }
            if (_eof) {
                return make_ready_future<stop_iteration>(stop_iteration::yes);
            }
            // If we're here, consumer consumed entire buffer and is ready for
            // more now. So we do not return, and rather continue the loop.
            // TODO: if we did too many iterations, schedule a call to
            // consume() instead of continuing the loop.
            return make_ready_future<stop_iteration>(stop_iteration::no);
        } else {
            // TODO: here we wait for the consumer to finish the previous
            // buffer (fulfilling "unconsumed") before starting to read the
            // next one. Consider reading ahead.
            return unconsumed.then([this] (unconsumed_remainder u) {
                if (u) {
                    // consumer is done
                    _buf = std::move(u.value());
                    return make_ready_future<stop_iteration>(stop_iteration::yes);
                } else {
                    // consumer consumed entire buffer, and is ready for more
                    return make_ready_future<stop_iteration>(stop_iteration::no);
                }
            });
        }
    });
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_up_to(size_t n) {
    using tmp_buf = temporary_buffer<CharType>;
    if (_buf.empty()) {
        if (_eof) {
            return make_ready_future<tmp_buf>();
        } else {
            return _fd.get().then([this, n] (tmp_buf buf) {
                _eof = buf.empty();
                _buf = std::move(buf);
                return read_up_to(n);
            });
        }
    } else if (_buf.size() <= n) {
        // easy case: steal buffer, return to caller
        return make_ready_future<tmp_buf>(std::move(_buf));
    } else {
        // buffer is larger than n, so share its head with a caller
        auto front = _buf.share(0, n);
        _buf.trim_front(n);
        return make_ready_future<tmp_buf>(std::move(front));
    }
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read() {
    using tmp_buf = temporary_buffer<CharType>;
    if (_eof) {
        return make_ready_future<tmp_buf>();
    }
    if (_buf.empty()) {
        return _fd.get().then([this] (tmp_buf buf) {
            _eof = buf.empty();
            return make_ready_future<tmp_buf>(std::move(buf));
        });
    } else {
        return make_ready_future<tmp_buf>(std::move(_buf));
    }
}

template <typename CharType>
future<>
input_stream<CharType>::skip(uint64_t n) {
    auto skip_buf = std::min(n, _buf.size());
    _buf.trim_front(skip_buf);
    n -= skip_buf;
    if (!n) {
        return make_ready_future<>();
    }
    return _fd.skip(n).then([this] (temporary_buffer<CharType> buffer) {
        _buf = std::move(buffer);
    });
}

// Writes @buf in chunks of _size length. The last chunk is buffered if smaller.
template <typename CharType>
future<>
output_stream<CharType>::split_and_put(temporary_buffer<CharType> buf) {
    assert(_end == 0);

    return repeat([this, buf = std::move(buf)] () mutable {
        if (buf.size() < _size) {
            if (!_buf) {
                _buf = _fd.allocate_buffer(_size);
            }
            std::copy(buf.get(), buf.get() + buf.size(), _buf.get_write());
            _end = buf.size();
            return make_ready_future<stop_iteration>(stop_iteration::yes);
        }
        auto chunk = buf.share(0, _size);
        buf.trim_front(_size);
        return put(std::move(chunk)).then([] {
            return stop_iteration::no;
        });
    });
}

template <typename CharType>
future<>
output_stream<CharType>::write(const char_type* buf, size_t n) {
    assert(!_zc_bufs && "Mixing buffered writes and zero-copy writes not supported yet");
    auto bulk_threshold = _end ? (2 * _size - _end) : _size;
    if (n >= bulk_threshold) {
        if (_end) {
            auto now = _size - _end;
            std::copy(buf, buf + now, _buf.get_write() + _end);
            _end = _size;
            temporary_buffer<char> tmp = _fd.allocate_buffer(n - now);
            std::copy(buf + now, buf + n, tmp.get_write());
            _buf.trim(_end);
            _end = 0;
            return put(std::move(_buf)).then([this, tmp = std::move(tmp)]() mutable {
                if (_trim_to_size) {
                    return split_and_put(std::move(tmp));
                } else {
                    return put(std::move(tmp));
                }
            });
        } else {
            temporary_buffer<char> tmp = _fd.allocate_buffer(n);
            std::copy(buf, buf + n, tmp.get_write());
            if (_trim_to_size) {
                return split_and_put(std::move(tmp));
            } else {
                return put(std::move(tmp));
            }
        }
    }

    if (!_buf) {
        _buf = _fd.allocate_buffer(_size);
    }

    auto now = std::min(n, _size - _end);
    std::copy(buf, buf + now, _buf.get_write() + _end);
    _end += now;
    if (now == n) {
        return make_ready_future<>();
    } else {
        temporary_buffer<char> next = _fd.allocate_buffer(_size);
        std::copy(buf + now, buf + n, next.get_write());
        _end = n - now;
        std::swap(next, _buf);
        return put(std::move(next));
    }
}

template <typename CharType>
future<>
output_stream<CharType>::flush() {
    if (!_batch_flushes) {
        if (_end) {
            _buf.trim(_end);
            _end = 0;
            return put(std::move(_buf)).then([this] {
                return _fd.flush();
            });
        } else if (_zc_bufs) {
            return zero_copy_put(std::move(_zc_bufs)).then([this] {
                return _fd.flush();
            });
        }
    } else {
        if (_ex) {
            // flush is a good time to deliver outstanding errors
            return make_exception_future<>(std::move(_ex));
        } else {
            _flush = true;
            if (!_in_batch) {
                add_to_flush_poller(this);
                _in_batch = promise<>();
            }
        }
    }
    return make_ready_future<>();
}

void add_to_flush_poller(output_stream<char>* x);

template <typename CharType>
future<>
output_stream<CharType>::put(temporary_buffer<CharType> buf) {
    // if flush is scheduled, disable it, so it will not try to write in parallel
    _flush = false;
    if (_flushing) {
        // flush in progress, wait for it to end before continuing
        return _in_batch.value().get_future().then([this, buf = std::move(buf)] () mutable {
            return _fd.put(std::move(buf));
        });
    } else {
        return _fd.put(std::move(buf));
    }
}

template <typename CharType>
void output_stream<CharType>::poll_flush() {
    if (!_flush) {
        // flush was canceled, do nothing
        _flushing = false;
        _in_batch.value().set_value();
        _in_batch = std::nullopt;
        return;
    }
    auto f = make_ready_future();
    _flush = false;
    _flushing = true; // make whoever wants to write into the fd to wait for flush to complete

    if (_end) {
        // send whatever is in the buffer right now
        _buf.trim(_end);
        _end = 0;
        f = _fd.put(std::move(_buf));
    } else if(_zc_bufs) {
        f = _fd.put(std::move(_zc_bufs));
    }

    f.then([this] {
        return _fd.flush();
    }).then_wrapped([this] (future<> f) {
        try {
            f.get();
        } catch (...) {
            _ex = std::current_exception();
        }
        // if flush() was called while flushing flush once more
        poll_flush();
    });
}

template <typename CharType>
future<>
output_stream<CharType>::close() {
    return flush().finally([this] {
        if (_in_batch) {
            return _in_batch.value().get_future();
        } else {
            return make_ready_future();
        }
    }).then([this] {
        // report final exception as close error
        if (_ex) {
            std::rethrow_exception(_ex);
        }
    }).finally([this] {
        return _fd.close();
    });
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
    std::vector<sstring> ret;
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
        const tcp_keepalive_params& pms = std::get<tcp_keepalive_params>(params);
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

template <>
class posix_connected_socket_operations<transport::SCTP> {
public:
    void set_nodelay(file_desc& _fd, bool nodelay) {
        _fd.setsockopt(SOL_SCTP, SCTP_NODELAY, int(nodelay));
    }
    bool get_nodelay(file_desc& _fd) const {
        return _fd.getsockopt<int>(SOL_SCTP, SCTP_NODELAY);
    }
    void set_keepalive(file_desc& _fd, bool keepalive) {
        auto heartbeat = _fd.getsockopt<sctp_paddrparams>(SOL_SCTP, SCTP_PEER_ADDR_PARAMS);
        if (keepalive) {
            heartbeat.spp_flags |= SPP_HB_ENABLE;
        } else {
            heartbeat.spp_flags &= ~SPP_HB_ENABLE;
        }
        _fd.setsockopt(SOL_SCTP, SCTP_PEER_ADDR_PARAMS, heartbeat);
    }
    bool get_keepalive(file_desc& _fd) const {
        return _fd.getsockopt<sctp_paddrparams>(SOL_SCTP, SCTP_PEER_ADDR_PARAMS).spp_flags & SPP_HB_ENABLE;
    }
    void set_keepalive_parameters(file_desc& _fd, const keepalive_params& kpms) {
        const sctp_keepalive_params& pms = std::get<sctp_keepalive_params>(kpms);
        auto params = _fd.getsockopt<sctp_paddrparams>(SOL_SCTP, SCTP_PEER_ADDR_PARAMS);
        params.spp_hbinterval = pms.interval.count() * 1000; // in milliseconds
        params.spp_pathmaxrxt = pms.count;
        _fd.setsockopt(SOL_SCTP, SCTP_PEER_ADDR_PARAMS, params);
    }
    keepalive_params get_keepalive_parameters(file_desc& _fd) const {
        auto params = _fd.getsockopt<sctp_paddrparams>(SOL_SCTP, SCTP_PEER_ADDR_PARAMS);
        return sctp_keepalive_params {
            std::chrono::seconds(params.spp_hbinterval/1000), // in seconds
            params.spp_pathmaxrxt
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

    virtual future<connected_socket> connect(socket_address sa, socket_address local, transport proto = transport::TCP) override {
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
future<connected_socket, socket_address>
posix_server_socket_impl<Transport>::accept() {
    return _lfd.accept().then([this] (pollable_fd fd, socket_address sa) {
        static unsigned balance = 0;
        auto cpu = balance++ % smp::count;

        if (cpu == engine().cpu_id()) {
            std::unique_ptr<connected_socket_impl> csi(
                    new posix_connected_socket_impl<Transport>(std::make_shared<pollable_fd>(std::move(fd))));
            return make_ready_future<connected_socket, socket_address>(
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
future<connected_socket, socket_address> posix_ap_server_socket_impl<Transport>::accept() {
    auto conni = get_conn_q().find(_sa.as_posix_sockaddr_in());
    if (conni != get_conn_q().end()) {
        connection c = std::move(conni->second);
        get_conn_q().erase(conni);
        try {
            std::unique_ptr<connected_socket_impl> csi(
                    new posix_connected_socket_impl<Transport>(std::make_shared<pollable_fd>(std::move(c.fd))));
            return make_ready_future<connected_socket, socket_address>(connected_socket(std::move(csi)), std::move(c.addr));
        } catch (...) {
            return make_exception_future<connected_socket, socket_address>(std::current_exception());
        }
    } else {
        try {
            auto i = get_sockets().emplace(std::piecewise_construct, std::make_tuple(_sa.as_posix_sockaddr_in()), std::make_tuple());
            assert(i.second);
            return i.first->second.get_future();
        } catch (...) {
            return make_exception_future<connected_socket, socket_address>(std::current_exception());
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
future<connected_socket, socket_address>
posix_reuseport_server_socket_impl<Transport>::accept() {
    return _lfd.accept().then([] (pollable_fd fd, socket_address sa) {
        std::unique_ptr<connected_socket_impl> csi(
                new posix_connected_socket_impl<Transport>(std::make_shared(std::move(fd))));
        return make_ready_future<connected_socket, socket_address>(
            connected_socket(std::move(csi)), sa);
    });
}

template <transport Transport>
void
posix_reuseport_server_socket_impl<Transport>::abort_accept() {
    _lfd.abort_reader(std::make_exception_ptr(std::system_error(ECONNABORTED, std::system_category())));
}

template <transport Transport>
void  posix_ap_server_socket_impl<Transport>::move_connected_socket(socket_address sa, pollable_fd fd, socket_address addr) {
    auto i = get_sockets().find(sa.as_posix_sockaddr_in());
    if (i != get_sockets().end()) {
        try {
            std::unique_ptr<connected_socket_impl> csi(new posix_connected_socket_impl<Transport>(std::make_shared(std::move(fd))));
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
posix_network_stack::listen(socket_address sa, listen_options opt) {
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
posix_ap_network_stack::listen(socket_address sa, listen_options opt) {
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
        socket_address _src_addr;
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
        socket_address _dst;
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

void connected_socket::set_keepalive_parameters(const net::keepalive_params& p) {
    _csi->set_keepalive_parameters(p);
}

net::keepalive_params connected_socket::get_keepalive_parameters() const {
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

future<connected_socket> net::socket::connect(socket_address sa, socket_address local, transport proto) {
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
future<connected_socket, socket_address> server_socket::accept() {
    return _ssi->accept();
}
void server_socket::abort_accept() {
    _ssi->abort_accept();
}

socket_address::socket_address(ipv4_addr addr)
    : socket_address(make_ipv4_address(addr))
{}


bool socket_address::operator==(const socket_address& a) const {
    // TODO: handle ipv6
    return std::tie(u.in.sin_family, u.in.sin_port, u.in.sin_addr.s_addr)
                    == std::tie(a.u.in.sin_family, a.u.in.sin_port,
                                    a.u.in.sin_addr.s_addr);
}

pollable_fd
reactor::posix_listen(socket_address sa, listen_options opts) {
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
reactor::make_pollable_fd(socket_address sa, transport proto) {
    file_desc fd = file_desc::socket(sa.u.sa.sa_family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, int(proto));
    return std::make_shared<pollable_fd>(pollable_fd(std::move(fd)));
}

future<>
reactor::posix_connect(std::shared_ptr<pollable_fd> pfd, socket_address sa, socket_address local) {
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
reactor::listen(socket_address sa, listen_options opt) {
    return server_socket(_network_stack->listen(sa, opt));
}

future<connected_socket>
reactor::connect(socket_address sa) {
    return _network_stack->connect(sa);
}

future<connected_socket>
reactor::connect(socket_address sa, socket_address local, transport proto) {
    return _network_stack->connect(sa, local, proto);
}




inline
future<pollable_fd, socket_address>
reactor::accept(pollable_fd_state& listenfd) {
    return readable(listenfd).then([&listenfd] () mutable {
        socket_address sa;
        socklen_t sl = sizeof(&sa.u.sas);
        file_desc fd = listenfd.fd.accept(sa.u.sa, sl, SOCK_NONBLOCK | SOCK_CLOEXEC);
        pollable_fd pfd(std::move(fd), pollable_fd::speculation(EPOLLOUT));
        return make_ready_future<pollable_fd, socket_address>(std::move(pfd), std::move(sa));
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

// Some kernels can append to xfs filesystems, some cannot; determine
// from kernel version.
static
unsigned
xfs_concurrency_from_kernel_version() {
    // auto num = [] (std::csub_match x) {
    //     auto b = x.first;
    //     auto e = x.second;
    //     if (*b == '.') {
    //         ++b;
    //     }
    //     return std::stoi(std::string(b, e));
    // };
    // struct utsname buf;
    // auto r = ::uname(&buf);

    // throw_system_error_on(r == -1);
    // // 2-4 dotted decimal numbers, optional "-anything"
    // auto generic_re = std::regex(R"XX((\d+)(\.\d+)(\.\d+)?(\.\d+)?(-.*)?)XX");
    // std::cmatch m1;
    // // try to see if this is a mainline kernel with xfs append fixed (3.15+)
    // if (std::regex_match(buf.release, m1, generic_re)) {
    //     auto maj = num(m1[1]);
    //     auto min = num(m1[2]);
    //     if (maj > 3 || (maj == 3 && min >= 15)) {
    //         // Can append, but not concurrently
    //         return 1;
    //     }
    // }
    // // 3.10.0-num1.num2?.num3?.el7.anything
    // auto rhel_re = std::regex(R"XX(3\.10\.0-(\d+)(\.\d+)?(\.\d+)?\.el7.*)XX");
    // std::cmatch m2;
    // // try to see if this is a RHEL kernel with the backported fix (3.10.0-325.el7+)
    // if (std::regex_match(buf.release, m2, rhel_re)) {
    //     auto rmaj = num(m2[1]);
    //     if (rmaj >= 325) {
    //         // Can append, but not concurrently
    //         return 1;
    //     }
    // }
    // Cannot append at all; need ftrucnate().
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
        if (fd != -1) {
            fsxattr attr = {};
            if (options.extent_allocation_size_hint) {
                attr.fsx_xflags |= XFS_XFLAG_EXTSIZE;
                attr.fsx_extsize = options.extent_allocation_size_hint;
            }
            // Ignore error; may be !xfs, and just a hint anyway
            ::ioctl(fd, XFS_IOC_FSSETXATTR, &attr);
        }
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

::file_handle
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
        seastar_logger.warn("double close() detected, contact support");
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
            report_exception("Running ::close() in reactor thread, submission failed with exception", std::current_exception());
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