#pragma once

#include "future.hh"
#include "expiring_fifo.hh"

class broken_semaphore : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Semaphore broken";
    }
};
class semaphore_timed_out : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Semaphore timedout";
    }
};
struct semaphore_default_exception_factory {
    static semaphore_timed_out timeout() {
        return semaphore_timed_out();
    }
    static broken_semaphore broken() {
        return broken_semaphore();
    }
};

template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
class basic_semaphore {
public:
    using duration = typename timer<Clock>::duration;
    using clock = typename timer<Clock>::clock;
    using time_point = typename timer<Clock>::time_point;//这里类似类型萃取.
private:
    ssize_t _count;
    std::exception_ptr _ex;
    struct entry {
        promise<> pr;
        size_t nr;
        entry(promise<>&& pr_, size_t nr_) : pr(std::move(pr_)), nr(nr_) {}
    };
    struct expiry_handler {
        void operator()(entry& e) noexcept {
            e.pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout()));
        }
    };
    expiring_fifo<entry, expiry_handler, clock> _wait_list;
    bool has_available_units(size_t nr) const {
        return _count >= 0 && (static_cast<size_t>(_count) >= nr);
    }
    bool may_proceed(size_t nr) const {
        return has_available_units(nr) && _wait_list.empty();
    }
public:
    static constexpr size_t max_counter() {
        return std::numeric_limits<decltype(_count)>::max();
    }
    basic_semaphore(size_t count) : _count(count) {}
    future<> wait(size_t nr = 1) {
        return wait(time_point::max(), nr);
    }
    future<> wait(duration timeout, size_t nr = 1) {
        return wait(Clock::now() + timeout, nr);
    }
    future<> wait(time_point timeout, size_t nr = 1) {
        if (may_proceed(nr)) {
            _count -= nr;
            return make_ready_future<>();
        }
        if (_ex) {
            return make_exception_future(_ex);
        }
        promise<> pr;
        auto fut = pr.get_future();
        _wait_list.push_back(entry(std::move(pr), nr), timeout);
        return fut;
    }
    void signal(size_t nr = 1) {
        if (_ex) {
            return;
        }
        _count += nr;
        while (!_wait_list.empty() && has_available_units(_wait_list.front().nr)) {
            auto& x = _wait_list.front();
            _count -= x.nr;
            x.pr.set_value();
            _wait_list.pop_front();
        }
    }
    void consume(size_t nr = 1) {
        if (_ex) {
            return;
        }
        _count -= nr;
    }
    bool try_wait(size_t nr = 1) {
        if (may_proceed(nr)) {
            _count -= nr;
            return true;
        } else {
            return false;
        }
    }
    size_t current() const { return std::max(_count, ssize_t(0)); }
    ssize_t available_units() const { return _count; }
    size_t waiters() const { return _wait_list.size(); }
    void broken() { broken(std::make_exception_ptr(ExceptionFactory::broken())); }
    template <typename Exception>
    void broken(const Exception& ex) {
        broken(std::make_exception_ptr(ex));
    }
    void broken(std::exception_ptr ex);
    void ensure_space_for_waiters(size_t n) {
        _wait_list.reserve(n);
    }
};

template<typename ExceptionFactory = semaphore_default_exception_factory, typename Clock = typename timer<>::clock>
class semaphore_units {
    basic_semaphore<ExceptionFactory, Clock>& _sem;
    size_t _n;
public:
    semaphore_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t n) noexcept : _sem(sem), _n(n) {}
    semaphore_units(semaphore_units&& o) noexcept : _sem(o._sem), _n(o._n) {
        o._n = 0;
    }
    semaphore_units& operator=(semaphore_units&& o) noexcept {
        if (this != &o) {
            this->~semaphore_units();
            new (this) semaphore_units(std::move(o));
        }
        return *this;
    }
    semaphore_units(const semaphore_units&) = delete;
    ~semaphore_units() noexcept {
        if (_n) {
            _sem.signal(_n);
        }
    }
    /// Releases ownership of the units. The semaphore will not be signalled.
    ///
    /// \return the number of units held
    size_t release() {
        return std::exchange(_n, 0);
    }
};

using semaphore = basic_semaphore<semaphore_default_exception_factory>;

class broken_condition_variable : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Condition variable is broken";
    }
};

class condition_variable_timed_out : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Condition variable timed out";
    }
};


class condition_variable {
    using duration = semaphore::duration;
    using clock = semaphore::clock;
    using time_point = semaphore::time_point;
    struct condition_variable_exception_factory {
        static condition_variable_timed_out timeout() {
            return condition_variable_timed_out();
        }
        static broken_condition_variable broken() {
            return broken_condition_variable();
        }
    };
    basic_semaphore<condition_variable_exception_factory> _sem;
public:
    condition_variable() : _sem(0) {}
    future<> wait() {
        return _sem.wait();
    }

    future<> wait(time_point timeout) {
        return _sem.wait(timeout);
    }
    future<> wait(duration timeout) {
        return _sem.wait(timeout);
    }
    template<typename Pred>
    future<> wait(Pred&& pred) {
        return do_until(std::forward<Pred>(pred), [this] {
            return wait();
        });
    }
    template<typename Pred>
    future<> wait(time_point timeout, Pred&& pred) {
        return do_until(std::forward<Pred>(pred), [this, timeout] () mutable {
            return wait(timeout);
        });
    }
    template<typename Pred>
    future<> wait(duration timeout, Pred&& pred) {
        return wait(clock::now() + timeout, std::forward<Pred>(pred));
    }
    void signal() {
        if (_sem.waiters()) {
            _sem.signal();
        }
    }
    void broadcast() {
        _sem.signal(_sem.waiters());
    }
    void broken() {
        _sem.broken();
    }
};
