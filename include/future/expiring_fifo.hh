/*
 * This file is open source software, licensed to you under the terms
 * of the Apache License, Version 2.0 (the "License").  See the NOTICE file
 * distributed with this work for additional information regarding copyright
 * ownership.  You may not use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/*
 * Copyright (C) 2016 ScyllaDB
 */

#pragma once

#include"future_all5.hh"
#include<stdexcept>
#include<exception>
#include<deque>

template<typename T>
struct dummy_expiry {
    void operator()(T&) noexcept {};
};
template<typename... T>
struct promise_expiry {
    void operator()(promise<T...>& pr) noexcept {
        pr.set_exception(std::make_exception_ptr(timed_out_error()));
    };
};

template <typename T, typename OnExpiry = dummy_expiry<T>, typename Clock = lowres_clock>
class expiring_fifo {
public:
    using clock = Clock;
    using time_point = typename Clock::time_point;
private:
    struct entry {
        std::experimental::optional<T> payload;
        timer<Clock> tr;
        entry(T&& payload_) : payload(std::move(payload_)) {}
        entry(const T& payload_) : payload(payload_) {}
        entry(T payload_, expiring_fifo& ef, time_point timeout)
                : payload(std::move(payload_))
                , tr([this, &ef] {
                    ef._on_expiry(*payload);
                    payload = std::experimental::nullopt;
                    --ef._size;
                    ef.drop_expired_front();
                })
        {
            tr.arm(timeout);
        }
        entry(entry&& x) = delete;
        entry(const entry& x) = delete;
    };

    std::deque<entry> _list;
    OnExpiry _on_expiry;
    size_t _size = 0;

    void drop_expired_front() {
        while (!_list.empty() && !_list.front().payload) {
            _list.pop_front();
        }
    }
public:
    expiring_fifo() = default;
    expiring_fifo(OnExpiry on_expiry) : _on_expiry(std::move(on_expiry)) {}

    bool empty() const {
        return _size == 0;
    }

    explicit operator bool() const {
        return !empty();
    }

    T& front() {
        return *_list.front().payload;
    }

    const T& front() const {
        return *_list.front().payload;
    }

    size_t size() const {
        return _size;
    }
    void reserve(size_t size) {
        return _list.reserve(size);
    }
    void push_back(const T& payload) {
        _list.emplace_back(payload);
        ++_size;
    }
    void push_back(T&& payload) {
        _list.emplace_back(std::move(payload));
        ++_size;
    }
    void push_back(T payload, time_point timeout) {
        if (timeout < time_point::max()) {
            _list.emplace_back(std::move(payload), *this, timeout);
        } else {
            _list.emplace_back(std::move(payload));
        }
        ++_size;
    }
    void pop_front() {
        _list.pop_front();
        --_size;
        drop_expired_front();
    }
};
