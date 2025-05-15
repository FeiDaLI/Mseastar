// #pragma once

// #include <chrono>
// #include <atomic>

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

// // Implement these functions in their respective .cc files
// // or in a separate implementation section after engine.hh is included
// // DO NOT implement them here to avoid circular dependencies

