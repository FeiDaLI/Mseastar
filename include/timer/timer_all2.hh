// #include <chrono>
// #include <limits>
// #include <bitset>
// #include <array>
// #include <atomic>
// #include <list>
// #include <deque>
// #include <unordered_map>
// #include <boost/program_options.hpp>
// #include <boost/filesystem.hpp>
// #include <experimental/optional>
// #include <iostream>
// #include <time.h>
// #include <signal.h>
// #include "../task/task.hh"
// #include "bitset-iter.hh"
// #include <thread>
// #include <iomanip>

// using namespace std::chrono_literals;
// std::ostream& operator<<(std::ostream& os, const std::chrono::steady_clock::time_point& tp) {
//     auto duration = tp.time_since_epoch();
//     auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
//     duration -= hours;
//     auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
//     duration -= minutes;
//     auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
//     os << hours.count() << "h " << minutes.count() << "m " << seconds.count() << "s";
//     return os;
// }

// #include <bitset>
// #include <limits>


// inline
// sigset_t make_empty_sigset_mask() {
//     sigset_t set;
//     sigemptyset(&set);
//     return set;
// }

// inline int alarm_signal() {
//     // We don't want to use SIGALRM, because the boost unit test library
//     // also plays with it.
//     return SIGALRM;
//     return SIGRTMIN;
// }

// inline
// sigset_t make_sigset_mask(int signo) {
//     sigset_t set;
//     sigemptyset(&set);
//     sigaddset(&set, signo);
//     return set;
// }

// template <typename T>
// inline
// void throw_pthread_error(T r) {
//     if (r != 0) {
//         throw std::system_error(r, std::system_category());
//     }
// }



// struct signals {
//         signals();
//         ~signals();
//         bool poll_signal();
//         bool pure_poll_signal() const;
//         void handle_signal(int signo, std::function<void ()>&& handler);
//         void handle_signal_once(int signo, std::function<void ()>&& handler);
//         static void action(int signo, siginfo_t* siginfo, void* ignore);
//         struct signal_handler {
//             signal_handler(int signo, std::function<void ()>&& handler);
//             std::function<void ()> _handler;
//         };
//         std::atomic<uint64_t> _pending_signals;
//         std::unordered_map<int, signal_handler> _signal_handlers;
        
// };








// namespace bitsets {

// static constexpr int ulong_bits = std::numeric_limits<unsigned long>::digits;

// /**
//  * Returns the number of leading zeros in value's binary representation.
//  *
//  * If value == 0 the result is undefied. If T is signed and value is negative
//  * the result is undefined.
//  *
//  * The highest value that can be returned is std::numeric_limits<T>::digits - 1,
//  * which is returned when value == 1.
//  */
// template<typename T>
// inline size_t count_leading_zeros(T value);

// /**
//  * Returns the number of trailing zeros in value's binary representation.
//  *
//  * If value == 0 the result is undefied. If T is signed and value is negative
//  * the result is undefined.
//  *
//  * The highest value that can be returned is std::numeric_limits<T>::digits - 1.
//  */
// template<typename T>
// static inline size_t count_trailing_zeros(T value);

// template<>
// inline size_t count_leading_zeros<unsigned long>(unsigned long value)
// {
//     return __builtin_clzl(value);
// }

// template<>
// inline size_t count_leading_zeros<long>(long value)
// {
//     return __builtin_clzl((unsigned long)value) - 1;
// }

// template<>
// inline size_t count_leading_zeros<long long>(long long value)
// {
//     return __builtin_clzll((unsigned long long)value) - 1;
// }

// template<>
// inline
// size_t count_trailing_zeros<unsigned long>(unsigned long value)
// {
//     return __builtin_ctzl(value);
// }

// template<>
// inline
// size_t count_trailing_zeros<long>(long value)
// {
//     return __builtin_ctzl((unsigned long)value);
// }

// /**
//  * Returns the index of the first set bit.
//  * Result is undefined if bitset.any() == false.
//  */
// template<size_t N>
// static inline size_t get_first_set(const std::bitset<N>& bitset)
// {
//     static_assert(N <= ulong_bits, "bitset too large");
//     return count_trailing_zeros(bitset.to_ulong());
// }

// /**
//  * Returns the index of the last set bit in the bitset.
//  * Result is undefined if bitset.any() == false.
//  */
// template<size_t N>
// static inline size_t get_last_set(const std::bitset<N>& bitset)
// {
//     static_assert(N <= ulong_bits, "bitset too large");
//     return ulong_bits - 1 - count_leading_zeros(bitset.to_ulong());
// }

// template<size_t N>
// class set_iterator : public std::iterator<std::input_iterator_tag, int>
// {
// private:
//     void advance()
//     {
//         if (_bitset.none()) {
//             _index = -1;
//         } else {
//             auto shift = get_first_set(_bitset) + 1;
//             _index += shift;
//             _bitset >>= shift;
//         }
//     }
// public:
//     set_iterator(std::bitset<N> bitset, int offset = 0)
//         : _bitset(bitset)
//         , _index(offset - 1)
//     {
//         static_assert(N <= ulong_bits, "This implementation is inefficient for large bitsets");
//         _bitset >>= offset;
//         advance();
//     }

//     void operator++()
//     {
//         advance();
//     }

//     int operator*() const
//     {
//         return _index;
//     }

//     bool operator==(const set_iterator& other) const
//     {
//         return _index == other._index;
//     }

//     bool operator!=(const set_iterator& other) const
//     {
//         return !(*this == other);
//     }
// private:
//     std::bitset<N> _bitset;
//     int _index;
// };

// template<size_t N>
// class set_range
// {
// public:
//     using iterator = set_iterator<N>;
//     using value_type = int;

//     set_range(std::bitset<N> bitset, int offset = 0)
//         : _bitset(bitset)
//         , _offset(offset)
//     {
//     }

//     iterator begin() const { return iterator(_bitset, _offset); }
//     iterator end() const { return iterator(0); }
// private:
//     std::bitset<N> _bitset;
//     int _offset;
// };

// template<size_t N>
// static inline set_range<N> for_each_set(std::bitset<N> bitset, int offset = 0)
// {
//     return set_range<N>(bitset, offset);
// }

// }



// class reactor;
// reactor& engine();
// template <typename Clock> class timer;
// using steady_clock_type = std::chrono::steady_clock;

// class manual_clock {
//     public:
//         using rep = int64_t;
//         using period = std::chrono::nanoseconds::period;
//         using duration = std::chrono::duration<rep, period>;
//         using time_point = std::chrono::time_point<manual_clock, duration>;
//     private:
//         static std::atomic<rep> _now;
//     public:
//         manual_clock();
//         static time_point now() {
//             return time_point(duration(_now.load(std::memory_order_relaxed)));
//         }
//         static void advance(duration d);
//         static void expire_timers();
// };



// class lowres_clock {
// public:
//     typedef int64_t rep;
//     // The lowres_clock's resolution is 10ms. However, to make it is easier to
//     // do calcuations with std::chrono::milliseconds, we make the clock's
//     // period to 1ms instead of 10ms.
//     typedef std::ratio<1, 1000> period;
//     typedef std::chrono::duration<rep, period> duration;
//     typedef std::chrono::time_point<lowres_clock, duration> time_point;
//     lowres_clock();
//     ~lowres_clock();
//     static time_point now() {
//         auto nr = _now.load(std::memory_order_relaxed);
//         return time_point(duration(nr));
//     }
// private:
//     static void update();
//     // _now is updated by cpu0 and read by other cpus. Make _now on its own
//     // cache line to avoid false sharing.
//     static std::atomic<rep> _now [[gnu::aligned(64)]];
//     // High resolution timer to drive this low resolution clock
//     struct timer_deleter {
//         void operator()(void*) const;
//     };
//     std::unique_ptr<void, timer_deleter> _timer;
//     // High resolution timer expires every 10 milliseconds
//     static constexpr std::chrono::milliseconds _granularity{10};
// };




// // Utility function for error handling
// inline void throw_system_error_on(bool condition) {
//     if (condition) {
//         throw std::system_error(errno, std::system_category());
//     }
// }

// using steady_clock_type = std::chrono::steady_clock;
// // timer 有默认参数,所以timer<>表示一个steady_clock_type
// template <typename Clock = steady_clock_type>
// class timer {
// public:
//     timer();
//     using time_point = typename Clock::time_point;
//     using duration = typename Clock::duration;
//     typedef Clock clock;
//     using callback_t = std::function<void()>;
//     using iterator = typename std::list<timer*>::iterator;
//     iterator it; // 新增的迭代器成员
//     iterator expired_it;  // 过期链表中的位置
//     // boost::intrusive::list_member_hook<> _link;
//     callback_t _callback;
//     time_point _expiry; //到期时间点,每个timer都有一个到期时间点.
//     std::experimental::optional<duration> _period;
//     bool _armed = false;
//     bool _queued = false;
//     bool _expired = false;
//     void readd_periodic();
//     void arm_state(time_point until, std::experimental::optional<duration> period);
//     timer(timer&& t) noexcept;
//     explicit timer(callback_t&& callback);
//     ~timer();
//     // future<> expired();
//     void set_callback(callback_t&& callback);
//     void arm(time_point until, std::experimental::optional<duration> period = {});
//     void rearm(time_point until, std::experimental::optional<duration> period = {});
//     void rearm(duration delta) { rearm(Clock::now() + delta); }
//     void arm(duration delta);
//     void arm_periodic(duration delta);
//     bool armed() const { return _armed; }
//     bool cancel();
//     time_point get_timeout();
// };

// // Now include the timer set implementation
// #include "time_set_.hh"

// // Implement timer methods
// template <typename Clock>
// inline timer<Clock>::timer() = default;

// template <typename Clock>
// inline
// timer<Clock>::timer(callback_t&& callback) : _callback(std::move(callback)) {
//     std::cout<<"绑定回调函数"<<std::endl;
//     // _callback();
// }

// template <typename Clock>
// inline
// timer<Clock>::~timer() {
//     if (_queued) {
//         del_timer(this);
//     }
// }

// template <typename Clock>
// inline
// void timer<Clock>::set_callback(callback_t&& callback) {
//     _callback = std::move(callback);
// }

// template <typename Clock>
// inline
// void timer<Clock>::arm_state(time_point until, std::experimental::optional<duration> period) {
//     assert(!_armed);
//     _period = period;
//     _armed = true;
//     _expired = false;
//     _expiry = until;
//     _queued = true;//_queued表示这个_timer是否在list中
// }



// template <typename Clock>
// inline
// void timer<Clock>::arm(time_point until, std::experimental::optional<duration> period) {
//     arm_state(until, period);
//     add_timer(this);
// }

// template <typename Clock>
// inline
// void timer<Clock>::rearm(time_point until, std::experimental::optional<duration> period) {
//     if (_armed) {
//         cancel();
//     }
//     arm(until, period);
// }

// template <typename Clock>
// inline
// void timer<Clock>::arm(duration delta) {
//     return arm(Clock::now() + delta);
// }

// template <typename Clock>
// inline
// void timer<Clock>::arm_periodic(duration delta) {
//     arm(Clock::now() + delta, {delta});
// }

// /*
// 这个函数有什么用？
// 如果timer是周期性的，那么arm_state会设置一个周期，然后调用queue_timer.
// */

// template <typename Clock>
// inline
// void timer<Clock>::readd_periodic() {
//     arm_state(Clock::now() + _period.value(), {_period.value()});
//     queue_timer(this);
// }

// template <typename Clock>
// inline
// bool timer<Clock>::cancel() {
//     if (!_armed) {
//         return false;
//     }
//     _armed = false;
//     if (_queued) {
//         del_timer(this);
//         _queued = false;
//     }
//     return true;
// }
// /*
// 所以timer是一个状态机。
// */

// template <typename Clock>
// inline
// typename timer<Clock>::time_point timer<Clock>::get_timeout() {
//     return _expiry;
// }



// template <typename Clock>
// inline
// timer<Clock>::timer(timer&& t) noexcept
//     : _callback(std::move(t._callback))
//     , _expiry(t._expiry)
//     , _period(t._period)
//     , _armed(t._armed)
//     , _queued(false)
//     , _expired(t._expired){
//     if (t._queued) {
//         del_timer(&t);
//         t._queued = false;
//     }
//     t._armed = false;
//     t._expired = false;
//     if (_armed) {
//         _queued = true;
//         add_timer(this);
//     }
// }

// // Forward declarations for timer-related functions
// void enable_timer(steady_clock_type::time_point when);

// // Function declarations
// bool queue_timer(timer<steady_clock_type>* tmr);
// void add_timer(timer<steady_clock_type>* tmr);
// void del_timer(timer<steady_clock_type>* tmr);

// bool queue_timer(timer<lowres_clock>* tmr);
// void add_timer(timer<lowres_clock>* tmr);
// void del_timer(timer<lowres_clock>* tmr);

// bool queue_timer(timer<manual_clock>* tmr);
// void add_timer(timer<manual_clock>* tmr);
// void del_timer(timer<manual_clock>* tmr);

