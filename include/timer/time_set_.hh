// #ifndef __TIMER_SET_HH
// #define __TIMER_SET_HH

// #include <chrono>
// #include <limits>
// #include <bitset>
// #include <array>
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
// #endif
