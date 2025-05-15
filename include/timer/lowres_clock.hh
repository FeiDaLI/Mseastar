// #pragma once

// #include <chrono>
// #include <atomic>
// #include <memory>

// // Forward declarations
// template <typename Clock> class timer;
// using steady_clock_type = std::chrono::steady_clock;

// // Forward declaration only
// timespec to_timespec(steady_clock_type::time_point t);

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

// // Note: Implementation is in lowres_clock_impl.hh
