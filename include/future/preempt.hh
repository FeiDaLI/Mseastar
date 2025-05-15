// #pragma once
// #include <atomic>
// extern __thread bool g_need_preempt;
// inline bool need_preempt() {
// #ifndef DEBUG
//     // prevent compiler from eliminating loads in a loop
//     std::atomic_signal_fence(std::memory_order_seq_cst);
//     return g_need_preempt;
// #else
//     return true;
// #endif
// }



