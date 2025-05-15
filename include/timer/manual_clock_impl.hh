// #pragma once

// #include "manual_clock.hh"
// #include "lowres_clock.hh"
// #include "../task/reactor_demo.hh"
// #include <iostream>

// struct reactor;
// reactor& engine();
// // Forward declarations
// void expire_manual_timers();

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