// template<typename Timer>
// class timer_set {
// public:
//     using time_point = typename Timer::time_point;
//     using timer_list_t = std::list<Timer*>;
//     using duration = typename Timer::duration;
//     using timestamp_t = typename duration::rep;
//     static constexpr timestamp_t max_timestamp = std::numeric_limits<timestamp_t>::max();
//     static constexpr int timestamp_bits = std::numeric_limits<timestamp_t>::digits;//63
//     static constexpr int n_buckets = timestamp_bits + 1;//64
//     std::array<timer_list_t, n_buckets> _buckets;
//     timestamp_t _last;
//     timestamp_t _next; 
//     std::bitset<n_buckets> _non_empty_buckets;
//     /// \brief 获取时间点对应的时间戳（计数）
//     /// \param tp 时间点对象
//     /// \return 时间点自纪元以来的计数值
//     static timestamp_t get_timestamp(time_point tp) {
//         return tp.time_since_epoch().count();
//     }
//     /// \brief 获取定时器的超时时间戳
//     /// \param timer 定时器对象
//     /// \return 定时器超时时间的时间戳
//     static timestamp_t get_timestamp(Timer& timer) {
//         return get_timestamp(timer.get_timeout());
//     }
//     /// \brief 根据时间戳计算对应的桶索引
//     /// \param timestamp 要计算的定时器时间戳
//     /// \return 对应的桶索引
//     int get_index(timestamp_t timestamp) const {
//         if (timestamp <= _last) {
//             return n_buckets - 1;
//         }
//         auto index = bitsets::count_leading_zeros(timestamp ^ _last);
//         assert(index < n_buckets - 1);
//         return index;
//     }
//     /*
//     这个需要手动推导一下
//     */

//     /// \brief 获取定时器对应的桶索引
//     /// \param timer 定时器对象
//     /// \return 对应的桶索引
//     int get_index(Timer& timer) const {
//         return get_index(get_timestamp(timer));
//     }
//     /*
//     一个timer有唯一的一个过期时间。
//     */

//     /// \brief 获取最后一个非空桶的索引
//     /// \return 最后一个非空桶的索引
//     int get_last_non_empty_bucket() const {
//         return bitsets::get_last_set(_non_empty_buckets);
//     }

// public:
//     /// \brief 构造函数初始化成员变量
//     timer_set() : _last(0), _next(max_timestamp), _non_empty_buckets(0) {}

//     ~timer_set() {
//         // 清理所有定时器资源
//         for (auto& list : _buckets) {
//             while (!list.empty()) {
//                 auto* timer = list.front();
//                 list.pop_front();
//                 timer->cancel();
//             }
//         }
//     }

//     /// \brief 将定时器插入到对应的桶中
//     /// \param timer 要插入的定时器对象
//     /// \return true 如果插入后_next被更新为更小的值，否则false
//     bool insert(Timer& timer) {
//         auto timestamp = get_timestamp(timer);
//         auto index = get_index(timestamp);
//         auto& list = _buckets[index];
//         list.push_back(&timer);
//         timer.it = --list.end();//timer.it是timer在list中的迭代器(使用尾插法,所以end前一个位置就是最后一个元素前开后闭)
//         _non_empty_buckets[index] = true;
//         if (timestamp < _next) {
//             _next = timestamp;
//             return true;
//         }
//         return false;
//     }
//     /*
//         next是边插入边维护的一个变量.表示下一次过期的时间.
//     */

//     /// \brief 从集合中移除定时器
//     /// \param timer 要移除的定时器对象
//     void remove(Timer& timer) {
//         auto index = get_index(timer);
//         auto& list = _buckets[index];
//         list.erase(timer.it);//erase一个节点会造成内存泄漏吗? 不会:见 STL源码, 解析
//         if (list.empty()) {
//             _non_empty_buckets[index] = false;
//         }
//     }
//     /** 
//      * 
//      * 这个地方像是RTOS优先级位图
//      * 
//     */
//     /// \brief 获取已到期的定时器列表
//     /// \param now 当前时间点
//     /// \return 包含所有已到期定时器的列表
//     timer_list_t expire(time_point now) {
//         timer_list_t exp;
//         auto timestamp = get_timestamp(now);
//         if (timestamp < _last) {
//             abort();
//         }
//         //当前时间一定>=_last
//         auto index = get_index(timestamp);
//         // 处理所有在当前时间之前的非空桶
//         for (int i : bitsets::for_each_set(_non_empty_buckets, index + 1)) {
//             exp.splice(exp.end(), _buckets[i]);
//             _non_empty_buckets[i] = false;
//         }
//         /*
//             把所有过期的链表添加到exp后面(exp是一个临时的链表，操作时间间复杂度O(1)).
//         */
//         _last = timestamp;//所以_last就是最后一次处理过期定时器的时间.
//         _next = max_timestamp;//_next设置为无穷.
//         auto& list = _buckets[index];
//         // 处理当前索引的桶中的定时器
//         while (!list.empty()) {
//             auto* timer = list.front();
//             list.pop_front();
//             if (timer->get_timeout() <= now) {
//                 exp.push_back(timer);
//             } else {
//                 insert(*timer);
//             }
//         }
//         _non_empty_buckets[index] = !list.empty();
//         if (_next == max_timestamp && _non_empty_buckets.any()) {
//             // 更新_next为最后一个非空桶中的最小时间戳
//             for (auto* timer : _buckets[get_last_non_empty_bucket()]) {
//                 _next = std::min(_next, get_timestamp(*timer));
//             }
//         }
//         return exp;//返回这个链表
//     }

//     /// \brief 获取下一个超时时间点
//     /// \return 下一个超时时间点
//     time_point get_next_timeout() const {
//         return time_point(duration(std::max(_last, _next)));
//     }

//     /// \brief 清空所有定时器
//     void clear() {
//         for (auto& list : _buckets) {
//             list.clear();
//         }
//         _non_empty_buckets.reset();
//     }

//     /// \brief 获取集合中定时器总数
//     /// \return 定时器数量
//     size_t size() const {
//         size_t res = 0;
//         for (const auto& list : _buckets) {
//             res += list.size();
//         }
//         return res;
//     }

//     /// \brief 判断集合是否为空
//     /// \return true 如果集合中没有定时器，否则false
//     bool empty() const {
//         return _non_empty_buckets.none();
//     }

//     /// \brief 获取当前时间点
//     /// \return 当前时间点
//     time_point now() {
//         return Timer::clock::now();
//     }
// };
// /*---------------------------------------------信号量和条件变量-------------------------------------------------------------*/

// /// \addtogroup fiber-module
// /// @{

// /// Exception thrown when a semaphore is broken by
// /// \ref semaphore::broken().
// class broken_semaphore : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Semaphore broken";
//     }
// };

// /// Exception thrown when a semaphore wait operation
// /// times out.
// ///
// /// \see semaphore::wait(typename timer<>::duration timeout, size_t nr)
// class semaphore_timed_out : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Semaphore timedout";
//     }
// };

// /// Exception Factory for standard semaphore
// ///
// /// constructs standard semaphore exceptions
// /// \see semaphore_timed_out and broken_semaphore
// struct semaphore_default_exception_factory {
//     static semaphore_timed_out timeout() {
//         return semaphore_timed_out();
//     }
//     static broken_semaphore broken() {
//         return broken_semaphore();
//     }
// };

// /// \brief Counted resource guard.
// ///
// /// This is a standard computer science semaphore, adapted
// /// for futures.  You can deposit units into a counter,
// /// or take them away.  Taking units from the counter may wait
// /// if not enough units are available.
// ///
// /// To support exceptional conditions, a \ref broken() method
// /// is provided, which causes all current waiters to stop waiting,
// /// with an exceptional future returned.  This allows causing all
// /// fibers that are blocked on a semaphore to continue.  This is
// /// similar to POSIX's `pthread_cancel()`, with \ref wait() acting
// /// as a cancellation point.
// ///
// /// \tparam ExceptionFactory template parameter allows modifying a semaphore to throw
// /// customized exceptions on timeout/broken(). It has to provide two static functions
// /// ExceptionFactory::timeout() and ExceptionFactory::broken() which return corresponding
// /// exception object.
// template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
// class basic_semaphore {
// public:
//     using duration = typename timer<Clock>::duration;
//     using clock = typename timer<Clock>::clock;
//     using time_point = typename timer<Clock>::time_point;
// private:
//     ssize_t _count;
//     std::exception_ptr _ex;
//     struct entry {
//         promise<> pr;
//         size_t nr;
//         entry(promise<>&& pr_, size_t nr_) : pr(std::move(pr_)), nr(nr_) {}
//     };
//     struct expiry_handler {
//         void operator()(entry& e) noexcept {
//             e.pr.set_exception(ExceptionFactory::timeout());
//         }
//     };
//     expiring_fifo<entry, expiry_handler, clock> _wait_list;
//     bool has_available_units(size_t nr) const {
//         return _count >= 0 && (static_cast<size_t>(_count) >= nr);
//     }
//     bool may_proceed(size_t nr) const {
//         return has_available_units(nr) && _wait_list.empty();
//     }
// public:
//     /// Returns the maximum number of units the semaphore counter can hold
//     static constexpr size_t max_counter() {
//         return std::numeric_limits<decltype(_count)>::max();
//     }

//     /// Constructs a semaphore object with a specific number of units
//     /// in its internal counter. E.g., starting it at 1 is suitable for use as
//     /// an unlocked mutex.
//     ///
//     /// \param count number of initial units present in the counter.
//     basic_semaphore(size_t count) : _count(count) {}
//     /// Waits until at least a specific number of units are available in the
//     /// counter, and reduces the counter by that amount of units.
//     ///
//     /// \note Waits are serviced in FIFO order, though if several are awakened
//     ///       at once, they may be reordered by the scheduler.
//     ///
//     /// \param nr Amount of units to wait for (default 1).
//     /// \return a future that becomes ready when sufficient units are available
//     ///         to satisfy the request.  If the semaphore was \ref broken(), may
//     ///         contain an exception.
//     future<> wait(size_t nr = 1) {
//         return wait(time_point::max(), nr);
//     }
//     /// Waits until at least a specific number of units are available in the
//     /// counter, and reduces the counter by that amount of units.  If the request
//     /// cannot be satisfied in time, the request is aborted.
//     ///
//     /// \note Waits are serviced in FIFO order, though if several are awakened
//     ///       at once, they may be reordered by the scheduler.
//     ///
//     /// \param timeout expiration time.
//     /// \param nr Amount of units to wait for (default 1).
//     /// \return a future that becomes ready when sufficient units are available
//     ///         to satisfy the request.  On timeout, the future contains a
//     ///         \ref semaphore_timed_out exception.  If the semaphore was
//     ///         \ref broken(), may contain an exception.
//     future<> wait(time_point timeout, size_t nr = 1) {
//         if (may_proceed(nr)) {
//             _count -= nr;
//             return make_ready_future<>();
//         }
//         if (_ex) {
//             return make_exception_future(_ex);
//         }
//         promise<> pr;
//         auto fut = pr.get_future();
//         _wait_list.push_back(entry(std::move(pr), nr), timeout);
//         return fut;
//     }

//     /// Waits until at least a specific number of units are available in the
//     /// counter, and reduces the counter by that amount of units.  If the request
//     /// cannot be satisfied in time, the request is aborted.
//     ///
//     /// \note Waits are serviced in FIFO order, though if several are awakened
//     ///       at once, they may be reordered by the scheduler.
//     ///
//     /// \param timeout how long to wait.
//     /// \param nr Amount of units to wait for (default 1).
//     /// \return a future that becomes ready when sufficient units are available
//     ///         to satisfy the request.  On timeout, the future contains a
//     ///         \ref semaphore_timed_out exception.  If the semaphore was
//     ///         \ref broken(), may contain an exception.
//     future<> wait(duration timeout, size_t nr = 1) {
//         return wait(clock::now() + timeout, nr);
//     }
//     /// Deposits a specified number of units into the counter.
//     ///
//     /// The counter is incremented by the specified number of units.
//     /// If the new counter value is sufficient to satisfy the request
//     /// of one or more waiters, their futures (in FIFO order) become
//     /// ready, and the value of the counter is reduced according to
//     /// the amount requested.
//     ///
//     /// \param nr Number of units to deposit (default 1).
//     void signal(size_t nr = 1) {
//         if (_ex) {
//             return;
//         }
//         _count += nr;
//         while (!_wait_list.empty() && has_available_units(_wait_list.front().nr)) {
//             auto& x = _wait_list.front();
//             _count -= x.nr;
//             x.pr.set_value();
//             _wait_list.pop_front();
//         }
//     }

//     /// Consume the specific number of units without blocking
//     //
//     /// Consume the specific number of units now, regardless of how many units are available
//     /// in the counter, and reduces the counter by that amount of units. This operation may
//     /// cause the counter to go negative.
//     ///
//     /// \param nr Amount of units to consume (default 1).
//     void consume(size_t nr = 1) {
//         if (_ex) {
//             return;
//         }
//         _count -= nr;
//     }

