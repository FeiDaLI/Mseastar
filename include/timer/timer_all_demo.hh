
// #include <chrono>
// #include <limits>
// #include <bitset>
// #include <array>
// #include <chrono>
// #include <atomic>
// #include <list>
// #include "bitset-iter.hh"

// template<typename Timer>
// class timer_set {
// public:
//     using time_point = typename Timer::time_point;
//     using timer_list_t = std::list<Timer*>;
//     using duration = typename Timer::duration;
//     using timestamp_t = typename duration::rep;
//     static constexpr timestamp_t max_timestamp = std::numeric_limits<timestamp_t>::max();
//     static constexpr int timestamp_bits = std::numeric_limits<timestamp_t>::digits;
//     static constexpr int n_buckets = timestamp_bits + 1;
//     std::array<timer_list_t, n_buckets> _buckets;
//     timestamp_t _last;
//     timestamp_t _next;
//     std::bitset<n_buckets> _non_empty_buckets;

//     static timestamp_t get_timestamp(time_point tp) {
//         return tp.time_since_epoch().count();
//     }

//     static timestamp_t get_timestamp(Timer& timer) {
//         return get_timestamp(timer.get_timeout());
//     }

//     int get_index(timestamp_t timestamp) const {
//         if (timestamp <= _last) {
//             return n_buckets - 1;
//         }
//         auto index = bitsets::count_leading_zeros(timestamp ^ _last);
//         assert(index < n_buckets - 1);
//         return index;
//     }

//     int get_index(Timer& timer) const {
//         return get_index(get_timestamp(timer));
//     }

//     int get_last_non_empty_bucket() const {
//         return bitsets::get_last_set(_non_empty_buckets);
//     }

// public:
//     timer_set() : _last(0), _next(max_timestamp), _non_empty_buckets(0) {}

//     ~timer_set() {
//         for (auto& list : _buckets) {
//             while (!list.empty()) {
//                 auto* timer = list.front();
//                 list.pop_front();
//                 timer->cancel();
//             }
//         }
//     }

//     bool insert(Timer& timer) {
//         auto timestamp = get_timestamp(timer);
//         auto index = get_index(timestamp);
//         auto& list = _buckets[index];
//         list.push_back(&timer);
//         timer.it = --list.end();
//         _non_empty_buckets[index] = true;
//         if (timestamp < _next) {
//             _next = timestamp;
//             return true;
//         }
//         return false;
//     }

//     void remove(Timer& timer) {
//         auto index = get_index(timer);
//         auto& list = _buckets[index];
//         list.erase(timer.it);
//         if (list.empty()) {
//             _non_empty_buckets[index] = false;
//         }
//     }

//     timer_list_t expire(time_point now) {
//         timer_list_t exp;
//         auto timestamp = get_timestamp(now);
//         if (timestamp < _last) {
//             abort();
//         }
//         auto index = get_index(timestamp);
//         for (int i : bitsets::for_each_set(_non_empty_buckets, index + 1)) {
//             exp.splice(exp.end(), _buckets[i]);
//             _non_empty_buckets[i] = false;
//         }
//         _last = timestamp;
//         _next = max_timestamp;
//         auto& list = _buckets[index];
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
//             for (auto* timer : _buckets[get_last_non_empty_bucket()]) {
//                 _next = std::min(_next, get_timestamp(*timer));
//             }
//         }
//         return exp;
//     }

//     time_point get_next_timeout() const {
//         return time_point(duration(std::max(_last, _next)));
//     }

//     void clear() {
//         for (auto& list : _buckets) {
//             list.clear();
//         }
//         _non_empty_buckets.reset();
//     }

//     size_t size() const {
//         size_t res = 0;
//         for (const auto& list : _buckets) {
//             res += list.size();
//         }
//         return res;
//     }

//     bool empty() const {
//         return _non_empty_buckets.none();
//     }

//     time_point now() {
//         return Timer::clock::now();
//     }
// };




// // Forward declarations
// class engine;
// template <typename T, typename E, typename EnableFunc>
// void complete_timers(T& timers, E& expired_timers, EnableFunc&& enable_fn);

// class manual_clock {
// public:
//     using rep = int64_t;
//     using period = std::chrono::nanoseconds::period;
//     using duration = std::chrono::duration<rep, period>;
//     using time_point = std::chrono::time_point<manual_clock, duration>;
// private:
//     static std::atomic<rep> _now;
// public:
//     manual_clock();
//     static time_point now() {
//         return time_point(duration(_now.load(std::memory_order_relaxed)));
//     }
//     static void advance(duration d);
//     static void expire_timers();
// };






// // Initialize static member
// std::atomic<manual_clock::rep> manual_clock::_now{0};

// void manual_clock::expire_timers() {
//     expire_manual_timers();
// }

// void manual_clock::advance(manual_clock::duration d) {
//     _now.fetch_add(d.count());
//     engine().schedule_urgent(make_task(&manual_clock::expire_timers));
//     //smp::invoke_on_all(&manual_clock::expire_timers);
//     return;
// }

// // Helper functions
// void expire_manual_timers() {
//     complete_timers(engine()._manual_timers, engine()._expired_manual_timers, []{
//         std::cout<<"到期"<<std::endl;
//     });
// }

