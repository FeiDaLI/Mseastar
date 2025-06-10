
#pragma once
#ifndef IO_QUEUE_HH
#define IO_QUEUE_HH

#include "semaphore.hh"
#include <atomic>       
#include <deque>     
#include <functional>     
#include <memory>
#include <numeric>   
#include <queue>  
#include <set>  
#include <string>   
#include <thread>  
#include <type_traits> 
#include <unordered_map>  
#include <vector>         

// C++11时间库头文件
#include <chrono>          // 时间处理（std::chrono::steady_clock等）


using shard_id = unsigned;

class io_queue;
class io_priority_class {
    unsigned val;
    friend io_queue;
public:
    unsigned id() const {
        return val;
    }
};

class priority_class {
public:
    struct request {
        promise<> pr;
        unsigned weight;
    };
    friend class fair_queue;
    uint32_t _shares = 0;
    float _accumulated = 0;
    std::deque<request> _queue;
    bool _queued = false;
    // friend struct shared_ptr_no_esft<priority_class>;
    explicit priority_class(uint32_t shares) : _shares(shares) {}
};


using priority_class_ptr = std::shared_ptr<priority_class>;

class fair_queue {
    friend priority_class;
    struct class_compare {
        bool operator() (const priority_class_ptr& lhs, const priority_class_ptr& rhs) const {
            return lhs->_accumulated > rhs->_accumulated;
        }
    };
    semaphore _sem;
    unsigned _capacity;
    using clock_type = std::chrono::steady_clock::time_point;
    clock_type _base;
    std::chrono::microseconds _tau;
    using prioq = std::priority_queue<priority_class_ptr, std::vector<priority_class_ptr>, class_compare>;
    prioq _handles;
    std::unordered_set<priority_class_ptr> _all_classes;
    void push_priority_class(priority_class_ptr pc) {
        if (!pc->_queued) {
            _handles.push(pc);
            pc->_queued = true;
        }
    }
    priority_class_ptr pop_priority_class() {
        assert(!_handles.empty());
        auto h = _handles.top();
        _handles.pop();
        assert(h->_queued);
        h->_queued = false;
        return h;
    }
    void execute_one() {
        _sem.wait().then([this] {
            priority_class_ptr h;
            do {
                h = pop_priority_class();
            } while (h->_queue.empty());

            auto req = std::move(h->_queue.front());
            h->_queue.pop_front();
            req.pr.set_value();
            auto delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _base);
            auto req_cost  = float(req.weight) / h->_shares;
            auto cost  = expf(1.0f/_tau.count() * delta.count()) * req_cost;
            float next_accumulated = h->_accumulated + cost;
            while (std::isinf(next_accumulated)) {
                normalize_stats();
                // If we have renormalized, our time base will have changed. This should happen very infrequently
                delta = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _base);
                cost  = expf(1.0f/_tau.count() * delta.count()) * req_cost;
                next_accumulated = h->_accumulated + cost;
            }
            h->_accumulated = next_accumulated;
            if (!h->_queue.empty()) {
                push_priority_class(h);
            }
            return make_ready_future<>();
        });
    }
    float normalize_factor() const {
        return std::numeric_limits<float>::min();
    }
    void normalize_stats() {
        auto time_delta = std::log(normalize_factor()) * _tau;
        // time_delta is negative; and this may advance _base into the future
        _base -= std::chrono::duration_cast<clock_type::duration>(time_delta);
        for (auto& pc: _all_classes) {
            pc->_accumulated *= normalize_factor();
        }
    }
public:
    explicit fair_queue(unsigned capacity, std::chrono::microseconds tau = std::chrono::milliseconds(100))
                                           : _sem(capacity)
                                           , _capacity(capacity)
                                           , _base(std::chrono::steady_clock::now())
                                           , _tau(tau) {
    }
    priority_class_ptr register_priority_class(uint32_t shares) {
        priority_class_ptr pclass = std::make_shared<priority_class>(shares);
        _all_classes.insert(pclass);
        return pclass;
    }
    void unregister_priority_class(priority_class_ptr pclass) {
        assert(pclass->_queue.empty());
        _all_classes.erase(pclass);
    }
    size_t waiters() const {
        return _sem.waiters();
    }
    template <typename Func>
    futurize_t<std::result_of_t<Func()>> queue(priority_class_ptr pc, unsigned weight, Func func) {
        // We need to return a future in this function on which the caller can wait.
        // Since we don't know which queue we will use to execute the next request - if ours or
        // someone else's, we need a separate promise at this point.
        promise<> pr;
        auto fut = pr.get_future();
        push_priority_class(pc);
        pc->_queue.push_back(priority_class::request{std::move(pr), weight});
        try {
            execute_one();
        } catch (...) {
            pc->_queue.pop_back();
            throw;
        }
        return fut.then([func = std::move(func)] {
            return func();
        }).finally([this] {
            _sem.signal();
        });
    }
    /// \param new_shares the new number of shares for this priority class
    static void update_shares(priority_class_ptr pc, uint32_t new_shares) {
        pc->_shares = new_shares;
    }
};


class smp;
class io_queue {
private:
    shard_id _coordinator;
    size_t _capacity;
    std::vector<shard_id> _io_topology;
    struct priority_class_data {
        priority_class_ptr ptr;
        size_t bytes;
        uint64_t ops;
        uint32_t nr_queued;
        std::chrono::duration<double> queue_time;
        // metrics::metric_groups _metric_groups;
        priority_class_data(std::string name, priority_class_ptr ptr, shard_id owner);
    };
    std::unordered_map<unsigned, std::shared_ptr<priority_class_data>> _priority_classes;
    fair_queue _fq;
    static constexpr unsigned _max_classes = 1024;
    static std::array<std::atomic<uint32_t>, _max_classes> _registered_shares;
    static std::array<std::string, _max_classes> _registered_names;
    static io_priority_class register_one_priority_class(std::string name, uint32_t shares);
    priority_class_data& find_or_create_class(const io_priority_class& pc, shard_id owner);
    static void fill_shares_array();
    friend smp;
public:
    io_queue(shard_id coordinator, size_t capacity, std::vector<shard_id> topology);
    ~io_queue();
    template <typename Func>
    static future<io_event>
    queue_request(shard_id coordinator, const io_priority_class& pc, size_t len, Func do_io);
    size_t capacity() const {
        return _capacity;
    }
    size_t queued_requests() const {
        return _fq.waiters();
    }

    shard_id coordinator() const {
        return _coordinator;
    }
    shard_id coordinator_of_shard(shard_id shard) const {
        return _io_topology[shard];
    }
    friend class reactor;
};







#endif