//     /// Attempts to reduce the counter value by a specified number of units.
//     ///
//     /// If sufficient units are available in the counter, and if no
//     /// other fiber is waiting, then the counter is reduced.  Otherwise,
//     /// nothing happens.  This is useful for "opportunistic" waits where
//     /// useful work can happen if the counter happens to be ready, but
//     /// when it is not worthwhile to wait.
//     ///
//     /// \param nr number of units to reduce the counter by (default 1).
//     /// \return `true` if the counter had sufficient units, and was decremented.
//     bool try_wait(size_t nr = 1) {
//         if (may_proceed(nr)) {
//             _count -= nr;
//             return true;
//         } else {
//             return false;
//         }
//     }
//     /// Returns the number of units available in the counter.
//     ///
//     /// Does not take into account any waiters.
//     size_t current() const { return std::max(_count, ssize_t(0)); }

//     /// Returns the number of available units.
//     ///
//     /// Takes into account units consumed using \ref consume() and therefore
//     /// may return a negative value.
//     ssize_t available_units() const { return _count; }

//     /// Returns the current number of waiters
//     size_t waiters() const { return _wait_list.size(); }

//     /// Signal to waiters that an error occurred.  \ref wait() will see
//     /// an exceptional future<> containing a \ref broken_semaphore exception.
//     /// The future is made available immediately.
//     void broken() { broken(std::make_exception_ptr(ExceptionFactory::broken())); }

//     /// Signal to waiters that an error occurred.  \ref wait() will see
//     /// an exceptional future<> containing the provided exception parameter.
//     /// The future is made available immediately.
//     template <typename Exception>
//     void broken(const Exception& ex) {
//         broken(std::make_exception_ptr(ex));
//     }

//     /// Signal to waiters that an error occurred.  \ref wait() will see
//     /// an exceptional future<> containing the provided exception parameter.
//     /// The future is made available immediately.
//     void broken(std::exception_ptr ex);

//     /// Reserve memory for waiters so that wait() will not throw.
//     void ensure_space_for_waiters(size_t n) {
//         _wait_list.reserve(n);
//     }
// };

// template<typename ExceptionFactory, typename Clock>
// inline
// void
// basic_semaphore<ExceptionFactory, Clock>::broken(std::exception_ptr xp) {
//     _ex = xp;
//     _count = 0;
//     while (!_wait_list.empty()) {
//         auto& x = _wait_list.front();
//         x.pr.set_exception(xp);
//         _wait_list.pop_front();
//     }
// }

// template<typename ExceptionFactory = semaphore_default_exception_factory, typename Clock = typename timer<>::clock>
// class semaphore_units {
//     basic_semaphore<ExceptionFactory, Clock>& _sem;
//     size_t _n;
// public:
//     semaphore_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t n) noexcept : _sem(sem), _n(n) {}
//     semaphore_units(semaphore_units&& o) noexcept : _sem(o._sem), _n(o._n) {
//         o._n = 0;
//     }
//     semaphore_units& operator=(semaphore_units&& o) noexcept {
//         if (this != &o) {
//             this->~semaphore_units();
//             new (this) semaphore_units(std::move(o));
//         }
//         return *this;
//     }
//     semaphore_units(const semaphore_units&) = delete;
//     ~semaphore_units() noexcept {
//         if (_n) {
//             _sem.signal(_n);
//         }
//     }
//     /// Releases ownership of the units. The semaphore will not be signalled.
//     ///
//     /// \return the number of units held
//     size_t release() {
//         return std::exchange(_n, 0);
//     }
// };

// /// \brief Take units from semaphore temporarily
// ///
// /// Takes units from the semaphore and returns them when the \ref semaphore_units object goes out of scope.
// /// This provides a safe way to temporarily take units from a semaphore and ensure
// /// that they are eventually returned under all circumstances (exceptions, premature scope exits, etc).
// ///
// /// Unlike with_semaphore(), the scope of unit holding is not limited to the scope of a single async lambda.
// ///
// /// \param sem The semaphore to take units from
// /// \param units  Number of units to take
// /// \return a \ref future<> holding \ref semaphore_units object. When the object goes out of scope
// ///         the units are returned to the semaphore.
// ///
// /// \note The caller must guarantee that \c sem is valid as long as
// ///      \ref seaphore_units object is alive.
// ///
// /// \related semaphore


// template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
// future<semaphore_units<ExceptionFactory, Clock>>
// get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
//     return sem.wait(units).then([&sem, units] {
//         return semaphore_units<ExceptionFactory, Clock>{ sem, units };
//     });
// }

// /// \brief Take units from semaphore temporarily with time bound on wait
// ///
// /// Like \ref get_units(basic_semaphore<ExceptionFactory>&, size_t) but when
// /// timeout is reached before units are granted throws semaphore_timed_out exception.
// ///
// /// \param sem The semaphore to take units from
// /// \param units  Number of units to take
// /// \return a \ref future<> holding \ref semaphore_units object. When the object goes out of scope
// ///         the units are returned to the semaphore.
// ///
// /// \note The caller must guarantee that \c sem is valid as long as
// ///      \ref seaphore_units object is alive.
// ///
// /// \related semaphore
// template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
// future<semaphore_units<ExceptionFactory, Clock>>
// get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, typename basic_semaphore<ExceptionFactory, Clock>::time_point timeout) {
//     return sem.wait(timeout, units).then([&sem, units] {
//         return semaphore_units<ExceptionFactory, Clock>{ sem, units };
//     });
// }

// /// \brief Consume units from semaphore temporarily
// ///
// /// Consume units from the semaphore and returns them when the \ref semaphore_units object goes out of scope.
// /// This provides a safe way to temporarily take units from a semaphore and ensure
// /// that they are eventually returned under all circumstances (exceptions, premature scope exits, etc).
// ///
// /// Unlike get_units(), this calls the non-blocking consume() API.
// ///
// /// Unlike with_semaphore(), the scope of unit holding is not limited to the scope of a single async lambda.
// ///
// /// \param sem The semaphore to take units from
// /// \param units  Number of units to consume
// template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
// semaphore_units<ExceptionFactory, Clock>
// consume_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
//     sem.consume(units);
//     return semaphore_units<ExceptionFactory, Clock>{ sem, units };
// }

// /// \brief Runs a function protected by a semaphore
// ///
// /// Acquires a \ref semaphore, runs a function, and releases
// /// the semaphore, returning the the return value of the function,
// /// as a \ref future.
// ///
// /// \param sem The semaphore to be held while the \c func is
// ///            running.
// /// \param units  Number of units to acquire from \c sem (as
// ///               with semaphore::wait())
// /// \param func   The function to run; signature \c void() or
// ///               \c future<>().
// /// \return a \ref future<> holding the function's return value
// ///         or exception thrown; or a \ref future<> containing
// ///         an exception from one of the semaphore::broken()
// ///         variants.
// ///
// /// \note The caller must guarantee that \c sem is valid until
// ///       the future returned by with_semaphore() resolves.
// ///
// /// \related semaphore
// template <typename ExceptionFactory, typename Func, typename Clock = typename timer<>::clock>
// inline
// futurize_t<std::result_of_t<Func()>>
// with_semaphore(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, Func&& func) {
//     return get_units(sem, units).then([func = std::forward<Func>(func)] (auto units) mutable {
//         return futurize_apply(std::forward<Func>(func)).finally([units = std::move(units)] {});
//     });
// }

// /// default basic_semaphore specialization that throws semaphore specific exceptions
// /// on error conditions.
// using semaphore = basic_semaphore<semaphore_default_exception_factory>;



// /*--------------------------------------------------条件变量------------------------------------------------------------------*/

// /// \addtogroup fiber-module
// /// @{

// /// Exception thrown when a condition variable is broken by
// /// \ref condition_variable::broken().
// class broken_condition_variable : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Condition variable is broken";
//     }
// };

// /// Exception thrown when wait() operation times out
// /// \ref condition_variable::wait(time_point timeout).
// class condition_variable_timed_out : public std::exception {
// public:
//     /// Reports the exception reason.
//     virtual const char* what() const noexcept {
//         return "Condition variable timed out";
//     }
// };

// /// \brief Conditional variable.
// ///
// /// This is a standard computer science condition variable sans locking,
// /// since in seastar access to variables is atomic anyway, adapted
// /// for futures.  You can wait for variable to be notified.
// ///
// /// To support exceptional conditions, a \ref broken() method
// /// is provided, which causes all current waiters to stop waiting,
// /// with an exceptional future returned.  This allows causing all
// /// fibers that are blocked on a condition variable to continue.
// /// This issimilar to POSIX's `pthread_cancel()`, with \ref wait()
// /// acting as a cancellation point.

// class condition_variable {
//     using duration = semaphore::duration;
//     using clock = semaphore::clock;
//     using time_point = semaphore::time_point;
//     struct condition_variable_exception_factory {
//         static condition_variable_timed_out timeout() {
//             return condition_variable_timed_out();
//         }
//         static broken_condition_variable broken() {
//             return broken_condition_variable();
//         }
//     };
//     basic_semaphore<condition_variable_exception_factory> _sem;
// public:
//     /// Constructs a condition_variable object.
//     /// Initialzie the semaphore with a default value of 0 to enusre
//     /// the first call to wait() before signal() won't be waken up immediately.
//     condition_variable() : _sem(0) {}

//     /// Waits until condition variable is signaled, may wake up without condition been met
//     ///
//     /// \return a future that becomes ready when \ref signal() is called
//     ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
//     ///         exception.
//     future<> wait() {
//         return _sem.wait();
//     }

//     /// Waits until condition variable is signaled or timeout is reached
//     ///
//     /// \param timeout time point at which wait will exit with a timeout
//     ///
//     /// \return a future that becomes ready when \ref signal() is called
//     ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
//     ///         exception. If timepoint is reached will return \ref condition_variable_timed_out exception.
//     future<> wait(time_point timeout) {
//         return _sem.wait(timeout);
//     }

//     /// Waits until condition variable is signaled or timeout is reached
//     ///
//     /// \param timeout duration after which wait will exit with a timeout
//     ///
//     /// \return a future that becomes ready when \ref signal() is called
//     ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
//     ///         exception. If timepoint is passed will return \ref condition_variable_timed_out exception.
//     future<> wait(duration timeout) {
//         return _sem.wait(timeout);
//     }
//     /// Waits until condition variable is notified and pred() == true, otherwise
//     /// wait again.
//     ///
//     /// \param pred predicate that checks that awaited condition is true
//     ///
//     /// \return a future that becomes ready when \ref signal() is called
//     ///         If the condition variable was \ref broken(), may contain an exception.
//     template<typename Pred>
//     future<> wait(Pred&& pred) {
//         return do_until(std::forward<Pred>(pred), [this] {
//             return wait();
//         });
//     }

//     /// Waits until condition variable is notified and pred() == true or timeout is reached, otherwise
//     /// wait again.
//     ///
//     /// \param timeout time point at which wait will exit with a timeout
//     /// \param pred predicate that checks that awaited condition is true
//     ///
//     /// \return a future that becomes ready when \ref signal() is called
//     ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
//     ///         exception. If timepoint is reached will return \ref condition_variable_timed_out exception.
//     /// \param
//     template<typename Pred>
//     future<> wait(time_point timeout, Pred&& pred) {
//         return do_until(std::forward<Pred>(pred), [this, timeout] () mutable {
//             return wait(timeout);
//         });
//     }

//     /// Waits until condition variable is notified and pred() == true or timeout is reached, otherwise
//     /// wait again.
//     ///
//     /// \param timeout duration after which wait will exit with a timeout
//     /// \param pred predicate that checks that awaited condition is true
//     ///
//     /// \return a future that becomes ready when \ref signal() is called
//     ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
//     ///         exception. If timepoint is passed will return \ref condition_variable_timed_out exception.
//     template<typename Pred>
//     future<> wait(duration timeout, Pred&& pred) {
//         return wait(clock::now() + timeout, std::forward<Pred>(pred));
//     }
//     /// Notify variable and wake up a waiter if there is one
//     void signal() {
//         if (_sem.waiters()) {
//             _sem.signal();
//         }
//     }
//     /// Notify variable and wake up all waiter
//     void broadcast() {
//         _sem.signal(_sem.waiters());
//     }
//     /// Signal to waiters that an error occurred.  \ref wait() will see
//     /// an exceptional future<> containing the provided exception parameter.
//     /// The future is made available immediately.
//     void broken() {
//         _sem.broken();
//     }
// };
















// __thread bool g_need_preempt;
// struct reactor {

// /*---------构造函数和析构函数----------------*/
//     reactor();
//     ~reactor();
//     reactor(const reactor&) = delete;
//     reactor& operator=(const reactor&) = delete;
//     unsigned _id = 0;
// /*----------定时器相关------------------------*/
//     lowres_clock::time_point _lowres_next_timeout;
//     timer_t _steady_clock_timer = {};
//     timer_set<timer<steady_clock_type>> _timers;
//     typename timer_set<timer<steady_clock_type>>::timer_list_t _expired_timers;
//     timer_set<timer<lowres_clock>> _lowres_timers;
//     typename timer_set<timer<lowres_clock>>::timer_list_t _expired_lowres_timers;
//     timer_set<timer<manual_clock>> _manual_timers;
//     typename timer_set<timer<manual_clock>>::timer_list_t _expired_manual_timers;
//     using steady_timer = timer<steady_clock_type>;
//     using lowres_timer = timer<lowres_clock>;
//     using manual_timer = timer<manual_clock>;
// /*---------------------------------------------*/
//     void add_timer(steady_timer* tmr);
//     bool queue_timer(steady_timer* tmr);
//     void del_timer(steady_timer* tmr);
//     void add_timer(lowres_timer* tmr);
//     bool queue_timer(lowres_timer* tmr);
//     void del_timer(lowres_timer* tmr);
//     void add_timer(manual_timer* tmr);
//     bool queue_timer(manual_timer* tmr);
//     void del_timer(manual_timer* tmr);
//     void enable_timer(steady_clock_type::time_point when);
//     bool do_expire_lowres_timers();
//     template <typename T, typename E, typename EnableFunc>
//     void complete_timers(T&, E&, EnableFunc&& enable_fn);
//     bool do_check_lowres_timers() const;
//     void expire_manual_timers();

