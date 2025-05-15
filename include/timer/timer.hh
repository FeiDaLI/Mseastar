// #pragma once
// #include <chrono>
// #include <experimental/optional>
// #include <atomic>
// #include <functional>
// #include <sys/time.h>
// #include <system_error>
// #include <list>
// // #include "../future/all.hh"
// #include "lowres_clock.hh"
// #include "manual_clock.hh"
// #include "time_set_.hh"
// #include "lowres_clock_impl.hh"
// #include "manual_clock_impl.hh"
// #include "../task/reactor_demo.hh"

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
//     std::list<timer*>::iterator it; // 新增的迭代器成员
//     std::list<timer*>::iterator expired_it;  // 过期链表中的位置
//     // boost::intrusive::list_member_hook<> _link;
//     callback_t _callback;
//     time_point _expiry;
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
//     _queued = true;
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

// void del_timer(timer<lowres_clock>* tmr) {
//     engine().del_timer(tmr);
// }

// void del_timer(timer<steady_clock_type>* tmr) {
//     engine().del_timer(tmr);
// }
// void del_timer(timer<manual_clock>* tmr) {
//     engine().del_timer(tmr);
// }


