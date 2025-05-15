
#pragma once
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <iosfwd>
#include <vector>
// Invokes func for each frame passing return address as argument.
template<typename Func>
void backtrace(Func&& func) noexcept(noexcept(func(0))) {
    unw_context_t context;
    if (unw_getcontext(&context) < 0) {
        return;
    }

    unw_cursor_t cursor;
    if (unw_init_local(&cursor, &context) < 0) {
        return;
    }

    while (unw_step(&cursor) > 0) {
        unw_word_t ip;
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
            break;
        }
        if (!ip) {
            break;
        }
        func(ip);
    }
}

class saved_backtrace {
    std::vector<unw_word_t> _frames;
public:
    saved_backtrace() = default;
    saved_backtrace(std::vector<unw_word_t> f) : _frames(std::move(f)) {}
    size_t hash() const;

    friend std::ostream& operator<<(std::ostream& out, const saved_backtrace&);

    bool operator==(const saved_backtrace& o) const {
        return _frames == o._frames;
    }

    bool operator!=(const saved_backtrace& o) const {
        return !(*this == o);
    }
};

namespace std {

template<>
struct hash<::saved_backtrace> {
    size_t operator()(const ::saved_backtrace& b) const {
        return b.hash();
    }
};

}

saved_backtrace current_backtrace();

#include <functional>

size_t saved_backtrace::hash() const {
    size_t h = 0;
    for (auto frame : _frames) {
        h = std::hash<unw_word_t>()(frame) + (h << 6) + (h << 16) - h;
    }
    return h;
}

saved_backtrace current_backtrace() {
    std::vector<unw_word_t> v;
    backtrace([&] (uintptr_t addr) {
        v.push_back(addr);
    });
    return saved_backtrace(std::move(v));
} 