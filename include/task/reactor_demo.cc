// #include "reactor_demo.hh"

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
// }

// reactor::~reactor() {
//     if (_steady_clock_timer) {
//         timer_delete(_steady_clock_timer);
//     }
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

// void reactor::enable_timer(steady_clock_type::time_point when) {
//     itimerspec its;
//     its.it_interval = {};
//     its.it_value = to_timespec(when);
//     auto ret = timer_settime(_steady_clock_timer, TIMER_ABSTIME, &its, NULL);
//     throw_system_error_on(ret == -1);
// }

// void reactor::add_timer(steady_timer *tmr) {
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
//     return _timers.insert(*tmr);
// }



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