#pragma once
#include<functional>
#include<signal.h>
#include <unordered_map>
#include <atomic>
struct signals {
        signals();
        ~signals();
        bool poll_signal();
        bool pure_poll_signal() const;
        void handle_signal(int signo, std::function<void ()>&& handler);
        void handle_signal_once(int signo, std::function<void ()>&& handler);
        static void action(int signo, siginfo_t* siginfo, void* ignore);
        struct signal_handler {
            signal_handler(int signo, std::function<void ()>&& handler);
            std::function<void ()> _handler;
        };
        std::atomic<uint64_t> _pending_signals;
        std::unordered_map<int, signal_handler> _signal_handlers;

};