// /*---------------信号处理相关------------------------*/
//     signals _signals;
// /*----------任务相关-------------------*/
//     bool _stopping = false;
//     bool _stopped = false;
//     int _return = 0;
//     condition_variable _stop_requested;


//     std::deque<std::unique_ptr<task>> _pending_tasks;
//     void run_tasks(std::deque<std::unique_ptr<task>>& tasks);
//     std::vector<std::function<future<> ()>> _exit_funcs;//为什么不用引用?
//     void add_task(std::unique_ptr<task>&& t) { _pending_tasks.push_back(std::move(t)); }
//     void add_urgent_task(std::unique_ptr<task>&& t) { _pending_tasks.push_front(std::move(t)); }
//     void force_poll() {
//         g_need_preempt = true;
//     }
//     void reactor::at_exit(std::function<future<> ()> func);
//     void reactor::exit(int ret);
//     void reactor::stop();
//     future<> run_exit_tasks();

// /*-----------定时器相关----------------*/

// /*----------------全局--------------*/
//     // condition_variable _stop_requested;
//     int run();
//     // template <typename Rep, typename Period>
//     // future<> wait_for_stop(std::chrono::duration<Rep, Period> timeout) {
//     //     return _stop_requested.wait(timeout, [this] { return _stopping; });
//     // }
// /*----------配置相关----------------*/
//    static boost::program_options::options_description reactor::get_options_description();

// };

// reactor::reactor() {
//     // 初始化系统定时器
//     sigevent sev;
//     memset(&sev, 0, sizeof(sev));
//     sev.sigev_notify = SIGEV_SIGNAL;
//     sev.sigev_signo = SIGALRM;
//     int ret = timer_create(CLOCK_MONOTONIC, &sev, &_steady_clock_timer);
//     if (ret < 0) {
//         throw std::system_error(errno, std::system_category());
//     }
//     // thread_impl::init();
// }


// reactor::~reactor() {
//     if (_steady_clock_timer) {
//         timer_delete(_steady_clock_timer);
//     }
// }

// void reactor::at_exit(std::function<future<> ()> func) {
//     assert(!_stopping);
//     _exit_funcs.push_back(std::move(func));
// }


// void reactor::run_tasks(std::deque<std::unique_ptr<task>>& tasks) {
//     while (!tasks.empty()) {
//         std::cout<<"run_task开始执行"<<std::endl;
//         auto tsk = std::move(tasks.front());
//         tasks.pop_front();
//         tsk->run();
//         std::cout<<"run_task结束执行"<<std::endl;
//         tsk.reset();
//     }
// }

// void reactor::exit(int ret) {
//     smp::submit_to(0, [this, ret] { _return = ret; stop(); });
// }

// void reactor::stop() {
//     assert(engine()._id == 0);
//     smp::cleanup_cpu();
//     if (!_stopping) {
//         // run_exit_tasks().then([this] {
//         //     do_with(semaphore(0), [this] (semaphore& sem) {
//         //         for (unsigned i = 1; i < smp::count; i++) {
//         //             smp::submit_to<>(i, []() {
//         //                 smp::cleanup_cpu();
//         //                 return engine().run_exit_tasks().then([] {
//         //                         engine()._stopped = true;
//         //                 });
//         //             }).then([&sem]() {
//         //                 sem.signal();
//         //             });
//         //         }
//         //         return sem.wait(smp::count - 1).then([this] {
//         //             _stopped = true;
//         //         });
//         //     });
//         // });
//     }
// }

// future<> reactor::run_exit_tasks() {
//     _stop_requested.broadcast();
//     _stopping = true;
//     // stop_aio_eventfd_loop();
//     return do_for_each(_exit_funcs.rbegin(), _exit_funcs.rend(), [] (auto& func) {
//         return func();
//     });
// }

// void schedule_normal(std::unique_ptr<task> t) {
//     std::cout<<"调用schedule normal"<<std::endl;
//     engine().add_task(std::move(t));
// }

// void schedule_urgent(std::unique_ptr<task> t) {
//     std::cout<<"调用schedule urgent"<<std::endl;
//     engine().add_urgent_task(std::move(t));
// }



// timespec to_timespec(steady_clock_type::time_point t) {
//     using ns = std::chrono::nanoseconds;
//     auto n = std::chrono::duration_cast<ns>(t.time_since_epoch()).count();
//     return { n / 1'000'000'000, n % 1'000'000'000 };
// }

// //
// void reactor::enable_timer(steady_clock_type::time_point when) {
//     itimerspec its;
//     its.it_interval = {};
//     its.it_value = to_timespec(when);
//     auto ret = timer_settime(_steady_clock_timer, TIMER_ABSTIME, &its, NULL);
//     // throw_system_error_on(ret == -1);
// }


// /*
// 信号触发​:默认情况下，定时器到期会发送一个信号（如 SIGALRM）到进程。进程可以通过信号处理函数来处理定时器到期事件.
// */

// void reactor::add_timer(steady_timer *tmr) {
//     std::cout<<"reactor add timer"<<std::endl;
//     if (queue_timer(tmr)) {
//         enable_timer(_timers.get_next_timeout());
//     }
// }
// void reactor::add_timer(lowres_timer* tmr) {
//     if (queue_timer(tmr)) {
//         _lowres_next_timeout = _lowres_timers.get_next_timeout();
//     }
// }

// void reactor::add_timer(manual_timer* tmr) {
//     queue_timer(tmr);
// }
// bool reactor::queue_timer(lowres_timer* tmr) {
//     return _lowres_timers.insert(*tmr);
// }

// bool reactor::queue_timer(manual_timer* tmr) {
//     return _manual_timers.insert(*tmr);
// }

// bool reactor::queue_timer(steady_timer* tmr) {
//     std::cout<<"reaactor queue timer"<<std::endl;
//     return _timers.insert(*tmr);
// }


// /*
//     del_timer什么时候调用?
// */
// void reactor::del_timer(steady_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_timers.erase(tmr->expired_it);  // 直接使用保存的迭代器
//         tmr->_expired = false;
//     } else {
//         _timers.remove(*tmr);  // 通过 it 成员快速删除
//     }
// }

// // 同理修改其他 del_timer 函数：
// void reactor::del_timer(lowres_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_lowres_timers.erase(tmr->expired_it);
//         tmr->_expired = false;
//     } else {
//         _lowres_timers.remove(*tmr);
//     }
// }

// void reactor::del_timer(manual_timer* tmr) {
//     if (tmr->_expired) {
//         _expired_manual_timers.erase(tmr->expired_it);
//         tmr->_expired = false;
//     } else {
//         _manual_timers.remove(*tmr);
//     }
// }

// reactor& engine(){
//     static reactor r;
//     return r;
// }


// // Implementation of global functions
// void enable_timer(steady_clock_type::time_point when)
// {
//     engine().enable_timer(when);
// }

// bool queue_timer(timer<steady_clock_type>* tmr) {
//     return engine().queue_timer(tmr);
// }

// void add_timer(timer<steady_clock_type>* tmr) {
//     engine().add_timer(tmr);
// }

// void add_timer(timer<lowres_clock>* tmr) {
//     engine().add_timer(tmr);
// }

// void add_timer(timer<manual_clock>* tmr) {
//      engine().add_timer(tmr);
// }

// bool queue_timer(timer<manual_clock>* tmr) {
//     return engine().queue_timer(tmr);
// }

// bool queue_timer(timer<lowres_clock>* tmr) {
//     return engine().queue_timer(tmr);
// }
// void manual_clock::advance(manual_clock::duration d) {
//     _now.fetch_add(d.count());
//     // engine().schedule_urgent(make_task(&manual_clock::expire_timers));
//     //smp::invoke_on_all(&manual_clock::expire_timers);
//     return;
// }

// void del_timer(timer<lowres_clock>* tmr) {
//     engine().del_timer(tmr);
// }

// void del_timer(timer<steady_clock_type>* tmr) {
//     engine().del_timer(tmr);
// }
// void del_timer(timer<manual_clock>* tmr) {
//     engine().del_timer(tmr);
// }


// bool reactor::do_check_lowres_timers() const{
//     // std::cout<<"do_check_lowres_timers"<<std::endl;
//     if (_lowres_next_timeout == lowres_clock::time_point()) {
//         return false;
//     }
//     return lowres_clock::now() > _lowres_next_timeout;
// }


// template <typename T, typename E, typename EnableFunc>
// void reactor::complete_timers(T& timers, E& expired_timers, EnableFunc&& enable_fn) {
//     expired_timers = timers.expire(timers.now()); // 获取过期的定时器
//     std::cout << "Expired " << expired_timers.size() << " timers" << std::endl;
//     // 处理所有过期定时器
//     for (auto* timer_ptr : expired_timers) {
//         if (timer_ptr) {
//             std::cout << "Marking timer " << " as expired" << std::endl;
//             timer_ptr->_expired = true;
//         }
//     }
//     // arm表示定时器是否有一个过期时间.
//     while (!expired_timers.empty()) {
//         auto* timer_ptr = expired_timers.front();
//         expired_timers.pop_front();
//         if (timer_ptr) {
//             std::cout << "Processing timer "<<std::endl;
//             timer_ptr->_queued = false;
//             if (timer_ptr->_armed) {
//                 timer_ptr->_armed = false;
//                 if (timer_ptr->_period) {
//                     // std::cout << "Re-adding periodic timer " << timer_ptr->_timerid << std::endl;
//                     timer_ptr->readd_periodic();//周期定时器

//                 }
//                 try {
//                     std::cout << "Executing timer callback for " << std::endl;
//                     timer_ptr->_callback();
//                 } catch (const std::exception& e) {
//                     // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed: " << e.what() << std::endl;
//                 } catch (...) {
//                     // std::cerr << "Timer " << timer_ptr->_timerid << " callback failed with unknown error" << std::endl;
//                 }
//             }
//         }
//     }
    
//     enable_fn();
// }

// int reactor::run(){
//     _signals.handle_signal(alarm_signal(), [this] {
//         complete_timers(_timers, _expired_timers, [this] {
//         if (!_timers.empty()) {
//                 std::cout << "Enabling timer for " << _timers.get_next_timeout()<<std::endl;//这行是有执行的.
//                 enable_timer(_timers.get_next_timeout());
//             }
//         });
//     });
//     while(true){
//         run_tasks(_pending_tasks);
//          _signals.poll_signal();
//         // do_check_lowres_timers();
//         // do_expire_lowres_timers();
//         // std::this_thread::sleep_for(100ms);
//     }
//     return 0;
// }



// boost::program_options::options_description
// reactor::get_options_description() {
//     namespace bpo = boost::program_options;
//     bpo::options_description opts("Core options");
//     auto net_stack_names = network_stack_registry::list();
//     opts.add_options()
//         ("network-stack", bpo::value<std::string>(),
//                 sprint("select network stack (valid values: %s)",
//                         format_separated(net_stack_names.begin(), net_stack_names.end(), ", ")).c_str())
//         ("no-handle-interrupt", "ignore SIGINT (for gdb)")
//         ("poll-mode", "poll continuously (100% cpu use)")
//         ("idle-poll-time-us", bpo::value<unsigned>()->default_value(calculate_poll_time() / 1us),
//                 "idle polling time in microseconds (reduce for overprovisioned environments or laptops)")
//         ("poll-aio", bpo::value<bool>()->default_value(true),
//                 "busy-poll for disk I/O (reduces latency and increases throughput)")
//         ("task-quota-ms", bpo::value<double>()->default_value(2.0), "Max time (ms) between polls")
//         ("max-task-backlog", bpo::value<unsigned>()->default_value(1000), "Maximum number of task backlog to allow; above this we ignore I/O")
//         ("blocked-reactor-notify-ms", bpo::value<unsigned>()->default_value(2000), "threshold in miliseconds over which the reactor is considered blocked if no progress is made")
//         ("relaxed-dma", "allow using buffered I/O if DMA is not available (reduces performance)")
//         ("overprovisioned", "run in an overprovisioned environment (such as docker or a laptop); equivalent to --idle-poll-time-us 0 --thread-affinity 0 --poll-aio 0")
//         ("abort-on-seastar-bad-alloc", "abort when seastar allocator cannot allocate memory")
// #ifdef SEASTAR_HEAPPROF
//         ("heapprof", "enable seastar heap profiling")
// #endif
//         ;
//     opts.add(network_stack_registry::options_description());
//     return opts;
// }





