#pragma once
#include <mutex>
#include <condition_variable>
#include <optional>

// Single-element blocking queue
template <typename T>
class exchanger {
private:
    std::mutex _mutex;
    std::condition_variable _cv;
    std::optional<T> _element;
    std::exception_ptr _exception;
private:
    void interrupt_ptr(std::exception_ptr e) {
        std::unique_lock<std::mutex> lock(_mutex);
        if (!_exception) {
            _exception = e;
            _cv.notify_all();
        }
        // FIXME: log if already interrupted
    }
public:
    template <typename Exception>
    void interrupt(Exception e) {
        try {
            throw e;
        } catch (...) {
            interrupt_ptr(std::current_exception());
        }
    }
    void give(T value) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this] { return !_element || _exception; });
        if (_exception) {
            std::rethrow_exception(_exception);
        }
        _element = value;
        _cv.notify_one();
    }
    T take() {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this] { return bool(_element) || _exception; });
        if (_exception) {
            std::rethrow_exception(_exception);
        }
        auto v = *_element;
        _element = {};
        _cv.notify_one();
        return v;
    }
};
