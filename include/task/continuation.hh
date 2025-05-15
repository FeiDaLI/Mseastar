// #pragma once
// #include "task.hh"
// #include "../future/future_state.hh"
// #include <functional>
// #include <iostream>

// // Forward declare the schedule_urgent function
// void schedule_urgent(std::unique_ptr<task> t);

// template <typename Func, typename... T>
// struct continuation final : public task {
//     using future_state_type = future_state<T...>;
//     // Constructor with state
//     continuation(Func&& func, future_state_type&& state)
//         : _state(std::move(state)), _func(std::move(func)) {

//         }
//     // Constructor without state
//     continuation(Func&& func) 
//         : _func(std::move(func)) {}
//     virtual void run() noexcept override {
//         if constexpr (std::is_invocable_v<Func, future_state_type&&>) {
//             try {
//                 _func(std::move(_state));
//             } catch (const std::exception& e) {
//                 std::cerr << "Exception in continuation: " << e.what() << std::endl;
//             } catch (...) {
//                 std::cerr << "Unknown exception in continuation" << std::endl;
//             }
//         } else {
//             // If Func doesn't match the expected signature, this is a fallback
//             std::cerr << "Error: continuation function has incorrect signature" << std::endl;
//         }
//     }
    
//     future_state_type _state;
//     Func _func;
// };
