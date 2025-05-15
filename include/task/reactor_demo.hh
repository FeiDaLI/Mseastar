// #pragma once
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

// // First include the clock types
// #include "../timer/lowres_clock.hh"
// #include "../timer/manual_clock.hh"

// // Then include the timer implementation
// #include "../timer/timer.hh"

// // Finally include the timer set implementation
// #include "../timer/time_set_.hh"

// #include "task.hh"

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