// bool do_expire_lowres_timers() {
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

// bool do_check_lowres_timers() {
//     if (engine()._lowres_next_timeout == lowres_clock::time_point()) {
//         return false;
//     }
//     return lowres_clock::now() > engine()._lowres_next_timeout;
// } 







// // // Forward declarations
// // template <typename Clock> class timer;
// // using steady_clock_type = std::chrono::steady_clock;

// // // Forward declaration only
// // timespec to_timespec(steady_clock_type::time_point t);

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



// #include <deque>
// #include <atomic>
// #include <unordered_map>
// #include <chrono>
// #include <boost/program_options.hpp>
// #include <boost/filesystem.hpp>
// #include <experimental/optional>
// #include <iostream>
// #include <time.h>
// #include <signal.h>



// struct reactor {
//     lowres_clock::time_point _lowres_next_timeout;
//     timer_t _steady_clock_timer = {};

//     // Timer related members
//     timer_set<timer<steady_clock_type>> _timers;
//     typename timer_set<timer<steady_clock_type>>::timer_list_t _expired_timers;
//     timer_set<timer<lowres_clock>> _lowres_timers;
//     typename timer_set<timer<lowres_clock>>::timer_list_t _expired_lowres_timers;
//     timer_set<timer<manual_clock>> _manual_timers;
//     typename timer_set<timer<manual_clock>>::timer_list_t _expired_manual_timers;

//     using steady_timer = timer<steady_clock_type>;
//     using lowres_timer = timer<lowres_clock>;
//     using manual_timer = timer<manual_clock>;

// /*构造函数和析构函数*/
//     reactor();
//     ~reactor();
//     reactor(const reactor&) = delete;
//     reactor& operator=(const reactor&) = delete;
// /*任务相关*/
//     std::deque<std::unique_ptr<task>> _pending_tasks;
//     void schedule(std::unique_ptr<task> t);
//     void schedule_urgent(std::unique_ptr<task> t);
//     void run_tasks(std::deque<std::unique_ptr<task>>& tasks);
// /*定时器相关*/
//     // timer_set<timer<steady_clock_type>, &timer<steady_clock_type>::_link> _timers;
//     // timer_set<timer<steady_clock_type>, &timer<steady_clock_type>::_link>::timer_list_t _expired_timers;
//     // timer_set<timer<lowres_clock>, &timer<lowres_clock>::_link> _lowres_timers;
//     // timer_set<timer<lowres_clock>, &timer<lowres_clock>::_link>::timer_list_t _expired_lowres_timers;
//     // timer_set<timer<manual_clock>, &timer<manual_clock>::_link> _manual_timers;
//     // timer_set<timer<manual_clock>, &timer<manual_clock>::_link>::timer_list_t _expired_manual_timers;

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

// };

// reactor::reactor(){
//     // 初始化系统定时器
//     sigevent sev;
//     memset(&sev, 0, sizeof(sev));
//     sev.sigev_notify = SIGEV_SIGNAL;
//     sev.sigev_signo = SIGALRM;
//     int ret = timer_create(CLOCK_MONOTONIC, &sev, &_steady_clock_timer);
//     //所以使用的是singal.
//     if (ret < 0) {
//         throw std::system_error(errno, std::system_category());
//     }
// }

// reactor::~reactor(){
//     if (_steady_clock_timer) {
//             timer_delete(_steady_clock_timer);
//     }

// }


// reactor& engin(){
//     static reactor r;
//     return r;
// }

// void reactor::schedule(std::unique_ptr<task> t) {
//     _pending_tasks.push_back(std::move(t));
// }

// void reactor::schedule_urgent(std::unique_ptr<task> t) {
//     _pending_tasks.push_front(std::move(t));
// }


// void reactor::run_tasks(std::deque<std::unique_ptr<task>>& tasks) {
//     while (!tasks.empty()) {
//         auto tsk = std::move(tasks.front());
//         tasks.pop_front();
//         tsk->run();
//         tsk.reset();
//     }
// }

// template <typename T, typename E, typename EnableFunc>
// void complete_timers(T& timers, E& expired_timers, EnableFunc&& enable_fn) {
//     // 获取过期的定时器
//     expired_timers = timers.expire(timers.now());
    
//     // 处理所有过期定时器
//     for (auto* timer_ptr : expired_timers) {
//         if (timer_ptr) {
//             timer_ptr->_expired = true;
//         }
//     }
    
//     while (!expired_timers.empty()) {
//         auto* timer_ptr = expired_timers.front();
//         expired_timers.pop_front();
        
//         if (timer_ptr) {
//             timer_ptr->_queued = false;
//             if (timer_ptr->_armed) {
//                 timer_ptr->_armed = false;
//                 if (timer_ptr->_period) {
//                     timer_ptr->readd_periodic();
//                 }
//                 try {
//                     timer_ptr->_callback();
//                 } catch (const std::exception& e) {
//                     std::cout << "Timer callback failed: " << e.what() << std::endl;
//                 } catch (...) {
//                     std::cout << "Timer callback failed with unknown error" << std::endl;
//                 }
//             }
//         }
//     }
    
//     enable_fn();
// }