// // Initialize static members
// std::atomic<lowres_clock::rep> lowres_clock::_now;
// constexpr std::chrono::milliseconds lowres_clock::_granularity;
// using steady_clock_type = std::chrono::steady_clock;
// // Implementation for timer_deleter
// void lowres_clock::timer_deleter::operator()(void* ptr) const {
//     if (ptr) {
//         auto* timer_ptr = static_cast<timer<steady_clock_type>*>(ptr);
//         timer_ptr->cancel();  // Make sure to cancel the timer before deleting
//         delete timer_ptr;
//     }
// }

// // Implementation


// lowres_clock::lowres_clock() {
//     auto tmr = new timer<steady_clock_type>();
//     if (!tmr) {
//         throw std::bad_alloc();
//     }
//     _timer.reset(tmr);
//     update();
//     tmr->set_callback(&lowres_clock::update);
//     tmr->arm_periodic(_granularity);
// }

// lowres_clock::~lowres_clock() = default;

// void lowres_clock::update() {
//     using namespace std::chrono;
//     auto now = steady_clock_type::now();
//     auto ticks = duration_cast<milliseconds>(now.time_since_epoch()).count();
//     _now.store(ticks, std::memory_order_relaxed);
// } 



// // Initialize static member
// std::atomic<manual_clock::rep> manual_clock::_now{0};


// void reactor::expire_manual_timers() {
//     complete_timers(engine()._manual_timers, engine()._expired_manual_timers, []{
//         std::cout<<"到期"<<std::endl;
//     });
// }

// void manual_clock::expire_timers() {
//     engine().expire_manual_timers();
// }


// // Helper functions


// bool reactor::do_expire_lowres_timers() {
//     if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
//         return false;
//     }
//     auto now = lowres_clock::now();
//     if (now > engine()._lowres_next_timeout) {
//         complete_timers(engine()._lowres_timers, engine()._expired_lowres_timers, [] {
//             if (!engine()._lowres_timers.empty()) {
//                 engine()._lowres_next_timeout = engine()._lowres_timers.get_next_timeout();
//             } else {
//                 engine()._lowres_next_timeout = lowres_clock::time_point();
//             }
//         });
//         return true;
//     }
//     return false;
// }


// // bool reactor::do_check_lowres_timers() const{
// //     if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
// //         return false;
// //     }
// //     return lowres_clock::now() > engine()._lowres_next_timeout;
// // } 



// signals::signals() : _pending_signals(0) {
// }

// signals::~signals() {
//     sigset_t mask;
//     sigfillset(&mask);
//     ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
// }

// signals::signal_handler::signal_handler(int signo, std::function<void ()>&& handler)
//         : _handler(std::move(handler)) {
//             std::cout<<"调用signal_handler"<<std::endl;
//     struct sigaction sa;
//     sa.sa_sigaction = action;//这个是信号处理函数.
//     sa.sa_mask = make_empty_sigset_mask();
//     sa.sa_flags = SA_SIGINFO | SA_RESTART;
//     engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
//     std::cout<<"engine().signals"<<engine()._signals._pending_signals<<std::endl;
//     auto r = ::sigaction(signo, &sa, nullptr);
//     // throw_system_error_on(r == -1);
//     auto mask = make_sigset_mask(signo);
//     r = ::pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
//     throw_pthread_error(r);
// }

// void signals::handle_signal(int signo, std::function<void ()>&& handler) {
//     std::cout<<"handle signal"<<std::endl;
//     _signal_handlers.emplace(std::piecewise_construct,std::make_tuple(signo), std::make_tuple(signo, std::move(handler)));
//     //插入singo,和对应的handler.
// }

// void signals::handle_signal_once(int signo, std::function<void ()>&& handler) {
//     return handle_signal(signo, [fired = false, handler = std::move(handler)] () mutable {
//         if (!fired) {
//             fired = true;
//             handler();
//         }
//     });
// }

// bool signals::poll_signal() {
//     auto signals = _pending_signals.load(std::memory_order_relaxed);
//     //为什么一直是0.

//     if (signals) {
//             std::cout<<"signals "<<signals<<std::endl;
//         _pending_signals.fetch_and(~signals, std::memory_order_relaxed);
//         for (size_t i = 0; i < sizeof(signals)*8; i++) {
//             // std::cout<<"遍历"<<std::endl;
//             if (signals & (1ull << i)) {
//                 //执行handler
//                 std::cout<<"执行handler"<<std::endl;
//                _signal_handlers.at(i)._handler();
//             }
//         }
//     }
//     return signals;
// }

// bool signals::pure_poll_signal() const {
//     return _pending_signals.load(std::memory_order_relaxed);
// }

// void signals::action(int signo, siginfo_t* siginfo, void* ignore) {
//     std::cout<<"action########"<<std::endl;
//     engine()._signals._pending_signals.fetch_or(1ull << signo, std::memory_order_relaxed);
// }
// /*----------------------------metrics---------------------------------------*/

// namespace metrics {
// /*!
//  *  \brief Histogram bucket type
//  *  A histogram bucket contains an upper bound and the number
//  *  of events in the buckets.
//  */
// struct histogram_bucket {
//     uint64_t count = 0; // number of events.
//     double upper_bound = 0;      // Inclusive.
// };
// /*!
//  * \brief Histogram data type
//  *
//  * The histogram struct is a container for histogram values.
//  * It is not a histogram implementation but it will be used by histogram
//  * implementation to return its internal values.
//  */
// struct histogram {
//     uint64_t sample_count = 0;
//     double sample_sum = 0;
//     std::vector<histogram_bucket> buckets; // Ordered in increasing order of upper_bound, +Inf bucket is optional.
//     /*!
//      * \brief Addition assigning a historgram
//      *
//      * The histogram must match the buckets upper bounds
//      * or an exception will be thrown
//      */
//     histogram& operator+=(const histogram& h);
//     /*!
//      * \brief Addition historgrams
//      *
//      * Add two histograms and return the result as a new histogram
//      * The histogram must match the buckets upper bounds
//      * or an exception will be thrown
//      */
//     histogram operator+(const histogram& h) const;
//     /*!
//      * \brief Addition historgrams
//      *
//      * Add two histograms and return the result as a new histogram
//      * The histogram must match the buckets upper bounds
//      * or an exception will be thrown
//      */
//     histogram operator+(histogram&& h) const;
// };
// }


// #include <boost/variant.hpp>
// /*!
//  * \file metrics_registration.hh
//  * \brief holds the metric_groups definition needed by class that reports metrics
//  *
//  * If class A needs to report metrics,
//  * typically you include metrics_registration.hh, in A header file and add to A:
//  * * metric_groups _metrics as a member
//  * * set_metrics() method that would be called in the constructor.
//  * \code
//  * class A {
//  *   metric_groups _metrics
//  *
//  *   void setup_metrics();
//  *
//  * };
//  * \endcode
//  * To define the metrics, include in your source file metircs.hh
//  * @see metrics.hh for the definition for adding a metric.
//  */
// namespace metrics {
// namespace impl {
// class metric_groups_def;
// struct metric_definition_impl;
// class metric_groups_impl;
// }

// using group_name_type = std::string; /*!< A group of logically related metrics */
// class metric_groups;

// class metric_definition {
//     std::unique_ptr<impl::metric_definition_impl> _impl;
// public:
//     metric_definition(const impl::metric_definition_impl& impl) noexcept;
//     metric_definition(metric_definition&& m) noexcept;
//     ~metric_definition();
//     friend metric_groups;
//     friend impl::metric_groups_impl;
// };
// class metric_group_definition {
// public:
//     group_name_type name;
//     std::initializer_list<metric_definition> metrics;
//     metric_group_definition(const group_name_type& name, std::initializer_list<metric_definition> l);
//     metric_group_definition(const metric_group_definition&) = delete;
//     ~metric_group_definition();
// };
// /*!
//  * metric_groups
//  * \brief holds the metric definition.
//  *
//  * Add multiple metric groups definitions.
//  * Initialization can be done in the constructor or with a call to add_group
//  * @see metrics.hh for example and supported metrics
//  */
// class metric_groups {
//     std::unique_ptr<impl::metric_groups_def> _impl;
// public:
//     metric_groups() noexcept;
//     metric_groups(metric_groups&&) = default;
//     virtual ~metric_groups();
//     metric_groups& operator=(metric_groups&&) = default;
//     /*!
//      * \brief add metrics belong to the same group in the constructor.
//      *
//      * combine the constructor with the add_group functionality.
//      */
//     metric_groups(std::initializer_list<metric_group_definition> mg);
//     /*!
//      * \brief add metrics belong to the same group.
//      * use the metrics creation functions to add metrics.
//      * for example:
//      *  _metrics.add_group("my_group", {
//      *      make_counter("my_counter_name1", counter, description("my counter description")),
//      *      make_counter("my_counter_name2", counter, description("my second counter description")),
//      *      make_gauge("my_gauge_name1", gauge, description("my gauge description")),
//      *  });
//      *
//      *  metric name should be unique inside the group.
//      *  you can change add_group calls like:
//      *  _metrics.add_group("my group1", {...}).add_group("my group2", {...});
//      */
//     metric_groups& add_group(const group_name_type& name, const std::initializer_list<metric_definition>& l);
//     /*!
//      * \brief clear all metrics groups registrations.
//      */
//     void clear();
// };
// /*!
//  * \brief hold a single metric group
//  * Initialization is done in the constructor or
//  * with a call to add_group
//  */
// class metric_group : public metric_groups {
// public:
//     metric_group() noexcept;
//     metric_group(const metric_group&) = delete;
//     metric_group(metric_group&&) = default;
//     virtual ~metric_group();
//     metric_group& operator=(metric_group&&) = default;
//     /*!
//      * \brief add metrics belong to the same group in the constructor.
//      *
//      *
//      */
//     metric_group(const group_name_type& name, std::initializer_list<metric_definition> l);
// };
// }





// /*!
//  * \namespace metrics
//  * \brief metrics creation and registration
//  *
//  * the metrics namespace holds the relevant method and classes to generate metrics.
//  *
//  * The metrics layer support registering metrics, that later will be
//  * exported via different API protocols.
//  *
//  * To be able to support multiple protocols the following simplifications where made:
//  * 1. The id of the metrics is based on the collectd id
//  * 2. A metric could be a single value either a reference or a function
//  *
//  * To add metrics definition to class A do the following:
//  * * Add a metrics_group memeber to A
//  * * Add a a set_metrics() method that would be called in the constructor.
//  *
//  *
//  * In A header file
//  * \code
//  * #include "core/metrics_registration.hh"
//  * class A {
//  *   metric_groups _metrics
//  *
//  *   void setup_metrics();
//  *
//  * };
//  * \endcode
//  *
//  * In A source file:
//  *
//  * \code
//  * include "core/metrics.hh"
//  *
//  * void A::setup_metrics() {
//  *   namespace sm = seastar::metrics;
//  *   _metrics = sm::create_metric_group();
//  *   _metrics->add_group("cache", {sm::make_gauge("bytes", "used", [this] { return _region.occupancy().used_space(); })});
//  * }
//  * \endcode
//  */

// namespace metrics {

// /*!
//  * \defgroup metrics_types metrics type definitions
//  * The following are for the metric layer use, do not use them directly
//  * Instead use the make_counter, make_gauge, make_absolute and make_derived
//  *
//  */
// using metric_type_def = std::string; /*!< Used to hold an inherit type (like bytes)*/
// using metric_name_type = std::string; /*!<  The metric name'*/
// using instance_id_type = std::string; /*!<  typically used for the shard id*/

// /*!
//  * \brief Human-readable description of a metric/group.
//  *
//  *
//  * Uses a separate class to deal with type resolution
//  *
//  * Add this to metric creation:
//  *
//  * \code
//  * _metrics->add_group("groupname", {
//  *   sm::make_gauge("metric_name", value, description("A documentation about the return value"))
//  * });
//  * \endcode
//  *
//  */
// class description {
// public:
//     description(std::string s) : _s(std::move(s))
//     {}
//     const std::string& str() const {
//         return _s;
//     }
// private:
//     std::string _s;
// };//这个地方有问题

// /*!
//  * \brief Label a metrics
//  *
//  * Label are useful for adding information about a metric that
//  * later you would need to aggregate by.
//  * For example, if you have multiple queues on a shard.
//  * Adding the queue id as a Label will allow you to use the same name
//  * of the metrics with multiple id instances.
//  *
//  * label_instance holds an instance of label consist of a key and value.
//  *
//  * Typically you will not generate a label_instance yourself, but use a label
//  * object for that.
//  * @see label for more information
//  *
//  *
//  */
// class label_instance {
//     std::string _key;
//     std::string _value;
// public:
//     /*!
//      * \brief create a label_instance
//      * label instance consists of key and value.
//      * The key is an sstring.
//      * T - the value type can be any type that can be lexical_cast to string
//      * (ie. if it support the redirection operator for stringstream).
//      *
//      * All primitive types are supported so all the following examples are valid:
//      * label_instance a("smp_queue", 1)
//      * label_instance a("my_key", "my_value")
//      * label_instance a("internal_id", -1)
//      */
//     template<typename T>
//     label_instance(const std::string& key, T v) : _key(key), _value(boost::lexical_cast<std::string>(v)){}

