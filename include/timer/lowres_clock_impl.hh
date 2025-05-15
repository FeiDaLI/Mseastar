// #pragma once

// #include "lowres_clock.hh"
// #include "timer.hh"

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
// timespec to_timespec(steady_clock_type::time_point t) {
//     using ns = std::chrono::nanoseconds;
//     auto n = std::chrono::duration_cast<ns>(t.time_since_epoch()).count();
//     return { n / 1'000'000'000, n % 1'000'000'000 };
// }

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