//     /*!
//      * \brief returns the label key
//      */
//     const std::string key() const {
//         return _key;
//     }

//     /*!
//      * \brief returns the label value
//      */
//     const std::string value() const {
//         return _value;
//     }
//     bool operator<(const label_instance&) const;
//     bool operator==(const label_instance&) const;
//     bool operator!=(const label_instance&) const;
// };


// /*!
//  * \brief Class that creates label instances
//  *
//  * A factory class to create label instance
//  * Typically, the same Label name is used in multiple places.
//  * label is a label factory, you create it once, and use it to create the label_instance.
//  *
//  * In the example we would like to label the smp_queue with with the queue owner
//  *
//  * seastar::metrics::label smp_owner("smp_owner");
//  *
//  * now, when creating a new smp metric we can add a label to it:
//  *
//  * sm::make_queue_length("send_batch_queue_length", _last_snt_batch, {smp_owner(cpuid)})
//  *
//  * where cpuid in this case is unsiged.
//  */
// class label {
//     std::string key;
// public:
//     using instance = label_instance;
//     /*!
//      * \brief creating a label
//      * key is the label name, it will be the key for all label_instance
//      * that will be created from this label.
//      */
//     explicit label(const std::string& key) : key(key) {
//     }

//     /*!
//      * \brief creating a label instance
//      *
//      * Use the function operator to create a new label instance.
//      * T - the value type can be any type that can be lexical_cast to string
//      * (ie. if it support the redirection operator for stringstream).
//      *
//      * All primitive types are supported so if lab is a label, all the following examples are valid:
//      * lab(1)
//      * lab("my_value")
//      * lab(-1)
//      */
//     template<typename T>
//     instance operator()(T value) const {
//         return label_instance(key, std::forward<T>(value));
//     }

//     /*!
//      * \brief returns the label name
//      */
//     const std::string& name() const {
//         return key;
//     }
// };

// /*!
//  * \namesapce impl
//  * \brief holds the implementation parts of the metrics layer, do not use directly.
//  *
//  * The metrics layer define a thin API for adding metrics.
//  * Some of the implementation details need to be in the header file, they should not be use directly.
//  */
// namespace impl {

// // The value binding data types
// enum class data_type : uint8_t {
//     COUNTER, // unsigned int 64
//     GAUGE, // double
//     DERIVE, // signed int 64
//     ABSOLUTE, // unsigned int 64
//     HISTOGRAM,
// };

// /*!
//  * \breif A helper class that used to return metrics value.
//  *
//  * Do not use directly @see metrics_creation
//  */
// struct metric_value {
//     boost::variant<double, histogram> u;
//     data_type _type;
//     data_type type() const {
//         return _type;
//     }

//     double d() const {
//         return boost::get<double>(u);
//     }

//     uint64_t ui() const {
//         return boost::get<double>(u);
//     }

//     int64_t i() const {
//         return boost::get<double>(u);
//     }

//     metric_value()
//             : _type(data_type::GAUGE) {
//     }

//     metric_value(histogram&& h, data_type t = data_type::HISTOGRAM) :
//         u(std::move(h)), _type(t) {
//     }
//     metric_value(const histogram& h, data_type t = data_type::HISTOGRAM) :
//         u(h), _type(t) {
//     }

//     metric_value(double d, data_type t)
//             : u(d), _type(t) {
//     }

//     metric_value& operator=(const metric_value& c) = default;

//     metric_value& operator+=(const metric_value& c) {
//         *this = *this + c;
//         return *this;
//     }

//     metric_value operator+(const metric_value& c);
//     const histogram& get_histogram() const {
//         return boost::get<histogram>(u);
//     }
// };

// using metric_function = std::function<metric_value()>;

// struct metric_type {
//     data_type base_type;
//     metric_type_def type_name;
// };

// struct metric_definition_impl {
//     metric_name_type name;
//     metric_type type;
//     metric_function f;
//     description d;
//     bool enabled = true;
//     std::map<std::string, std::string> labels;
//     metric_definition_impl& operator ()(bool enabled);
//     metric_definition_impl& operator ()(const label_instance& label);
//     metric_definition_impl(
//         metric_name_type name,
//         metric_type type,
//         metric_function f,
//         description d,
//         std::vector<label_instance> labels);
// };

// class metric_groups_def {
// public:
//     metric_groups_def() = default;
//     virtual ~metric_groups_def() = default;
//     metric_groups_def(const metric_groups_def&) = delete;
//     metric_groups_def(metric_groups_def&&) = default;
//     virtual metric_groups_def& add_metric(group_name_type name, const metric_definition& md) = 0;
//     virtual metric_groups_def& add_group(group_name_type name, const std::initializer_list<metric_definition>& l) = 0;
//     virtual metric_groups_def& add_group(group_name_type name, const std::vector<metric_definition>& l) = 0;
// };

// instance_id_type shard();

// template<typename T, typename En = std::true_type>
// struct is_callable;

// template<typename T>
// struct is_callable<T, typename std::integral_constant<bool, !std::is_void<typename std::result_of<T()>::type>::value>::type> : public std::true_type {
// };

// template<typename T>
// struct is_callable<T, typename std::enable_if<std::is_fundamental<T>::value, std::true_type>::type> : public std::false_type {
// };

// template<typename T, typename = std::enable_if_t<is_callable<T>::value>>
// metric_function make_function(T val, data_type dt) {
//     return [dt, val] {
//         return metric_value(val(), dt);
//     };
// }

// template<typename T, typename = std::enable_if_t<!is_callable<T>::value>>
// metric_function make_function(T& val, data_type dt) {
//     return [dt, &val] {
//         return metric_value(val, dt);
//     };
// }
// }

// extern const bool metric_disabled;

// extern label shard_label;
// extern label type_label;

// /*
//  * The metrics definition are defined to be compatible with collectd metrics defintion.
//  * Typically you should used gauge or derived.
//  */


// /*!
//  * \brief Gauge are a general purpose metric.
//  *
//  * They can support floating point and can increase or decrease
//  */
// template<typename T>
// impl::metric_definition_impl make_gauge(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, labels};
// }

// /*!
//  * \brief Gauge are a general purpose metric.
//  *
//  * They can support floating point and can increase or decrease
//  */
// template<typename T>
// impl::metric_definition_impl make_gauge(metric_name_type name,
//         description d, T&& val) {
//     return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, {}};
// }

// /*!
//  * \brief Gauge are a general purpose metric.
//  *
//  * They can support floating point and can increase or decrease
//  */
// template<typename T>
// impl::metric_definition_impl make_gauge(metric_name_type name,
//         description d, std::vector<label_instance> labels, T&& val) {
//     return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, labels};
// }


// /*!
//  * \brief Derive are used when a rate is more interesting than the value.
//  *
//  * Derive is an integer value that can increase or decrease, typically it is used when looking at the
//  * derivation of the value.
//  *
//  * It is OK to use it when counting things and if no wrap-around is expected (it shouldn't) it's prefer over counter metric.
//  */
// template<typename T>
// impl::metric_definition_impl make_derive(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, labels};
// }


// /*!
//  * \brief Derive are used when a rate is more interesting than the value.
//  *
//  * Derive is an integer value that can increase or decrease, typically it is used when looking at the
//  * derivation of the value.
//  *
//  * It is OK to use it when counting things and if no wrap-around is expected (it shouldn't) it's prefer over counter metric.
//  */
// template<typename T>
// impl::metric_definition_impl make_derive(metric_name_type name, description d,
//         T&& val) {
//     return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, {}};
// }


// /*!
//  * \brief Derive are used when a rate is more interesting than the value.
//  *
//  * Derive is an integer value that can increase or decrease, typically it is used when looking at the
//  * derivation of the value.
//  *
//  * It is OK to use it when counting things and if no wrap-around is expected (it shouldn't) it's prefer over counter metric.
//  */
// template<typename T>
// impl::metric_definition_impl make_derive(metric_name_type name, description d, std::vector<label_instance> labels,
//         T&& val) {
//     return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, labels};
// }


// /*!
//  * \brief create a counter metric
//  *
//  * Counters are similar to derived, but they assume monotony, so if a counter value decrease in a series it is count as a wrap-around.
//  * It is better to use large enough data value than to use counter.
//  *
//  */
// template<typename T>
// impl::metric_definition_impl make_counter(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return {name, {impl::data_type::COUNTER, "counter"}, make_function(std::forward<T>(val), impl::data_type::COUNTER), d, labels};
// }

// /*!
//  * \brief create an absolute metric.
//  *
//  * Absolute are used for metric that are being erased after each time they are read.
//  * They are here for compatibility reasons and should general be avoided in most applications.
//  */
// template<typename T>
// impl::metric_definition_impl make_absolute(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return {name, {impl::data_type::ABSOLUTE, "absolute"}, make_function(std::forward<T>(val), impl::data_type::ABSOLUTE), d, labels};
// }

// /*!
//  * \brief create a histogram metric.
//  *
//  * Histograms are a list o buckets with upper values and counter for the number
//  * of entries in each bucket.
//  */
// template<typename T>
// impl::metric_definition_impl make_histogram(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {}) {
//     return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, labels};
// }

// /*!
//  * \brief create a histogram metric.
//  *
//  * Histograms are a list o buckets with upper values and counter for the number
//  * of entries in each bucket.
//  */
// template<typename T>
// impl::metric_definition_impl make_histogram(metric_name_type name,
//         description d, std::vector<label_instance> labels, T&& val) {
//     return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, labels};
// }


// /*!
//  * \brief create a histogram metric.
//  *
//  * Histograms are a list o buckets with upper values and counter for the number
//  * of entries in each bucket.
//  */
// template<typename T>
// impl::metric_definition_impl make_histogram(metric_name_type name,
//         description d, T&& val) {
//     return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, {}};
// }


// /*!
//  * \brief create a total_bytes metric.
//  *
//  * total_bytes are used for an ever growing counters, like the total bytes
//  * passed on a network.
//  */

// template<typename T>
// impl::metric_definition_impl make_total_bytes(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {},
//         instance_id_type instance = impl::shard()) {
//     return make_derive(name, std::forward<T>(val), d, labels)(type_label("total_bytes"));
// }

// /*!
//  * \brief create a current_bytes metric.
//  *
//  * current_bytes are used to report on current status in bytes.
//  * For example the current free memory.
//  */

// template<typename T>
// impl::metric_definition_impl make_current_bytes(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {},
//         instance_id_type instance = impl::shard()) {
//     return make_derive(name, std::forward<T>(val), d, labels)(type_label("bytes"));
// }


// /*!
//  * \brief create a queue_length metric.
//  *
//  * queue_length are used to report on queue length
//  */

// template<typename T>
// impl::metric_definition_impl make_queue_length(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {},
//         instance_id_type instance = impl::shard()) {
//     return make_gauge(name, std::forward<T>(val), d, labels)(type_label("queue_length"));
// }


// /*!
//  * \brief create a total operation metric.
//  *
//  * total_operations are used for ever growing operation counter.
//  */

// template<typename T>
// impl::metric_definition_impl make_total_operations(metric_name_type name,
//         T&& val, description d=description(), std::vector<label_instance> labels = {},
//         instance_id_type instance = impl::shard()) {
//     return make_derive(name, std::forward<T>(val), d, labels)(type_label("total_operations"));
// }

// /*! @} */
// }







// /*------------------posix_thread-----------------------------------*/
// #include "../util/align.hh"
// class posix_thread {
// public:
//     class attr;
// private:
//     // must allocate, since this class is moveable
//     std::unique_ptr<std::function<void ()>> _func;
//     pthread_t _pthread;
//     bool _valid = true;
//     mmap_area _stack;
// private:
//     static void* start_routine(void* arg) noexcept;
// public:
//     posix_thread(std::function<void ()> func);
//     posix_thread(attr a, std::function<void ()> func);
//     posix_thread(posix_thread&& x);
//     ~posix_thread();
//     void join();
// public:
//     class attr {
//     public:
//         struct stack_size { size_t size = 0; };
//         attr() = default;
//         template <typename... A>
//         attr(A... a) {
//             set(std::forward<A>(a)...);
//         }
//         void set() {}
//         template <typename A, typename... Rest>
//         void set(A a, Rest... rest) {
//             set(std::forward<A>(a));
//             set(std::forward<Rest>(rest)...);
//         }
//         void set(stack_size ss) { _stack_size = ss; }
//     private:
//         stack_size _stack_size;
//         friend class posix_thread;
//     };
// };

// void* posix_thread::start_routine(void* arg) noexcept {
//     auto pfunc = reinterpret_cast<std::function<void ()>*>(arg);
//     (*pfunc)();
//     return nullptr;
// }

// posix_thread::posix_thread(std::function<void ()> func)
//     : posix_thread(attr{}, std::move(func)) {
// }

// posix_thread::posix_thread(attr a, std::function<void ()> func)
//     : _func(std::make_unique<std::function<void ()>>(std::move(func))) {
//     pthread_attr_t pa;
//     auto r = pthread_attr_init(&pa);
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
//     auto stack_size = a._stack_size.size;
//     if (!stack_size) {
//         stack_size = 2 << 20;
//     }
//     // allocate guard area as well
//     _stack = mmap_anonymous(nullptr, stack_size + (4 << 20),
//         PROT_NONE, MAP_PRIVATE | MAP_NORESERVE);
//     auto stack_start = align_up(_stack.get() + 1, 2 << 20);
//     mmap_area real_stack = mmap_anonymous(stack_start, stack_size,
//         PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_STACK);
//     real_stack.release(); // protected by @_stack
//     ::madvise(stack_start, stack_size, MADV_HUGEPAGE);
//     r = pthread_attr_setstack(&pa, stack_start, stack_size);
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
//     r = pthread_create(&_pthread, &pa,
//                 &posix_thread::start_routine, _func.get());
//     if (r) {
//         throw std::system_error(r, std::system_category());
//     }
// }

// posix_thread::posix_thread(posix_thread&& x)
//     : _func(std::move(x._func)), _pthread(x._pthread), _valid(x._valid)
//     , _stack(std::move(x._stack)) {
//     x._valid = false;
// }

// posix_thread::~posix_thread() {
//     assert(!_valid);
// }

// void posix_thread::join() {
//     assert(_valid);
//     pthread_join(_pthread, NULL);
//     _valid = false;
// }
// /*-----------------------------smp_queue------------------------------------------------------------*/
// #include <boost/lockfree/spsc_queue.hpp>
// class smp_message_queue {
//     static constexpr size_t queue_length = 128;
//     static constexpr size_t batch_size = 16;
//     static constexpr size_t prefetch_cnt = 2;
//     struct work_item;
//     struct lf_queue_remote {
//         reactor* remote;
//     };
//     using lf_queue_base = boost::lockfree::spsc_queue<work_item*,
//                             boost::lockfree::capacity<queue_length>>;
//     //use inheritence to control placement order
//     struct lf_queue : lf_queue_remote, lf_queue_base {
//         lf_queue(reactor* remote) : lf_queue_remote{remote} {}
//         void maybe_wakeup();
//     };
//     lf_queue _pending;
//     lf_queue _completed;
//     struct alignas(64) {
//         size_t _sent = 0;
//         size_t _compl = 0;
//         size_t _last_snt_batch = 0;
//         size_t _last_cmpl_batch = 0;
//         size_t _current_queue_length = 0;
//     };
//     // keep this between two structures with statistics
//     // this makes sure that they have at least one cache line
//     // between them, so hw prefecther will not accidentally prefetch
//     // cache line used by aother cpu.
//     metrics::metric_groups _metrics;
//     struct alignas(64) {
//         size_t _received = 0;
//         size_t _last_rcv_batch = 0;
//     };
//     struct work_item {
//         virtual ~work_item() {}
//         virtual future<> process() = 0;
//         virtual void complete() = 0;
//     };
//     template <typename Func>
//     struct async_work_item : work_item {
//         Func _func;
//         using futurator = futurize<std::result_of_t<Func()>>;
//         using future_type = typename futurator::type;
//         using value_type = typename future_type::value_type;
//         std::experimental::optional<value_type> _result;
//         std::exception_ptr _ex; // if !_result
//         typename futurator::promise_type _promise; // used on local side
//         async_work_item(Func&& func) : _func(std::move(func)) {}
//         virtual future<> process() override {
//             try {
//                 return futurator::apply(this->_func).then_wrapped([this] (auto&& f) {
//                     try {
//                         _result = f.get();
//                     } catch (...) {
//                         _ex = std::current_exception();
//                     }
//                 });
//             } catch (...) {
//                 _ex = std::current_exception();
//                 return make_ready_future();
//             }
//         }
//         virtual void complete() override {
//             if (_result) {
//                 _promise.set_value(std::move(*_result));
//             } else {
//                 // FIXME: _ex was allocated on another cpu
//                 _promise.set_exception(std::move(_ex));
//             }
//         }
//         future_type get_future() { return _promise.get_future(); }
//     };
//     union tx_side {
//         tx_side() {}
//         ~tx_side() {}
//         void init() { new (&a) aa; }
//         struct aa {
//             std::deque<work_item*> pending_fifo;
//         } a;
//     } _tx;
//     std::vector<work_item*> _completed_fifo;
// public:
//     smp_message_queue(reactor* from, reactor* to);
//     template <typename Func>
//     futurize_t<std::result_of_t<Func()>> submit(Func&& func) {
//         auto wi = std::make_unique<async_work_item<Func>>(std::forward<Func>(func));
//         auto fut = wi->get_future();
//         submit_item(std::move(wi));
//         return fut;
//     }
//     void start(unsigned cpuid);
//     template<size_t PrefetchCnt, typename Func>
//     size_t process_queue(lf_queue& q, Func process);
//     size_t process_incoming();
//     size_t process_completions();
//     void stop();
// private:
//     void work();
//     void submit_item(std::unique_ptr<work_item> wi);
//     void respond(work_item* wi);
//     void move_pending();
//     void flush_request_batch();
//     void flush_response_batch();
//     bool has_unflushed_responses() const;
//     bool pure_poll_rx() const;
//     bool pure_poll_tx() const;

//     friend class smp;
// };



// smp_message_queue::smp_message_queue(reactor* from, reactor* to)
//     : _pending(to)
//     , _completed(from)
// {
// }

// void smp_message_queue::stop() {
//     _metrics.clear();
// }
// void smp_message_queue::move_pending() {
//     auto begin = _tx.a.pending_fifo.cbegin();
//     auto end = _tx.a.pending_fifo.cend();
//     end = _pending.push(begin, end);
//     if (begin == end) {
//         return;
//     }
//     auto nr = end - begin;
//     _pending.maybe_wakeup();
//     _tx.a.pending_fifo.erase(begin, end);
//     _current_queue_length += nr;
//     _last_snt_batch = nr;
//     _sent += nr;
// }

// bool smp_message_queue::pure_poll_tx() const {
//     // can't use read_available(), not available on older boost
//     // empty() is not const, so need const_cast.
//     return !const_cast<lf_queue&>(_completed).empty();
// }

// void smp_message_queue::submit_item(std::unique_ptr<smp_message_queue::work_item> item) {
//     _tx.a.pending_fifo.push_back(item.get());
//     item.release();
//     if (_tx.a.pending_fifo.size() >= batch_size) {
//         move_pending();
//     }
// }

// void smp_message_queue::respond(work_item* item) {
//     _completed_fifo.push_back(item);
//     if (_completed_fifo.size() >= batch_size || engine()._stopped) {
//         flush_response_batch();
//     }
// }

// void smp_message_queue::flush_response_batch() {
//     if (!_completed_fifo.empty()) {
//         auto begin = _completed_fifo.cbegin();
//         auto end = _completed_fifo.cend();
//         end = _completed.push(begin, end);
//         if (begin == end) {
//             return;
//         }
//         _completed.maybe_wakeup();
//         _completed_fifo.erase(begin, end);
//     }
// }

// bool smp_message_queue::has_unflushed_responses() const {
//     return !_completed_fifo.empty();
// }

// bool smp_message_queue::pure_poll_rx() const {
//     // can't use read_available(), not available on older boost
//     // empty() is not const, so need const_cast.
//     return !const_cast<lf_queue&>(_pending).empty();
// }

// void
// smp_message_queue::lf_queue::maybe_wakeup() {
//     // Called after lf_queue_base::push().
//     //
//     // This is read-after-write, which wants memory_order_seq_cst,
//     // but we insert that barrier using systemwide_memory_barrier()
//     // because seq_cst is so expensive.
//     //
//     // However, we do need a compiler barrier:
//     std::atomic_signal_fence(std::memory_order_seq_cst);
//     if (remote->_sleeping.load(std::memory_order_relaxed)) {
//         // We are free to clear it, because we're sending a signal now
//         remote->_sleeping.store(false, std::memory_order_relaxed);
//         remote->wakeup();
//     }
// }

// template<size_t PrefetchCnt, typename Func>
// size_t smp_message_queue::process_queue(lf_queue& q, Func process) {
//     // copy batch to local memory in order to minimize
//     // time in which cross-cpu data is accessed
//     work_item* items[queue_length + PrefetchCnt];
//     work_item* wi;
//     if (!q.pop(wi))
//         return 0;
//     // start prefecthing first item before popping the rest to overlap memory
//     // access with potential cache miss the second pop may cause
//     prefetch<2>(wi);
//     auto nr = q.pop(items);
//     std::fill(std::begin(items) + nr, std::begin(items) + nr + PrefetchCnt, nr ? items[nr - 1] : wi);
//     unsigned i = 0;
//     do {
//         prefetch_n<2>(std::begin(items) + i, std::begin(items) + i + PrefetchCnt);
//         process(wi);
//         wi = items[i++];
//     } while(i <= nr);

//     return nr + 1;
// }

// size_t smp_message_queue::process_completions() {
//     auto nr = process_queue<prefetch_cnt*2>(_completed, [] (work_item* wi) {
//         wi->complete();
//         delete wi;
//     });
//     _current_queue_length -= nr;
//     _compl += nr;
//     _last_cmpl_batch = nr;

//     return nr;
// }

// void smp_message_queue::flush_request_batch() {
//     if (!_tx.a.pending_fifo.empty()) {
//         move_pending();
//     }
// }

// size_t smp_message_queue::process_incoming() {
//     auto nr = process_queue<prefetch_cnt>(_pending, [this] (work_item* wi) {
//         wi->process().then([this, wi] {
//             respond(wi);
//         });
//     });
//     _received += nr;
//     _last_rcv_batch = nr;
//     return nr;
// }

// void smp_message_queue::start(unsigned cpuid) {
//     _tx.init();
//     namespace sm = seastar::metrics;
//     char instance[10];
//     std::snprintf(instance, sizeof(instance), "%u-%u", engine().cpu_id(), cpuid);
//     _metrics.add_group("smp", {
//             // queue_length     value:GAUGE:0:U
//             // Absolute value of num packets in last tx batch.
//             sm::make_queue_length("send_batch_queue_length", _last_snt_batch, sm::description("Current send batch queue length"), {sm::shard_label(instance)})(sm::metric_disabled),
//             sm::make_queue_length("receive_batch_queue_length", _last_rcv_batch, sm::description("Current receive batch queue length"), {sm::shard_label(instance)})(sm::metric_disabled),
//             sm::make_queue_length("complete_batch_queue_length", _last_cmpl_batch, sm::description("Current complete batch queue length"), {sm::shard_label(instance)})(sm::metric_disabled),
//             sm::make_queue_length("send_queue_length", _current_queue_length, sm::description("Current send queue length"), {sm::shard_label(instance)})(sm::metric_disabled),
//             // total_operations value:DERIVE:0:U
//             sm::make_derive("total_received_messages", _received, sm::description("Total number of received messages"), {sm::shard_label(instance)})(sm::metric_disabled),
//             // total_operations value:DERIVE:0:U
//             sm::make_derive("total_sent_messages", _sent, sm::description("Total number of sent messages"), {sm::shard_label(instance)})(sm::metric_disabled),
//             // total_operations value:DERIVE:0:U
//             sm::make_derive("total_completed_messages", _compl, sm::description("Total number of messages completed"), {sm::shard_label(instance)})(sm::metric_disabled)
//     });
// }






// /*----------------------------------------smp------------------------------------------------------*/
// #include <boost/thread/barrier.hpp>
// class smp {
//     static std::vector<posix_thread> _threads;
//     static std::vector<std::function<void ()>> _thread_loops; // for dpdk
//     static std::experimental::optional<boost::barrier> _all_event_loops_done;
//     static std::vector<reactor*> _reactors;
//     static smp_message_queue** _qs;
//     static std::thread::id _tmain;
//     static bool _using_dpdk;

//     template <typename Func>
//     using returns_future = is_future<std::result_of_t<Func()>>;
//     template <typename Func>
//     using returns_void = std::is_same<std::result_of_t<Func()>, void>;
// public:
//     static boost::program_options::options_description get_options_description();
//     static void configure(boost::program_options::variables_map vm);
//     static void cleanup();
//     static void cleanup_cpu();
//     static void arrive_at_event_loop_end();
//     static void join_all();
//     static bool main_thread() { return std::this_thread::get_id() == _tmain; }

//     /// Runs a function on a remote core.
//     ///
//     /// \param t designates the core to run the function on (may be a remote
//     ///          core or the local core).
//     /// \param func a callable to run on core \c t.  If \c func is a temporary object,
//     ///          its lifetime will be extended by moving it.  If @func is a reference,
//     ///          the caller must guarantee that it will survive the call.
//     /// \return whatever \c func returns, as a future<> (if \c func does not return a future,
//     ///         submit_to() will wrap it in a future<>).
//     template <typename Func>
//     static futurize_t<std::result_of_t<Func()>> submit_to(unsigned t, Func&& func) {
//         using ret_type = std::result_of_t<Func()>;
//         if (t == engine().cpu_id()) {
//             try {
//                 if (!is_future<ret_type>::value) {
//                     // Non-deferring function, so don't worry about func lifetime
//                     return futurize<ret_type>::apply(std::forward<Func>(func));
//                 } else if (std::is_lvalue_reference<Func>::value) {
//                     // func is an lvalue, so caller worries about its lifetime
//                     return futurize<ret_type>::apply(func);
//                 } else {
//                     // Deferring call on rvalue function, make sure to preserve it across call
//                     auto w = std::make_unique<std::decay_t<Func>>(std::move(func));
//                     auto ret = futurize<ret_type>::apply(*w);
//                     return ret.finally([w = std::move(w)] {});
//                 }
//             } catch (...) {
//                 // Consistently return a failed future rather than throwing, to simplify callers
//                 return futurize<std::result_of_t<Func()>>::make_exception_future(std::current_exception());
//             }
//         } else {
//             return _qs[t][engine().cpu_id()].submit(std::forward<Func>(func));
//         }
//     }
//     static bool poll_queues();
//     static bool pure_poll_queues();
//     static boost::integer_range<unsigned> all_cpus() {
//         return boost::irange(0u, count);
//     }
//     // Invokes func on all shards.
//     // The returned future resolves when all async invocations finish.
//     // The func may return void or future<>.
//     // Each async invocation will work with a separate copy of func.
//     template<typename Func>
//     static future<> invoke_on_all(Func&& func) {
//         static_assert(std::is_same<future<>, typename futurize<std::result_of_t<Func()>>::type>::value, "bad Func signature");
//         return parallel_for_each(all_cpus(), [&func] (unsigned id) {
//             return smp::submit_to(id, Func(func));
//         });
//     }
// private:
//     static void start_all_queues();
//     static void pin(unsigned cpu_id);
//     static void allocate_reactor(unsigned id);
//     static void create_thread(std::function<void ()> thread_loop);
// public:
//     static unsigned count;
// };


// unsigned smp::count = 1;
// bool smp::_using_dpdk;

// void smp::start_all_queues()
// {
//     for (unsigned c = 0; c < count; c++) {
//         if (c != engine().cpu_id()) {
//             _qs[c][engine().cpu_id()].start(c);
//         }
//     }
// }


// void smp::join_all()
// {

//     for (auto&& t: smp::_threads) {
//         t.join();
//     }
// }

// void smp::pin(unsigned cpu_id) {
//     if (_using_dpdk) {
//         // dpdk does its own pinning
//         return;
//     }
//     pin_this_thread(cpu_id);
// }

// void smp::arrive_at_event_loop_end() {
//     if (_all_event_loops_done) {
//         _all_event_loops_done->wait();
//     }
// }

// void smp::allocate_reactor(unsigned id) {
//     assert(!reactor_holder);

//     // we cannot just write "local_engin = new reactor" since reactor's constructor
//     // uses local_engine
//     void *buf;
//     int r = posix_memalign(&buf, 64, sizeof(reactor));
//     assert(r == 0);
//     local_engine = reinterpret_cast<reactor*>(buf);
//     new (buf) reactor(id);
//     reactor_holder.reset(local_engine);
// }

// void smp::cleanup() {
//     smp::_threads = std::vector<posix_thread>();
//     _thread_loops.clear();
// }

// void smp::cleanup_cpu() {
//     size_t cpuid = engine().cpu_id();

//     if (_qs) {
//         for(unsigned i = 0; i < smp::count; i++) {
//             _qs[i][cpuid].stop();
//         }
//     }
// }

// void smp::create_thread(std::function<void ()> thread_loop) {
//     if (_using_dpdk) {
//         _thread_loops.push_back(std::move(thread_loop));
//     } else {
//         _threads.emplace_back(std::move(thread_loop));
//     }
// }


// void smp::configure(boost::program_options::variables_map configuration)
// {
//     // Mask most, to prevent threads (esp. dpdk helper threads)
//     // from servicing a signal.  Individual reactors will unmask signals
//     // as they become prepared to handle them.
//     //
//     // We leave some signals unmasked since we don't handle them ourself.
//     sigset_t sigs;
//     sigfillset(&sigs);
//     for (auto sig : {SIGHUP, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGSEGV,
//             SIGALRM, SIGCONT, SIGSTOP, SIGTSTP, SIGTTIN, SIGTTOU}) {
//         sigdelset(&sigs, sig);
//     }
//     pthread_sigmask(SIG_BLOCK, &sigs, nullptr);

//     install_oneshot_signal_handler<SIGSEGV, sigsegv_action>();
//     install_oneshot_signal_handler<SIGABRT, sigabrt_action>();


//     auto thread_affinity = configuration["thread-affinity"].as<bool>();
//     if (configuration.count("overprovisioned")
//            && configuration["thread-affinity"].defaulted()) {
//         thread_affinity = false;
//     }
//     if (!thread_affinity && _using_dpdk) {
//         print("warning: --thread-affinity 0 ignored in dpdk mode\n");
//     }

//     smp::count = 1;
//     smp::_tmain = std::this_thread::get_id();
//     auto nr_cpus = resource::nr_processing_units();
//     resource::cpuset cpu_set;
//     std::copy(boost::counting_iterator<unsigned>(0), boost::counting_iterator<unsigned>(nr_cpus),
//             std::inserter(cpu_set, cpu_set.end()));
//     if (configuration.count("cpuset")) {
//         cpu_set = configuration["cpuset"].as<cpuset_bpo_wrapper>().value;
//     }
//     if (configuration.count("smp")) {
//         nr_cpus = configuration["smp"].as<unsigned>();
//     } else {
//         nr_cpus = cpu_set.size();
//     }
//     smp::count = nr_cpus;
//     _reactors.resize(nr_cpus);
//     resource::configuration rc;
//     if (configuration.count("memory")) {
//         rc.total_memory = parse_memory_size(configuration["memory"].as<std::string>());

//     }
//     if (configuration.count("reserve-memory")) {
//         rc.reserve_memory = parse_memory_size(configuration["reserve-memory"].as<std::string>());
//     }
//     std::experimental::optional<std::string> hugepages_path;
//     if (configuration.count("hugepages")) {
//         hugepages_path = configuration["hugepages"].as<std::string>();
//     }
//     auto mlock = false;
//     if (configuration.count("lock-memory")) {
//         mlock = configuration["lock-memory"].as<bool>();
//     }
//     if (mlock) {
//         auto r = mlockall(MCL_CURRENT | MCL_FUTURE);
//         if (r) {
//             // Don't hard fail for now, it's hard to get the configuration right
//             print("warning: failed to mlockall: %s\n", strerror(errno));
//         }
//     }

//     rc.cpus = smp::count;
//     rc.cpu_set = std::move(cpu_set);
//     if (configuration.count("max-io-requests")) {
//         rc.max_io_requests = configuration["max-io-requests"].as<unsigned>();
//     }

//     if (configuration.count("num-io-queues")) {
//         rc.io_queues = configuration["num-io-queues"].as<unsigned>();
//     }

//     auto resources = resource::allocate(rc);
//     std::vector<resource::cpu> allocations = std::move(resources.cpus);
//     if (thread_affinity) {
//         smp::pin(allocations[0].cpu_id);
//     }
//     memory::configure(allocations[0].mem, hugepages_path);

//     if (configuration.count("abort-on-seastar-bad-alloc")) {
//         memory::enable_abort_on_allocation_failure();
//     }

//     bool heapprof_enabled = configuration.count("heapprof");
//     memory::set_heap_profiling_enabled(heapprof_enabled);


//     // Better to put it into the smp class, but at smp construction time
//     // correct smp::count is not known.
//     static boost::barrier reactors_registered(smp::count);
//     static boost::barrier smp_queues_constructed(smp::count);
//     static boost::barrier inited(smp::count);

//     auto io_info = std::move(resources.io_queues);

//     std::vector<io_queue*> all_io_queues;
//     all_io_queues.resize(io_info.coordinators.size());
//     io_queue::fill_shares_array();

//     auto alloc_io_queue = [io_info, &all_io_queues] (unsigned shard) {
//         auto cid = io_info.shard_to_coordinator[shard];
//         int vec_idx = 0;
//         for (auto& coordinator: io_info.coordinators) {
//             if (coordinator.id != cid) {
//                 vec_idx++;
//                 continue;
//             }
//             if (shard == cid) {
//                 all_io_queues[vec_idx] = new io_queue(coordinator.id, coordinator.capacity, io_info.shard_to_coordinator);
//             }
//             return vec_idx;
//         }
//         assert(0); // Impossible
//     };

//     auto assign_io_queue = [&all_io_queues] (shard_id id, int queue_idx) {
//         if (all_io_queues[queue_idx]->coordinator() == id) {
//             engine().my_io_queue.reset(all_io_queues[queue_idx]);
//         }
//         engine()._io_queue = all_io_queues[queue_idx];
//         engine()._io_coordinator = all_io_queues[queue_idx]->coordinator();
//     };

//     _all_event_loops_done.emplace(smp::count);

//     unsigned i;
//     for (i = 1; i < smp::count; i++) {
//         auto allocation = allocations[i];
//         create_thread([configuration, hugepages_path, i, allocation, assign_io_queue, alloc_io_queue, thread_affinity, heapprof_enabled] {
//             auto thread_name = seastar::format("reactor-{}", i);
//             pthread_setname_np(pthread_self(), thread_name.c_str());
//             if (thread_affinity) {
//                 smp::pin(allocation.cpu_id);
//             }
//             memory::configure(allocation.mem, hugepages_path);
//             memory::set_heap_profiling_enabled(heapprof_enabled);
//             sigset_t mask;
//             sigfillset(&mask);
//             for (auto sig : { SIGSEGV }) {
//                 sigdelset(&mask, sig);
//             }
//             auto r = ::pthread_sigmask(SIG_BLOCK, &mask, NULL);
//             throw_pthread_error(r);
//             allocate_reactor(i);
//             _reactors[i] = &engine();
//             auto queue_idx = alloc_io_queue(i);
//             reactors_registered.wait();
//             smp_queues_constructed.wait();
//             start_all_queues();
//             assign_io_queue(i, queue_idx);
//             inited.wait();
//             engine().configure(configuration);
//             engine().run();
//         });
//     }

//     allocate_reactor(0);
//     _reactors[0] = &engine();
//     auto queue_idx = alloc_io_queue(0);


//     reactors_registered.wait();
//     smp::_qs = new smp_message_queue* [smp::count];
//     for(unsigned i = 0; i < smp::count; i++) {
//         smp::_qs[i] = reinterpret_cast<smp_message_queue*>(operator new[] (sizeof(smp_message_queue) * smp::count));
//         for (unsigned j = 0; j < smp::count; ++j) {
//             new (&smp::_qs[i][j]) smp_message_queue(_reactors[j], _reactors[i]);
//         }
//     }
//     smp_queues_constructed.wait();
//     start_all_queues();
//     assign_io_queue(0, queue_idx);
//     inited.wait();

//     engine().configure(configuration);
//     engine()._lowres_clock = std::make_unique<lowres_clock>();
// }

// bool smp::poll_queues() {
//     size_t got = 0;
//     for (unsigned i = 0; i < count; i++) {
//         if (engine().cpu_id() != i) {
//             auto& rxq = _qs[engine().cpu_id()][i];
//             rxq.flush_response_batch();
//             got += rxq.has_unflushed_responses();
//             got += rxq.process_incoming();
//             auto& txq = _qs[i][engine()._id];
//             txq.flush_request_batch();
//             got += txq.process_completions();
//         }
//     }
//     return got != 0;
// }

// bool smp::pure_poll_queues() {
//     for (unsigned i = 0; i < count; i++) {
//         if (engine().cpu_id() != i) {
//             auto& rxq = _qs[engine().cpu_id()][i];
//             rxq.flush_response_batch();
//             auto& txq = _qs[i][engine()._id];
//             txq.flush_request_batch();
//             if (rxq.pure_poll_rx() || txq.pure_poll_tx() || rxq.has_unflushed_responses()) {
//                 return true;
//             }
//         }
//     }
//     return false;
// }

// #define HAVE_HWLOC
// boost::program_options::options_description
// smp::get_options_description()
// {
//     namespace bpo = boost::program_options;
//     bpo::options_description opts("SMP options");
//     opts.add_options()
//         ("smp,c", bpo::value<unsigned>(), "number of threads (default: one per CPU)")
//         ("cpuset", bpo::value<cpuset_bpo_wrapper>(), "CPUs to use (in cpuset(7) format; default: all))")
//         ("memory,m", bpo::value<std::string>(), "memory to use, in bytes (ex: 4G) (default: all)")
//         ("reserve-memory", bpo::value<std::string>(), "memory reserved to OS (if --memory not specified)")
//         ("hugepages", bpo::value<std::string>(), "path to accessible hugetlbfs mount (typically /dev/hugepages/something)")
//         ("lock-memory", bpo::value<bool>(), "lock all memory (prevents swapping)")
//         ("thread-affinity", bpo::value<bool>()->default_value(true), "pin threads to their cpus (disable for overprovisioning)")
//         ("num-io-queues", bpo::value<unsigned>(), "Number of IO queues. Each IO unit will be responsible for a fraction of the IO requests. Defaults to the number of threads")
//         ("max-io-requests", bpo::value<unsigned>(), "Maximum amount of concurrent requests to be sent to the disk. Defaults to 128 times the number of IO queues")
        
// #endif
//         ;
//     return opts;
// }